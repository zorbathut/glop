#include <stdio.h>

#include "NetworkManager.h"
#include "third_party/raknet/PluginInterface.h"
#include "third_party/raknet/RakNetworkFactory.h"
#include "third_party/raknet/RakPeerInterface.h"
#include "third_party/raknet/MessageIdentifiers.h"
#include "third_party/raknet/BitStream.h"

#include <string>
using namespace std;

enum GlopPacketIDs {
  ID_BASIC_DATA = ID_USER_PACKET_ENUM,
};

// Raknet System Address to Glop Network Address
GlopNetworkAddress RSA2GNA(SystemAddress rsa) {
  GlopNetworkAddress gna;
  gna.first = rsa.binaryAddress;
  gna.second = rsa.port;
  return gna;
}

SystemAddress GNA2RSA(GlopNetworkAddress gna) {
  SystemAddress rsa;
  rsa.binaryAddress = gna.first;
  rsa.port = gna.second;
  return rsa;
}

class GlopPlugin : public PluginInterface {
 public:
  virtual void Update(RakPeerInterface* peer, Packet* packet) {
//    printf("Update\n");
  }
  virtual PluginReceiveResult OnReceive(RakPeerInterface* peer, Packet* packet) {
//    printf("Received a packet\n");
    return (PluginReceiveResult)true;
  }
 private:
};

NetworkManager::NetworkManager()
  : rakpeer_(NULL),
    host_search_port_(0) {
}

bool NetworkManager::Startup(int port) {
//  printf("Starting on port %d\n", port);
  rakpeer_ = RakNetworkFactory::GetRakPeerInterface();
	SocketDescriptor server_socket(port, 0);
	if (!rakpeer_->Startup(32, 5, &server_socket, 1)) {
    return false;
	}
  rakpeer_->AttachPlugin(new GlopPlugin);
  rakpeer_->SetMaximumIncomingConnections(16);
  return true;
}

NetworkManager::~NetworkManager() {
  if (rakpeer_ != NULL) {
    rakpeer_->Shutdown(0);
    RakNetworkFactory::DestroyRakPeerInterface(rakpeer_);
  }
}

void NetworkManager::StartHosting(const string& data) {
  rakpeer_->SetOfflinePingResponse(data.c_str(), data.size() + 1);
}

void NetworkManager::StopHosting() {
  rakpeer_->SetOfflinePingResponse(NULL, 0);
}

// TODO: this should probably just clear out old host entries
void NetworkManager::FindHosts(int port) {
//  printf("This: %x\n", this);
  host_search_port_ = port;
  map<GlopNetworkAddress, string>::iterator it;
  vector<map<GlopNetworkAddress, string>::iterator> trash;
  for (it = hosts_.begin(); it != hosts_.end(); it++) {
    if (it->first.second != port) {
      trash.push_back(it);
    }
  }
  for (int i = 0; i < trash.size(); i++) {
    hosts_.erase(trash[i]);
  }
	rakpeer_->Ping("255.255.255.255", port, true);
}

void NetworkManager::ClearHosts() {
  hosts_.clear();
}

vector<pair<GlopNetworkAddress, string> > NetworkManager::AvailableHosts() const {
  vector<pair<GlopNetworkAddress, string> > ret;
  map<GlopNetworkAddress, string>::const_iterator it;
  for (it = hosts_.begin(); it != hosts_.end(); it++) {
    ret.push_back(*it);
  }
  return ret;
}

void NetworkManager::Connect(GlopNetworkAddress gna) {
  int ip = gna.first;
  char buf[16];
  sprintf(buf, "%d.%d.%d.%d", ip & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff);
  if (rakpeer_->Connect(buf, gna.second, 0, 0, 0)) {
//    printf("Connection successfully initiated\n");
  } else {
//    printf("Couldn't initiate connection.\n");
  }
}

void NetworkManager::Disconnect(GlopNetworkAddress gna) {
  //TODO: fill me in! :'(
/*  int ip = gna.first;
  char buf[16];
  sprintf(buf, "%d.%d.%d.%d", ip & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff);
//  printf("Connceting to %s:%d\n", buf, gna.second);
  if (rakpeer_->Connect(buf, gna.second, 0, 0, 0)) {
//    printf("Connection successfully initiated\n");
  } else {
//    printf("Couldn't initiate connection.\n");
  }
*/
}

vector<GlopNetworkAddress> NetworkManager::GetConnections() const {
  SystemAddress connections[256];
  unsigned short num_connections = 256;
  rakpeer_->GetConnectionList(connections, &num_connections);
  vector<GlopNetworkAddress> ret;
  for (int i = 0; i < num_connections; i++) {
    GlopNetworkAddress g = RSA2GNA(connections[i]);
    ret.push_back(RSA2GNA(connections[i]));
  }
/*
  for (int i = 0; i < ret.size(); i++) {
        char buf[25];
        int ip = ret[i].first;
        sprintf(buf, "%d.%d.%d.%d", ip & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff);
        printf("Connected to %s\n", buf);
        if (!rakpeer_->IsConnected(connections[i], true, false)) { continue; }
    
  }
*/
  return ret;
}

void NetworkManager::SendData(GlopNetworkAddress gna, const string& data) {
  RakNet::BitStream bs;
  char basic_data = ID_BASIC_DATA;
  bs.WriteAlignedBytes((unsigned char*)&basic_data, 1);
  bs.WriteAlignedBytes((unsigned char*)data.c_str(), data.size());
  rakpeer_->Send(&bs, HIGH_PRIORITY, RELIABLE, 0, GNA2RSA(gna), false);
}

bool NetworkManager::ReceiveData(GlopNetworkAddress* gna, string* data) {
  List<pair<GlopNetworkAddress, string> >::iterator it;
  for (it = incoming_data_.begin(); it != incoming_data_.end(); it++) {
    *gna = it->first;
    *data = it->second;
    incoming_data_.erase(it);
    return true;
  }
  return false;
}

bool NetworkManager::ReceiveData(GlopNetworkAddress gna, string* data) {
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

bool NetworkManager::ReceiveData(GlopNetworkAddress* gna, const string& data) {
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

int NetworkManager::PendingData() const {
  return incoming_data_.size();
}

void NetworkManager::Think() {
  Packet* p = rakpeer_->Receive();
  while (p != NULL) {
    if (p->data[0] == ID_PONG) {
      if (p->systemAddress.port == host_search_port_) {
        string pong((const char*)(p->data + 5));
        char buf[16];
        int ip = p->systemAddress.binaryAddress;
        sprintf(buf, "%d.%d.%d.%d",
            ip & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff);
        if (pong.size() > 0) {
          hosts_[RSA2GNA(p->systemAddress)] = pong;
        }
      }
    }
    if (p->data[0] == ID_CONNECTION_REQUEST_ACCEPTED) {
    }
    if (p->data[0] == ID_CONNECTION_ATTEMPT_FAILED) {
    }
    if (p->data[0] == ID_BASIC_DATA) {
      incoming_data_.push_back(
          pair<GlopNetworkAddress, string>(
              RSA2GNA(p->systemAddress),
              string((const char*)(p->data + 1), (p->bitSize - 8) / 8)));
    }
    p = rakpeer_->Receive();
  }
}
