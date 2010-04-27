#include <stdio.h>

#include <gtest/gtest.h>
#include "third_party/raknet/RakPeerInterface.h"
#include "third_party/raknet/RakNetworkFactory.h"
#include "NetworkManager.h"
#include "../System.h"
#include "../Thread.h"

#include <set>
using namespace std;

class GlopEnvironment : public testing::Environment {
 public:
  virtual void SetUp() {
    System::Init();
  }
};
testing::Environment* const global_env = testing::AddGlobalTestEnvironment(new GlopEnvironment);

class SendDataThread : public Thread {
 public:
  SendDataThread(NetworkManager* network_manager, GlopNetworkAddress gna, const string& data)
    : network_manager_(network_manager),
      gna_(gna),
      data_(data) {
  }
 protected:
  virtual void Run() {
    network_manager_->SendData(gna_, data_);
  }
 private:
  NetworkManager* network_manager_;
  GlopNetworkAddress gna_;
  const string& data_;
};

// Waits a short time for exactly num_hosts to be established
int WaitForHosts(vector<NetworkManager*> nm, int port, int index, int num_hosts) {
  int t = system()->GetTime();
  nm[index]->ClearHosts();
  while (nm[index]->AvailableHosts().size() != num_hosts && t + 250 > system()->GetTime()) {
    for (int i = 0; i < nm.size(); i++) {
      nm[i]->Think();
    }
    system()->Sleep(1);
    nm[index]->FindHosts(port);
  }
  return nm[index]->AvailableHosts().size();
}
int WaitForHosts(NetworkManager* nm, int port, int num_hosts) {
  return WaitForHosts(vector<NetworkManager*>(1, nm), port, 0, num_hosts);
}

// Waits a short time and for exactly num_connections to be established
int WaitForConnections(vector<NetworkManager*> nm, int index, int num_connections) {
  int t = system()->GetTime();
  while (nm[index]->GetConnections().size() != num_connections && t + 250 > system()->GetTime()) {
    for (int i = 0; i < nm.size(); i++) {
      nm[i]->Think();
    }
    system()->Sleep(1);
  }
  return nm[index]->GetConnections().size();
}
int WaitForConnections(NetworkManager* nm, int num_connections) {
  return WaitForConnections(vector<NetworkManager*>(1, nm), 0, num_connections);
}

// Waits a short time for num_messages messages to be received
vector<pair<GlopNetworkAddress, string> > WaitForData(NetworkManager* nm, int num_messages) {
  int t = system()->GetTime();
  vector<pair<GlopNetworkAddress, string> > ret;
  while (ret.size() < num_messages && t + 250 > system()->GetTime()) {
    nm->Think();
    GlopNetworkAddress gna;
    string data;
    if (nm->ReceiveData(&gna, &data)) {
      ret.push_back(pair<GlopNetworkAddress, string>(gna, data));
    }
    system()->Sleep(1);
  }
  return ret;
}

// Waits a short time for num_messages messages to be received from a certain person
vector<string> WaitForData(
    NetworkManager* nm,
    GlopNetworkAddress gna,
    int num_messages) {
  int t = system()->GetTime();
  vector<string> ret;
  // We'll wait up to 5 seconds here so that our large packets have lots of time to make it through.
  while (ret.size() < num_messages && t + 5000 > system()->GetTime()) {
    nm->Think();
    string data;
    if (nm->ReceiveData(gna, &data)) {
      ret.push_back(data);
    }
    system()->Sleep(1);
  }
  return ret;
}

// Simply makes sure that a manager closes all of its open connections when it is destroyed.
TEST(NetTest, TestNetworkManagersConstructAndDeconstructProperly) {
  {
    NetworkManager host;
    ASSERT_TRUE(host.Startup(65000));
  }
  {
    NetworkManager host;
    ASSERT_TRUE(host.Startup(65000));
  }
}

// TODO: This test requires that the computer actually be connceted to a router, perhaps some sort
// of mock router can be created so this isn't required?  Raknet would have to have support for this
// sort of thing.
TEST(NetTest, TestMultipleClientsCanConnectToASingleHost) {
  NetworkManager host;
  host.Startup(65000);

  vector<NetworkManager*> clients(5);
  for (int i = 0; i < 5; i++) {
    clients[i] = new NetworkManager;
    clients[i]->Startup(65001 + i);
  }
  host.StartHosting("foobar thundergun");

  for (int i = 0; i < clients.size(); i++) {
    ASSERT_EQ(1, WaitForHosts(clients, 65000, i, 1));
  }

  for (int i = 0; i < clients.size(); i++) {
    clients[i]->Connect(clients[i]->AvailableHosts()[0].first);
    ASSERT_EQ(1, WaitForConnections(clients, i, 1));
  }
  ASSERT_EQ(5, WaitForConnections(&host, 5));;

  for (int i = 0; i < clients.size(); i++) {
    delete clients[i];
  }
}

