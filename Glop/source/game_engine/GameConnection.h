#ifndef GAMEENGINE_GAMECONNECTION_H
#define GAMEENGINE_GAMECONNECTION_H

#include "../net/NetworkManagerInterface.h"

#include <vector>
#include <string>
#include <map>
using namespace std;

#include "P2PNG.h"

class GameEvent;
class GamePlayer;

struct EventPackageID {
  EventPackageID(StateTimestep t, EngineID e) : state_timestep(t), engine_id(e) {}
  EventPackageID() : state_timestep(-1), engine_id(-1) {}
  StateTimestep state_timestep;
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

  /// Queues up all events for one NetTimestep/EngineID pair.  These events will get sent the next
  /// time that SendEvents() is called on this channel.
  void QueueEvents(int channel, EventPackageID id, const vector<GameEvent*>& events);

  /// Sends all events that have been queued up by QueueEvents() on this channel.
  void SendEvents(int channel);

  /// Sends all events that have been queued up by QueueEvents() on all channels.
  void SendAllEvents();

  /// Receive all available events on this connection.
  void ReceiveEvents(vector<pair<EventPackageID, vector<GameEvent*> > >* events);

 protected:
  /// Subclasses implement this function to send data to whoever is on the other end of the
  /// connection.
  virtual void SendData(const string& data) = 0;

  /// Subclasses implement this function to receive all of the data that has been sent to it all at
  /// once.
  virtual void ReceiveData(vector<string>* data) = 0; 

 private:
  void SerializeEvents(EventPackageID id, const vector<GameEvent*>& events, string* data);
  void DeserializeEvents(const string& data, EventPackageID* id, vector<GameEvent*>* events);

  // map of channel to buffer.  The buffers will be sent over the connection when the appropriate
  // send method is called.
  map<int, string>  buffers_;
};

class PeerConnection : public GameConnection {
 public:
  PeerConnection(NetworkManagerInterface* network_manager, GlopNetworkAddress gna)
      : network_manager_(network_manager),
        gna_(gna) {}
  virtual ~PeerConnection() {}

 protected:
  virtual void SendData(const string& data);
  virtual void ReceiveData(vector<string>* data);

 private:
  NetworkManagerInterface* network_manager_;
  GlopNetworkAddress gna_;
};

class SelfConnection : public GameConnection {
  public:
  SelfConnection() {}
  virtual ~SelfConnection() {}
 
  protected:
   virtual void SendData(const string& data);
   virtual void ReceiveData(vector<string>* data);
 
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
  virtual void SendData(const string& data) {
    connection_->data_.push_back(data);
  }

  virtual void ReceiveData(vector<string>* data) {
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
