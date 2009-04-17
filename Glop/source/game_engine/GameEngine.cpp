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
  StandardFrameCalculator() {
    reference_time_ = system()->GetTime();
  }
  virtual int GetTime() const {
    return system()->GetTime() - reference_time_;
  }
  virtual void SetTime(int reference_time) {
    reference_time_ = system()->GetTime() - reference_time;
  }
 private:
  int reference_time_;
};

GameEngine::GameEngine(const GameState& reference)
  : think_state_(kIdle),
    engine_id_(-1),
    next_game_engine_id_(0),
    host_(false),
    frame_calculator_(new StandardFrameCalculator()),
    network_manager_(new NetworkManager()),
    networking_enabled_(false),
    reference_state_(reference.Copy()),
    num_thinks_(0),
    num_rethinks_(0) {
    
}

GameEngine::GameEngine(
    const GameState& initial_state,
    int max_frames,
    int ms_per_net_frame,
    int ms_per_state_frame,
    int ms_delay)
    : host_(true),
      max_frames_(max_frames),
      ms_per_net_frame_(ms_per_net_frame),
      ms_per_state_frame_(ms_per_state_frame),
      ms_delay_(ms_delay),
      last_queue_event_timestep_(-1),
      last_send_event_timestep_(0),
      oldest_dirty_timestep_(0),
      latest_complete_state_timestep_(-1),
      network_manager_(new NetworkManager()),
      networking_enabled_(false),
      port_(-1),
      reference_state_(initial_state.Copy()),
      game_states_(max_frames_ + 1, -1),
      game_events_(max_frames_ * 2 + 1, -1),
      game_engine_infos_(max_frames_ + 1, -1),
      frame_calculator_(new StandardFrameCalculator()),
      engine_id_(0),
      source_engine_id_(-1),
      next_game_engine_id_(1),
      num_thinks_(0),
      num_rethinks_(0) {
  /// \todo jwills - There should probably be functionality for a default value in MovingWindow
  for (StateTimestep t = game_states_.GetFirstIndex(); t < game_states_.GetLastIndex(); t++) {
    game_states_[t] = NULL;
  }

  // Just make these asserts when real asserts are available
  if (ms_per_net_frame_ < ms_per_state_frame_) {
    assert(!"ms_per_net_frame must be greater than or equal to ms_per_state_frame\n");
  }
  if (ms_delay_ >= ms_per_net_frame_) {
    assert(!"ms_delay must be less than ms_per_net_frame\n");
  }

  game_states_[-1] = reference_state_->Copy();
  game_engine_infos_[-1].state_timestep = -1;

  game_engine_infos_[-1].engine_ids.insert(0);
  game_events_[-1][0].clear();

  think_state_ = kPlaying;
}
/// \todo jwills - Might want to consider making some generic DeleteSTL things like google has.
/// \todo jwills - This destructor actually needs to clean things up.
GameEngine::~GameEngine() {
  delete frame_calculator_;
  delete network_manager_;
}

// This is here because we default to the standard calculator, but we can install our own for tests
// if we want.
void GameEngine::InstallFrameCalculator(GameEngineFrameCalculator* frame_calculator) {
  delete frame_calculator_;
  frame_calculator_ = frame_calculator;
}

void GameEngine::InstallNetworkManager(NetworkManagerInterface* manager) {
  if (network_manager_ != NULL) {
    delete network_manager_;
  }
  network_manager_ = manager;
}

void GameEngine::QueueEvents(StateTimestep current_state_timestep) {
  printf("current_state_timestep: %d\n",current_state_timestep );
  for (StateTimestep t = last_queue_event_timestep_ + 1; t <= current_state_timestep; t++) {
    game_events_[t][engine_id_] = local_events_;
    if (t < oldest_dirty_timestep_) {
      oldest_dirty_timestep_ = t;
    }
    for (int i = 0; i < all_connections_.size(); i++) {
      all_connections_[i]->QueueEvents(EventPackageID(t, engine_id_), local_events_);
    }
    local_events_.clear();
  }

  // This check shouldn't really be needed, but just in case...
//  if (current_state_timestep > last_queue_event_timestep_) {
    printf("set lqet: %d -> %d\n", last_queue_event_timestep_, current_state_timestep);
    last_queue_event_timestep_ = current_state_timestep;
//  }  
}

void GameEngine::SendEvents(NetTimestep current_net_timestep) {
  if (current_net_timestep > last_send_event_timestep_) {
  printf("Sending events for net step %d\n", current_net_timestep);
    for (int i = 0; i < all_connections_.size(); i++) {
      all_connections_[i]->SendEvents();
    }
    last_send_event_timestep_ = current_net_timestep;
  }
}

