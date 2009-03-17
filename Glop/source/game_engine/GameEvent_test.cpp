#include <gtest/gtest.h>
#include "GameEvent.h"
#include <stdio.h>
#include "TestProtos.pb.h"

// TODO: Still need to test things like
// * Return values from ApplyToGameState get freed if they are non-null
// * Cosmetic effects, and everything related to them

class GameEventTestResult : public GameEventResult {
 public:
  GameEventTestResult(int _val) : val(_val) {}
  int val;
};

class FooEvent : public GameEvent {
 public:
  FooEvent() {
    data_ = new Foo;
  }
  virtual GameEventResult* ApplyToGameState(GameState* state) const {
    return new GameEventTestResult(1);
  }
  Foo* GetData() {
    Foo* f = static_cast<Foo*>(data_);
    return f;
  }
};
REGISTER_EVENT(128, FooEvent);

class BarEvent : public GameEvent {
 public:
   BarEvent() {
     data_ = new Bar;
   }
  virtual GameEventResult* ApplyToGameState(GameState* state) const {
    return new GameEventTestResult(2);
  }
  Bar* GetData() {
    return static_cast<Bar*>(data_);
  }
};
REGISTER_EVENT(129, BarEvent);

class NegativeEvent : public GameEvent {
 public:
   NegativeEvent() {
     data_ = new Foo;
   }
   Foo* GetData() {
     Foo* f = static_cast<Foo*>(data_);
     return f;
   }
};
REGISTER_EVENT(-100, NegativeEvent);


TEST(GameEventTest, TestFactoryGeneratesTheCorrectGameEvents) {
  FooEvent* foo = NewFooEvent();
  GameEventTestResult* foo_result =
      static_cast<GameEventTestResult*>(foo->ApplyToGameState(NULL));
  ASSERT_TRUE(foo_result != NULL);
  EXPECT_EQ(1, foo_result->val);

  BarEvent* bar = NewBarEvent();
  GameEventTestResult* bar_result =
      static_cast<GameEventTestResult*>(bar->ApplyToGameState(NULL));
  ASSERT_TRUE(bar_result != NULL);
  EXPECT_EQ(2, bar_result->val);
}

TEST(GameEventTest, TestEventsSerializeAndDeserializeCorrectly) {
  FooEvent* foo_event = NewFooEvent();
  Foo* foo_proto = foo_event->GetData();
  foo_proto->set_foo(123);
  foo_proto->set_bar(22);

  string s;
  GameEventFactory::Serialize(foo_event, &s);
  GameEvent* event = GameEventFactory::Deserialize(s);
  EXPECT_EQ(128, event->type());
  const Foo& foo_proto2 = static_cast<const Foo&>(event->GetData());

  EXPECT_TRUE(foo_proto->has_foo());
  EXPECT_EQ(foo_proto->has_foo(), foo_proto2.has_foo());
  if (foo_proto->has_foo()) {
    EXPECT_EQ(foo_proto->foo(), foo_proto2.foo());
  }
  EXPECT_TRUE(foo_proto->has_bar());
  EXPECT_EQ(foo_proto->has_bar(), foo_proto2.has_bar());
  if (foo_proto->has_bar()) {
    EXPECT_EQ(foo_proto->bar(), foo_proto2.bar());
  }
}

TEST(GameEventTest, TestEventsSerializeAndDeserializeNegativeValuedEventTypesCorrectly) {
  NegativeEvent* negative_event = NewNegativeEvent();
  Foo* foo_proto = negative_event->GetData();
  foo_proto->set_foo(123);
  foo_proto->set_bar(22);

  string s;
  GameEventFactory::Serialize(negative_event, &s);
  GameEvent* event = GameEventFactory::Deserialize(s);
  EXPECT_EQ(-100, event->type());
}
