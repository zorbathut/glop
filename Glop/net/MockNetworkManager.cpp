#include "MockNetworkManager.h"
#include "MockRouter.h"

MockNetworkManager::MockNetworkManager(MockRouter* router)
  : router_(router),
    key_(0),
    search_port_(0),
    port_(0) {
}

MockNetworkManager::~MockNetworkManager() {
}

bool MockNetworkManager::Startup(int port) {
  if (key_ != 0) {
    return false;
  }
  port_ = port;
  key_ = router_->GetKey(port_);
  return true;
}

void MockNetworkManager::StartHosting(const string& data) {
  router_->StartHosting(key_, data);
}

void MockNetworkManager::StopHosting() {
  router_->StopHosting(key_);
}

void MockNetworkManager::FindHosts(int port) {
  search_port_ = port;
}

void MockNetworkManager::ClearHosts() {
  hosts_.clear();
}

vector<pair<GlopNetworkAddress, string> > MockNetworkManager::AvailableHosts() const {
  vector<pair<GlopNetworkAddress, string> > ret;
  map<GlopNetworkAddress, string>::const_iterator it;
  for (it = hosts_.begin(); it != hosts_.end(); it++) {
    ret.push_back(*it);
  }
  return ret;
}

void MockNetworkManager::Connect(GlopNetworkAddress gna) {
  router_->Connect(key_, gna);
}

void MockNetworkManager::Disconnect(GlopNetworkAddress gna) {
  router_->Disconnect(key_, gna);
}

vector<GlopNetworkAddress> MockNetworkManager::GetConnections() const {
  return router_->GetConnections(key_);
}


void MockNetworkManager::SendData(GlopNetworkAddress gna, const string& data) {
  router_->SendData(key_, gna, data);
}

bool MockNetworkManager::ReceiveData(GlopNetworkAddress* gna, string* data) {
  List<pair<GlopNetworkAddress, string> >::iterator it;
  for (it = incoming_data_.begin(); it != incoming_data_.end(); it++) {
    *gna = it->first;
    *data = it->second;
    incoming_data_.erase(it);
    return true;
  }
  return false;
}

bool MockNetworkManager::ReceiveData(GlopNetworkAddress gna, string* data) {
  List<pair<GlopNetworkAddress, string> >::iterator it;
  for (it = incoming_data_.begin(); it != incoming_data_.end(); it++) {
    if (it->first == gna) {
      *data = it->second;
      incoming_data_.erase(it);
      return true;
    }
  }
  return false;
}

bool MockNetworkManager::ReceiveData(GlopNetworkAddress* gna, const string& data) {
  List<pair<GlopNetworkAddress, string> >::iterator it;
  for (it = incoming_data_.begin(); it != incoming_data_.end(); it++) {
    if (it->second == data) {
      *gna = it->first;
      incoming_data_.erase(it);
      return true;
    }
  }
  return false;
}


int MockNetworkManager::PendingData() const {
  return incoming_data_.size();
}

void MockNetworkManager::Think() {
  if (search_port_ != 0) {
    vector<pair<GlopNetworkAddress, string> > hosters = router_->AvailableHosts(search_port_);
    search_port_ = 0;
    for (int i = 0; i < hosters.size(); i++) {
      GlopNetworkAddress gnax = hosters[i].first;
      hosts_[hosters[i].first] = hosters[i].second;
    }
  }
  GlopNetworkAddress gna;
  string data;
  while (router_->ReceiveData(key_, &gna, &data)) {
    incoming_data_.push_back(make_pair(gna, data));
  }
}