StateTimestep GameEngine::GetCurrentDelayedStateTimestep(int time_ms) {
  if (ms_delay_ == 0) {
    return time_ms / ms_per_state_frame_;
  }
  int x = time_ms + ms_per_net_frame_ - (time_ms % ms_per_net_frame_) + ms_delay_;
  int t = x + ms_per_state_frame_ - (x % ms_per_state_frame_);
  return t / ms_per_state_frame_;
}

void GameEngine::ApplyEvent(GameEvent* event) {
  QueueEvents(GetCurrentDelayedStateTimestep(frame_calculator_->GetTime() / ms_per_state_frame_));
  local_events_.push_back(event);
}

void GameEngine::ApplyEvents(const vector<GameEvent*>& events) {
  QueueEvents(GetCurrentDelayedStateTimestep(frame_calculator_->GetTime() / ms_per_state_frame_));
  for (int i = 0; i < events.size(); i++) {
    local_events_.push_back(events[i]);
  }
}

const GameState& GameEngine::GetCurrentGameState() {
  for (StateTimestep t = game_states_.GetFirstIndex() + 1; t < game_states_.GetLastIndex(); t++) {
    if (game_engine_infos_[t].state_timestep != t) {
      return *game_states_[t - 1];
    }
  }
}

/*
const GameState& GameEngine::GetCompleteGameState() {
  return *game_states_[GetCompleteStateTimestep()];
}

const GameState& GameEngine::GetSpecificGameState(StateTimestep state_timestep) {
  return *game_states_[state_timestep];
}
*/

// This method returns whether the GameState at state_timestep is complete, assuming that the
// previous GameState is complete, and that all available events in game_events_ that should be
// applied to this GameState have already been.
bool GameEngine::IsStateComplete(StateTimestep state_timestep) {
  set<EngineID>::const_iterator it;
  for (it = game_engine_infos_[state_timestep].engine_ids.begin();
       it != game_engine_infos_[state_timestep].engine_ids.end();
       it++) {
    if (!game_events_[state_timestep].count(*it)) {
//      printf("don't have %d on %d\n", *it, state_timestep);
      return false;
    }
  }
  if (!game_events_[state_timestep].count(0)) {
//    printf("don't have host\n");
    return false;
  }
  if (game_engine_infos_[state_timestep].state_timestep != state_timestep) {
//    printf("timestep not set: %d %d\n", game_engine_infos_[state_timestep].state_timestep, state_timestep);
    return false;
  }
  return true;
}

/*
NetTimestep GameEngine::GetCurrentNetTimestep() {
  NetTimestep current_timestep;
  NetTimestep delayed_timestep;
  frame_calculator_->GetFrame(&current_timestep, &delayed_timestep);
  return current_timestep;
}
*/

void GameEngine::RecreateState(StateTimestep state_timestep) {
  printf("RecreateState(%d)\n", state_timestep);
  if (game_states_[state_timestep] != NULL) {
    delete game_states_[state_timestep];
  }

  if (game_engine_infos_[state_timestep].state_timestep == state_timestep) {
    num_rethinks_++;
  }
  num_thinks_++;

  game_states_[state_timestep] = game_states_[state_timestep - 1]->Copy();
  game_engine_infos_[state_timestep] = game_engine_infos_[state_timestep - 1];
  game_engine_infos_[state_timestep].state_timestep = state_timestep;


  ApplyEventsToGameState(
      state_timestep,
      game_events_[state_timestep],
      game_states_[state_timestep],
      &game_engine_infos_[state_timestep]);

  game_states_[state_timestep]->Think();

  if (latest_complete_state_timestep_ == state_timestep - 1 && IsStateComplete(state_timestep)) {
    while (latest_complete_state_timestep_ == game_states_.GetFirstIndex() + 1) {
      game_states_.Advance();
      game_events_.Advance();
      game_engine_infos_.Advance();
    }
    printf("Timestep %d is done.\n", latest_complete_state_timestep_);
    latest_complete_state_timestep_++;
  }
}

