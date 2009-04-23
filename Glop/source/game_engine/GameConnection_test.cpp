#include <gtest/gtest.h>
#include "GameConnection.h"
#include "GameEvent.h"
#include "TestProtos.pb.h"

#include "../System.h"
#include "../net/MockNetworkManager.h"


class GlopEnvironment : public testing::Environment {
 public:
  virtual void SetUp() {
    System::Init();
  }
};
testing::Environment* const global_env = testing::AddGlobalTestEnvironment(new GlopEnvironment);

class FooEvent : public GameEvent {
 public:
  FooEvent() {
    data_ = new Foo;
  }
  virtual GameEventResult* ApplyToGameState(GameState* state) const {
    return NULL;
  }
  Foo* GetData() {
    Foo* f = static_cast<Foo*>(data_);
    return f;
  }
};
REGISTER_EVENT(1, FooEvent);

class BarEvent : public GameEvent {
 public:
   BarEvent() {
     data_ = new Bar;
   }
  virtual GameEventResult* ApplyToGameState(GameState* state) const {
    return NULL;
  }
  Bar* GetData() {
    return static_cast<Bar*>(data_);
  }
};
REGISTER_EVENT(2, BarEvent);


TEST(GameConnectionTest, TestConnectionCanSendAndReceiveEvents) {
  TestConnection* in = new TestConnection();
  TestConnection* out = new TestConnection();
  in->SetOutput(out);
  out->SetOutput(in);

  BarEvent* e1 = NewBarEvent();
  e1->GetData()->set_wingding(1234);
  e1->GetData()->set_barbaz(2345);
  FooEvent* e2 = NewFooEvent();
  e2->GetData()->set_foo(23456789);
  e2->GetData()->set_bar(0);
  BarEvent* e3 = NewBarEvent();
  e3->GetData()->set_wingding(125);
  e3->GetData()->set_barbaz(7);
  vector<GameEvent*> v;
  v.push_back(e1);
  v.push_back(e2);
  in->QueueEvents(0, EventPackageID(1,100), v);
  v.resize(0);
  v.push_back(e3);
  in->QueueEvents(0, EventPackageID(2,101), v);
  in->SendEvents(0);
  delete e1;
  delete e2;
  delete e3;

  vector<pair<EventPackageID, vector<GameEvent*> > > output_events;
  out->ReceiveEvents(&output_events);
  ASSERT_EQ(2, output_events.size());

  EXPECT_EQ(1,   output_events[0].first.state_timestep);
  EXPECT_EQ(100, output_events[0].first.engine_id);
  EXPECT_EQ(2, output_events[0].second.size());
  if (output_events[0].second.size() == 2) {
    EXPECT_EQ(2, GameEventFactory::GetGameEventType(output_events[0].second[0]));
    BarEvent* bar = (BarEvent*)output_events[0].second[0];
    EXPECT_EQ(1234, bar->GetData()->wingding());
    EXPECT_EQ(2345, bar->GetData()->barbaz());
    EXPECT_EQ(1, GameEventFactory::GetGameEventType(output_events[0].second[1]));
  }

  EXPECT_EQ(2,   output_events[1].first.state_timestep);
  EXPECT_EQ(101, output_events[1].first.engine_id);
  EXPECT_EQ(1, output_events[1].second.size());
  if (output_events[1].second.size() == 1) {
    EXPECT_EQ(2, GameEventFactory::GetGameEventType(output_events[1].second[0]));
  }
  BarEvent* bar = (BarEvent*)output_events[1].second[0];
  EXPECT_EQ(125, bar->GetData()->wingding());
  EXPECT_EQ(7, bar->GetData()->barbaz());
  
  delete in;
  delete out;
}

