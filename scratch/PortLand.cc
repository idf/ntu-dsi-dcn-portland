#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

#include "ns3/applications-module.h"
#include "ns3/bridge-helper.h"
#include "ns3/bridge-net-device.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-nix-vector-helper.h"
#include "ns3/network-module.h"
#include "ns3/openflow-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/random-variable.h"

using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE("PortLand-Architecture");

// Function to create address string from numbers
//
char *toString(int a, int b, int c, int d) {
  int first = a;
  int second = b;
  int third = c;
  int fourth = d;

  char *address = new char[30];
  char firstOctet[30], secondOctet[30], thirdOctet[30], fourthOctet[30];
  bzero(address, 30);

  snprintf(firstOctet, 10, "%d", first);
  strcat(firstOctet, ".");
  snprintf(secondOctet, 10, "%d", second);
  strcat(secondOctet, ".");
  snprintf(thirdOctet, 10, "%d", third);
  strcat(thirdOctet, ".");
  snprintf(fourthOctet, 10, "%d", fourth);

  strcat(thirdOctet, fourthOctet);
  strcat(secondOctet, thirdOctet);
  strcat(firstOctet, secondOctet);
  strcat(address, firstOctet);

  return address;
}

// Function to create MAC address string from numbers
//
char *toPMACAddress(int pod, int pos, int port, int vmid) {
  char *add = new char[18];

  if (pod <= 0xff && vmid <= 0xff) {
    sprintf(add, "00:%02x:%02x:%02x:00:%02x", pod, pos, port, vmid);
  } else {
    std::cout << "Not implemented yet" << std::endl;
    exit(1);
  }

  return add;
}

map<Ipv4Address, Mac48Address> fabricManagerIpPmacMap;

