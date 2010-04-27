#include <gtest/gtest.h>
#include "MockNetworkManager.h"

// Simply makes sure that a manager closes all of its open connections when it is destroyed.
TEST(MockNetTest, TestNetworkManagersConstructAndDeconstructProperly) {
  MockRouter router;
  {
    MockNetworkManager host(&router);
    ASSERT_TRUE(host.Startup(65000));
  }
  {
    MockNetworkManager host(&router);
    ASSERT_TRUE(host.Startup(65000));
  }
}

void ThinkAll(vector<MockNetworkManager*> v) {
  for (int i = 0; i < v.size(); i++) {
    v[i]->Think();
  }
}

TEST(MockNetTest, TestMultipleClientsCanConnectToASingleHost) {
  MockRouter router;
  MockNetworkManager host(&router);
  ASSERT_TRUE(host.Startup(65000));
  host.StartHosting("foobar thundergun");

  vector<MockNetworkManager*> clients(5);
  for (int i = 0; i < 5; i++) {
    clients[i] = new MockNetworkManager(&router);
    clients[i]->Startup(65001 + i);
    clients[i]->FindHosts(65000);
  }

  ThinkAll(clients);
  host.Think();
  for (int i = 0; i < clients.size(); i++) {
    ASSERT_EQ(1, clients[i]->AvailableHosts().size());
  }

  for (int i = 0; i < clients.size(); i++) {
    clients[i]->Connect(clients[i]->AvailableHosts()[0].first);
    EXPECT_EQ(1, clients[i]->GetConnections().size());
  }
  EXPECT_EQ(5, host.GetConnections().size());

  for (int i = 0; i < 5; i++) {
    delete clients[i];
  }
}

TEST(MockNetTest, TestClientCanSendMultiplePacketsToHostThatAllArrive) {
  MockRouter router;
  MockNetworkManager host(&router);
  MockNetworkManager client(&router);

  vector<MockNetworkManager*> all;
  all.push_back(&host);
  all.push_back(&client);

  host.Startup(65000);
  client.Startup(65001);

  host.StartHosting("A");
  client.FindHosts(65000);

  ThinkAll(all);
  vector<pair<GlopNetworkAddress, string> > hosts = client.AvailableHosts();
  ASSERT_EQ(1, hosts.size()) << "Unable to find host.";
  ASSERT_EQ("A", hosts[0].second);

  client.Connect(client.AvailableHosts()[0].first);
  ThinkAll(all);
  ASSERT_EQ(1, client.GetConnections().size());
  ASSERT_EQ(1, host.GetConnections().size());

  GlopNetworkAddress host_gna = hosts[0].first;
  client.SendData(host_gna, "foobar");
  client.SendData(host_gna, "wingding");
  client.SendData(host_gna, "thundergun");

  ThinkAll(all);

  EXPECT_EQ(3, host.PendingData());

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


TEST(MockNetTest, TestThatStartAndStopHostingWork) {
  MockRouter router;
  MockNetworkManager host(&router);
  MockNetworkManager client(&router);
  host.Startup(65000);
  client.Startup(65001);

  host.StartHosting("host");
  client.FindHosts(65000);
  host.Think();
  client.Think();
  EXPECT_EQ(1, client.AvailableHosts().size());

  client.ClearHosts();
  host.StopHosting();
  client.FindHosts(65000);
  host.Think();
  client.Think();
  EXPECT_EQ(0, client.AvailableHosts().size());
}

TEST(MockNetTest, TestMultiplePeersCanSeeEachOther) {
  MockRouter router;
  vector<MockNetworkManager*> c(3);
  for (int i = 0; i < c.size(); i++) {
    c[i] = new MockNetworkManager(&router);
    c[i]->Startup(65000 + i);
  }
  c[0]->StartHosting("0");
  c[1]->StartHosting("1");
  c[2]->StartHosting("2");

  vector<pair<GlopNetworkAddress, string> > hosts;

  c[0]->FindHosts(65001);
  c[0]->Think();
  hosts = c[0]->AvailableHosts();
  ASSERT_EQ(1, hosts.size());
  EXPECT_EQ(65001, hosts[0].first.second);
  EXPECT_EQ("1", hosts[0].second);
  c[0]->ClearHosts();
  c[0]->FindHosts(65002);
  c[0]->Think();
  hosts = c[0]->AvailableHosts();
  ASSERT_EQ(1, hosts.size());
  EXPECT_EQ(65002, hosts[0].first.second);
  EXPECT_EQ("2", hosts[0].second);

  c[1]->FindHosts(65000);
  c[1]->Think();
  hosts = c[1]->AvailableHosts();
  ASSERT_EQ(1, hosts.size());
  EXPECT_EQ(65000, hosts[0].first.second);
  EXPECT_EQ("0", hosts[0].second);
  c[1]->ClearHosts();
  c[1]->FindHosts(65002);
  c[1]->Think();
  hosts = c[1]->AvailableHosts();
  ASSERT_EQ(1, hosts.size());
  EXPECT_EQ(65002, hosts[0].first.second);
  EXPECT_EQ("2", hosts[0].second);

  c[2]->FindHosts(65000);
  c[2]->Think();
  hosts = c[2]->AvailableHosts();
  ASSERT_EQ(1, hosts.size());
  EXPECT_EQ(65000, hosts[0].first.second);
  EXPECT_EQ("0", hosts[0].second);
  c[2]->ClearHosts();
  c[2]->FindHosts(65001);
  c[2]->Think();
  hosts = c[2]->AvailableHosts();
  ASSERT_EQ(1, hosts.size());
  EXPECT_EQ(65001, hosts[0].first.second);
  EXPECT_EQ("1", hosts[0].second);

  for (int i = 0; i < c.size(); i++) {
    delete c[i];
  }
}

// Create a fully connected graph, then make sure everyone can send messages to everyone else.
TEST(MockNetTest, TestMultiplePeersCanCommunicateWithEachOther) {
  MockRouter router;
  vector<MockNetworkManager*> peer(5);
  for (int i = 0; i < peer.size(); i++) {
    peer[i] = new MockNetworkManager(&router);
    peer[i]->Startup(65000 + i);
    char buf[16];
    sprintf(buf, "%d", i);
    string id(buf);
    peer[i]->StartHosting(id);
  }

  // Make a completely connected graph
  for (int i = 0; i < peer.size(); i++) {
    for (int j = i + 1; j < peer.size(); j++) {
      peer[i]->ClearHosts();
      peer[i]->FindHosts(65000 + j);
      ThinkAll(peer);
      ASSERT_EQ(1, peer[0]->AvailableHosts().size());

      char buf[16];
      sprintf(buf, "%d", j);
      string id(buf);
      EXPECT_EQ(id, peer[i]->AvailableHosts()[0].second);
      peer[i]->Connect(peer[i]->AvailableHosts()[0].first);
    }
  }
  for (int i = 0; i < peer.size(); i++) {
    ASSERT_EQ(peer.size() - 1, peer[i]->GetConnections().size());
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

  ThinkAll(peer);
  for (int i = 0; i < peer.size(); i++) {
    GlopNetworkAddress gna;
    string message;
    int count = 0;
    vector<pair<GlopNetworkAddress, string> > messages;
    while (peer[i]->ReceiveData(&gna, &message)) {
      messages.push_back(make_pair(gna, message));
    }
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

/*
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
*/
