#ifndef GAMEENGINE_GAMEENGINE_H
#define GAMEENGINE_GAMEENGINE_H

#include <vector>
#include <map>
#include <set>
using namespace std;

#include "P2PNG.h"
#include "MovingWindow.h"
#include "GameEvent.h"
#include "GameState.h"
#include "GameConnection.h"
#include "GameProtos.pb.h"
#include "../List.h"
#include "../net/Net.h"

// CORNER CASES: This is a list of problematic situations, all of which will need to be dealt with
// * Player sends out events and then is retroacively dropped by the host.  Events from that player
//   that happen after the drop event need to be purged.
// * An engine might receive events for a player that it doesn't know exists until later, when it
//   receives the AddPlayer event.  Those events need to be kept around, but shouldn't gate
//   anything. (I think this is dealt with)
// * Players might receive events from an existing player that affect a player that they don't think
//   exists yet (but does).  This might cause them to mess with data in the GameState obejct that
//   doesn't exist yet since that player hasn't yet been added.  This can probably cause crashes
//   pretty easily, and users should be aware that they need to plan for this situation.
// * In general there should be no harm done in sending events for a player that doesn't exist, they
//   should just be saved until it is clear they are not needed.
// * If we requested a new player for our engine, and we didn't receive the AddPlayer event right
//   away, we need to remember to send empty events for the timesteps we missed.

class GameConnection;
class GameState;

/// This struct maintains important information about the GameEngine that could change from frame to
/// frame.
struct GameEngineInfo {
  GameEngineInfo() : timestep(-1) { }

  /// set of the IDs of all engines currently in the game
  set<EngineID> engine_ids;

  /// timestep of this frame (possibly not needed since its in a MovingWindow?)
  int timestep;
};

enum GameEngineThinkState {
  kIdle,              // Nothing of interest is going on, might be time to destroy the engine.
  kPlaying,           // Currently playing a game, keep calling Think() to keep playing.
  kLagging,           // We're waiting on packets from another player before we can continue.
  kGameOver,          // Game *just* finished, will return kIdle afte this.
  kConnecting,        // In the process of connecting to a game.
  kJoining,           // Connected to a game and waiting to be sent all of the appropriate info.
  kReady,             // Ready to start playing, just waiting for the signal to go from the host.
  kConnectionFailed,  // Failed to connect to a game, will return kIdle after this.
};

class GameEngine;
/// This class allows tests to override the function that the GameEngine uses to determine what the
/// current frame is.
class GameEngineFrameCalculator {
 public:
  GameEngineFrameCalculator(const GameEngine& engine) : engine_(engine) { }
  virtual void GetFrame(Timestep* current_frame, Timestep* delayed_frame) const = 0;
  virtual void Set(Timestep timestep) = 0;

 protected:
  const GameEngine& engine_;
};

/// GameEngine handles all of the work for maintain a P2PNG game.  This includes making/breaking
/// connections with other GameEngines (in the same executable, on the same machine, or across the
/// internet), updating the game state with game events, backtracking the game state when events
/// arrive late, ensuring other GameEngines are synchronized with the host, and
/// stopping/slowing-down/speeding-up the game to keep in sync with other GameEngines.
class GameEngine {
 public:
  GameEngine(
      const GameState& initial_state,
      int initial_timestep,
      bool host,
      bool multithreaded,
      int max_frames,
      int ms_per_frame,
      int port);
  ~GameEngine();

  /// Returns the most accurate GameState object for the current timestep.  This will block until
  /// all pending GameEvents have been applied to the GameState history.
  const GameState& GetCurrentGameState();

  /// Returns the most recent GameState object that is completely accurate.
  const GameState& GetCompleteGameState();

  const GameState& GetSpecificGameState(Timestep);

  /// Returns the most recent Timestep for which we have a completely accurate state.
  Timestep GetCompleteTimestep();

  /// Returns the current Timestep according to the framerate calculator.
  Timestep GetCurrentTimestep();

  /// Applies an event to the current GameState, and packages it to be sent out when appropriate to
  /// other GameEngines.
  void ApplyEvent(GameEvent* event);
  void ApplyEvents(const vector<GameEvent*>& events);

  /// Used for installing new logic in the engine, primarily here for testing purposes.
  void InstallFrameCalculator(GameEngineFrameCalculator* frame_calculator);

  /// Think should be called regularly in the main loop of the game.
  GameEngineThinkState Think();



  // Starts the network manager if it is not already started.  Returns true iff after this function
  // is called the network manager is ready to rock.
  bool StartNetworkManager();

  /// Starts a NetworkManager and sets the broadcast message that is sent to anyone looking for a
  /// game.  Returns false iff there was an error.
  bool AllowIncomingConnections(const string& message);

  /// Ignores new requests to join this game.  Connections that have already been established and
  /// put into ready_connections_ remain in tact.  Returns false iff there was an error.
  bool DisallowIncomingConnections();