void GameEngine::ApplyEventsToGameState(
    int think_count,
    const map<EngineID, vector<GameEvent*> >& events,
    GameState* game_state,
    GameEngineInfo* game_engine_info) {

  // For simplicity, we always apply game engine events (ID < 0) in the order of engine id
  map<EngineID, vector<GameEvent*> >::const_iterator map_it;
  for (map_it = events.begin(); map_it != events.end(); map_it++) {
    for (int i = 0; i < map_it->second.size(); i++) {
      /// \todo jwills - Should probably check that these events are coming from the host?
      if (map_it->second[i]->type() < 0) {
        map_it->second[i]->ApplyToGameEngineInfo(game_engine_info);
      }
    }
  }

  // events is sorted by PlayerID, but to be fair we don't always apply the first player's events
  // first.  Instead we start with an index that may or may not be the first player, then we just
  // loop through all of the indices in order from there, wrapping around at the end.  It could be
  // more fair, but this is dead simple and probably good enough.
  // TODO: Don't forget to make a test to ensure this fair ordering actually happens
  vector<EngineID> ids;
  for (map_it = events.begin(); map_it != events.end(); map_it++) {
    ids.push_back(map_it->first);
  }
  int total = 0;
  for (int i = 0; i < events.size(); i++) {
    int index = (i + think_count) % events.size();
    const vector<GameEvent*> events_package = events.find(ids[index])->second;
    for (int j = 0; j < events_package.size(); j++) {
      if (events_package[j]->type() > 0) {
        events_package[j]->ApplyToGameState(game_state);
        total++;
      }
    }
  }
}

void GameEngine::ThinkPlaying() {
  ASSERT(game_states_.GetFirstIndex() == game_engine_infos_.GetFirstIndex());
  ASSERT(game_states_.GetFirstIndex() == game_events_.GetFirstIndex());
  int time_ms = frame_calculator_->GetTime();
  StateTimestep current_state_timestep = time_ms / ms_per_state_frame_;
  NetTimestep current_net_timestep = time_ms / ms_per_net_frame_;
  StateTimestep delayed_state_timestep = GetCurrentDelayedStateTimestep(time_ms);
  printf("%d %d %d\n", current_state_timestep, delayed_state_timestep, current_net_timestep);
  if (think_state_ == kPlaying) {
    QueueEvents(delayed_state_timestep);
    SendEvents(current_net_timestep);
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
 printf("Received events for engine %d on timestep %d\n", events[j].first.engine_id, events[j].first.state_timestep);
      for (int k = 0; k < events[j].second.size(); k++) {
        if (events[j].second[k]->type() == -2) {
          assert(events[j].second.size() == 1); // This event should always be by itself
          // This event can come from any time, so we set it to our current timestep so that we
          // don't put it into a place in our history we've already forgotten about.
          events[j].first.state_timestep = current_state_timestep;
          ReadyToPlayEvent* r2p = (ReadyToPlayEvent*)events[j].second[k];
          if (host_) {
            NewEngineEvent* nen = NewNewEngineEvent();
            printf("r2p: %d %d\n", r2p->origin(), r2p->temporary());
            nen->SetData(r2p->origin(), r2p->temporary(), next_game_engine_id_++);
            printf("nen: %d %d\n", nen->origin(), nen->temporary());
            printf("Got R2P event, sent NEE\n");
            ApplyEvent(nen);
          }
        }
// TODO: Need to make sure to not to reset oldest_dirty_timestep if we receive events for someone
// on a timestep before they actually exist.  It's clearly an error, but we shouldn't crash.
        if (events[j].second[k]->type() == -3) {
          NewEngineEvent* nen = (NewEngineEvent*)events[j].second[k];
          printf("%d Got NEE\n", engine_id_);
          printf("%d %d %d %d\n", nen->origin(), source_engine_id_, nen->temporary(), engine_id_);
          if (nen->origin() == source_engine_id_ && nen->temporary() == engine_id_) {
            printf("%d Got NEE\n", engine_id_);
            engine_id_ = nen->engine();
            think_state_ = kPlaying;
            last_queue_event_timestep_ = events[j].first.state_timestep - 1;
            last_send_event_timestep_ = (last_queue_event_timestep_ * ms_per_state_frame_) / ms_per_net_frame_ - 1;

            if (last_queue_event_timestep_ * ms_per_state_frame_ > frame_calculator_->GetTime()) {
              frame_calculator_->SetTime(last_queue_event_timestep_ * ms_per_state_frame_);
            }
          }
        }
      }

      // NOTE: This check must be at the end of this block because in the event of a ReadyToPlay
      // event we actually modify the timestep of the event package.
      if (events[j].first.state_timestep < oldest_dirty_timestep_) {
        oldest_dirty_timestep_ = events[j].first.state_timestep;
      }
    }

    // With a tree connection graph, all we have to do is take all incoming events and send them to
    // all of our other connections.
    // TODO: If we ever change to any graph that isn't a tree this won't work and we'll actually
    // need to keep track of who we should send which events to,
    for (int j = 0; j < all_connections_.size(); j++) {
      if (all_connections_[j] == playing_connections_[i]) { continue; }
      for (int k = 0; k < events.size(); k++) {
        all_connections_[j]->QueueEvents(events[k].first, events[k].second);
      }
    }
    for (int j = 0; j < events.size(); j++) {
      EngineID engine_id = events[j].first.engine_id;
      StateTimestep state_timestep = events[j].first.state_timestep;
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
      if (game_events_[state_timestep].count(engine_id)) {
        printf("Timestep: %d\n", state_timestep);
        printf("Engine %d has received a second batch of events from engine %d\n", engine_id_, engine_id);
        printf("Sizes (prev/cur): %d/%d\n", game_events_[state_timestep][engine_id].size(), events[j].second.size());
        // TODO: Maybe this is because of duplicated packets?  Investigate more, we might just be
        // able to ignore this when it happens to long as we get the same packets each time.
        assert(!game_events_[state_timestep].count(engine_id));
      }
      game_events_[state_timestep][engine_id] = events[j].second;
    }
  }
  for (NetTimestep t = oldest_dirty_timestep_; t <= current_state_timestep; t++) {
    printf("Thunder: %d\n", t);
    RecreateState(t);
  }