// Just like the test above, except that the packages are queued in the opposite order.  This
// reveals a bug that happened in the connections at one point.
TEST(GameConnectionTest, TestConnectionCanSendAndReceiveEventsRedux) {
  TestConnection* in = new TestConnection();
  TestConnection* out = new TestConnection();
  in->SetOutput(out);
  out->SetOutput(in);

  BarEvent* e1 = NewBarEvent();
  e1->GetData()->set_wingding(1234);
  e1->GetData()->set_barbaz(2345);
  FooEvent* e2 = NewFooEvent();
  e2->GetData()->set_foo(23456789);
  e2->GetData()->set_bar(0);
  BarEvent* e3 = NewBarEvent();
  e3->GetData()->set_wingding(125);
  e3->GetData()->set_barbaz(7);
  vector<GameEvent*> v;
  v.push_back(e3);
  in->QueueEvents(0, EventPackageID(2,101), v);
  v.resize(0);
  v.push_back(e1);
  v.push_back(e2);
  in->QueueEvents(0, EventPackageID(1,100), v);
  in->SendEvents(0);
  delete e1;
  delete e2;
  delete e3;

  vector<pair<EventPackageID, vector<GameEvent*> > > output_events;
  out->ReceiveEvents(&output_events);
  ASSERT_EQ(2, output_events.size());

  EXPECT_EQ(2,   output_events[0].first.state_timestep);
  EXPECT_EQ(101, output_events[0].first.engine_id);
  EXPECT_EQ(1, output_events[0].second.size());
  if (output_events[0].second.size() == 1) {
    EXPECT_EQ(2, GameEventFactory::GetGameEventType(output_events[0].second[0]));
  }
  BarEvent* bar = (BarEvent*)output_events[0].second[0];
  EXPECT_EQ(125, bar->GetData()->wingding());
  EXPECT_EQ(7, bar->GetData()->barbaz());
  
  EXPECT_EQ(1,   output_events[1].first.state_timestep);
  EXPECT_EQ(100, output_events[1].first.engine_id);
  EXPECT_EQ(2, output_events[1].second.size());
  if (output_events[1].second.size() == 2) {
    EXPECT_EQ(2, GameEventFactory::GetGameEventType(output_events[1].second[0]));
    BarEvent* bar = (BarEvent*)output_events[1].second[0];
    EXPECT_EQ(1234, bar->GetData()->wingding());
    EXPECT_EQ(2345, bar->GetData()->barbaz());
    EXPECT_EQ(1, GameEventFactory::GetGameEventType(output_events[1].second[1]));
  }

  delete in;
  delete out;
}

// Queues events on two channels so that they should arrive in the opposite order they are queued.
TEST(GameConnectionTest, TestConnectionsCanSendOneChannelAtATime) {
  TestConnection* in = new TestConnection();
  TestConnection* out = new TestConnection();
  in->SetOutput(out);
  out->SetOutput(in);

  BarEvent* e1 = NewBarEvent();
  e1->GetData()->set_wingding(1234);
  e1->GetData()->set_barbaz(2345);
  in->QueueEvents(1, EventPackageID(2,101), vector<GameEvent*>(1,e1));
  delete e1;

  FooEvent* e2 = NewFooEvent();
  e2->GetData()->set_foo(23456789);
  e2->GetData()->set_bar(0);
  in->QueueEvents(0, EventPackageID(2,100), vector<GameEvent*>(1,e2));
  delete e2;

  in->SendEvents(0);
  vector<pair<EventPackageID, vector<GameEvent*> > > output_events;
  out->ReceiveEvents(&output_events);
  ASSERT_EQ(1, output_events.size());
  EXPECT_EQ(2,   output_events[0].first.state_timestep);
  EXPECT_EQ(100, output_events[0].first.engine_id);
  EXPECT_EQ(1, output_events[0].second.size());
  if (output_events[0].second.size() == 1) {
    EXPECT_EQ(1, GameEventFactory::GetGameEventType(output_events[0].second[0]));
  }
  
  in->SendEvents(1);
  output_events.clear();
  out->ReceiveEvents(&output_events);
  ASSERT_EQ(1, output_events.size());
  EXPECT_EQ(2,   output_events[0].first.state_timestep);
  EXPECT_EQ(101, output_events[0].first.engine_id);
  EXPECT_EQ(1, output_events[0].second.size());
  if (output_events[0].second.size() == 1) {
    EXPECT_EQ(2, GameEventFactory::GetGameEventType(output_events[0].second[0]));
  }
  
  delete in;
  delete out;
}

