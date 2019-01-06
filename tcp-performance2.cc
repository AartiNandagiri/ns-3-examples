/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 NITK Surathkal
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Aarti Nandagiri <aarti.nandagiri@gmail.com>
            Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

// Network topology
//
//       n0 ---+      +--- n2
//             |      |
//             n4 -- n5
//             |      |
//       n1 ---+      +--- n3


//Include necessary header files
#include <fstream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"

//Use ns3 namespace
using namespace ns3;

//Enable log for this program
NS_LOG_COMPONENT_DEFINE ("Prog2");

// Application to generate TCP traffic
class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();

  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);

  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;

};

MyApp::MyApp ()
  : m_socket (0),
  m_peer (),
  m_packetSize (0),
  m_nPackets (0),
  m_dataRate (0),
  m_sendEvent (),
  m_running (false),
  m_packetsSent (0)

{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{

  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{

  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

// Main function
int main (int argc, char *argv[])
{
  // Delay and data rate of bottleneck link
  std::string lat2 = "3ms";
  std::string rate2 = "1Mbps";

  // Delay and data rate of other point-to-point links
  std::string lat1 = "1ms";
  std::string rate1 = "10Mbps";

  // Create nodes
  NS_LOG_INFO ("Create nodes.");
  NodeContainer c;
  c.Create (6);

  // Node containers for the nodes in the network
  NodeContainer n0n4 = NodeContainer (c.Get (0), c.Get (4));
  NodeContainer n1n4 = NodeContainer (c.Get (1), c.Get (4));
  NodeContainer n2n5 = NodeContainer (c.Get (2), c.Get (5));
  NodeContainer n3n5 = NodeContainer (c.Get (3), c.Get (5));
  NodeContainer n4n5 = NodeContainer (c.Get (4), c.Get (5));

  // Install Internet Stack on all nodes
  InternetStackHelper internet;
  internet.Install (c);

  // Create point-to-point links and configure its attributes
  NS_LOG_INFO ("Create channels.");
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue (rate1));
  p2p.SetChannelAttribute ("Delay", StringValue (lat1));

  PointToPointHelper p2p2;
  p2p2.SetDeviceAttribute ("DataRate", StringValue (rate2));
  p2p2.SetChannelAttribute ("Delay", StringValue (lat2));

  // Network device container for the nodes in the network
  NetDeviceContainer d0d4 = p2p.Install (n0n4);
  NetDeviceContainer d1d4 = p2p.Install (n1n4);
  NetDeviceContainer d2d5 = p2p.Install (n2n5);
  NetDeviceContainer d3d5 = p2p.Install (n3n5);
  NetDeviceContainer d4d5 = p2p2.Install (n4n5);

  // Assign IP address to every interface in point-to-point network
  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i0i4 = ipv4.Assign (d0d4);

  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i1i4 = ipv4.Assign (d1d4);

  ipv4.SetBase ("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer i2i5 = ipv4.Assign (d2d5);

  ipv4.SetBase ("10.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer i3i5 = ipv4.Assign (d3d5);

  ipv4.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer i4i5 = ipv4.Assign (d4d5);

  // Initialize routing table for each node
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Create PacketSinkApplication and install it on node 2
  NS_LOG_INFO ("Create Applications.");
  uint16_t sinkPort = 8080;
  Address sinkAddress (InetSocketAddress (i2i5.GetAddress (0), sinkPort));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  ApplicationContainer sinkApps = packetSinkHelper.Install (c.Get (2));
  sinkApps.Start (Seconds (0.));
  sinkApps.Stop (Seconds (200.));

  Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (c.Get (0), TcpSocketFactory::GetTypeId ());

  // TCP source application at node 0
  Ptr<MyApp> app = CreateObject<MyApp> ();
  app->Setup (ns3TcpSocket, sinkAddress, 512, 1000000, DataRate ("10Mbps"));
  c.Get (0)->AddApplication (app);
  app->SetStartTime (Seconds (0.));
  app->SetStopTime (Seconds (200.));

  // Udp server application on node 3
  uint16_t port = 6;
  UdpServerHelper server (port);
  ApplicationContainer apps = server.Install (c.Get (3));
  apps.Start (Seconds (0.));
  apps.Stop (Seconds (200.));

  // Udp trace application on node 1 which sends packets from the trace file of a MPEG4 stream
  Address serverAddress (InetSocketAddress (i3i5.GetAddress (0), port));
  uint32_t MaxPacketSize = 512;
  UdpTraceClientHelper client (serverAddress, port,"starwars.dat");
  client.SetAttribute ("MaxPacketSize", UintegerValue (MaxPacketSize));
  apps = client.Install (c.Get (1));
  apps.Start (Seconds (10.));
  apps.Stop (Seconds (200.));

  Simulator::Stop (Seconds (200.0));

  // Enable pcap files
  p2p.EnablePcapAll ("q");

  // Run the simulation
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();

  // Release resources at the end of simulation
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
