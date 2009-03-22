#ifndef GAMEENGINE_GAMESTATE_H
#define GAMEENGINE_GAMESTATE_H

#include <google/protobuf/message.h>

#include <string>
using namespace std;

#include "P2PNG.h"

class GameState {
 public:
  GameState() {}
  virtual ~GameState() {}

  virtual bool Think() = 0;

  virtual GameState* Copy() const = 0;
  virtual void SerializeToString(string* data) const = 0;
  virtual void ParseFromString(const string& data) = 0;
};

#endif // GAMEENGINE_GAMESTATE_H