TEST(GameConnectionTest, TestSendingAndReceivingVariousEngineIDsAndStateTimesteps) {
  TestConnection* in = new TestConnection();
  TestConnection* out = new TestConnection();
  in->SetOutput(out);
  out->SetOutput(in);

  int v[] = {0, 1, 2, 3, 254, 255, 256, 511, 512, 513, 65535, 65536, 65537, 2147483647};
  vector<int> values(v, v + 14);
  for (int i = 0; i < values.size(); i++) {
    for (int j = 0; j < values.size(); j++) {
      StateTimestep state_timestep = values[i];
      EngineID engine_id = values[j];
      vector<GameEvent*> v;
      in->QueueEvents(0, EventPackageID(state_timestep, engine_id), v);
      in->SendEvents(0);
      vector<pair<EventPackageID, vector<GameEvent*> > > output_events;
      out->ReceiveEvents(&output_events);
      ASSERT_EQ(1, output_events.size());
      EXPECT_EQ(state_timestep, output_events[0].first.state_timestep);
      EXPECT_EQ(engine_id, output_events[0].first.engine_id);
    }
  }

  delete in;
  delete out;
}

TEST(GameConnectionTest, TestNetworkConnections) {
  MockRouter router;
  MockNetworkManager nm1(&router);
  MockNetworkManager nm2(&router);
  
  nm1.Startup(65000);
  nm1.StartHosting("host 1");

  nm2.Startup(65001);
  nm2.StartHosting("host 2");
  
  int the_time = system()->GetTime();
  while ((nm1.AvailableHosts().size() == 0 || nm2.AvailableHosts().size() == 0)
      && the_time + 250 > system()->GetTime()) {
    nm1.Think();
    nm2.Think();
    system()->Sleep(1);
    nm1.FindHosts(65001);
    nm2.FindHosts(65000);
  }

  vector<pair<GlopNetworkAddress, string> > ah1 = nm1.AvailableHosts();
  EXPECT_EQ(1, ah1.size());
  
  vector<pair<GlopNetworkAddress, string> > ah2 = nm2.AvailableHosts();
  EXPECT_EQ(1, ah2.size());

  nm1.Connect(ah1[0].first);
  nm2.Connect(ah2[0].first);
  the_time = system()->GetTime();
  while ((nm1.GetConnections().size() == 0 || nm2.GetConnections().size() == 0)
      && the_time + 250 > system()->GetTime()) {
    nm1.Think();
    nm2.Think();
  }
  EXPECT_EQ(1, nm1.GetConnections().size());
  EXPECT_EQ(1, nm2.GetConnections().size());

  GameConnection* p1 = new PeerConnection(&nm1, nm1.GetConnections()[0]);
  GameConnection* p2 = new PeerConnection(&nm2, nm2.GetConnections()[0]);

  int v[] = {0, 1, 2, 3, 254, 255, 256, 511, 512, 513, 65535, 65536, 65537, 2147483647};
  vector<int> values(v, v + 14);
  for (int i = 0; i < values.size(); i++) {
    for (int j = 0; j < values.size(); j++) {
      StateTimestep t = values[i];
      EngineID e = values[j];
      vector<GameEvent*> v;
      p1->QueueEvents(0, EventPackageID(t,e), v);
      p1->SendEvents(0);
      the_time = system()->GetTime();
      vector<pair<EventPackageID, vector<GameEvent*> > > output_events;
      while (output_events.size() == 0 && the_time + 250 > system()->GetTime()) {
        p2->ReceiveEvents(&output_events);
        nm1.Think();
        nm2.Think();
      }
      ASSERT_EQ(1, output_events.size());
      EXPECT_EQ(t, output_events[0].first.state_timestep);
      EXPECT_EQ(e, output_events[0].first.engine_id);
    }
  }

  delete p1;
  delete p2;
}

