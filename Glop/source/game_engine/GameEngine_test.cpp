#include <gtest/gtest.h>
#include "GameEngine.h"
#include "TestProtos.pb.h"
#include "GameState.h"
#include "GameConnection.h"
#include "../System.h"

#include "../net/MockRouter.h"
#include "../net/MockNetworkManager.h"

class TestState : public GameState {
 public:
  TestState() {
    AddPlayer();
    state.set_applies(0);
    state.set_thinks(0);
  }
  virtual ~TestState() {}

  virtual bool Think() {
    for (int i = 0; i < state.positions_size(); i++) {
      state.mutable_positions(i)->set_x(state.positions(i).x() + 1);
      state.mutable_positions(i)->set_y(state.positions(i).y() + 2);
      if (state.positions(i).x() < 0) {
        state.mutable_positions(i)->set_x(0);
      }
      if (state.positions(i).y() < 0) {
        state.mutable_positions(i)->set_y(0);
      }
    }
    state.set_thinks(state.thinks() + 1);
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
    printf("Apply event\n");
    TestState* state = static_cast<TestState*>(game_state);
    if (state->state.positions_size() > typed_data_->player()) {
      int x = state->state.positions(typed_data_->player()).x();
      state->state.mutable_positions(typed_data_->player())->set_x(x + typed_data_->x());
      int y = state->state.positions(typed_data_->player()).y();
      state->state.mutable_positions(typed_data_->player())->set_y(y + typed_data_->y());
    }
    state->state.set_applies(state->state.applies() + 1);
    return NULL;
  }
 private:
  TestEngineMoveEvent* typed_data_;
};
REGISTER_EVENT(1, MovePlayerEvent);

