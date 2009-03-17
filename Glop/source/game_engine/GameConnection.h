#ifndef GAMEENGINE_GAMECONNECTION_H
#define GAMEENGINE_GAMECONNECTION_H

#include "../net/Net.h"

#include <vector>
#include <string>
#include <map>
using namespace std;

#include "P2PNG.h"

class GameEvent;
class GamePlayer;

struct EventPackageID {
  EventPackageID(Timestep t, EngineID e) : timestep(t), engine_id(e) {}
  EventPackageID() : timestep(-1), engine_id(-1) {}
  Timestep timestep;
  EngineID engine_id;
};

/// This class handles the communication between GameEngines.  This class should be subclassed for
/// connections like NetworkConnection for connections over the internet, and ApplicationConnection
/// for connections to GameEngines on the same machine (useful for testing).  Each connection is
/// connected to exactly one other connection, to/from which data is sent.  Subclasses on this will
/// need to supply some way of creating the connection and actually associating it with another
/// connection.
class GameConnection {
 public:
  GameConnection() {};
  virtual ~GameConnection() {};

  /// Send all events for one timestep/player pair, in the order that they should be executed.
  void SendEvents(EventPackageID id, const vector<GameEvent*>& events);

  /// Receive all available events on this connection.
  void ReceiveEvents(vector<pair<EventPackageID, vector<GameEvent*> > >* events);

 protected:
  /// Subclasses implement this function to send data from this connection to the connection on the
  /// other end.
  virtual void DoSendEvents(const string& data) = 0;

  /// Subclasses implement this function to receive all of the data that has been sent to it all at
  /// once.
  virtual void DoReceiveEvents(vector<string>* data) = 0; 

 private:
  void SerializeEvents(EventPackageID id, const vector<GameEvent*>& events, string* data);
  void DeserializeEvents(const string& data, EventPackageID* id, vector<GameEvent*>* events);
};

class PeerConnection : public GameConnection {
 public:
  PeerConnection(NetworkManager* network_manager, GlopNetworkAddress gna)
      : network_manager_(network_manager),
        gna_(gna) {}
  virtual ~PeerConnection() {}

 protected:
  virtual void DoSendEvents(const string& data);
  virtual void DoReceiveEvents(vector<string>* data);

 private:
  NetworkManager* network_manager_;
  GlopNetworkAddress gna_;
};

class SelfConnection : public GameConnection {
  public:
  SelfConnection() {}
  virtual ~SelfConnection() {}
 
  protected:
   virtual void DoSendEvents(const string& data);
   virtual void DoReceiveEvents(vector<string>* data);
 
  private:
  vector<string> data_;
};
 
// Simple connection that can be used for testing basic stuff
class TestConnection : public GameConnection {
 public:
  TestConnection() {}
  virtual ~TestConnection() {}

  void SetOutput(TestConnection* connection) {
    connection_ = connection;
  }
 protected:
  virtual void DoSendEvents(const string& data) {
    connection_->data_.push_back(data);
  }

  virtual void DoReceiveEvents(vector<string>* data) {
    for (int i = 0; i < data_.size(); i++) {
      data->push_back(data_[i]);
    }
    data_.resize(0);
  }

 private:
  TestConnection* connection_;
  vector<string> data_;
};

#endif // GAMEENGINE_GAMECONNECTION_H
