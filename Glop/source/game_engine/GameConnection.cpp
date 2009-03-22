#include "GameConnection.h"
#include "GameEvent.h"

#include "../net/Net.h"

bool operator < (const EventPackageID& a, const EventPackageID& b) {
  if (a.timestep != b.timestep) {
    return a.timestep < b.timestep;
  }
  return a.engine_id < b.engine_id;
}

void GameConnection::SendEvents(EventPackageID id, const vector<GameEvent*>& events) {
  string data;
  SerializeEvents(id, events, &data);
  DoSendEvents(data);
}

void GameConnection::ReceiveEvents(vector<pair<EventPackageID, vector<GameEvent*> > >* events) {
  vector<string> data;
  DoReceiveEvents(&data);
  for (int i = 0; i < data.size(); i++) {
    EventPackageID id;
    vector<GameEvent*> batch;
    DeserializeEvents(data[i], &id, &batch);
    events->push_back(pair<EventPackageID, vector<GameEvent*> >(id, batch));
  }
}

/// \todo jwills - This serialization is incredibly inefficient.  Replace it with something that
/// doesn't send 4 bytes for ints that can EASILY be compressed.
void GameConnection::SerializeEvents(
    EventPackageID id,
    const vector<GameEvent*>& events,
    string* data) {
  ASSERT(data->size() == 0);
  data->resize(8);
  (*data)[0] = (id.timestep  >>  0) & 0xff;
  (*data)[1] = (id.timestep  >>  8) & 0xff;
  (*data)[2] = (id.timestep  >> 16) & 0xff;
  (*data)[3] = (id.timestep  >> 24) & 0xff;
  (*data)[4] = (id.engine_id >>  0) & 0xff;
  (*data)[5] = (id.engine_id >>  8) & 0xff;
  (*data)[6] = (id.engine_id >> 16) & 0xff;
  (*data)[7] = (id.engine_id >> 24) & 0xff;
  for (int i = 0; i < events.size(); i++) {
    string event_data;
    GameEventFactory::Serialize(events[i], &event_data);
    string size;
    size.resize(4);
    size[0] = (event_data.size() >>  0) & 0xff;
    size[1] = (event_data.size() >>  8) & 0xff;
    size[2] = (event_data.size() >> 16) & 0xff;
    size[3] = (event_data.size() >> 24) & 0xff;
    (*data) += size + event_data;
  }
}

void GameConnection::DeserializeEvents(
    const string& data,
    EventPackageID* id,
    vector<GameEvent*>* events) {
  ASSERT(data.size() >= 8);
  id->timestep = 0;
  id->timestep  |= ((unsigned char)data[0]) <<  0;
  id->timestep  |= ((unsigned char)data[1]) <<  8;
  id->timestep  |= ((unsigned char)data[2]) << 16;
  id->timestep  |= ((unsigned char)data[3]) << 24;
  id->engine_id = 0;
  id->engine_id |= ((unsigned char)data[4]) <<  0;
  id->engine_id |= ((unsigned char)data[5]) <<  8;
  id->engine_id |= ((unsigned char)data[6]) << 16;
  id->engine_id |= ((unsigned char)data[7]) << 24;
  int index = 8;
  while (index < data.size()) {
    int size = 0;
    size |= data[index + 0] <<  0;
    size |= data[index + 1] <<  8;
    size |= data[index + 2] << 16;
    size |= data[index + 3] << 24;
    index += 4;
    events->push_back(GameEventFactory::Deserialize(data.substr(index, size)));
    index += size;
  }
}

void PeerConnection::DoSendEvents(const string& data) {
  network_manager_->SendData(gna_, data);
}

void PeerConnection::DoReceiveEvents(vector<string>* data) {
  string message;
  data->clear();
  while (network_manager_->ReceiveData(gna_, &message)) {
    data->push_back(message);
  }
}

void SelfConnection::DoSendEvents(const string& data) {
  data_.push_back(data);
}

void SelfConnection::DoReceiveEvents(vector<string>* data) {
  for (int i = 0; i < data_.size(); i++) {
    data->push_back(data_[i]);
  }
  data_.resize(0);
}