TEST(NetTest, TestClientCanSendMultiplePacketsToHostThatAllArrive) {
  NetworkManager host;
  host.Startup(65000);
  NetworkManager client;
  client.Startup(65001);

  host.StartHosting("A");

  client.FindHosts(65000);

  bool host_found = false;
  int t = system()->GetTime();
  while (!host_found && t + 2000 > system()->GetTime()) {
    host.Think();
    client.Think();
    host_found = client.AvailableHosts().size() == 1;
  }
  ASSERT_TRUE(host_found) << "Unable to find host.";
  vector<pair<GlopNetworkAddress, string> > hosts = client.AvailableHosts();
  ASSERT_EQ("A", hosts[0].second);

  client.Connect(client.AvailableHosts()[0].first);
  ASSERT_EQ(1, WaitForConnections(&client, 1));
  ASSERT_EQ(1, WaitForConnections(&host, 1));

  GlopNetworkAddress host_gna = hosts[0].first;
  client.SendData(host_gna, "foobar");
  client.SendData(host_gna, "wingding");
  client.SendData(host_gna, "thundergun");

  bool data_available = false;
  t = system()->GetTime();
  while (!data_available && t + 2000 > system()->GetTime()) {
    host.Think();
    client.Think();
    data_available = host.PendingData() == 3;
  }

  GlopNetworkAddress client_gna = host.GetConnections()[0];
  GlopNetworkAddress client_message_gna;
  string data;
  set<string> data_set;
  ASSERT_TRUE(host.ReceiveData(&client_message_gna, &data));
  EXPECT_EQ(client_gna.first, client_message_gna.first);
  EXPECT_EQ(client_gna.second, client_message_gna.second);
  if (data == "foobar" || data == "wingding" || data == "thundergun") {
    data_set.insert(data);
  }
  EXPECT_EQ(1, data_set.size()) << "Unexpected data received";
  ASSERT_TRUE(host.ReceiveData(&client_message_gna, &data));
  EXPECT_EQ(client_gna.first, client_message_gna.first);
  EXPECT_EQ(client_gna.second, client_message_gna.second);
  if (data == "foobar" || data == "wingding" || data == "thundergun") {
    data_set.insert(data);
  }
  EXPECT_EQ(2, data_set.size()) << "Unexpected data received";
  ASSERT_TRUE(host.ReceiveData(&client_message_gna, &data));
  EXPECT_EQ(client_gna.first, client_message_gna.first);
  EXPECT_EQ(client_gna.second, client_message_gna.second);
  if (data == "foobar" || data == "wingding" || data == "thundergun") {
    data_set.insert(data);
  }
  EXPECT_EQ(3, data_set.size()) << "Unexpected data received";
}

TEST(NetTest, TestClientCanSendVeryLargePacket) {
  NetworkManager host;
  host.Startup(65000);
  NetworkManager client;
  client.Startup(65001);

  host.StartHosting("A");

  client.FindHosts(65000);

  bool host_found = false;
  int t = system()->GetTime();
  while (!host_found && t + 2000 > system()->GetTime()) {
    host.Think();
    client.Think();
    host_found = client.AvailableHosts().size() == 1;
  }
  ASSERT_TRUE(host_found) << "Unable to find host.";
  vector<pair<GlopNetworkAddress, string> > hosts = client.AvailableHosts();
  ASSERT_EQ("A", hosts[0].second);
  
  client.Connect(client.AvailableHosts()[0].first);
  ASSERT_EQ(1, WaitForConnections(&client, 1));
  ASSERT_EQ(1, WaitForConnections(&host, 1));

  GlopNetworkAddress host_gna = hosts[0].first;
  string large_data(10000000, 'X');  // 10 megs of 'X'
  client.SendData(host_gna, large_data);

  bool data_available = false;

  GlopNetworkAddress client_gna = host.GetConnections()[0];
  GlopNetworkAddress client_message_gna;
  vector<string> v = WaitForData(&host, client_gna, 1);
  ASSERT_EQ(1, v.size());
  ASSERT_EQ(large_data.size(), v[0].size()) << "large_data size is " << large_data.size()
      << ", but only " << v[0].size() << " was received.";
  for (int i = 0; i < large_data.size(); i++) {
    // Don't want to check the whole string otherwise the error message would be a disaster
    EXPECT_EQ(large_data[i], v[0][i]);
  }
}

