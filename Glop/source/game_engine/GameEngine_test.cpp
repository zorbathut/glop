#include <gtest/gtest.h>
#include "GameEngine.h"
#include "TestProtos.pb.h"
#include "GameState.h"
#include "GameConnection.h"
#include "../System.h"

class TestState : public GameState {
 public:
  TestState() {
    AddPlayer();
    state.set_applies(0);
  }
  virtual ~TestState() {}

  virtual bool Think() {
    for (int i = 0; i < state.positions_size(); i++) {
      state.mutable_positions(i)->set_x(state.positions(i).x() + 1);
      state.mutable_positions(i)->set_y(state.positions(i).y() + 2);
    }
    state.set_thinks(state.thinks() + 1);
    thinks++;
  };

  virtual GameState* Copy() const {
    TestState* new_state = new TestState;
    new_state->state.CopyFrom(state);
    return new_state;
  }

  virtual void SerializeToString(string* data) const {
    state.SerializeToString(data);
  }

  virtual void ParseFromString(const string& data) {
    state.ParseFromString(data);
  }

  void AddPlayer() {
    PlayerPosition* pos = state.add_positions();
    pos->set_x(0);
    pos->set_y(0);
  }

  TestGameState state;
  static int thinks;
};
int TestState::thinks = 0;

class MovePlayerEvent : public GameEvent {
 public:
  MovePlayerEvent() {
    typed_data_ = new TestEngineMoveEvent;
    data_ = typed_data_;
  }
  ~MovePlayerEvent() {
    delete typed_data_;
  }
  void SetData(int player, int x, int y) {
    typed_data_->set_player(player);
    typed_data_->set_x(x);
    typed_data_->set_y(y);
  }
  virtual GameEventResult* ApplyToGameState(GameState* game_state) const {
    TestState* state = static_cast<TestState*>(game_state);
    if (state->state.positions_size() > typed_data_->player()) {
      int x = state->state.positions(typed_data_->player()).x();
      state->state.mutable_positions(typed_data_->player())->set_x(x + typed_data_->x());
      int y = state->state.positions(typed_data_->player()).y();
      state->state.mutable_positions(typed_data_->player())->set_y(y + typed_data_->y());
    }
    state->state.set_applies(state->state.applies() + 1);
    return  NULL;
  }
 private:
  TestEngineMoveEvent* typed_data_;
};
REGISTER_EVENT(1, MovePlayerEvent);

// We have to install out own frame calculator so that we have complete control over what the
// current frame and delayed frame are during a test.
class TestFrameCalculator : public GameEngineFrameCalculator {
 public:
  TestFrameCalculator(const GameEngine& engine)
    : GameEngineFrameCalculator(engine),
      current_timestep(0),
      delayed_timestep(0) {}
  virtual void GetFrame(Timestep* current_frame, Timestep* delayed_frame) const {
    *current_frame = current_timestep;
    *delayed_frame = delayed_timestep;
  }
  virtual void Set(Timestep timestep) {
    current_timestep = timestep;
    delayed_timestep = timestep;
  }
  Timestep current_timestep;
  Timestep delayed_timestep;
};

class GlopEnvironment : public testing::Environment {
 public:
  GlopEnvironment() {
    testing::AddGlobalTestEnvironment(this);
  }
  virtual void SetUp() {
    System::Init();
  }
};
static GlopEnvironment* env = new GlopEnvironment;

