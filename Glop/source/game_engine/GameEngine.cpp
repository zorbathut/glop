#include "GameEngine.h"
#include "GameEvent.h"
#include "GameState.h"

#include "GameProtos.pb.h"

#include "../Base.h"
#include "../System.h"

#include <set>
using namespace std;

REGISTER_EVENT(-1, GameStateEvent);
REGISTER_EVENT(-2, ReadyToPlayEvent);
REGISTER_EVENT(-3, NewEngineEvent);


class StandardFrameCalculator : public GameEngineFrameCalculator {
 public:
  StandardFrameCalculator(const GameEngine& engine) : GameEngineFrameCalculator(engine) {
    reference_time_ = system()->GetTime();
  }
  virtual void GetFrame(
      Timestep* current_frame,
      Timestep* delayed_frame) const {
    int current_time = system()->GetTime();
    // We add the delay to the current frame rather than subtracting it from delayed frame so that
    // we don't ever get a negative frame number when the engine starts up.
    *current_frame = (current_time - reference_time_ + engine_.ms_delay()) / engine_.ms_per_frame();
    *delayed_frame = (current_time - reference_time_) / engine_.ms_per_frame();
  }
  virtual void Set(Timestep timestep) {
    reference_time_ = system()->GetTime() - engine_.ms_per_frame() * timestep;
  }
 private:
  int reference_time_;
};

GameEngine::GameEngine(
    const GameState& initial_state,
    int initial_timestep,
    bool host,
    bool multithreaded,
    int max_frames,
    int ms_per_frame,
    int port)
    : source_engine_id_(-1),
      host_(host),
      multithreaded_(multithreaded),
      max_frames_(max_frames),
      ms_per_frame_(ms_per_frame),
      last_apply_event_timestep_(0),
      oldest_dirty_timestep_(0),
      network_manager_(NULL),
      port_(port),
      reference_state_(initial_state.Copy()),
      game_states_(max_frames_ + 1, initial_timestep - 1),
      game_events_(max_frames_ * 2 + 1, initial_timestep - 1),
      game_engine_infos_(max_frames_ + 1, initial_timestep - 1),
      frame_calculator_(new StandardFrameCalculator(*this)),
      next_game_engine_id_(1) {
        printf("Here\n");
  /// \todo jwills - There should probably be functionality for a default value in MovingWindow
  for (Timestep t = game_states_.GetFirstIndex(); t < game_states_.GetLastIndex(); t++) {
    game_states_[t] = NULL;
  }

  ms_delay_ = 25;

  game_states_[initial_timestep - 1] = reference_state_->Copy();
  game_engine_infos_[initial_timestep - 1].timestep = initial_timestep - 1;
//  all_connections_.push_back(self_connection_);
//  playing_connections_.push_back(self_connection_);

  if (host_) {
    think_state_ = kPlaying;
    engine_id_ = 0;
    game_engine_infos_[initial_timestep - 1].engine_ids.insert(0);
    game_events_[initial_timestep - 1][0].clear();
  } else {
    think_state_ = kIdle;
    engine_id_ = 1;  // A positive value indicates that this engine hasn't joined a game yet
  }
}
/// \todo jwills - Might want to consider making some generic DeleteSTL things like google has.
/// \todo jwills - This destructor actually needs to clean things up.
GameEngine::~GameEngine() {
  delete frame_calculator_;
  delete network_manager_;
}

// TODO: Shouldn't this just be in the constructor?  Why would you ever change it mid-game?
void GameEngine::InstallFrameCalculator(GameEngineFrameCalculator* frame_calculator) {
  delete frame_calculator_;
  frame_calculator_ = frame_calculator;
}

void GameEngine::SendEvents(Timestep current_timestep) {
  for (Timestep t = last_apply_event_timestep_; t <= current_timestep; t++) {
    game_events_[t][engine_id_] = local_events_;
    if (t < oldest_dirty_timestep_) {
      oldest_dirty_timestep_ = t;
    }
    for (int i = 0; i < all_connections_.size(); i++) {
      all_connections_[i]->SendEvents(EventPackageID(t, engine_id_), local_events_);
    }
    local_events_.clear();
  }
  last_apply_event_timestep_ = current_timestep + 1;
}

void GameEngine::ApplyEvent(GameEvent* event) {
  Timestep accurate = GetCompleteTimestep();
  Timestep current_timestep;
  Timestep delayed_timestep;
  frame_calculator_->GetFrame(&current_timestep, &delayed_timestep);
  SendEvents(current_timestep);
  local_events_.push_back(event);
}

