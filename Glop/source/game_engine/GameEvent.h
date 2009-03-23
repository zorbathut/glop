#ifndef GAMEENGINE_GAMEEVENT_H
#define GAMEENGINE_GAMEEVENT_H

#include <google/protobuf/message.h>

#include "../Thread.h"
#include "../Base.h"
#include "P2PNG.h"

#include <string>
#include <map>
using namespace std;

class GameState;
struct GameEngineInfo;
class GameEngine;

/// \todo jwills - We could potentially make GameEventResult contain a bool that says whether or not
/// the game state was actually modified.  This might help prevent excessive computations during a
/// backtrack in certain sorts of games.
/// This serves as a way for the ApplyToGameState function to return information that can be used by
/// the *CosmeticEffects functions.
/// Why use this instead of a void* you ask?  This way we can pass this object as a const reference
/// to the GameEvent functions, guaranteeing that the no one screws it up if we ever have to
/// backtrack.
class GameEventResult {
 public:
  GameEventResult() {}
  virtual ~GameEventResult() {}
};

/// GameEvent is the only way to modify a GameState object.  Any subclass definition should be
/// immediately followed by the REGISTER_EVENT macro.  For example, after defining FooEvent,
/// REGISTER_EVENT(1, FooEvent); should be called.  The 1 passed into that macro is an ID that is
/// unique among all calls to REGISTER_EVENT.  REGISTER_EVENT will register the event with the
/// GameEventFactory, which will make GameEventFactory::Serialize and GameEventFactory::Deserialize
/// available to the registered class.  Additionally, the registered class should never be
/// instantiated except through calls to GameEventFactory::GetEventByType(type), where type is the
/// ID passed to REGISTER_EVENT, or throught the convenience function NewFooType(), which has the
/// exact same effect.
/// All data in a GameEvent object should be contained within the data_ member variable, which a
/// subclass can instantiate as any protocol buffer.
class GameEvent {
 public:
  GameEvent() : type_(0) {}
  virtual ~GameEvent() {}

  /// If a GameEvent affects the GameState in any way, it does so by overriding this function.  This
  /// function may be called potentially many times if the GameState is backtracked, so be careful
  /// of CPU intensive operations inside of this function.  The return value from the first call to
  /// this function will be passed to the call to ImmediateCosmeticEffects.  The return value from
  /// the last call to this function will be passed to AccurateCosmeticEffects.  If the return value
  /// is non-NULL, it will be freed, so do not do so in the *CosmeticEffects functions.
  virtual GameEventResult* ApplyToGameState(GameState* state) const {
    return  NULL;
  }

  /// \todo jwills - Do these functions just need the 'visible' game state, or do they also need the
  /// GameState at the timestep that the effect was applied?

  /// This function will be called the very first time that the GameEngine applies this event to any
  /// GameState.  result is the return value from that call to ApplyToGameState.  Since the game
  /// state might be backtracked, it is possible that the value of result could change in the
  /// future.  Because of this, only minor cosmetic effects that will not adversley affect a player
  /// if incorrect should happen in this function.
  virtual void ImmediateCosmeticEffects(
      GameState* state,
      const GameEventResult& result) const {};

  /// This function will be called the very last time that the GameEngine applies this event to any
  /// GameState.  This will mean that the value of result passed in is guaranteed to be correct.
  virtual void AccurateCosmeticEffects(
      GameState* state,
      const GameEventResult& result) const {};


  virtual void ApplyToGameEngineInfo(GameEngineInfo* info) const {}

  const google::protobuf::Message& GetData() const {return *data_;}

  int type() const {return type_;}

 protected:
  /// Any data contained in any subclass of GameEvent should be contained within data_.
  google::protobuf::Message* data_;  

 private:
  /// GameEventFactory needs to be a friend so that we can guarantee that it is the only thing that
  /// can set the type_ value.  This means that the only way to create valid GameEvents will be
  /// via the factory.
  friend class GameEventFactory;

  void set_type(int type) {
    type_ = type;
  }
  int type_;

  DISALLOW_EVIL_CONSTRUCTORS(GameEvent);
};

/// \todo jwills - Static initialization isn't safe.  Do something smrt here.  Specifically I think
/// this requires something akin to pthread_once_init

/// All GameEvent subclasses are registered, instantiated, serialized, and deserialized through the
/// GameEventFactory.  To register a GameEvent subclass, call the REGISTER_EVENT macro.  See the
/// comments in GameEvent for more information.
class GameEventFactory {
 public:
  /// The REGISTER_EVENT macro instantiates a static GameEventFactory so that this constructor will
  /// be called during static initialization, so that all GameEvents will be registered before we
  /// hit main().
  GameEventFactory(int event_type, GameEvent* (*event_constructor)()) {
    if (event_constructors_ == NULL) {
      event_constructors_ = new map<int, GameEvent* (*)()>;
      mutex_ = new Mutex;
    }
    MutexLock lock(mutex_);
    (*event_constructors_)[event_type] = event_constructor;
  }

  /// Registered GameEvents can be instantiated with this method by passing in the ID that was used
  /// to register the event.
  static GameEvent* GetEventByType(int event_type) {
    assert(event_constructors_ != NULL);
    assert(event_constructors_->count(event_type));
    map<int, GameEvent* (*)()>::iterator it = event_constructors_->begin();
    GameEvent* event = (*event_constructors_)[event_type]();
    event->set_type(event_type);
printf("GameEventFactory::GetEventByType(%d)\n",event_type);
    return event;
  }

  /// Serializes the event into str.  str must be empty.
  static void Serialize(const GameEvent* event, string* str);

  /// Instantiates the appropriate GameEvent subclass and deserializes str into that event.
  static GameEvent* Deserialize(const string& str);

  /// Primarily for testing to make sure that an event is of the expected type.
  static int GetGameEventType(const GameEvent* event) {return event->type_;}

 private:
  GameEventFactory() {}
  static Mutex* mutex_;
  static map<int, GameEvent* (*)()>* event_constructors_;
  DISALLOW_EVIL_CONSTRUCTORS(GameEventFactory);
};

#define REGISTER_EVENT(EVENT_TYPE, EVENT_CLASS)                                    \
inline GameEvent* __ ## EVENT_CLASS ## __create() {                                \
  return new EVENT_CLASS;                                                          \
}                                                                                  \
static GameEventFactory __ ## EVENT_CLASS ##                                       \
    __creator(EVENT_TYPE, &__ ## EVENT_CLASS ## __create);                         \
EVENT_CLASS* New ## EVENT_CLASS() {                                                \
  return static_cast<EVENT_CLASS*>(GameEventFactory::GetEventByType(EVENT_TYPE));  \
}

#endif // GAMEENGINE_GAMEEVENT_H