// We have to install out own frame calculator so that we have complete control over what the
// current frame and delayed frame are during a test.
class TestFrameCalculator : public GameEngineFrameCalculator {
 public:
  TestFrameCalculator() : time_ms_(0) {}
  virtual int GetTime() const {
    return time_ms_;
  }
  virtual void SetTime(int time_ms) {
//    printf("SetTime: %d -> %d\n", time_ms_, time_ms);
    time_ms_ = time_ms;
  }
 private:
  int time_ms_;
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
/*
// Increment current frame and delayed frame and make sure that think happens only when the delayed
// frame increases.
TEST(GameEngineTest, TestThinksHappenAtTheCorrectTime) {
  TestState s;

  GameEngine engine(s, 50, 30, 10, 0);
  TestFrameCalculator* frame_calculator = new TestFrameCalculator();
  engine.InstallFrameCalculator(frame_calculator);

  for (int i = 0; i < 100; i++) {
    frame_calculator->SetTime(i);
    engine.Think();
    const TestState& ts = (const TestState&)engine.GetCurrentGameState();
    EXPECT_EQ(i/10 + 1, ts.state.thinks());
  }
}

TEST(GameEngineTest, TestEventsAreAppliedAtTheCorrectTime) {
  TestState s;

  GameEngine engine(s, 50, 30, 10, 0);
  TestFrameCalculator* frame_calculator = new TestFrameCalculator();
  engine.InstallFrameCalculator(frame_calculator);

  for (int i = 0; i < 100; i++) {
    printf("%d\n", i);
    frame_calculator->SetTime(i);
    if (i%10==5) {
      MovePlayerEvent* mpe = NewMovePlayerEvent();
      mpe->SetData(0, 1, 1);
      engine.ApplyEvent(mpe);
    }
    engine.Think();
    const TestState& ts = (const TestState&)engine.GetCurrentGameState();
    printf("player pos: %d %d\n", ts.state.positions(0).x(), ts.state.positions(0).y());
    EXPECT_EQ(i/10 + 1, ts.state.thinks());
    EXPECT_EQ((i/10 + 1) + i/10, ts.state.positions(0).x());
    EXPECT_EQ(2 * (i/10 + 1) + i/10, ts.state.positions(0).y());
  }
}

TEST(GameEngineTest, TestEventsAreAppliedAtTheCorrectTimeWhenWaitingForNetTimestep) {
  TestState s;

  GameEngine engine(s, 50, 30, 10, 15);
  TestFrameCalculator* frame_calculator = new TestFrameCalculator();
  engine.InstallFrameCalculator(frame_calculator);

  for (int i = 0; i < 100; i++) {
    printf("%d\n", i);
    frame_calculator->SetTime(i);
    if (i%10==5) {
      MovePlayerEvent* mpe = NewMovePlayerEvent();
      mpe->SetData(0, 1, 1);
      engine.ApplyEvent(mpe);
    }
    engine.Think();
    const TestState& ts = (const TestState&)engine.GetCurrentGameState();
    printf("player pos: %d %d\n", ts.state.positions(0).x(), ts.state.positions(0).y());
    EXPECT_EQ(i/10 + 1, ts.state.thinks());
    EXPECT_EQ((i/10 + 1) + i/10, ts.state.positions(0).x());
    EXPECT_EQ(2 * (i/10 + 1) + i/10, ts.state.positions(0).y());
  }
}

TEST(GameEngineTest, TestEventsAreAppliedInTheCorrectOrder) {
  TestState s;

  GameEngine engine(s, 50, 30, 10, 0);
  TestFrameCalculator* frame_calculator = new TestFrameCalculator();
  engine.InstallFrameCalculator(frame_calculator);

  // We set the time and create an event here because we can't get an event in
  // before the first think happens.
  frame_calculator->SetTime(5);
  {
    MovePlayerEvent* mpe = NewMovePlayerEvent();
    mpe->SetData(0, -100, -100);
    engine.ApplyEvent(mpe);
  }
  for (int i = 10; i < 100; i++) {
    printf("%d\n", i);
    frame_calculator->SetTime(i);
    if (i%10==5) {
      MovePlayerEvent* mpe = NewMovePlayerEvent();
      mpe->SetData(0, -100, -100);
      engine.ApplyEvent(mpe);
    }
    engine.Think();
    const TestState& ts = (const TestState&)engine.GetCurrentGameState();
    printf("player pos: %d %d\n", ts.state.positions(0).x(), ts.state.positions(0).y());
    EXPECT_EQ(i/10 + 1, ts.state.thinks());

    // We tried to move a player way negative, but think should prevent that.
    // Specifically, we are checking that apply events, then think, rather than thinking
    // and then applying events.
    EXPECT_EQ(0, ts.state.positions(0).x());
    EXPECT_EQ(0, ts.state.positions(0).y());
  }
}

TEST(GameEngineTest, TestThingsWorkAfterManyFramesHaveBeenProcessed) {
  TestState s;

  GameEngine engine(s, 50, 30, 10, 0);
  TestFrameCalculator* frame_calculator = new TestFrameCalculator();
  engine.InstallFrameCalculator(frame_calculator);

  for (int i = 0; i < 10000; i++) {
    frame_calculator->SetTime(i);
    engine.Think();
    const TestState& ts = (const TestState&)engine.GetCurrentGameState();
    EXPECT_EQ(i/10 + 1, ts.state.thinks());
  }
}

TEST(GameEngineTest, TestSkippingManyFramesAtATime) {
  TestState s;

  GameEngine engine(s, 50, 30, 10, 0);
  TestFrameCalculator* frame_calculator = new TestFrameCalculator();
  engine.InstallFrameCalculator(frame_calculator);

  frame_calculator->SetTime(5);
  MovePlayerEvent* mpe = NewMovePlayerEvent();
  mpe->SetData(0, -100, -100);
  engine.ApplyEvent(mpe);
  frame_calculator->SetTime(400);
  engine.Think();
  const TestState& ts = (const TestState&)engine.GetCurrentGameState();
  EXPECT_EQ(41, ts.state.thinks());
  EXPECT_EQ(39, ts.state.positions(0).x());
  EXPECT_EQ(78, ts.state.positions(0).y());
}

TEST(GameEngineTest, TestEnginesCanHostAndFindHosts) {
  TestState s;
  s.AddPlayer();

  GameEngine engine1(s, 50, 30, 10, 0);
  TestFrameCalculator* frame_calculator1 = new TestFrameCalculator();
  engine1.InstallFrameCalculator(frame_calculator1);

  ASSERT_TRUE(engine1.StartNetworkManager(65000));
  EXPECT_TRUE(engine1.AllowIncomingConnections("THUNDER awesome"));
  EXPECT_TRUE(engine1.DisallowIncomingConnections());
  ASSERT_TRUE(engine1.AllowIncomingConnections("AWESOME thunder"));


  GameEngine engine2(s);
  TestFrameCalculator* frame_calculator2 = new TestFrameCalculator();
  engine2.InstallFrameCalculator(frame_calculator2);

  // TODO: Make this a WaitForHost helper function
  ASSERT_TRUE(engine2.StartNetworkManager(65001));
  engine2.FindHosts(65000);
  int t = system()->GetTime();
  while (engine2.AvailableHosts().size() != 1 && system()->GetTime() < t + 250) {
    engine1.Think();
    engine2.Think();
    frame_calculator1->SetTime(frame_calculator1->GetTime());
    frame_calculator2->SetTime(frame_calculator2->GetTime());
    system()->Sleep(5);
  }
  vector<pair<GlopNetworkAddress, string> > hosts = engine2.AvailableHosts();
  ASSERT_EQ(1, hosts.size());
  EXPECT_EQ("AWESOME thunder", hosts[0].second);
}

TEST(GameEngineTest, TestEnginesCanConnect) {
  TestState s;
  s.AddPlayer();

  GameEngine engine1(s, 50, 10, 10, 0);
  TestFrameCalculator* frame_calculator1 = new TestFrameCalculator();
  engine1.InstallFrameCalculator(frame_calculator1);

  for (int i = 0; i < 100; i++) {
    frame_calculator1->SetTime(frame_calculator1->GetTime() + 1);
    engine1.Think();
  }
  

  ASSERT_TRUE(engine1.StartNetworkManager(65000));
  EXPECT_TRUE(engine1.AllowIncomingConnections("am host"));


  GameEngine engine2(s);
  TestFrameCalculator* frame_calculator2 = new TestFrameCalculator();
  engine2.InstallFrameCalculator(frame_calculator2);

  // TODO: Make this a WaitForHost helper function
  ASSERT_TRUE(engine2.StartNetworkManager(65001));
  engine2.FindHosts(65000);
  int t = system()->GetTime();
  while (engine2.AvailableHosts().size() != 1 && system()->GetTime() < t + 250) {
    engine1.Think();
    engine2.Think();
    frame_calculator1->SetTime(frame_calculator1->GetTime() + 1);
    frame_calculator2->SetTime(frame_calculator2->GetTime() + 1);
    system()->Sleep(5);
  }
  vector<pair<GlopNetworkAddress, string> > hosts = engine2.AvailableHosts();
  ASSERT_EQ(1, hosts.size());
  ASSERT_EQ("am host", hosts[0].second);

  engine2.Connect(hosts[0].first, hosts[0].second);
  t = system()->GetTime();
  GameEngineThinkState think_state;
  while ((think_state = engine2.Think()) == kConnecting && system()->GetTime() < t + 250) {
//    engine1.Think();
    system()->Sleep(5);
  }
  ASSERT_EQ(kJoining, think_state);

  t = system()->GetTime();
  while ((think_state = engine2.Think()) == kJoining && system()->GetTime() < t + 250) {
    engine1.Think();
    frame_calculator1->SetTime(frame_calculator1->GetTime() + 5);
    frame_calculator2->SetTime(frame_calculator2->GetTime() + 5);
    printf("Time: %d\n", frame_calculator2->GetTime());
    system()->Sleep(5);
  }
  printf("state: %d\n", think_state);
  ASSERT_EQ(kReady, think_state);

  t = system()->GetTime();
  while ((think_state = engine2.Think()) == kReady && system()->GetTime() < t + 1250) {
    engine1.Think();
    frame_calculator1->SetTime(frame_calculator1->GetTime() + 5);
    frame_calculator2->SetTime(frame_calculator2->GetTime() + 5);
    printf("Time: %d\n", frame_calculator2->GetTime());
    system()->Sleep(5);
  }
  printf("state: %d\n", think_state);
  ASSERT_EQ(kPlaying, think_state);

  for (int i = 0; i < 100; i++) {
    engine1.Think();
    engine2.Think();
    frame_calculator1->SetTime(frame_calculator1->GetTime() + 5);
    frame_calculator2->SetTime(frame_calculator2->GetTime() + 5);
    system()->Sleep(5);
  }
  for (int i = 0; i < 100; i++) {
    if (i%10==5) {
      MovePlayerEvent* mpe = NewMovePlayerEvent();
      mpe->SetData(0, -100, -100);
      engine1.ApplyEvent(mpe);
    }
    engine1.Think();
    engine2.Think();
    frame_calculator1->SetTime(frame_calculator1->GetTime() + 5);
    frame_calculator2->SetTime(frame_calculator2->GetTime() + 5);
    system()->Sleep(5);
  }
  if (frame_calculator1->GetTime() > frame_calculator2->GetTime()) {
    frame_calculator2->SetTime(frame_calculator1->GetTime());
  } else {
    frame_calculator1->SetTime(frame_calculator2->GetTime());
  }
  for (int i = 0; i < 100; i++) {
    if (i%10==5) {
      MovePlayerEvent* mpe = NewMovePlayerEvent();
      mpe->SetData(1, -100, -100);
      engine2.ApplyEvent(mpe);
    }
    engine1.Think();
    engine2.Think();
    frame_calculator1->SetTime(frame_calculator1->GetTime() + 5);
    frame_calculator2->SetTime(frame_calculator2->GetTime() + 5);
    system()->Sleep(5);
  }
  const TestState& ts1 = (const TestState&)engine1.GetCurrentGameState();
  const TestState& ts2 = (const TestState&)engine2.GetCurrentGameState();
  EXPECT_EQ(ts1.state.thinks(), ts2.state.thinks());
  EXPECT_EQ(ts1.state.positions(0).x(), ts2.state.positions(0).x());
  EXPECT_EQ(ts1.state.positions(0).y(), ts2.state.positions(0).y());
  EXPECT_EQ(ts1.state.positions(1).x(), ts2.state.positions(1).x());
  EXPECT_EQ(ts1.state.positions(1).y(), ts2.state.positions(1).y());
}
*/

class Waiter {
 public:
  virtual void Tick(GameEngineFrameCalculator* calculator) = 0;
  virtual void StartWaiting() = 0;
  virtual bool StillWaiting() = 0;
  virtual void Pause() = 0;
};

class RealTimerWaiter : public Waiter {
 public:
  RealTimerWaiter(int wait_duration, int pause_duration)
    : wait_duration_(wait_duration),
      pause_duration_(pause_duration) { }
  virtual void Tick(GameEngineFrameCalculator* calculator) { }
  virtual void StartWaiting() {
    reference_time_ = system()->GetTime();
  }
  virtual bool StillWaiting() {
    return system()->GetTime() - reference_time_ < wait_duration_;
  }
  virtual void Pause() {
    system()->Sleep(pause_duration_);
  }
 private:
  const int wait_duration_;
  int reference_time_;
  const int pause_duration_;
};

class TestTimerWaiter : public Waiter {
 public:
  TestTimerWaiter(int tick_amount, int wait_duration)
    : tick_amount_(tick_amount),
      wait_duration_(wait_duration) { }
  virtual void Tick(GameEngineFrameCalculator* calculator) {
    calculator->SetTime(calculator->GetTime() + tick_amount_);
  }
  virtual void StartWaiting() {
    reference_time_ = wait_duration_;
  }
  virtual bool StillWaiting() {
    reference_time_--;
    return reference_time_ >= 0;
  }
  virtual void Pause() {
    system()->Sleep(5);
  }
 private:
  const int tick_amount_;
  const int wait_duration_;
  int reference_time_;
};

GameEngineThinkState ThinkAll(
    GameEngine* client,
    set<pair<GameEngine*, GameEngineFrameCalculator*> > all,
    Waiter* waiter) {
  GameEngineThinkState ret = kIdle;
  set<pair<GameEngine*, GameEngineFrameCalculator*> >::iterator it;
  for (it = all.begin(); it != all.end(); it++) {
    GameEngineThinkState think_state = it->first->Think();
    if (it->first == client) {
      ret = think_state;
    }
    waiter->Tick(it->second);
  }
  return ret;
}

void ConnectEngines(
    GameEngine* host,
    int host_port,
    GameEngine* client,
    int client_port,
    set<pair<GameEngine*, GameEngineFrameCalculator*> > all_engines,
    Waiter* waiter) {
  ASSERT_TRUE(host->StartNetworkManager(host_port));
  EXPECT_TRUE(host->AllowIncomingConnections("I AM HOST"));

  ASSERT_TRUE(client->StartNetworkManager(client_port));
  client->FindHosts(host_port);

  waiter->StartWaiting();
  while (client->AvailableHosts().size() != 1 && waiter->StillWaiting()) {
    ThinkAll(client, all_engines, waiter);
    waiter->Pause();
  }
  vector<pair<GlopNetworkAddress, string> > hosts = client->AvailableHosts();
  ASSERT_EQ(1, hosts.size());
  EXPECT_EQ("I AM HOST", hosts[0].second);

  printf("***Found host\n");

  client->Connect(hosts[0].first, hosts[0].second);
  waiter->StartWaiting();
  while (ThinkAll(client, all_engines, waiter) == kConnecting && waiter->StillWaiting()) {
    printf("***Connecting...\n");
    waiter->Pause();
  }
  ASSERT_LE(kJoining, client->Think());

  printf("***Joining...\n");

  waiter->StartWaiting();
  while (ThinkAll(client, all_engines, waiter) == kJoining && waiter->StillWaiting()) {
    printf("***Joining\n");
    waiter->Pause();
  }
  ASSERT_LE(kReady, client->Think());

  printf("***Ready...\n");

  waiter->StartWaiting();
  while (ThinkAll(client, all_engines, waiter) == kReady && waiter->StillWaiting()) {
    printf("***Ready...\n");
    waiter->Pause();
  }
  ASSERT_LE(kPlaying, client->Think());

  printf("***playing\n");


  // Run them a bit just for kicks
  waiter->StartWaiting();
  while (waiter->StillWaiting()) {
    ThinkAll(client, all_engines, waiter);
    waiter->Pause();
  }
}

#if 1
TEST(GameEngineTest, TestEnginesCanConnectArbitrarilyX) {
  TestState s;
  s.AddPlayer();
  s.AddPlayer();

  MockRouter router;
  GameEngine engine1(s, 10, 30, 10, 5);
  engine1.InstallFrameCalculator(new TestFrameCalculator());
  engine1.InstallNetworkManager(new MockNetworkManager(&router));
  GameEngine engine2(s);
  engine2.InstallFrameCalculator(new TestFrameCalculator());
  engine2.InstallNetworkManager(new MockNetworkManager(&router));
  GameEngine engine3(s);
  engine3.InstallFrameCalculator(new TestFrameCalculator());
  engine3.InstallNetworkManager(new MockNetworkManager(&router));

  TestTimerWaiter waiter(5, 10);  // 5 * 10 > ms_per_net_frame
  set<pair<GameEngine*, GameEngineFrameCalculator*> > all_engines;
  all_engines.insert(make_pair(&engine1, engine1.GetFrameCalculator()));
  all_engines.insert(make_pair(&engine2, engine2.GetFrameCalculator()));
  ConnectEngines(&engine1, 65001, &engine2, 65002, all_engines, &waiter);
  //all_engines.insert(make_pair(&engine3, engine3.GetFrameCalculator()));
  //ConnectEngines(&engine1, 65001, &engine3, 65003, all_engines, &waiter);
}
#endif
#if 0
TEST(GameEngineTest, TestEnginesCanConnectArbitrarily) {
  TestState s;
  s.AddPlayer();
  s.AddPlayer();
  s.AddPlayer();
  s.AddPlayer();
  s.AddPlayer();

  GameEngine engine1(s, 50, 30, 10, 0);
  GameEngine engine2(s);
  GameEngine engine3(s);
  GameEngine engine4(s);
  GameEngine engine5(s);
  GameEngine engine6(s);
  GameEngine engine7(s);
  GameEngine engine8(s);
  GameEngine engine9(s);
  GameEngine engine10(s);
  GameEngine engine11(s);
  GameEngine engine12(s);
  GameEngine engine13(s);

  RealTimerWaiter waiter(250, 5);
  set<pair<GameEngine*, GameEngineFrameCalculator*> > all_engines;
  all_engines.insert(make_pair(&engine1, engine1.GetFrameCalculator()));
  all_engines.insert(make_pair(&engine2, engine2.GetFrameCalculator()));
  ConnectEngines(&engine1, 65001, &engine2, 65002, all_engines, &waiter);
  all_engines.insert(make_pair(&engine3, engine3.GetFrameCalculator()));
  ConnectEngines(&engine1, 65001, &engine3, 65003, all_engines, &waiter);
  all_engines.insert(make_pair(&engine4, engine4.GetFrameCalculator()));
  ConnectEngines(&engine1, 65001, &engine4, 65004, all_engines, &waiter);
  all_engines.insert(make_pair(&engine5, engine5.GetFrameCalculator()));
  ConnectEngines(&engine1, 65001, &engine5, 65005, all_engines, &waiter);
  all_engines.insert(make_pair(&engine6, engine6.GetFrameCalculator()));
  ConnectEngines(&engine4, 65004, &engine6, 65006, all_engines, &waiter);
  all_engines.insert(make_pair(&engine7, engine7.GetFrameCalculator()));
  ConnectEngines(&engine4, 65004, &engine7, 65007, all_engines, &waiter);
  all_engines.insert(make_pair(&engine8, engine8.GetFrameCalculator()));
  ConnectEngines(&engine7, 65007, &engine8, 65008, all_engines, &waiter);
  all_engines.insert(make_pair(&engine9, engine9.GetFrameCalculator()));
  ConnectEngines(&engine4, 65004, &engine9, 65009, all_engines, &waiter);

  all_engines.insert(make_pair(&engine10, engine10.GetFrameCalculator()));
  ConnectEngines(&engine7, 65007, &engine10, 65010, all_engines, &waiter);
  all_engines.insert(make_pair(&engine11, engine11.GetFrameCalculator()));
  ConnectEngines(&engine7, 65007, &engine11, 65011, all_engines, &waiter);
  all_engines.insert(make_pair(&engine12, engine12.GetFrameCalculator()));
  ConnectEngines(&engine7, 65007, &engine12, 65012, all_engines, &waiter);
  all_engines.insert(make_pair(&engine13, engine13.GetFrameCalculator()));
  ConnectEngines(&engine7, 65007, &engine13, 65013, all_engines, &waiter);
}
#endif
/*
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
    printf("engine %d at timestep: %d\n", i+1, all[i]->GetCompleteNetTimestep());
  }

  others.clear();
  others.push_back(&engine2);
  ConnectEngines(&engine1, &engine3, 65001, others);

  printf("Connected 3 to 1\n");
  for (int i = 0; i < all.size(); i++) {
    printf("engine %d at timestep: %d\n", i+1, all[i]->GetCompleteNetTimestep());
  }

  others.clear();
  others.push_back(&engine1);
  others.push_back(&engine3);
  ConnectEngines(&engine2, &engine4, 65002, others);

  printf("Connected 4 to 2\n");
  for (int i = 0; i < all.size(); i++) {
    printf("engine %d at timestep: %d\n", i+1, all[i]->GetCompleteNetTimestep());
  }

  others.clear();
  others.push_back(&engine1);
  others.push_back(&engine3);
  others.push_back(&engine4);
  ConnectEngines(&engine2, &engine5, 65002, others);

  printf("Connected 5 to 2\n");

  for (int i = 0; i < all.size(); i++) {
    printf("engine %d at timestep: %d\n", i+1, all[i]->GetCompleteNetTimestep());
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
  NetTimestep timestep = 10000000;
  for (int i = 0; i < all.size(); i++) {
    if (all[i]->GetCompleteNetTimestep() < timestep) {
      timestep = all[i]->GetCompleteNetTimestep();
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
*/