void GameEngine::ApplyEvents(const vector<GameEvent*>& events) {
  Timestep accurate = GetCompleteTimestep();
  Timestep current_timestep;
  Timestep delayed_timestep;
  frame_calculator_->GetFrame(&current_timestep, &delayed_timestep);
  SendEvents(current_timestep);
  for (int i = 0; i < events.size(); i++) {
    local_events_.push_back(events[i]);
  }
}

const GameState& GameEngine::GetCompleteGameState() {
  return *game_states_[GetCompleteTimestep()];
}

const GameState& GameEngine::GetCurrentGameState() {
  return *head_;
}

const GameState& GameEngine::GetSpecificGameState(Timestep t) {
  return *game_states_[t];
}

// TODO: The complete timestep should just be KNOWN at all times and this should just be an accessor
// method.  It should be kept track of wherever we end up putting calls to Finalize.
Timestep GameEngine::GetCompleteTimestep() {
  Timestep timestep = game_engine_infos_.GetFirstIndex();
  bool complete = true;
  for (timestep = game_engine_infos_.GetFirstIndex(); complete; timestep++) {
    int count = 0;
    for (set<EngineID>::iterator
        it = game_engine_infos_[timestep].engine_ids.begin();
        it != game_engine_infos_[timestep].engine_ids.end();
        it++) {
      if (!game_events_[timestep].count(*it)) {
        complete = false;
      }
    }
    if (!game_events_[timestep].count(0)) {
      complete = false;
    }
    if (game_engine_infos_[timestep].timestep != timestep) {
      complete = false;
    }
    if (!complete) {
      return timestep - 1;
    }
  }
  ASSERT(!"Didn't find a complete timestep.");
  return -2;
}

Timestep GameEngine::GetCurrentTimestep() {
  Timestep current_timestep;
  Timestep delayed_timestep;
  frame_calculator_->GetFrame(&current_timestep, &delayed_timestep);
  return current_timestep;
}

void GameEngine::RecreateState(Timestep timestep) {
  if (game_states_[timestep] != NULL) {
    delete game_states_[timestep];
  }
  game_states_[timestep] = game_states_[timestep - 1]->Copy();
  game_states_[timestep]->Think();
  game_engine_infos_[timestep] = game_engine_infos_[timestep - 1];
  game_engine_infos_[timestep].timestep = timestep;

  ApplyEventsToGameState(
      timestep,
      game_events_[timestep],
      game_states_[timestep],
      &game_engine_infos_[timestep]);
}

// This method advances GameEngine's various MovingWindows as far ahead as they can safely be
// advanced.  Specifically it will not advance past the most recent fully-completed state, because
// we might need to copy from that again in the future.
// TODO: why does this take parameters if it isn't using them?
void GameEngine::AdvanceAsFarAsPossible(Timestep current_timestep, Timestep delayed_timestep) {
  Timestep end = GetCompleteTimestep();
  while (game_engine_infos_.GetFirstIndex() < end - 2) {
    delete game_states_[game_states_.GetFirstIndex()];
    game_states_.Advance();

    game_engine_infos_.Advance();

    map<EngineID, vector<GameEvent*> > events = game_events_[game_events_.GetFirstIndex()];
    map<EngineID, vector<GameEvent*> >::iterator it;
    for (it = events.begin(); it != events.end(); it++) {
      for (int i = 0; i < it->second.size(); i++) {
        delete it->second[i];
      }
    }
    game_events_.Advance();
  }
}

void GameEngine::ApplyEventsToGameState(
    Timestep timestep,
    map<EngineID, vector<GameEvent*> > events,
    GameState* game_state,
    GameEngineInfo* game_engine_info) {

  // For simplicity, we always apply game engine events (ID < 0) in the order of engine id
  for (map<EngineID, vector<GameEvent*> >::iterator it = events.begin(); it != events.end(); it++) {
    for (int j = 0; j < it->second.size(); j++) {
      /// \todo jwills - Should probably check that these events are coming from the host?
      if (it->second[j]->type() < 0) {
        it->second[j]->ApplyToGameEngineInfo(game_engine_info);
      }
    }
  }

  // events is sorted by PlayerID, but to be fair we don't always apply the first player's events
  // first.  Instead we start with an index that may or may not be the first player, then we just
  // loop through all of the indices in order from there, wrapping around at the end.  It could be
  // more fair, but this is dead simple and probably good enough.
  // TODO: Don't forget to make a test to ensure this fair ordering actually happens
  vector<EngineID> ids;
  for (map<EngineID, vector<GameEvent*> >::iterator it = events.begin(); it != events.end(); it++) {
    ids.push_back(it->first);
  }
  int total = 0;
  for (int i = 0; i < events.size(); i++) {
    int index = (i + timestep) % events.size();
    for (int j = 0; j < events[ids[index]].size(); j++) {
      if (events[ids[index]][j]->type() > 0) {
        events[ids[index]][j]->ApplyToGameState(game_state);
        total++;
      }
    }
  }
}