/* I think it would be good for send data to be reentrant.  It isn't right now, but this would be a
   test for it once it is.
TEST(NetTest, TestSendDataIsReentrant) {
  NetworkManager host;
  host.Startup(65000);
  NetworkManager client;
  client.Startup(65001);

  host.StartHosting("A");

  client.FindHosts(65000);

  bool host_found = false;
  int t = system()->GetTime();
  while (!host_found && t + 2000 > system()->GetTime()) {
    host.Think();
    client.Think();
    host_found = client.AvailableHosts().size() == 1;
  }
  ASSERT_TRUE(host_found) << "Unable to find host.";
  vector<pair<GlopNetworkAddress, string> > hosts = client.AvailableHosts();
  ASSERT_EQ("A", hosts[0].second);
  
  client.Connect(client.AvailableHosts()[0].first);
  ASSERT_EQ(1, WaitForConnections(&client, 1));
  ASSERT_EQ(1, WaitForConnections(&host, 1));

  int test_size = 1000000;
  int num_threads = 5;
  vector<Thread*> vt;
  vector<string> vs;
  for (int i = 0; i < num_threads; i++) {
    vs.push_back(string(test_size, 'A' + i));
  }
  for (int i = 0; i < num_threads; i++) {
    vt.push_back(
        new SendDataThread(&client, client.AvailableHosts()[0].first, vs[i]));
  }
  for (int i = 0; i < num_threads; i++) {
    vt[i]->Start();
  }
  for (int i = 0; i < num_threads; i++) {
    vt[i]->Join();
  }

  bool data_available = false;

  GlopNetworkAddress client_gna = host.GetConnections()[0];
  vector<string> v = WaitForData(&host, client_gna, num_threads);
  ASSERT_EQ(num_threads, v.size());
  set<char> s;
  for (int i = 0; i < v.size(); i++) {
    s.insert(v[i][0]);
    EXPECT_EQ(test_size, v[i].size());
    bool all_equal = true;
    for (int j = 0; j < v[i].size(); j++) {
      if (v[i][j] != v[i][0]) {
        all_equal = false;
        break;
      }
    }
    EXPECT_TRUE(all_equal) << "String didn't come through correctly.";
  }
}
*/

TEST(NetTest, TestThatStartAndStopHostingWork) {
  NetworkManager host;
  host.Startup(65000);
  NetworkManager client;
  client.Startup(65001);

  host.StartHosting("host");
  ASSERT_EQ(1, WaitForHosts(&client, 65000, 1));

  host.StopHosting();
  ASSERT_EQ(0, WaitForHosts(&client, 65000, 0));
}

TEST(NetTest, TestMultiplePeersCanSeeEachOther) {
  vector<NetworkManager*> c(3);
  for (int i = 0; i < c.size(); i++) {
    c[i] = new NetworkManager;
    c[i]->Startup(65000 + i);
  }
  c[0]->StartHosting("0");
  c[1]->StartHosting("1");
  c[2]->StartHosting("2");

  ASSERT_EQ(0, WaitForHosts(c, 65000, 0, 0));
  ASSERT_EQ(1, WaitForHosts(c, 65001, 0, 1));
  EXPECT_EQ("1", c[0]->AvailableHosts()[0].second);
  ASSERT_EQ(1, WaitForHosts(c, 65002, 0, 1));
  EXPECT_EQ("2", c[0]->AvailableHosts()[0].second);

  ASSERT_EQ(1, WaitForHosts(c, 65000, 1, 1));
  EXPECT_EQ("0", c[1]->AvailableHosts()[0].second);
  ASSERT_EQ(0, WaitForHosts(c, 65001, 1, 0));
  ASSERT_EQ(1, WaitForHosts(c, 65002, 1, 1));
  EXPECT_EQ("2", c[1]->AvailableHosts()[0].second);

  ASSERT_EQ(1, WaitForHosts(c, 65000, 2, 1));
  EXPECT_EQ("0", c[2]->AvailableHosts()[0].second);
  ASSERT_EQ(1, WaitForHosts(c, 65001, 2, 1));
  EXPECT_EQ("1", c[2]->AvailableHosts()[0].second);
  ASSERT_EQ(0, WaitForHosts(c, 65002, 2, 0));

  for (int i = 0; i < c.size(); i++) {
    delete c[i];
  }
}

