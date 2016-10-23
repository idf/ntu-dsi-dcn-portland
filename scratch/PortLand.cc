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

// Function to create MAC address string from numbers
//
char *toPMAC(int pod, int position, int port, int vmid) {
  char *macAddress = new char[18];

  if (pod <= 0xff && vmid <= 0xff)
    sprintf(macAddress, "00:%02x:%02x:%02x:00:%02x", pod, position, port, vmid);
  else
    // Not implemented yet.
    exit(1);

  return macAddress;
}

// Function to create address string from numbers
//
char *toString(int a, int b, int c, int d) {

  int first = a;
  int second = b;
  int third = c;
  int fourth = d;

  char *address = new char[30];
  char firstOctet[30], secondOctet[30], thirdOctet[30], fourthOctet[30];
  // address = firstOctet.secondOctet.thirdOctet.fourthOctet;

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

// Fabric Manager
typedef map<Mac48Address, Ipv4Address> MacIpMap;
typedef map<Ipv4Address, Mac48Address> IpMacMap;
MacIpMap macIpMap;
IpMacMap ipMacMap;

// Main function
//
int main(int argc, char *argv[]) {
  /*
  export NS_LOG=PortLand-Architecture=level_all
  export NS_LOG=OpenFlowSwitchNetDevice=level_all
  */
  LogComponentEnable("PortLand-Architecture", LOG_LEVEL_INFO);
  LogComponentEnable("OpenFlowSwitchNetDevice", LOG_LEVEL_INFO);

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
  char filename[] =
      "statistics/PortLand.xml"; // filename for Flow Monitor xml output file

  // Define variables for On/Off Application
  // These values will be used to serve the purpose that addresses of server and
  // client are selected randomly
  // Note: the format of host's address is 10.pod.switch.(host+2)
  //
  int podRand = 0;  //
  int swRand = 0;   // Random values for servers' address
  int hostRand = 0; //

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
  char dataRate_OnOff[] = "1Mbps";
  char maxBytes[] = "0"; // unlimited

  // Initialize parameters for Csma and PointToPoint protocol
  //
  char dataRate[] = "1000Mbps"; // 1Gbps
  int delay = 0.001;            // 0.001 ms

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
  // Ipv4NixVectorHelper nixRouting;
  // Ipv4StaticRoutingHelper staticRouting;
  // Ipv4ListRoutingHelper list;
  // list.Add (staticRouting, 0);
  // list.Add (nixRouting, 10);
  // internet.SetRoutingHelper(list);

  //=========== Creation of Node Containers ===========//
  //
  NodeContainer core[num_group]; // NodeContainer for core switches
  for (i = 0; i < num_group; i++) {
    core[i].Create(num_core);
    // internet.Install (core[i]);
  }
  NodeContainer agg[num_pod]; // NodeContainer for aggregation switches
  for (i = 0; i < num_pod; i++) {
    agg[i].Create(num_agg);
    // internet.Install (agg[i]);
  }
  NodeContainer edge[num_pod]; // NodeContainer for edge switches
  for (i = 0; i < num_pod; i++) {
    edge[i].Create(num_edge);
    // internet.Install (edge[i]);
  }
  // NodeContainer bridge[num_pod];                // NodeContainer for edge
  // bridges
  //      for (i=0; i<num_pod;i++){
  //     bridge[i].Create (num_bridge);
  //     internet.Install (bridge[i]);
  // }
  NodeContainer host[num_pod][num_bridge]; // NodeContainer for hosts
  for (i = 0; i < k; i++) {
    for (j = 0; j < num_bridge; j++) {
      host[i][j].Create(num_host);
      internet.Install(host[i][j]);
    }
  }

  //=========== Initialize settings for On/Off Application ===========//
  //

  // Generate traffics for the simulation
  //
  ApplicationContainer app[total_host];
  for (i = 0; i < total_host; i++) {
    // Randomly select a server
    podRand = rand() % num_pod + 0;
    swRand = rand() % num_edge + 0;
    hostRand = rand() % num_host + 0;
    hostRand = hostRand + 2; // why add 2? due to bridge
//    podRand = 0;
//    swRand = 0;
//    hostRand = 1;
    char *add;
    add = toString(10, podRand, swRand, hostRand);

    // Initialize On/Off Application with addresss of server
    OnOffHelper oo =
        OnOffHelper("ns3::UdpSocketFactory",
                    Address(InetSocketAddress(Ipv4Address(add),
                                              port))); // ip address of server
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
    while (rand1 == podRand && swRand == rand2 && (rand3 + 2) == hostRand) {
      rand1 = rand() % num_pod + 0;
      rand2 = rand() % num_edge + 0;
      rand3 = rand() % num_host + 0;
    } // to make sure that client and server are different

   // rand1 = 0;
   // rand2 = 0;
   // rand3 = 1;

    // Install On/Off Application to the client
    NodeContainer onoff;
    onoff.Add(host[rand1][rand2].Get(rand3));
    app[i] = oo.Install(onoff);
  }
  std::cout << "Finished creating On/Off traffic"
            << "\n";

  // Inintialize Address Helper
  //
  Ipv4AddressHelper address;

  // Initialize PointtoPoint helper
  //
  // PointToPointHelper p2p;
  //      p2p.SetDeviceAttribute ("DataRate", StringValue (dataRate));
  //      p2p.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (delay)));

  // Initialize Csma helper
  //
  CsmaHelper csma;
  csma.SetChannelAttribute("DataRate", StringValue(dataRate));
  csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(delay)));

  //=========== Connect edge switches to hosts ===========//
  //
  NetDeviceContainer hostSw[num_pod][num_edge];
  // NetDeviceContainer bridgeDevices[num_pod][num_bridge];
  Ptr<OpenFlowSwitchNetDevice> edgeSwtchs[num_pod][num_edge];
  NetDeviceContainer hostDevices[num_pod][num_edge];
  Ipv4InterfaceContainer ipContainer[num_pod][num_edge];

  for (i = 0; i < num_pod; i++) {
    for (j = 0; j < num_edge; j++) {
      // NetDeviceContainer link1 = csma.Install(NodeContainer (edge[i].Get(j),
      // bridge[i].Get(j)));
      // hostSw[i][j].Add(link1.Get(0));
      // bridgeDevices[i][j].Add(link1.Get(1));
      NetDeviceContainer link1;
      for (h = 0; h < num_host; h++) {
        link1 = csma.Install(NodeContainer(edge[i].Get(j), host[i][j].Get(h)));
        hostSw[i][j].Add(link1.Get(0)); // duplication?
        // NetDeviceContainer link2 = csma.Install (NodeContainer
        // (host[i][j].Get(h), bridge[i].Get(j)));
        // hostSw[i][j].Add(link2.Get(0));
        // bridgeDevices[i][j].Add(link2.Get(1));
        hostDevices[i][j].Add(link1.Get(1));

        // Assign PMAC
        Mac48Address pmac = Mac48Address(toPMAC(i, j, h, 1));
        Ipv4Address ip = Ipv4Address(toString(10, i, j, h + 1));
        link1.Get(1)->SetAddress(pmac);
        macIpMap.insert(pair<Mac48Address, Ipv4Address>(pmac, ip));
        ipMacMap.insert(pair<Ipv4Address, Mac48Address>(ip, pmac));
      }
      // add switch
      Ptr<Node> switchNode = edge[i].Get(j);
      Ptr<ns3::ofi::LearningController> controller =
          CreateObject<ns3::ofi::LearningController>();
      edgeSwtchs[i][j] = new OpenFlowSwitchNetDevice();
      edgeSwtchs[i][j]->SetController(controller);
      switchNode->AddDevice(edgeSwtchs[i][j]);

      edgeSwtchs[i][j]->m_pod = i;
      edgeSwtchs[i][j]->m_pos = j;
      edgeSwtchs[i][j]->m_level = 0;
      edgeSwtchs[i][j]->IP_MAC_MAP = ipMacMap;

      for (h = 0; h < num_host; h++) {
        edgeSwtchs[i][j]->m_port_dir.insert(make_pair(h, false));
        edgeSwtchs[i][j]->AddSwitchPort(hostSw[i][j].Get(h));
      }

      // BridgeHelper bHelper;
      // bHelper.Install (bridge[i].Get(j), bridgeDevices[i][j]);
      // Assign address
      char *subnet;
      subnet = toString(10, i, j, 0);
      address.SetBase(subnet, "255.255.255.0");
      // incremental assigned
      ipContainer[i][j] = address.Assign(hostDevices[i][j]);
    }
  }
  std::cout << "Finished connecting edge switches and hosts  "
            << "\n";

  //=========== Connect aggregate switches to edge switches ===========//
  //
  NetDeviceContainer ae[num_pod][num_agg][num_edge];
  Ptr<OpenFlowSwitchNetDevice> aggSwtchs[num_pod][num_agg];
  // NetDeviceContainer aggSw[num_pod][num_agg];
  // NetDeviceContainer edgeSw[num_pod][num_edge];
  // Ipv4InterfaceContainer ipAeContainer[num_pod][num_agg][num_edge];
  for (i = 0; i < num_pod; i++) {
    for (j = 0; j < num_agg; j++) {
      for (h = 0; h < num_edge; h++) {
        ae[i][j][h] =
            csma.Install(NodeContainer(agg[i].Get(j), edge[i].Get(h)));
        // int second_octet = i;
        // int third_octet = j+(k/2);
        // int fourth_octet;
        // if (h==0) fourth_octet = 1;
        // else fourth_octet = h*2+1;
        // //Assign subnet
        // char *subnet;
        // subnet = toString(10, second_octet, third_octet, 0);
        // //Assign base
        // char *base;
        // base = toString(0, 0, 0, fourth_octet);
        // address.SetBase (subnet, "255.255.255.0",base);
        // ipAeContainer[i][j][h] = address.Assign(ae[i][j][h]);
      }
      // add agg switch
      Ptr<Node> switchNode = agg[i].Get(j);
      Ptr<ns3::ofi::LearningController> controller =
          CreateObject<ns3::ofi::LearningController>();
      aggSwtchs[i][j] = new OpenFlowSwitchNetDevice();
      aggSwtchs[i][j]->SetController(controller);
      switchNode->AddDevice(aggSwtchs[i][j]);

      aggSwtchs[i][j]->m_pod = i;
      aggSwtchs[i][j]->m_pos = -1;
      aggSwtchs[i][j]->m_level = 1;
      aggSwtchs[i][j]->IP_MAC_MAP = ipMacMap;

      for (h = 0; h < num_edge; h++) {
        aggSwtchs[i][j]->m_port_dir.insert(make_pair(h, false));
        aggSwtchs[i][j]->AddSwitchPort(ae[i][j][h].Get(0));
      }
    }
    for (h = 0; h < num_edge; h++) {
      for (j = 0; j < num_agg; j++) {
        edgeSwtchs[i][h]->m_port_dir.insert(make_pair(j + (k / 2), true));
        edgeSwtchs[i][h]->AddSwitchPort(ae[i][j][h].Get(1));
      }
    }
  }
  std::cout << "Finished connecting aggregation switches and edge switches  "
            << "\n";

  //=========== Connect core switches to aggregate switches ===========//
  //
  NetDeviceContainer ca[num_group][num_core][num_pod];
  // NetDeviceContainer coreSw[num_group][num_core][num_pod];
  // Ipv4InterfaceContainer ipCaContainer[num_group][num_core][num_pod];
  // int fourth_octet =1;
  Ptr<OpenFlowSwitchNetDevice> coreSwtchs[num_group][num_core];
  for (i = 0; i < num_group; i++) {
    for (j = 0; j < num_core; j++) {
      // fourth_octet = 1;
      for (h = 0; h < num_pod; h++) {
        ca[i][j][h] =
            csma.Install(NodeContainer(core[i].Get(j), agg[h].Get(i)));
        // coreSw[i][j][h].Add(ca[i][j][h].Get(0));
        // int second_octet = k+i;
        // int third_octet = j;
        // //Assign subnet
        // char *subnet;
        // subnet = toString(10, second_octet, third_octet, 0);
        // //Assign base
        // char *base;
        // base = toString(0, 0, 0, fourth_octet);
        // address.SetBase (subnet, "255.255.255.0",base);
        // ipCaContainer[i][j][h] = address.Assign(ca[i][j][h]);
        // fourth_octet +=2;
      }

      // add switch
      Ptr<Node> switchNode = core[i].Get(j);
      Ptr<ns3::ofi::LearningController> controller =
          CreateObject<ns3::ofi::LearningController>();
      coreSwtchs[i][j] = new OpenFlowSwitchNetDevice();
      coreSwtchs[i][j]->SetController(controller);
      switchNode->AddDevice(coreSwtchs[i][j]);

      coreSwtchs[i][j]->m_pod = -1;
      coreSwtchs[i][j]->m_pos = -1;
      coreSwtchs[i][j]->m_level = 2;
      coreSwtchs[i][j]->IP_MAC_MAP = ipMacMap;

      for (h = 0; h < num_pod; h++) {
        coreSwtchs[i][j]->m_port_dir.insert(make_pair(h, false));
        coreSwtchs[i][j]->AddSwitchPort(ca[i][j][h].Get(0));
      }
    }
    for (h = 0; h < num_pod; h++) {
      for (j = 0; j < num_core; j++) {
        aggSwtchs[h][i]->m_port_dir.insert(make_pair(j + (k / 2), true));
        aggSwtchs[h][i]->AddSwitchPort(ca[i][j][h].Get(1));
      }
    }
  }
  std::cout << "Finished connecting core switches and aggregation switches  "
            << "\n";
  std::cout << "------------- "
            << "\n";

  //=========== Start the simulation ===========//
  //

  std::cout << "Start Simulation.. "
            << "\n";
  for (i = 0; i < total_host; i++) {
    app[i].Start(Seconds(0.0));
    app[i].Stop(Seconds(100.0));
  }
  // Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  // Calculate Throughput using Flowmonitor
  //
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();
  // Run simulation.
  Packet::EnablePrinting();
  NS_LOG_INFO("Run Simulation.");
  Simulator::Stop(Seconds(101.0));
  Simulator::Run();

  monitor->CheckForLostPackets();
  monitor->SerializeToXmlFile(filename, true, true);

  std::cout << "Simulation finished "
            << "\n";

  Simulator::Destroy();
  NS_LOG_INFO("Done.");

////  NodeContainer host[num_pod][num_bridge]; // NodeContainer for hosts
//  for (i = 0; i < k; i++) {
//    for (j = 0; j < num_bridge; j++) {
////      std::cout << (host[i][j].Get(0))->GetId() << std::endl;
////      std::cout << (host[i][j].Get(1))->GetId() << std::endl;
//
//    }
//std::cout << (edge[i].Get(0))->GetId() << std::endl;
//std::cout << (edge[i].Get(1))->GetId() << std::endl;
//std::cout << (agg[i].Get(0))->GetId() << std::endl;
//std::cout << (agg[i].Get(1))->GetId() << std::endl;
//if(i<2)
//{
//std::cout << (core[i].Get(0))->GetId() << std::endl;
//std::cout << (core[i].Get(1))->GetId() << std::endl;
//
//}
//  }

  return 0;
}
