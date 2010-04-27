#ifndef GLOP_NET_NETWORK_MANAGER_INTERFACE_H__
#define GLOP_NET_NETWORK_MANAGER_INTERFACE_H__

#include <string>
#include <vector>
#include <map>
#include <queue>
using namespace std;

#include "NetworkManagerInterface.h"
#include "../List.h"

// This struct mirrors raknet's SystemAddress struct, this is just here so no one using Glop has to
// depend on raknet directly
typedef pair<unsigned int, unsigned short> GlopNetworkAddress;

class NetworkManagerInterface {
 public:
  virtual ~NetworkManagerInterface() {}
  virtual bool Startup(int port) = 0;
  // Must be called before anything else.  Return true iff startup was successful.

  virtual void StartHosting(const string& data) = 0;
  virtual void StopHosting() = 0;

  // You can only search for hosts on one port at a time.  If you call this again with a different
  // port, responses to previous FindHosts requests will be ignored.
  virtual void FindHosts(int port) = 0;
  virtual void ClearHosts() = 0;
  virtual vector<pair<GlopNetworkAddress, string> > AvailableHosts() const = 0;
  virtual void Connect(GlopNetworkAddress gna) = 0;
  virtual void Disconnect(GlopNetworkAddress gna) = 0;
  virtual vector<GlopNetworkAddress> GetConnections() const = 0;

  virtual void SendData(GlopNetworkAddress gna, const string& data) = 0;
  virtual bool ReceiveData(GlopNetworkAddress* gna, string* data) = 0;
  virtual bool ReceiveData(GlopNetworkAddress gna, string* data) = 0;
  virtual bool ReceiveData(GlopNetworkAddress* gna, const string& data) = 0;

  virtual int PendingData() const = 0;

  virtual void Think() = 0;
};

#endif // GLOP_NET_NETWORK_MANAGER_INTERFACE_H__
