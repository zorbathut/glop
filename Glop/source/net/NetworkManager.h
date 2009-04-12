#ifndef GLOP_NET_NETWORK_MANAGER_H__
#define GLOP_NET_NETWORK_MANAGER_H__

#include <string>
#include <vector>
#include <map>
#include <queue>
using namespace std;

#include "NetworkManagerInterface.h"
#include "../List.h"

class RakPeerInterface;
// FROM: third_party/raknet/RakPeerInterface.h

class SystemAddress;
// FROM: third_party/raknet/RakNetTypes.h

// This struct mirrors raknet's SystemAddress struct, this is just here so no one using Glop has to
// depend on raknet directly
typedef pair<unsigned int, unsigned short> GlopNetworkAddress;

class NetworkConnection {
 public:
  NetworkConnection();
  ~NetworkConnection();

 private:
  int src_port_;
  int dst_port_;
  int id_;
};

class NetworkManager : public NetworkManagerInterface {
 public:
  NetworkManager();
  ~NetworkManager();

  bool Startup(int port);
  // Must be called before anything else.  Return true iff startup was successful.

  void StartHosting(const string& data);
  void StopHosting();

  // You can only search for hosts on one port at a time.  If you call this again with a different
  // port, responses to previous FindHosts requests will be ignored.
  void FindHosts(int port);
  void ClearHosts();
  vector<pair<GlopNetworkAddress, string> > AvailableHosts() const;
  void Connect(GlopNetworkAddress gna);
  void Disconnect(GlopNetworkAddress gna);
  vector<GlopNetworkAddress> GetConnections() const;

  void SendData(GlopNetworkAddress gna, const string& data);
  bool ReceiveData(GlopNetworkAddress* gna, string* data);
  bool ReceiveData(GlopNetworkAddress gna, string* data);
  bool ReceiveData(GlopNetworkAddress* gna, const string& data);

  int PendingData() const;

  void Think();
 private:
  RakPeerInterface* rakpeer_;

  map<GlopNetworkAddress, string> hosts_;
  // If FindHosts() is called, this will aggregate a list of responses

  int host_search_port_;
  // When FindHost is called, this is the port we expect responses on.  Responses on other ports are
  // ignored.

  List<pair<GlopNetworkAddress, string> > incoming_data_;
};



#endif // GLOP_NET_NETWORK_MANAGER_H__