// Increment current frame and delayed frame and make sure that think happens only when the delayed
// frame increases.
TEST(GameEngineTest, TestThinksHappenAtTheCorrectTime) {
  TestState s;

  GameEngine engine(s, 0, true, false, 10, 50, 0);
  TestFrameCalculator* frame_calculator = new TestFrameCalculator(engine);
  engine.InstallFrameCalculator(frame_calculator);

  {
    frame_calculator->current_timestep = 0;
    frame_calculator->delayed_timestep = 0;
    engine.Think();
    const TestState& state = static_cast<const TestState&>(engine.GetCurrentGameState());
    ASSERT_EQ(1, state.state.positions_size());
    EXPECT_EQ(1, state.state.positions(0).x());
    EXPECT_EQ(2, state.state.positions(0).y());
  }

  {
    frame_calculator->current_timestep = 1;
    frame_calculator->delayed_timestep = 0;
    engine.Think();
    const TestState& state = static_cast<const TestState&>(engine.GetCurrentGameState());
    ASSERT_EQ(1, state.state.positions_size());
    EXPECT_EQ(1, state.state.positions(0).x());
    EXPECT_EQ(2, state.state.positions(0).y());
  }

  {
    frame_calculator->current_timestep = 1;
    frame_calculator->delayed_timestep = 1;
    engine.Think();
    const TestState& state = static_cast<const TestState&>(engine.GetCurrentGameState());
    ASSERT_EQ(1, state.state.positions_size());
    EXPECT_EQ(2, state.state.positions(0).x());
    EXPECT_EQ(4, state.state.positions(0).y());
  }

  {
    frame_calculator->current_timestep = 2;
    frame_calculator->delayed_timestep = 1;
    engine.Think();
    const TestState& state = static_cast<const TestState&>(engine.GetCurrentGameState());
    ASSERT_EQ(1, state.state.positions_size());
    EXPECT_EQ(2, state.state.positions(0).x());
    EXPECT_EQ(4, state.state.positions(0).y());
  }

  {
    frame_calculator->current_timestep = 2;
    frame_calculator->delayed_timestep = 2;
    engine.Think();
    const TestState& state = static_cast<const TestState&>(engine.GetCurrentGameState());
    ASSERT_EQ(1, state.state.positions_size());
    EXPECT_EQ(3, state.state.positions(0).x());
    EXPECT_EQ(6, state.state.positions(0).y());
  }
}

// Test that events that are applied take effect when the delayed frame exceeds the current frame
// that the event was applied on.
TEST(GameEngineTest, TestEventsHappenWhenDelayedTimeChanges) {
  TestState s;
  s.thinks = 0;

  GameEngine engine(s, 0, true, false, 10, 50, 0);
  TestFrameCalculator* frame_calculator = new TestFrameCalculator(engine);
  engine.InstallFrameCalculator(frame_calculator);

  {
    frame_calculator->current_timestep = 0;
    frame_calculator->delayed_timestep = 0;
    MovePlayerEvent* move_player = NewMovePlayerEvent();
    move_player->SetData(0, 1,1);
    engine.ApplyEvent(move_player);
    engine.Think();
    const TestState& state = static_cast<const TestState&>(engine.GetCurrentGameState());
    ASSERT_EQ(1, state.state.positions_size());
    EXPECT_EQ(1, state.state.positions(0).x());
    EXPECT_EQ(2, state.state.positions(0).y());
  }

  {
    frame_calculator->current_timestep = 1;
    frame_calculator->delayed_timestep = 0;
    MovePlayerEvent* move_player = NewMovePlayerEvent();
    move_player->SetData(0, 1,1);
    engine.ApplyEvent(move_player);
    engine.Think();
    const TestState& state = static_cast<const TestState&>(engine.GetCurrentGameState());
    ASSERT_EQ(1, state.state.positions_size());
    EXPECT_EQ(1, state.state.positions(0).x());
    EXPECT_EQ(2, state.state.positions(0).y());
  }

  {
    frame_calculator->current_timestep = 1;
    frame_calculator->delayed_timestep = 1;
    engine.Think();
    const TestState& state = static_cast<const TestState&>(engine.GetCurrentGameState());
    ASSERT_EQ(1, state.state.positions_size());
    EXPECT_EQ(3, state.state.positions(0).x());
    EXPECT_EQ(5, state.state.positions(0).y());
  }

  {
    frame_calculator->current_timestep = 2;
    frame_calculator->delayed_timestep = 1;
    engine.Think();
    const TestState& state = static_cast<const TestState&>(engine.GetCurrentGameState());
    ASSERT_EQ(1, state.state.positions_size());
    EXPECT_EQ(3, state.state.positions(0).x());
    EXPECT_EQ(5, state.state.positions(0).y());
  }

  {
    frame_calculator->current_timestep = 2;
    frame_calculator->delayed_timestep = 2;
    engine.Think();
    const TestState& state = static_cast<const TestState&>(engine.GetCurrentGameState());
    ASSERT_EQ(1, state.state.positions_size());
    EXPECT_EQ(5, state.state.positions(0).x());
    EXPECT_EQ(8, state.state.positions(0).y());
  }
}

// \todo jwills - Should probably have some death tests