void GameEngine::ThinkPlaying() {
  ASSERT(game_states_.GetFirstIndex() == game_engine_infos_.GetFirstIndex());
  ASSERT(game_states_.GetFirstIndex() == game_events_.GetFirstIndex());
  Timestep current_timestep;
  Timestep delayed_timestep;
  frame_calculator_->GetFrame(&current_timestep, &delayed_timestep);

  if (think_state_ == kPlaying) {
    SendEvents(current_timestep);
  }


  // First go through all of the connections and get any new events that are available and add them
  // game_events_.  Keep track of the oldest timestep for which we have received new events, this is
  // the one that we'll have to rewind to.
  for (int i = 0; i < playing_connections_.size(); i++) {
    vector<pair<EventPackageID, vector<GameEvent*> > > events;
    playing_connections_[i]->ReceiveEvents(&events);

    // Special processing for certain engine-level events.  This should probably be migrated to its
    // own method.
    for (int j = 0; j < events.size(); j++) {
      for (int k = 0; k < events[j].second.size(); k++) {
        if (events[j].second[k]->type() == -2) {
          assert(events[j].second.size() == 1); // This event should always be by itself

          // This event can come from any time, so we set it to our current timestep so that we
          // don't put it into a place in our history we've already forgotten about.
          events[j].first.timestep = current_timestep;
          ReadyToPlayEvent* r2p = (ReadyToPlayEvent*)events[j].second[k];
          if (host_) {
            NewEngineEvent* nen = NewNewEngineEvent();
            nen->SetData(r2p->origin(), r2p->temporary(), next_game_engine_id_++);
            ApplyEvent(nen);
          }
        }

        if (events[j].second[k]->type() == -3) {
          NewEngineEvent* nen = (NewEngineEvent*)events[j].second[k];
          if (nen->origin() == source_engine_id_ && nen->temporary() == engine_id_) {
            engine_id_ = nen->engine();
            think_state_ = kPlaying;
            last_apply_event_timestep_ = events[j].first.timestep;
          }
        }
      }

      // NOTE: This check must be at the end of this block because in the event of a ReadyToPlay
      // event we actually modify the timestep of the event package.
      if (events[j].first.timestep < oldest_dirty_timestep_) {
        oldest_dirty_timestep_ = events[j].first.timestep;
      }
    }

    // With a tree connection graph, all we have to do is take all incoming events and send them to
    // all of our other connections.
    // TODO: If we ever change to any graph that isn't a tree this won't work and we'll actually
    // need to keep track of who we should send which events to,
    for (int j = 0; j < all_connections_.size(); j++) {
      if (all_connections_[j] == playing_connections_[i]) { continue; }
      for (int k = 0; k < events.size(); k++) {
        all_connections_[j]->SendEvents(events[k].first, events[k].second);
        printf("Engine %d sent events through connection %d for engine %d and timestep %d\n", engine_id_, j, events[k].first.engine_id, events[k].first.timestep);
      }
    }
    for (int j = 0; j < events.size(); j++) {
      EngineID engine_id = events[j].first.engine_id;
      Timestep timestep = events[j].first.timestep;
      // In the case that the only events in a package are ReadyToPlay events, then we ignore that
      // package for the purposes of updating oldest_dirty_timestep_, since it won't actually change
      // anything, and can even cause a crash on the hosting engine.
      bool valid = events[j].second.size() == 0;
      for (int k = 0; k < events[j].second.size(); k++) {
        if (events[j].second[k]->type() != -2) {
          valid = true;
        }
      }
      if (!valid) { continue; }
      // Check that we haven't already received events on this timestep for this player
      if (game_events_[timestep].count(engine_id)) {
        printf("Engine %d has received a second batch of events from engine %d\n", engine_id_, engine_id);
        printf("Sizes (prev/cur): %d/%d\n", game_events_[timestep][engine_id].size(), events[j].second.size());
        // TODO: Maybe this is because of duplicated packets?  Investigate more, we might just be
        // able to ignore this when it happens to long as we get the same packets each time.
        assert(!game_events_[timestep].count(engine_id));
      }
      game_events_[timestep][engine_id] = events[j].second;
    }
  }

  for (Timestep t = oldest_dirty_timestep_; t <= delayed_timestep; t++) {
    RecreateState(t);
  }
  AdvanceAsFarAsPossible(current_timestep, delayed_timestep);
  oldest_dirty_timestep_ = delayed_timestep + 1;

  // We only go up to delayed_timestep here so that we have a little extra time to get everyone
  // else's events for the head frame before having to call think on it.  This prevents us from
  // having to run GameState.Think() more than once on any frame so long as we receive all events
  // for a frame before delayed_frame = current_frame.
  head_ = game_states_[delayed_timestep];
}

