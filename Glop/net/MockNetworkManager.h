#ifndef GLOP_NET_MOCK_NETWORK_MANAGER_H__
#define GLOP_NET_MOCK_NETWORK_MANAGER_H__

#include <string>
#include <vector>
#include <map>
#include <queue>
using namespace std;

#include "NetworkManagerInterface.h"
#include "../List.h"
#include "MockRouter.h"

class MockNetworkManager : public NetworkManagerInterface {
 public:
  MockNetworkManager(MockRouter* router);
  ~MockNetworkManager();

  virtual bool Startup(int port);
  // Must be called before anything else.  Return true iff startup was successful.

  virtual void StartHosting(const string& data);
  virtual void StopHosting();

  // You can only search for hosts on one port at a time.  If you call this again with a different
  // port, responses to previous FindHosts requests will be ignored.
  virtual void FindHosts(int port);
  virtual void ClearHosts();
  virtual vector<pair<GlopNetworkAddress, string> > AvailableHosts() const;
  virtual void Connect(GlopNetworkAddress gna);
  virtual void Disconnect(GlopNetworkAddress gna);
  virtual vector<GlopNetworkAddress> GetConnections() const;

  virtual void SendData(GlopNetworkAddress gna, const string& data);
  virtual bool ReceiveData(GlopNetworkAddress* gna, string* data);
  virtual bool ReceiveData(GlopNetworkAddress gna, string* data);
  virtual bool ReceiveData(GlopNetworkAddress* gna, const string& data);

  virtual int PendingData() const;

  virtual void Think();

 private:
  MockRouter* router_;
  RouterKey key_;
  int port_;
  int search_port_;
  map<GlopNetworkAddress, string> hosts_;
  List<pair<GlopNetworkAddress, string> > incoming_data_;
};



#endif // GLOP_NET_MOCK_NETWORK_MANAGER_H__