// Main function
//
int main(int argc, char *argv[]) {
  //=========== Define parameters based on value of k ===========//
  //
  int k = 4;                      // number of ports per switch
  int num_pod = k;                // number of pod
  int num_host = (k / 2);         // number of hosts under a switch
  int num_edge = (k / 2);         // number of edge switch in a pod
  int num_bridge = num_edge;      // number of bridge in a pod
  int num_agg = (k / 2);          // number of aggregation switch in a pod
  int num_group = k / 2;          // number of group of core switches
  int num_core = (k / 2);         // number of core switch in a group
  int total_host = k * k * k / 4; // number of hosts in the entire network
  char filename[128] =
      "statistics/PortLand.xml"; // filename for Flow Monitor xml output file

  // Define variables for On/Off Application
  int rand1 = 0; //
  int rand2 = 0; // Random values for clients' address
  int rand3 = 0; //

  // Initialize other variables
  //
  int i = 0;
  int j = 0;
  int h = 0;

  // Initialize parameters for On/Off application
  //
  int port = 9;
  int packetSize = 1024; // 1024 bytes
  char dataRate_OnOff[16] = "1Mbps";
  char maxBytes[] = "0"; // unlimited

  // Initialize parameters for Csma and PointToPoint protocol
  //
  char dataRate[16] = "1000Mbps"; // 1Gbps
  int delay = 0.001;            // 0.001 ms

	if (argc == 5){
		packetSize = std::atoi(argv[1]);
		strcpy(dataRate_OnOff, argv[2]);
		strcpy(dataRate, argv[3]);
		strcpy(filename, argv[4]);
	}

	cout << "packetSize: " << packetSize << endl;
	cout << "dataRate_OnOff: " << dataRate_OnOff << endl;
	cout << "dataRate: " << dataRate << endl;
	cout << "filename: " << filename << endl;

  // Output some useful information
  //
  std::cout << "Value of k =  " << k << "\n";
  std::cout << "Total number of hosts =  " << total_host << "\n";
  std::cout << "Number of hosts under each switch =  " << num_host << "\n";
  std::cout << "Number of edge switch under each pod =  " << num_edge << "\n";
  std::cout << "------------- "
            << "\n";

  // Initialize Internet Stack and Routing Protocols
  //
  InternetStackHelper internet;

  //=========== Creation of Node Containers ===========//
  //
  NodeContainer core[num_group]; // NodeContainer for core switches
  for (i = 0; i < num_group; i++) {
    core[i].Create(num_core);
  }
  NodeContainer agg[num_pod]; // NodeContainer for aggregation switches
  for (i = 0; i < num_pod; i++) {
    agg[i].Create(num_agg);
  }
  NodeContainer edge[num_pod]; // NodeContainer for edge switches
  for (i = 0; i < num_pod; i++) {
    edge[i].Create(num_edge);
  }
  NodeContainer host[num_pod][num_bridge]; // NodeContainer for hosts
  for (i = 0; i < k; i++) {
    for (j = 0; j < num_bridge; j++) {
      host[i][j].Create(num_host);
    }
  }

  //=========== Initialize settings for On/Off Application ===========//
  //

  // Generate traffics for the simulation
  //
  ApplicationContainer app[total_host];
  srand(1);
  for (i = 0; i < total_host; i++) {
    char *add;
    int idx = rand() % total_host;
    add = toString(10, 0, 0, idx + 1);
    OnOffHelper oo =
        OnOffHelper("ns3::UdpSocketFactory",
                    Address(InetSocketAddress(Ipv4Address(add),
                                              port)));
    oo.SetAttribute("OnTime", RandomVariableValue(ExponentialVariable(1)));
    oo.SetAttribute("OffTime", RandomVariableValue(ExponentialVariable(1)));
    oo.SetAttribute("PacketSize", UintegerValue(packetSize));
    oo.SetAttribute("DataRate", StringValue(dataRate_OnOff));
    oo.SetAttribute("MaxBytes", StringValue(maxBytes));

    // Randomly select a client
    rand1 = rand() % num_pod + 0;
    rand2 = rand() % num_edge + 0;
    rand3 = rand() % num_host + 0;

    // +2
    while ((rand1 * num_edge * num_host + rand2 * num_host + rand3) == idx) {
      rand1 = rand() % num_pod + 0;
      rand2 = rand() % num_edge + 0;
      rand3 = rand() % num_host + 0;
    } // to make sure that client and server are different

    // Install On/Off Application to the client
    NodeContainer onoff;
    onoff.Add(host[rand1][rand2].Get(rand3));
    app[i] = oo.Install(onoff);
  }
  std::cout << "Finished creating On/Off traffic"
            << "\n";

  CsmaHelper csma;
  csma.SetChannelAttribute("DataRate", StringValue(dataRate));
  csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(delay)));

  //=========== Connect edge switches to hosts ===========//
  //
  NetDeviceContainer hostSw[num_pod][num_edge];
  Ptr<OpenFlowSwitchNetDevice> edgeSwitchs[num_pod][num_edge];
  NetDeviceContainer hostDevices[num_pod][num_edge];

  int cnt = 0;
  for (i = 0; i < num_pod; i++) {
    for (j = 0; j < num_edge; j++) {
      NetDeviceContainer link1;
      for (h = 0; h < num_host; h++) {
        link1 = csma.Install(NodeContainer(edge[i].Get(j), host[i][j].Get(h)));
        hostSw[i][j].Add(link1.Get(0)); // duplication?
        hostDevices[i][j].Add(link1.Get(1));

        // Assign PMAC
        Mac48Address pmac = Mac48Address(toPMACAddress(i + 1, j, h, 1));
        Ipv4Address ip = Ipv4Address(toString(10, 0, 0, ++cnt));
        link1.Get(1)->SetAddress(pmac);
        fabricManagerIpPmacMap.insert(pair<Ipv4Address, Mac48Address>(ip, pmac));
      }
      // add switch
      Ptr<Node> switchNode = edge[i].Get(j);
      Ptr<ns3::ofi::LearningController> controller =
          CreateObject<ns3::ofi::LearningController>();
      edgeSwitchs[i][j] = new OpenFlowSwitchNetDevice();
      edgeSwitchs[i][j]->SetController(controller);
      switchNode->AddDevice(edgeSwitchs[i][j]);

      edgeSwitchs[i][j]->m_pod = i;
      edgeSwitchs[i][j]->m_pos = j;
      edgeSwitchs[i][j]->m_level = 0;

      for (h = 0; h < num_host; h++) {
        edgeSwitchs[i][j]->m_port_dir.insert(make_pair(h, false));
        edgeSwitchs[i][j]->AddSwitchPort(hostSw[i][j].Get(h));
      }
    }
  }

  for (i = 0; i < num_pod; i++) {
    for (j = 0; j < num_edge; j++) {
      edgeSwitchs[i][j]->IP_MAC_MAP = fabricManagerIpPmacMap;
      for (h = 0; h < num_host; h++) {
        host[i][j].Get(h)->IP_MAC_MAP = fabricManagerIpPmacMap;
      }
    }
  }
  std::cout << "Finished connecting edge switches and hosts  "
            << "\n";

  //=========== Connect aggregate switches to edge switches ===========//
  NetDeviceContainer ae[num_pod][num_agg][num_edge];
  Ptr<OpenFlowSwitchNetDevice> aggSwitchs[num_pod][num_agg];
  for (i = 0; i < num_pod; i++) {
    for (j = 0; j < num_agg; j++) {
      for (h = 0; h < num_edge; h++) {
        ae[i][j][h] =
            csma.Install(NodeContainer(agg[i].Get(j), edge[i].Get(h)));
      }
      // add agg switch
      Ptr<Node> switchNode = agg[i].Get(j);
      Ptr<ns3::ofi::LearningController> controller =
          CreateObject<ns3::ofi::LearningController>();
      aggSwitchs[i][j] = new OpenFlowSwitchNetDevice();
      aggSwitchs[i][j]->SetController(controller);
      switchNode->AddDevice(aggSwitchs[i][j]);

      aggSwitchs[i][j]->m_pod = i;
      aggSwitchs[i][j]->m_pos = -1;
      aggSwitchs[i][j]->m_level = 1;
      aggSwitchs[i][j]->IP_MAC_MAP = fabricManagerIpPmacMap;

      for (h = 0; h < num_edge; h++) {
        aggSwitchs[i][j]->m_port_dir.insert(make_pair(h, false));
        aggSwitchs[i][j]->AddSwitchPort(ae[i][j][h].Get(0));
      }
    }
  }
  for (i = 0; i < num_pod; i++) {
    for (h = 0; h < num_edge; h++) {
      for (j = 0; j < num_agg; j++) {
        edgeSwitchs[i][h]->m_port_dir.insert(make_pair(j + (k / 2), true));
        edgeSwitchs[i][h]->AddSwitchPort(ae[i][j][h].Get(1)); // up arbitrary
      }
    }
  }
  std::cout << "Finished connecting aggregation switches and edge switches  "
            << "\n";

  //=========== Connect core switches to aggregate switches ===========//
  //
  NetDeviceContainer ca[num_group][num_core][num_pod];
  Ptr<OpenFlowSwitchNetDevice> coreSwitchs[num_group][num_core];
  for (i = 0; i < num_group; i++) {
    for (j = 0; j < num_core; j++) {
      for (h = 0; h < num_pod; h++) {
        ca[i][j][h] =
            csma.Install(NodeContainer(core[i].Get(j), agg[h].Get(i)));
      }

      // add switch
      Ptr<Node> switchNode = core[i].Get(j);
      Ptr<ns3::ofi::LearningController> controller =
          CreateObject<ns3::ofi::LearningController>();
      coreSwitchs[i][j] = new OpenFlowSwitchNetDevice();
      coreSwitchs[i][j]->SetController(controller);
      switchNode->AddDevice(coreSwitchs[i][j]);

      coreSwitchs[i][j]->m_pod = -1;
      coreSwitchs[i][j]->m_pos = -1;
      coreSwitchs[i][j]->m_level = 2;
      coreSwitchs[i][j]->IP_MAC_MAP = fabricManagerIpPmacMap;

      for (h = 0; h < num_pod; h++) {
        coreSwitchs[i][j]->m_port_dir.insert(make_pair(h, false));
        coreSwitchs[i][j]->AddSwitchPort(ca[i][j][h].Get(0));
      }
    }
  }

  for (i = 0; i < num_group; i++) {
    for (h = 0; h < num_pod; h++) {
      for (j = 0; j < num_core; j++) {
        aggSwitchs[h][i]->m_port_dir.insert(make_pair(j + (k / 2), true));
        aggSwitchs[h][i]->AddSwitchPort(ca[i][j][h].Get(1));
      }
    }
  }
  std::cout << "Finished connecting core switches and aggregation switches  "
            << "\n";

  // Inintialize Address Helper
  //
  Ipv4AddressHelper address;

  // Incrementally assign IP addresses to hosts
  char *subnet;
  subnet = toString(10, 0, 0, 0);
  address.SetBase(subnet, "255.0.0.0");
  NodeContainer hostNodeContainer;
  NetDeviceContainer hostNetDeviceContainer;
  for (int i = 0; i < num_pod; ++i) {
    for (int j = 0; j < num_edge; ++j) {
      for (int h = 0; h < num_host; ++h) {
        hostNetDeviceContainer.Add(hostDevices[i][j].Get(h));
        hostNodeContainer.Add(host[i][j].Get(h));
      }
    }
  }
  internet.Install(hostNodeContainer);
  address.Assign(hostNetDeviceContainer);

  //=========== Start the simulation ===========//
  //
  std::cout << "Start Simulation.. "
            << "\n";
  double stop_time = 10.0; // 100.0;
  for (i = 0; i < total_host; i++) {
    app[i].Start(Seconds(0.0));
    app[i].Stop(Seconds(stop_time));
  }
  // Calculate Throughput using Flowmonitor
  //
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  // Run simulation.
  Packet::EnablePrinting();
  NS_LOG_INFO("Run Simulation.");
  Simulator::Stop(Seconds(stop_time + 1));
  Simulator::Run();

  monitor->CheckForLostPackets();
  monitor->SerializeToXmlFile(filename, true, true);

  std::cout << "Simulation finished "
            << "\n";
  Simulator::Destroy();
  NS_LOG_INFO("Done.");
  return 0;
}