void GameEngine::ThinkLagging() {
  
}

GameEngineThinkState GameEngine::Think() {
  ThinkNetworking();
  switch (think_state_) {
    case kIdle:
    case kGameOver:
    case kConnectionFailed: {
      think_state_ = kIdle;
      break;
    }
    case kReady:
    case kPlaying: {
      ThinkPlaying();
      break;
    }
    case kLagging: {
      ThinkLagging();
      break;
    }
    case kConnecting: {
      ThinkConnecting();
      break;
    }
    case kJoining: {
      ThinkJoining();
      break;
    }
  }
  return think_state_;

//  CheckForJoinRequests();
//  CheckForJoinAcknowledges();
}


// NETWORKING SCHTUFF
void GameEngine::ThinkNetworking() {
  if (network_manager_ == NULL) {
    return;
  }
  network_manager_->Think();

  // TODO: This should be in a different function entireyly
  if (think_state_ != kPlaying) {
    return;
  }
  // TODO: Should also notice when conncetions are lost so we can remove those Peers
  vector<GlopNetworkAddress> connections = network_manager_->GetConnections();
  for (int i = 0; i < connections.size(); i++) {
    if (!connected_gnas_.count(connections[i])) {

      GameConnection* peer = new PeerConnection(network_manager_, connections[i]);
      all_connections_.push_back(peer);
      playing_connections_.push_back(peer);

      // We got a new connection, we start by sending them the latest fully-completed game state
      // that we have
      Timestep timestep = GetCompleteTimestep();
      GameStateEvent* gse = NewGameStateEvent();
      string serialized_game_state;
      // TODO: Consider allowing a constructor for game events that uses a protobuf that the event
      // takes constrol of, that would prevent one case of copying this data around.
      game_states_[timestep]->SerializeToString(&serialized_game_state);


      gse->SetData(
          serialized_game_state,
          timestep,
          game_engine_infos_[timestep].engine_ids,
          engine_id_,
          next_game_engine_id_++);
      peer->SendEvents(EventPackageID(timestep-1, engine_id_), vector<GameEvent*>(1, gse));
      
      // Now we have to send any events that we have that happened on or after that timestep
      for (Timestep t = timestep; t <= game_events_.GetLastIndex(); t++) {
        map<EngineID, vector<GameEvent*> >::iterator it;
        for (it = game_events_[t].begin(); it != game_events_[t].end(); it++) {
          peer->SendEvents(EventPackageID(t, it->first), it->second);
        }
      }

      connected_gnas_.insert(connections[i]);
    }
  }
}

bool GameEngine::StartNetworkManager() {
  if (network_manager_ != NULL) { return true; }
  network_manager_ = new NetworkManager();
  if (!network_manager_->Startup(port_)) {
    delete network_manager_;
    network_manager_ = NULL;
    return false;
  }
  return true;
}

bool GameEngine::AllowIncomingConnections(const string& message) {
  if (!StartNetworkManager()) { return false; }
  network_manager_->StartHosting(message);
  return true;
}

bool GameEngine::DisallowIncomingConnections() {
  if (!StartNetworkManager()) { return false; }
  network_manager_->StopHosting();
  return true;
}

void GameEngine::FindHosts(int port) {
  if (network_manager_ == NULL) { return; }
  network_manager_->FindHosts(port);
}

void GameEngine::ClearHosts() {
  if (network_manager_ == NULL) { return; }
  network_manager_->ClearHosts();
}

