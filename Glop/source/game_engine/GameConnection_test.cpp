#include <gtest/gtest.h>
#include "GameConnection.h"
#include "GameEvent.h"
#include "TestProtos.pb.h"

#include "../System.h"

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

  GameEvent* e1 = NewFooEvent();
  GameEvent* e2 = NewBarEvent();
  GameEvent* e3 = NewBarEvent();
  vector<GameEvent*> v;
  v.push_back(e1);
  v.push_back(e2);
  in->SendEvents(EventPackageID(1,100), v);
  v.resize(0);
  v.push_back(e3);
  in->SendEvents(EventPackageID(2,101), v);
  delete e1;
  delete e2;
  delete e3;

  vector<pair<EventPackageID, vector<GameEvent*> > > output_events;
  out->ReceiveEvents(&output_events);
  EXPECT_EQ(2, output_events.size());
  if (output_events.size() >= 1) {
    EXPECT_EQ(1,   output_events[0].first.timestep);
    EXPECT_EQ(100, output_events[0].first.engine_id);
    EXPECT_EQ(2, output_events[0].second.size());
    if (output_events[0].second.size() == 2) {
      EXPECT_EQ(1, GameEventFactory::GetGameEventType(output_events[0].second[0]));
      EXPECT_EQ(2, GameEventFactory::GetGameEventType(output_events[0].second[1]));
    }
  }
  if (output_events.size() >= 2) {
    EXPECT_EQ(2,   output_events[1].first.timestep);
    EXPECT_EQ(101, output_events[1].first.engine_id);
    EXPECT_EQ(1, output_events[1].second.size());
    if (output_events[1].second.size() == 1) {
      EXPECT_EQ(2, GameEventFactory::GetGameEventType(output_events[1].second[0]));
    }
  }
  
  delete in;
  delete out;
}

TEST(GameConnectionTest, TestSendingAndReceivingVariousEngineIDsAndTimesteps) {
  TestConnection* in = new TestConnection();
  TestConnection* out = new TestConnection();
  in->SetOutput(out);
  out->SetOutput(in);

  int v[] = {0, 1, 2, 3, 254, 255, 256, 511, 512, 513, 65535, 65536, 65537, 2147483647};
  vector<int> values(v, v + 14);
  for (int i = 0; i < values.size(); i++) {
    for (int j = 0; j < values.size(); j++) {
      Timestep timestep = values[i];
      EngineID engine_id = values[j];
      vector<GameEvent*> v;
      in->SendEvents(EventPackageID(timestep, engine_id), v);
      vector<pair<EventPackageID, vector<GameEvent*> > > output_events;
      out->ReceiveEvents(&output_events);
      ASSERT_EQ(1, output_events.size());
      EXPECT_EQ(timestep, output_events[0].first.timestep);
      EXPECT_EQ(engine_id, output_events[0].first.engine_id);
    }
  }

  delete in;
  delete out;
}
/*
TEST(GameConnectionTest, TestNetworkConnections) {
  NetworkManager nm1;
  NetworkManager nm2;
  
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
      Timestep t = values[i];
      PlayerID p = values[j];
      vector<GameEvent*> v;
      p1->SendEvents(EventPackageID(t,p), v);
      the_time = system()->GetTime();
      vector<pair<EventPackageID, vector<GameEvent*> > > output_events;
      while (output_events.size() == 0 && the_time + 250 > system()->GetTime()) {
        p2->ReceiveEvents(&output_events);
        nm1.Think();
        nm2.Think();
      }
      ASSERT_EQ(1, output_events.size());
      EXPECT_EQ(t, output_events[0].first.timestep);
      EXPECT_EQ(p, output_events[0].first.engine_id);
    }
  }



  delete p1;
  delete p2;
}
*/