// Nothing special except that we're running through more frames than we can fit into the
// MovingWindows that we have
TEST(GameEngineTest, TestThingsWorkAfterManyFramesHaveBeenProcessed) {
  TestState s;

  GameEngine engine(s, 0, true, false, 10, 50, 0);
  TestFrameCalculator* frame_calculator = new TestFrameCalculator(engine);
  engine.InstallFrameCalculator(frame_calculator);

  for (int i = 0; i < 500; i++) {
    frame_calculator->current_timestep = i;
    frame_calculator->delayed_timestep = i;

    // \todo jwills - This is a ridiculous memory leak, should definitely fix it, but have to be
    // clear on exactly when we can delete the events.  Perhaps the engine should take ownership of
    // them?
    MovePlayerEvent* move_player = NewMovePlayerEvent();
    move_player->SetData(0, 1, 1);
    engine.ApplyEvent(move_player);
    engine.Think();
    const TestState& state = static_cast<const TestState&>(engine.GetCurrentGameState());
    ASSERT_EQ(1, state.state.positions_size());
    EXPECT_EQ((i+1)*1 + i*1, state.state.positions(0).x());
    EXPECT_EQ((i+1)*2 + i*1, state.state.positions(0).y());
  }
}

TEST(GameEngineTest, TestMultipleFramesPassingAtATime) {
  TestState s;

  GameEngine engine(s, 0, true, false, 10, 50, 0);
  TestFrameCalculator* frame_calculator = new TestFrameCalculator(engine);
  engine.InstallFrameCalculator(frame_calculator);

  for (int i = 0; i < 500; i++) {
    frame_calculator->current_timestep = i*5;
    frame_calculator->delayed_timestep = i*5;

    // \todo jwills - This is a ridiculous memory leak, should definitely fix it, but have to be
    // clear on exactly when we can delete the events.  Perhaps the engine should take ownership of
    // them?
    MovePlayerEvent* move_player = NewMovePlayerEvent();
    move_player->SetData(0, 1, 1);
    engine.ApplyEvent(move_player);
    engine.Think();
    const TestState& state = static_cast<const TestState&>(engine.GetCurrentGameState());
    ASSERT_EQ(1, state.state.positions_size());
    EXPECT_EQ((i*5+1)*1 + i*1, state.state.positions(0).x());
    EXPECT_EQ((i*5+1)*2 + i*1, state.state.positions(0).y());
  }
}