//  AdvanceAsFarAsPossible(current_timestep, delayed_timestep);
  // TODO: This line is dangerous i think
  oldest_dirty_timestep_ = current_state_timestep;

  // We only go up to delayed_timestep here so that we have a little extra time to get everyone
  // else's events for the head frame before having to call think on it.  This prevents us from
  // having to run GameState.Think() more than once on any frame so long as we receive all events
  // for a frame before delayed_frame = current_frame.
  head_ = game_states_[current_state_timestep];
}

void GameEngine::ThinkLagging() {
  
}

GameEngineThinkState GameEngine::Think() {
  printf("Engine(%d)::Think()\n", engine_id_);
  printf("time_ms: %d\n", frame_calculator_->GetTime());
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
//      ThinkLagging();
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
}


// NETWORKING SCHTUFF
void GameEngine::ThinkNetworking() {
  if (!networking_enabled_) { return; }
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
      GameStateEvent* gse = NewGameStateEvent();
      string serialized_game_state;
      // TODO: Consider allowing a constructor for game events that uses a protobuf that the event
      // takes constrol of, that would prevent one case of copying this data around.
      game_states_[latest_complete_state_timestep_]->SerializeToString(&serialized_game_state);
      gse->SetData(
          serialized_game_state,
          latest_complete_state_timestep_,
          game_engine_infos_[latest_complete_state_timestep_].engine_ids,
          engine_id_,
          next_game_engine_id_++,
          max_frames_,
          ms_per_net_frame_,
          ms_per_state_frame_,
          ms_delay_,
          frame_calculator_->GetTime());
      peer->QueueEvents(
          EventPackageID(latest_complete_state_timestep_ - 1, engine_id_),
          vector<GameEvent*>(1, gse));
      // Send this first, and apart from the events that we send next, because this might be big.
      peer->SendEvents();

      // Now we have to send any events that we have that happened after that timestep
      // TODO: prolly need to only go up to last_queue_event_timestep_
 printf("Send events up to lqet: %d\n",last_queue_event_timestep_); 
      for (StateTimestep t = latest_complete_state_timestep_ + 1;
           t <= last_queue_event_timestep_;
           //(last_send_event_timestep_ * ms_per_net_timestep_) / ms_per_state_timestep_;
           //t <= game_events_.GetLastIndex();
           t++) {
        map<EngineID, vector<GameEvent*> >::iterator it;
        for (it = game_events_[t].begin(); it != game_events_[t].end(); it++) {
          printf("Sending new engine events on timestep %d\n", t);
          peer->QueueEvents(EventPackageID(t, it->first), it->second);
        }
      }
      // TODO: Consider whether or not these should all be sent as one packet, it could be big.
      peer->SendEvents();

      connected_gnas_.insert(connections[i]);
    }
  }
}

bool GameEngine::StartNetworkManager(int port) {
  if (networking_enabled_) { return true; }
  port_ = port;
  if (!network_manager_->Startup(port_)) {
    delete network_manager_;
    network_manager_ = NULL;
    return false;
  }
  networking_enabled_ = true;
  return true;
}

bool GameEngine::AllowIncomingConnections(const string& message) {
  if (network_manager_ == NULL) { return false; }
  network_manager_->StartHosting(message);
  return true;
}