  /// Queries for all hosts that are hosting on the specified port.  Available hosts will be listed
  /// in AvailableHosts() as they are discovered.
  void FindHosts(int port);

  /// Clears out the list returned by AvailableHosts()
  void ClearHosts();

  /// Returns a list of hosts that were found with the FindHosts method.  Each host is represented
  /// by the pair (host's network address, host's message)
  vector<pair<GlopNetworkAddress, string> > AvailableHosts() const;

  /// Establish a connection with a host at the specified network address and sends along a message
  /// with it.
  void Connect(GlopNetworkAddress gna, const string& message);





  // Accessors
  int ms_per_frame() const { return ms_per_frame_; }
  int ms_delay() const { return ms_delay_; }

 private:
  friend class GameEvent;

  // Sub-Think methods
  void ThinkPlaying();
  void ThinkConnecting();
  void ThinkLagging();
  void ThinkJoining();
  void ThinkNetworking();

  // Sends out all events for all timesteps up to current_timestep
  void SendEvents(Timestep current_timestep);

  void RecreateState(Timestep timestep);

  void AdvanceAsFarAsPossible(Timestep current_timestep, Timestep delayed_timestep);

/*  
  void ConnectToHost();


  void CheckForJoinRequests();
  void CheckForJoinAcknowledges();

  void CheckForJoinEvent();
*/
  // Helper method that applies a batch of events to a GameState in the appropriate order for that
  // timestep.
  void ApplyEventsToGameState(
      Timestep timestep,
      map<EngineID, vector<GameEvent*> > events,
      GameState* game_state,
      GameEngineInfo* game_engine_info);

  EngineID engine_id_;
  EngineID source_engine_id_;

  // In the case of the host, this value will be used to assign EngineIDs to new engines when they
  // join.  In the case of non-hosts, this value will be used to distinguish between different
  // engines until they have joined and received a unique EngineID from the host.
  EngineID next_game_engine_id_;

  bool host_;
  bool multithreaded_;
  int max_frames_;
  int ms_per_frame_;

  /// For the standard frame calculator, this delay is imposed on the GetDelayFrame.
  int ms_delay_;

  /// The timestep most recently reported during a call to ApplyEvent_.
  Timestep last_apply_event_timestep_;

  Timestep oldest_dirty_timestep_;


  int port_;
  List<GlopNetworkAddress> connectees_;  // List of addresses that have asked to join this game.

  /// Just an instance of the GameState we're using so we can always ParseFromString or whatever we
  /// need to do.
  GameState* reference_state_;  

  /// MovingWindows of all the important data that changes from timestep to timestep.  The windows
  /// keep just enough in them so that we can get events from as far in the future or past as they
  /// can legally happen within the engine's specs.
  MovingWindow<GameState*> game_states_;
  MovingWindow<GameEngineInfo> game_engine_infos_;
  MovingWindow<map<EngineID, vector<GameEvent*> > > game_events_;

  /// The most up-to-date data we have, although maybe not totally accurate because it might need to
  /// be rewound.
  GameState* head_;

  vector<GameConnection*> all_connections_;
  vector<GameConnection*> playing_connections_;

  /// Current state of the engine.  This value of this is always returned by Think().
  GameEngineThinkState think_state_;

  /// List of GameEvents that have been generated by this engine.
  vector<GameEvent*> local_events_;

  GameEngineFrameCalculator* frame_calculator_;

  NetworkManager* network_manager_;
  GlopNetworkAddress connection_gna_;  // gna of the host we're trying to connect to.
  string connection_message_;  // Message that we'll send as soon as we are connected.
  set<GlopNetworkAddress> connected_gnas_;

  map<Timestep, map<EngineID, vector<GameEvent*> > > game_event_buffer_;
  // While we're waiting to join we may receive a lot of game events, this will hold them until
  // we've received the whole gamestate.
};

class GameStateEvent : public GameEvent {
 public:
  GameStateEvent() {
    typed_data_ = new GameStateEventData;
    data_ = typed_data_;
  }
  virtual ~GameStateEvent() {
    delete typed_data_;
  }
  void SetData(
      const string& game_state,
      Timestep timestep,
      set<EngineID> engine_ids,
      EngineID source_engine_id,
      EngineID temporary_engine_id) {
    typed_data_->set_game_state(game_state);
    typed_data_->set_timestep(timestep);
    for (set<EngineID>::iterator it = engine_ids.begin(); it != engine_ids.end(); it++) {
      typed_data_->add_engine_ids(*it);
    }
    typed_data_->set_source_engine_id(source_engine_id);
    typed_data_->set_temporary_engine_id(temporary_engine_id++);
  }
  const GameStateEventData& GetData() {
    return *typed_data_;
  }
 private:
  GameStateEventData* typed_data_;
};

