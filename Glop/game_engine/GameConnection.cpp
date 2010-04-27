#include "GameConnection.h"
#include "GameEvent.h"

#include "../net/NetworkManagerInterface.h"

bool operator < (const EventPackageID& a, const EventPackageID& b) {
  if (a.state_timestep != b.state_timestep) {
    return a.state_timestep < b.state_timestep;
  }
  return a.engine_id < b.engine_id;
}

void GameConnection::QueueEvents(int channel, EventPackageID id, const vector<GameEvent*>& events) {
  string data;
  SerializeEvents(id, events, &data);
  string size(sizeof(int), 0);
  ((int*)size.data())[0] = data.size();
  buffers_[channel] += size + data;
}

void GameConnection::SendEvents(int channel) {
  SendData(buffers_[channel]);
  buffers_[channel].clear();
}

void GameConnection::SendAllEvents() {
  map<int,string>::iterator it;
  for (it = buffers_.begin(); it != buffers_.end(); it++) {
    SendEvents(it->first);
  }
}

// TODO: Need to be sure this method is resiliant to corrupt data
void GameConnection::ReceiveEvents(vector<pair<EventPackageID, vector<GameEvent*> > >* events) {
  vector<string> data;
  ReceiveData(&data);
  for (int i = 0; i < data.size(); i++) {
    for (int pos = 0; pos < data[i].size(); ) {
      // TODO: This does unaligned memory access, is that bad?
      int len = *((int*)(&data[i].data()[pos]));
      pos += sizeof(int);
      EventPackageID id;
      vector<GameEvent*> batch;
      DeserializeEvents(data[i].substr(pos, len), &id, &batch);
      pos += len;
      events->push_back(pair<EventPackageID, vector<GameEvent*> >(id, batch));
    }
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
  (*data)[0] = (id.state_timestep  >>  0) & 0xff;
  (*data)[1] = (id.state_timestep  >>  8) & 0xff;
  (*data)[2] = (id.state_timestep  >> 16) & 0xff;
  (*data)[3] = (id.state_timestep  >> 24) & 0xff;
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
  assert(data.size() >= 8);
  id->state_timestep = 0;
  id->state_timestep  |= ((unsigned char)data[0]) <<  0;
  id->state_timestep  |= ((unsigned char)data[1]) <<  8;
  id->state_timestep  |= ((unsigned char)data[2]) << 16;
  id->state_timestep  |= ((unsigned char)data[3]) << 24;
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

void PeerConnection::SendData(const string& data) {
  network_manager_->SendData(gna_, data);
}

void PeerConnection::ReceiveData(vector<string>* data) {
  string message;
  data->clear();
  while (network_manager_->ReceiveData(gna_, &message)) {
    data->push_back(message);
  }
}

void SelfConnection::SendData(const string& data) {
  data_.push_back(data);
}

void SelfConnection::ReceiveData(vector<string>* data) {
  for (int i = 0; i < data_.size(); i++) {
    data->push_back(data_[i]);
  }
  data_.resize(0);
}
