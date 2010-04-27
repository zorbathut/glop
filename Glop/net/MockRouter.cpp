#include "MockRouter.h"

MockRouter::MockRouter() : next_key_(0) { }

RouterKey MockRouter::GetKey(int port) {
  RouterKey key = next_key_;
  next_key_++;
  GlopNetworkAddress gna = GlopNetworkAddress(key, port);
  key_to_gna_[key] = gna;
  gna_to_key_[gna] = key;
  return key;
}

void MockRouter::Connect(RouterKey key, GlopNetworkAddress gna) {
  // assert that we don't already have the connection
  if (gna_to_key_.count(gna)) {
    connections_[key].insert(gna);
    connections_[gna_to_key_[gna]].insert(key_to_gna_[key]);
  }
}

void MockRouter::Disconnect(RouterKey key, GlopNetworkAddress gna) {
  // assert something sensible here
  // TODO: Remove this key's gna from other data structures
  connections_[key].erase(gna);
  connections_[gna_to_key_[gna]].erase(key_to_gna_[key]);
}

vector<GlopNetworkAddress> MockRouter::GetConnections(RouterKey key) const {
  if (!connections_.count(key)) {
    return vector<GlopNetworkAddress>();
  }
  return vector<GlopNetworkAddress>(
      connections_.find(key)->second.begin(),
      connections_.find(key)->second.end());
}

void MockRouter::SendData(RouterKey key, GlopNetworkAddress gna, const string& data) {
  // assert instead of using an if here
  if (connections_[key].count(gna)) {
    sent_data_[gna_to_key_[gna]].push_back(make_pair(key_to_gna_[key], data));
  }
}

bool MockRouter::ReceiveData(RouterKey key, GlopNetworkAddress* gna, string* data) {
  if (sent_data_[key].size() == 0) {
    return false;
  }
  *gna = sent_data_[key].front().first;
  *data = sent_data_[key].front().second;
  sent_data_[key].pop_front();
  return true;
}

void MockRouter::StartHosting(RouterKey key, const string& data) {
  hosts_[key] = data;
}

void MockRouter::StopHosting(RouterKey key) {
  hosts_.erase(key);
}

vector<pair<GlopNetworkAddress, string> > MockRouter::AvailableHosts(int port) const {
  vector<pair<GlopNetworkAddress, string> > ret;
  map<RouterKey, string>::const_iterator it;
  for (it = hosts_.begin(); it != hosts_.end(); it++) {
    map<RouterKey, GlopNetworkAddress>::const_iterator gna_it = key_to_gna_.find(it->first);
    if (gna_it == key_to_gna_.end()) {
      continue;
    }
    if (gna_it->second.second == port) {
      ret.push_back(make_pair(gna_it->second, it->second));
    }
  }
  return ret;
}