vector<pair<GlopNetworkAddress, string> > GameEngine::AvailableHosts() const {
  if (network_manager_ == NULL) { return vector<pair<GlopNetworkAddress, string> >(); }
  return network_manager_->AvailableHosts();
}

// TODO: Currently connect doesn't fail, it will just always be waiting to succeed, should have a
// time limit of some kind though
void GameEngine::Connect(GlopNetworkAddress gna, const string& message) {
  if (network_manager_ == NULL) { return; }
  network_manager_->Connect(gna);
  connection_gna_ = gna;
  connection_message_ = message;
  think_state_ = kConnecting;
}

void GameEngine::ThinkConnecting() {
  vector<GlopNetworkAddress> gnas = network_manager_->GetConnections();
  for (int i = 0; i < gnas.size(); i++) {
    if (gnas[i] == connection_gna_) {
//      network_manager_->SendData(connection_gna_, connection_message_);
      think_state_ = kJoining;
      GameConnection* peer = new PeerConnection(network_manager_, connection_gna_);
      connected_gnas_.insert(connection_gna_);
      all_connections_.push_back(peer);
      playing_connections_.push_back(peer);
      game_event_buffer_.clear();  // Just in case.
      return;
    }
  }
}

void GameEngine::ThinkJoining() {
  ASSERT(all_connections_.size() == 1);
  ASSERT(playing_connections_.size() == 1);

  vector<pair<EventPackageID, vector<GameEvent*> > > events;
  all_connections_[0]->ReceiveEvents(&events);
  GameStateEvent* gse = NULL;
  for (int i = 0; i < events.size(); i++) {
    game_event_buffer_[events[i].first.timestep][events[i].first.engine_id] = events[i].second;
    for (int j = 0; j < events[i].second.size(); j++) {
      if (events[i].second[j]->type() == -1) {
        gse = (GameStateEvent*)events[i].second[j];
      }
    }
  }
  if (gse != NULL) {
    const GameStateEventData& data = gse->GetData();
    source_engine_id_ = data.source_engine_id();
    game_states_ = MovingWindow<GameState*>(max_frames_ + 1, data.timestep());
    game_engine_infos_ = MovingWindow<GameEngineInfo>(max_frames_ + 1, data.timestep());
    game_events_ =
        MovingWindow<map<EngineID, vector<GameEvent*> > >(max_frames_ * 2 + 1, data.timestep());

    game_states_[data.timestep()] = reference_state_->Copy();
    game_states_[data.timestep()]->ParseFromString(data.game_state());

    // TODO: VERY IMPORTANT: Right now we're assuming that we can get the whole gamestate event
    // within max_frames_, but this can't be something we rely on in practice.
    for (int i = 0; i < data.engine_ids_size(); i++) {
      game_events_[data.timestep()][data.engine_ids(i)].clear();
      game_engine_infos_[data.timestep()].engine_ids.insert(data.engine_ids(i));
      game_engine_infos_[data.timestep()].timestep = data.timestep();
    }

    engine_id_ = gse->GetData().temporary_engine_id();

//    void SendEvents(EventPackageID id, const vector<GameEvent*>& events);
    map<Timestep, map<EngineID, vector<GameEvent*> > >::iterator it;
    for (it = game_event_buffer_.begin(); it != game_event_buffer_.end(); it++) {
      map<EngineID, vector<GameEvent*> >::iterator xit;
      if (it->first <= data.timestep()) { continue; }
      for (xit = it->second.begin(); xit != it->second.end(); xit++) {
        game_events_[it->first][xit->first] = xit->second;
      }
    }
    it = game_event_buffer_.end();
    it--;
    // Advancing potentially as far as the most recent event we have.  This is the best we can do
    // since we haven't event set our FramerateCalculator yet.
    for (Timestep t = data.timestep() + 1; t <= it->first; t++) {
      RecreateState(t);
    }
    AdvanceAsFarAsPossible(it->first, it->first);
    Timestep complete = GetCompleteTimestep();

    if (it->first - complete > max_frames_) {
      printf("WE ARE IN BIIIIIG TROUBLE\n");
    }
    ReadyToPlayEvent* r2p = NewReadyToPlayEvent();
    r2p->SetData(source_engine_id_, engine_id_);
    all_connections_[0]->SendEvents(
        EventPackageID(complete, (unsigned int)-1),
        vector<GameEvent*>(1, r2p));
    oldest_dirty_timestep_ = complete + 1;
    frame_calculator_->Set(complete + 1);
    think_state_ = kReady;
  }
}