bool GameEngine::DisallowIncomingConnections() {
  if (network_manager_ == NULL) { return false; }
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

void GameEngine::Connect(GlopNetworkAddress gna, const string& message) {
  if (network_manager_ == NULL) { return; }
  network_manager_->Connect(gna);
//  network_manager_->Think();
  connection_gna_ = gna;
  connection_message_ = message;
  think_state_ = kConnecting;
/*  while (network_manager_->GetConnections().size() == 0) {
    network_manager_->Think();
    system()->Sleep(5);
  }
  printf("Get: %d %d\n", network_manager_->GetConnections()[0].first, network_manager_->GetConnections()[0].second);
  printf("connecting\n");
*/
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
    game_event_buffer_[events[i].first.state_timestep][events[i].first.engine_id] =
        events[i].second;
    for (int j = 0; j < events[i].second.size(); j++) {
      if (events[i].second[j]->type() == -1) {
        gse = (GameStateEvent*)events[i].second[j];
      }
    }
  }
  int mark = 0;
  if (gse != NULL) {
    const GameStateEventData& data = gse->GetData();
    source_engine_id_ = data.source_engine_id();

//    typed_data_->set_max_frames(max_frames);
//    typed_data_->set_ms_per_net_frame(max_frames);
//    typed_data_->set_ms_per_state_frame(ms_per_state_frame);
//    typed_data_->set_ms_delay(ms_delay);
    max_frames_ = data.max_frames();
    ms_per_net_frame_ = data.ms_per_net_frame();
    ms_per_state_frame_ = data.ms_per_state_frame();
    ms_delay_ = data.ms_delay();

    game_states_ = MovingWindow<GameState*>(max_frames_ + 1, data.timestep());
    game_engine_infos_ = MovingWindow<GameEngineInfo>(max_frames_ + 1, data.timestep());
    game_events_ =
        MovingWindow<map<EngineID, vector<GameEvent*> > >(max_frames_ * 2 + 1, data.timestep());

      // TODO: Templatize the class on GameState type so that we're not required to supply a sample GameState object as a reference state.
    game_states_[data.timestep()] = reference_state_->Copy();
    game_states_[data.timestep()]->ParseFromString(data.game_state());

    // TODO: VERY IMPORTANT: Right now we're assuming that we can get the whole gamestate event
    // within max_frames_, but this can't be something we rely on in practice.
    for (int i = 0; i < data.engine_ids_size(); i++) {
      game_events_[data.timestep()][data.engine_ids(i)].clear();
      game_engine_infos_[data.timestep()].engine_ids.insert(data.engine_ids(i));
      game_engine_infos_[data.timestep()].state_timestep = data.timestep();
    }

    printf("Temp engine id: %d\n", gse->GetData().temporary_engine_id());
    engine_id_ = gse->GetData().temporary_engine_id();

//    void SendEvents(EventPackageID id, const vector<GameEvent*>& events);
    map<StateTimestep, map<EngineID, vector<GameEvent*> > >::iterator it;
    for (it = game_event_buffer_.begin(); it != game_event_buffer_.end(); it++) {
      map<EngineID, vector<GameEvent*> >::iterator xit;
      if (it->first <= data.timestep()) { continue; }
      for (xit = it->second.begin(); xit != it->second.end(); xit++) {
        game_events_[it->first][xit->first] = xit->second;
      }
    }
    it = game_event_buffer_.end();
    it--;

    int complete = game_states_.GetFirstIndex();
    if (it->first - complete > max_frames_) {
      printf("WE ARE IN BIIIIIG TROUBLE\n");
    }
    ReadyToPlayEvent* r2p = NewReadyToPlayEvent();
    r2p->SetData(source_engine_id_, engine_id_);
    printf("outgoing r2p event: %d %d\n", r2p->origin(), r2p->temporary());
    all_connections_[0]->QueueEvents(
        EventPackageID(complete, (unsigned int)-1),
        vector<GameEvent*>(1, r2p));
    all_connections_[0]->SendEvents();
    printf("Sent ReadyToPlay event\n");

    latest_complete_state_timestep_ = complete;
    // Advancing potentially as far as the most recent event we have.  This is the best we can do
    for (StateTimestep t = data.timestep() + 1; t <= it->first; t++) {
      printf("Rawr: %d\n", t);
      RecreateState(t);
    }

    oldest_dirty_timestep_ = latest_complete_state_timestep_ + 1;
    printf("Timestep %d is done.\n", latest_complete_state_timestep_);
    int time_ms_estimate = (oldest_dirty_timestep_ + 1) * ms_per_state_frame_;
    if (ms_delay_ > 0) {
      time_ms_estimate += ms_per_net_frame_ + ms_delay_;
    }
//    frame_calculator_->SetTime(time_ms_estimate);
    frame_calculator_->SetTime(data.time_ms());

    think_state_ = kReady;
    last_queue_event_timestep_ = -1;
  }
}