TEST(GameEngineTest, TestEnginesCanConnect) {
  TestState s;
  s.AddPlayer();

  GameEngine engine1(s, 0, true, false, 50, 50, 65000);
  TestFrameCalculator* frame_calculator1 = new TestFrameCalculator(engine1);
  engine1.InstallFrameCalculator(frame_calculator1);

  ASSERT_TRUE(engine1.StartNetworkManager());
  EXPECT_TRUE(engine1.AllowIncomingConnections("THUNDER awesome"));
  EXPECT_TRUE(engine1.DisallowIncomingConnections());
  ASSERT_TRUE(engine1.AllowIncomingConnections("AWESOME thunder"));


  GameEngine engine2(s, 0, false, false, 50, 50, 65001);
  TestFrameCalculator* frame_calculator2 = new TestFrameCalculator(engine2);
  engine2.InstallFrameCalculator(frame_calculator2);

  // TODO: Make this a WaitForHost helper function
  ASSERT_TRUE(engine2.StartNetworkManager());
  engine2.FindHosts(65000);
  int t = system()->GetTime();
  while (engine2.AvailableHosts().size() != 1 && system()->GetTime() < t + 250) {
    engine1.Think();
    engine2.Think();
    frame_calculator1->current_timestep++;
    frame_calculator1->delayed_timestep++;
    frame_calculator2->current_timestep++;
    frame_calculator2->delayed_timestep++;
    system()->Sleep(5);
  }
  vector<pair<GlopNetworkAddress, string> > hosts = engine2.AvailableHosts();
  ASSERT_EQ(1, hosts.size());
  EXPECT_EQ("AWESOME thunder", hosts[0].second);

  engine2.Connect(hosts[0].first, hosts[0].second);
  t = system()->GetTime();
  while (engine2.Think() == kConnecting && system()->GetTime() < t + 250) {
    engine1.Think();
    frame_calculator1->current_timestep++;
    frame_calculator1->delayed_timestep++;
    frame_calculator2->current_timestep++;
    frame_calculator2->delayed_timestep++;
    system()->Sleep(5);
  }
  ASSERT_EQ(kJoining, engine2.Think());
  t = system()->GetTime();
  while (engine2.Think() == kJoining && system()->GetTime() < t + 250) {
    engine1.Think();
    frame_calculator1->current_timestep++;
    frame_calculator1->delayed_timestep++;
    frame_calculator2->current_timestep++;
    frame_calculator2->delayed_timestep++;
    system()->Sleep(5);
  }
  ASSERT_EQ(kReady, engine2.Think());
  t = system()->GetTime();
  while (engine2.Think() == kReady && system()->GetTime() < t + 250) {
    engine1.Think();
    frame_calculator1->current_timestep++;
    frame_calculator1->delayed_timestep++;
    frame_calculator2->current_timestep++;
    frame_calculator2->delayed_timestep++;
    system()->Sleep(5);
  }
  ASSERT_EQ(kPlaying, engine2.Think());
  // TODO: It's possible for the connecting engine to start a little bit behind, so the engine
  // should advance the calculator as necessary if it sees future events coming from other players
  // regularly.
  frame_calculator2->current_timestep = frame_calculator1->current_timestep;
  frame_calculator2->delayed_timestep = frame_calculator1->delayed_timestep;
  t = system()->GetTime();
  while (engine1.GetCompleteTimestep() != engine2.GetCompleteTimestep() &&
         system()->GetTime() < t + 250) {
    frame_calculator1->current_timestep++;
    frame_calculator1->delayed_timestep++;
    frame_calculator2->current_timestep = frame_calculator1->current_timestep;
    frame_calculator2->delayed_timestep = frame_calculator1->delayed_timestep;
    engine1.Think();
    engine2.Think();
    system()->Sleep(10);
  }
  ASSERT_EQ(engine1.GetCompleteTimestep(), engine2.GetCompleteTimestep());
  engine1.Think();
  engine2.Think();
  string s1;
  string s2;
  for (int i = 0; i < 50; i++) {
    engine1.Think();
    engine2.Think();
    Timestep t1 = engine1.GetCompleteTimestep();
    Timestep t2 = engine2.GetCompleteTimestep();
    Timestep tx = (t1<t2) ? t1 : t2;
    engine1.GetSpecificGameState(tx).SerializeToString(&s1);
    engine2.GetSpecificGameState(tx).SerializeToString(&s2);
    const TestGameState& ts1 = ((const TestState&)(engine1.GetSpecificGameState(tx))).state;
    const TestGameState& ts2 = ((const TestState&)(engine2.GetSpecificGameState(tx))).state;
    ASSERT_EQ(s1, s2);
    frame_calculator1->current_timestep++;
    frame_calculator1->delayed_timestep++;
    frame_calculator2->current_timestep++;
    frame_calculator2->delayed_timestep++;

    // \todo jwills - This is a ridiculous memory leak, should definitely fix it, but have to be
    // clear on exactly when we can delete the events.  Perhaps the engine should take ownership of
    // them?
    MovePlayerEvent* move_player = NewMovePlayerEvent();
    move_player->SetData(0, 1, 1);
    engine1.ApplyEvent(move_player);

    move_player = NewMovePlayerEvent();
    move_player->SetData(1, 3, 5);
    engine2.ApplyEvent(move_player);
    system()->Sleep(10);
  }
  t = system()->GetTime();
  while (engine1.GetCompleteTimestep() != engine2.GetCompleteTimestep() &&
         system()->GetTime() < t + 1250) {
    engine1.Think();
    engine2.Think();
    system()->Sleep(10);
  }
  EXPECT_EQ(engine1.GetCompleteTimestep(), engine2.GetCompleteTimestep());
  engine1.GetCompleteGameState().SerializeToString(&s1);
  engine2.GetCompleteGameState().SerializeToString(&s2);
  EXPECT_EQ(s1, s2);
  const TestGameState& ts1 = ((const TestState&)(engine1.GetCompleteGameState())).state;
  const TestGameState& ts2 = ((const TestState&)(engine2.GetCompleteGameState())).state;
}