class ReadyToPlayEvent : public GameEvent {
 public:
  ReadyToPlayEvent() {
    typed_data_ = new ReadyToPlayEventData;
    typed_data_->set_origin(-1);
    data_ = typed_data_;
  }
  ~ReadyToPlayEvent() {
    delete typed_data_;
  }
  void SetData(int origin, int temporary) {
    typed_data_->set_origin(origin);
    typed_data_->set_temporary(temporary);
  }
  int origin() const {
    return typed_data_->origin();
  }
  int temporary() const {
    typed_data_->temporary();
  }
 private:
  ReadyToPlayEventData* typed_data_;
};

class NewEngineEvent : public GameEvent {
 public:
  NewEngineEvent() {
    typed_data_ = new NewEngineEventData;
    data_ = typed_data_;
  }
  ~NewEngineEvent() {
    delete typed_data_;
  }
  virtual void ApplyToGameEngineInfo(GameEngineInfo* info) const {
    info->engine_ids.insert(typed_data_->engine());
  }
  void SetData(int origin, int temporary, int engine) {
    typed_data_->set_origin(origin);
    typed_data_->set_temporary(temporary);
    typed_data_->set_engine(engine);
  }
  int origin() const {
    return typed_data_->origin();
  }
  int temporary() const {
    return typed_data_->temporary();
  }
  int engine() const {
    return typed_data_->engine();
  }
 private:
  NewEngineEventData* typed_data_;
};

/*class AddPlayerEvent : public GameEvent {
 public:
  AddPlayerEvent() {
    typed_data_ = new AddPlayerEventData;
    data_ = typed_data_;
  }
  virtual ~AddPlayerEvent() {
    delete typed_data_;
  }
  void SetPlayerID(PlayerID player_id) {
    typed_data_->set_player_id(player_id);
  }
  virtual GameEventResult* ApplyToGameState(PlayerID player_id, GameState* game_state) const {
    game_state->AddPlayer(typed_data_->player_id());
    return  NULL;
  }
  virtual void ApplyToGameEngineInfo(GameEngineInfo* info) const {
    if (info->player_ids.count(typed_data_->player_id())) {
      printf("Adding a player that already exists! (%d)\n");
    }
    info->player_ids.insert(typed_data_->player_id());
  }
 private:
  AddPlayerEventData* typed_data_;
};

class DropPlayerEvent : public GameEvent {
 public:
  DropPlayerEvent() {
    DropPlayerEventData* typed_data_ = new DropPlayerEventData;
    data_ = typed_data_;
  }
  virtual ~DropPlayerEvent() {
    delete typed_data_;
  }
  void DropPlayedID(int id) {
    typed_data_->add_player_id(id);
  }
  vector<int> GetPlayerIDs() const {
    vector<int> v(typed_data_->player_id_size());
    for (int i = 0; i < typed_data_->player_id_size(); i++) {
      v.push_back(typed_data_->player_id(i));
    }
    return v;
  }
  virtual void ApplyToGameEngineInfo(GameEngineInfo* info) const {
  }
 private:
  DropPlayerEventData* typed_data_;
};


// Hosting: For an engine to host, it will call engine.Host(message).  This will start up the hosting service of the engine and send the message to any engine that is looking for a game.
// Connection: If an engine wants to connect to another engine, first it calls engine.FindHosts().  This will search for hosts on the LAN and as they are found will become available through engine.AvailableHosts().  For each host, AvailableHosts will supply a GlopNetworkAddress along with the message that the host supplied when it called engine.Host.  Then to connect to a host, call engine.Connect(gna) with the gna of the host that you want to connect to.
// Once a connection is established the host will send a SetGameStateEvent.  From then on the host will send all events necessary for updating the GameState through that connection.  Once the client has received the SetGameStateEvent and is up-to-date enough to start playing, he will send a GoodToGoEvent.  On the NEXT event the GameEngine will be ready to receive local events through GameEngine.ApplyEvent(s).

// Calling GameEngine.Think() will return one of the following values
// PLAYING - Currently playing a game.  This will be set if a GameState is being updated on Think.
// GAMEOVER - Was PLAYING, but now the game is done.  This value will be returned once, then IDLE
//   will be returned.
// CONNECTING - Currently trying to conncet to a host engine.
// CONNECTION_FAILED - Tried to conncet to a host, but unsuccessful, this value will be returned
//   once, afterwards IDLE will be returned.
// IDLE - Nothing of interest is going on.

class AcknowledgeStartGameEvent : public GameEvent {
 public:
  AcknowledgeStartGameEvent() {
    typed_data_ = new AcknowledgeStartGameEventData;
    data_ = typed_data_;
  }
  void SetData(int timestep) {
    typed_data_->set_timestep(timestep);
  }
  virtual ~AcknowledgeStartGameEvent() {
    delete typed_data_;
  }
 private:
  AcknowledgeStartGameEventData* typed_data_;
};
*/
#endif // GAMEENGINE_GAMEENGINE_H
