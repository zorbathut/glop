#ifndef GLOP_NET_MOCK_ROUTER_H__
#define GLOP_NET_MOCK_ROUTER_H__

#include "NetworkManagerInterface.h"

#include <map>
#include <set>
#include <string>
#include <vector>
using namespace std;

typedef int RouterKey;

class MockRouter {
 public:
  MockRouter();
  RouterKey GetKey(int port);
  void Connect(RouterKey key, GlopNetworkAddress gna);
  void Disconnect(RouterKey key, GlopNetworkAddress gna);
  vector<GlopNetworkAddress> GetConnections(RouterKey key) const;
  void SendData(RouterKey key, GlopNetworkAddress gna, const string& data);
  bool ReceiveData(RouterKey key, GlopNetworkAddress* gna, string* data);

  void StartHosting(RouterKey key, const string& data);
  void StopHosting(RouterKey key);
  vector<pair<GlopNetworkAddress, string> > AvailableHosts(int port) const;

 private:
  map<RouterKey, GlopNetworkAddress> key_to_gna_;
  map<GlopNetworkAddress, RouterKey> gna_to_key_;
  map<RouterKey, set<GlopNetworkAddress> > connections_;
  map<RouterKey, List<pair<GlopNetworkAddress, string> > > sent_data_;
  map<RouterKey, string> hosts_;
  RouterKey next_key_;
};

#endif // GLOP_NET_MOCK_ROUTER_H__