// Convenience method for connecting one engine to another.  Assumes they both use some sort of time
// based framerate calculator.
void ConnectEngines(GameEngine* host, GameEngine* client, int port, vector<GameEngine*> others) {
  ASSERT_TRUE(host->StartNetworkManager());
  EXPECT_TRUE(host->AllowIncomingConnections("I AM HOST"));

  ASSERT_TRUE(client->StartNetworkManager());
  client->FindHosts(port);

  int t = system()->GetTime();
  while (client->AvailableHosts().size() != 1 && system()->GetTime() < t + 250) {
    host->Think();
    client->Think();
    for (int i = 0; i < others.size(); i++) {
      others[i]->Think();
    }
    system()->Sleep(5);
  }
  vector<pair<GlopNetworkAddress, string> > hosts = client->AvailableHosts();
  ASSERT_EQ(1, hosts.size());
  EXPECT_EQ("I AM HOST", hosts[0].second);

  printf("Found host\n");

  client->Connect(hosts[0].first, hosts[0].second);
  t = system()->GetTime();
  while (client->Think() == kConnecting && system()->GetTime() < t + 250) {
    host->Think();
    for (int i = 0; i < others.size(); i++) {
      others[i]->Think();
    }
    system()->Sleep(5);
  }
  ASSERT_EQ(kJoining, client->Think());

  printf("Joinging...\n");

  t = system()->GetTime();
  while (client->Think() == kJoining && system()->GetTime() < t + 250) {
    host->Think();
    for (int i = 0; i < others.size(); i++) {
      others[i]->Think();
    }
    system()->Sleep(5);
  }
  ASSERT_EQ(kReady, client->Think());

  printf("Ready\n");

  t = system()->GetTime();
  while (client->Think() == kReady && system()->GetTime() < t + 250) {
    host->Think();
    for (int i = 0; i < others.size(); i++) {
      others[i]->Think();
    }
    system()->Sleep(5);
  }
  ASSERT_EQ(kPlaying, client->Think());

  printf("playing\n");


  // Run them a bit just for kicks
  t = system()->GetTime();
  while (system()->GetTime() < t + 250) {
    host->Think();
    client->Think();
    for (int i = 0; i < others.size(); i++) {
      others[i]->Think();
    }
    system()->Sleep(5);
  }
}

TEST(GameEngineTest, TestEnginesCanConnectArbitrarily) {
  TestState s;
  s.AddPlayer();
  s.AddPlayer();
  s.AddPlayer();
  s.AddPlayer();

  GameEngine engine1(s, 0, true, false, 50, 50, 65001);
  GameEngine engine2(s, 0, false, false, 50, 50, 65002);
  GameEngine engine3(s, 0, false, false, 50, 50, 65003);
  GameEngine engine4(s, 0, false, false, 50, 50, 65004);
  GameEngine engine5(s, 0, false, false, 50, 50, 65005);

  vector<GameEngine*> all;
  all.push_back(&engine1);
  all.push_back(&engine2);
  all.push_back(&engine3);
  all.push_back(&engine4);
  all.push_back(&engine5);

  vector<GameEngine*> others;

  ConnectEngines(&engine1, &engine2, 65001, others);

  printf("Connected 2 to 1\n");
  for (int i = 0; i < all.size(); i++) {
    printf("engine %d at timestep: %d\n", i+1, all[i]->GetCompleteTimestep());
  }

  others.clear();
  others.push_back(&engine2);
  ConnectEngines(&engine1, &engine3, 65001, others);

  printf("Connected 3 to 1\n");
  for (int i = 0; i < all.size(); i++) {
    printf("engine %d at timestep: %d\n", i+1, all[i]->GetCompleteTimestep());
  }

  others.clear();
  others.push_back(&engine1);
  others.push_back(&engine3);
  ConnectEngines(&engine2, &engine4, 65002, others);

  printf("Connected 4 to 2\n");
  for (int i = 0; i < all.size(); i++) {
    printf("engine %d at timestep: %d\n", i+1, all[i]->GetCompleteTimestep());
  }

  others.clear();
  others.push_back(&engine1);
  others.push_back(&engine3);
  others.push_back(&engine4);
  ConnectEngines(&engine2, &engine5, 65002, others);

  printf("Connected 5 to 2\n");

  for (int i = 0; i < all.size(); i++) {
    printf("engine %d at timestep: %d\n", i+1, all[i]->GetCompleteTimestep());
  }




  int t = system()->GetTime();
  while (system()->GetTime() < t + 500) {
    for (int i = 0; i < all.size(); i++) {
      MovePlayerEvent* move_player = NewMovePlayerEvent();
      move_player->SetData(i, i, i);
      all[i]->ApplyEvent(move_player);
    }
    for (int i = 0; i < all.size(); i++) {
      all[i]->Think();
    }
    system()->Sleep(40);
  }
  Timestep timestep = 10000000;
  for (int i = 0; i < all.size(); i++) {
    if (all[i]->GetCompleteTimestep() < timestep) {
      timestep = all[i]->GetCompleteTimestep();
    }
  }
  vector<string> serialized(all.size());
  for (int i = 0; i < all.size(); i++) {
    all[i]->GetSpecificGameState(timestep).SerializeToString(&serialized[i]);
  }
  for (int i = 1; i < all.size(); i++) {
    EXPECT_EQ(serialized[0], serialized[i]);
  }
}
