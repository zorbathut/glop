#include "GameEvent.h"
#include "../Base.h"

Mutex* GameEventFactory::mutex_;
map<int, GameEvent* (*)()>* GameEventFactory::event_constructors_;

void GameEventFactory::Serialize(const GameEvent* event, string* str) {
  ASSERT(str->size() == 0);
  str->resize(4);
  (*str)[0] = ((event->type_) >>  0) & 0xff;
  (*str)[1] = ((event->type_) >>  8) & 0xff;
  (*str)[2] = ((event->type_) >> 16) & 0xff;
  (*str)[3] = ((event->type_) >> 24) & 0xff;

  if (event->type() == 0) {
    assert(false);
    printf("ZZEROREOR!n\n");
  }
 
  event->data_->AppendToString(str);
}

GameEvent* GameEventFactory::Deserialize(const string& str) {
  if (str.size() < 4) {
    printf("%d %d %d\n", str[0], str[1], str[2]);
    printf("Tried to deserialize a string of lenght %d\n", str.size());
    assert(false);
  }
  int type = 0;
  type |= ((unsigned char)str[0]) <<  0;
  type |= ((unsigned char)str[1]) <<  8;
  type |= ((unsigned char)str[2]) << 16;
  type |= ((unsigned char)str[3]) << 24;
  GameEvent* event = GetEventByType(type);

  /// \todo jwills - This could suck if we don't have a copy-on-write implementation of string, look
  /// into proto buffers a bit more and see if there is some way to parse from an arbitrary point in
  /// a string
  event->data_->ParseFromString(str.substr(4));
  return event;
}