// Create a fully connected graph, then make sure everyone can send messages to everyone else.
TEST(NetTest, TestMultiplePeersCanCommunicateWithEachOther) {
  vector<NetworkManager*> peer(5);
  for (int i = 0; i < peer.size(); i++) {
    peer[i] = new NetworkManager;
    peer[i]->Startup(65000 + i);
    char buf[16];
    sprintf(buf, "%d", i);
    string id(buf);
    peer[i]->StartHosting(id);
  }

  // Make a completely connected graph
  for (int i = 0; i < peer.size(); i++) {
    for (int j = i + 1; j < peer.size(); j++) {
      ASSERT_EQ(1, WaitForHosts(peer, 65000 + j, i, 1));

      char buf[16];
      sprintf(buf, "%d", j);
      string id(buf);
      EXPECT_EQ(id, peer[i]->AvailableHosts()[0].second);
      peer[i]->Connect(peer[i]->AvailableHosts()[0].first);
      EXPECT_EQ(j, WaitForConnections(peer, i, j));
    }
  }
  for (int i = 0; i < peer.size(); i++) {
    EXPECT_EQ(peer.size() - 1, WaitForConnections(peer, i, peer.size() - 1));
  }

  // Now each person sends a unique string to everyone else.
  vector<string> data(peer.size());
  data[0] = "aaaaaaaaaaaaaaaaaaaaa";
  data[1] = "foobar thundergun";
  data[2] = "1234567890!@#$^&*(),./;'[]";
  data[3] = ".";
  data[4] = "";
  for (int i = 0; i < peer.size(); i++) {
    vector<GlopNetworkAddress> connections = peer[i]->GetConnections();
    for (int j = 0; j < connections.size(); j++) {
      peer[i]->SendData(connections[j], data[i]);
    }
  }

  for (int i = 0; i < peer.size(); i++) {
    GlopNetworkAddress gna;
    string message;
    int count = 0;
    vector<pair<GlopNetworkAddress, string> > messages = WaitForData(peer[i], peer.size() - 1);
    EXPECT_EQ(peer.size() - 1, messages.size());
    for (int j = 0; j < messages.size(); j++) {
      // messages[j].first.second is the port that this messages was sent from, so that minus 65000
      // will give us an index into data.
      EXPECT_EQ(data[messages[j].first.second - 65000], messages[j].second);
    }
  }

  for (int i = 0; i < peer.size(); i++) {
    delete peer[i];
  }
}

TEST(NetTest, TestMultiplePeersCanCommunicateWithEachOtherWithAlternateReceiveEvents) {
  vector<NetworkManager*> peer(5);
  for (int i = 0; i < peer.size(); i++) {
    peer[i] = new NetworkManager;
    peer[i]->Startup(65000 + i);
    char buf[16];
    sprintf(buf, "%d", i);
    string id(buf);
    peer[i]->StartHosting(id);
  }

  // Make a completely connected graph
  for (int i = 0; i < peer.size(); i++) {
    for (int j = i + 1; j < peer.size(); j++) {
      ASSERT_EQ(1, WaitForHosts(peer, 65000 + j, i, 1));
      char buf[16];
      sprintf(buf, "%d", j);
      string id(buf);
      EXPECT_EQ(id, peer[i]->AvailableHosts()[0].second);
      peer[i]->Connect(peer[i]->AvailableHosts()[0].first);
      EXPECT_EQ(j, WaitForConnections(peer, i, j));
    }
  }
  for (int i = 0; i < peer.size(); i++) {
    EXPECT_EQ(peer.size() - 1, WaitForConnections(peer, i, peer.size() - 1));
  }
  // All the connections are on this machine, so rather than store all of the addresses, we just
  // store this.
  unsigned int local_host = peer[0]->AvailableHosts()[0].first.first;

  // Now each person sends a unique string to everyone else.
  vector<string> data(peer.size());
  data[0] = "aaaaaaaaaaaaaaaaaaaaa";
  data[1] = "foobar thundergun";
  data[2] = "1234567890!@#$^&*(),./;'[]";
  data[3] = ".";
  data[4] = "";
  for (int i = 0; i < peer.size(); i++) {
    vector<GlopNetworkAddress> connections = peer[i]->GetConnections();
    for (int j = 0; j < connections.size(); j++) {
      peer[i]->SendData(connections[j], data[i]);
    }
  }

  for (int i = 0; i < peer.size(); i++) {
    for (int j = 0; j < peer.size(); j++) {
      vector<string> messages =
          WaitForData(peer[i], GlopNetworkAddress(local_host, 65000 + j), (i==j) ? 0 : 1);
      if (i == j) {
        EXPECT_EQ(0, messages.size());
      } else {
        EXPECT_EQ(1, messages.size());
        if (messages.size() == 1) {
          EXPECT_EQ(data[j], messages[0]);
        }
      }
    }
  }

  for (int i = 0; i < peer.size(); i++) {
    delete peer[i];
  }
}
