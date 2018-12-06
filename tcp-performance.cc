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

//Include necessary header files
#include <fstream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/gnuplot.h"

//Use ns3 namespace
using namespace ns3;

//Enable log for this program
NS_LOG_COMPONENT_DEFINE ("tcp-performance");

//Create an application to generate traffic
class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();

  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);
  void ChangeRate (DataRate newrate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);
  void SendPacket2 (int);

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
MyApp::SendPacket2 (int m_bufSize)
{
  uint8_t* buffer = new uint8_t[m_bufSize];
  Ptr<Packet> packet = Create<Packet> (buffer, m_packetSize);
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

void
MyApp::ChangeRate (DataRate newrate)
{
  m_dataRate = newrate;
  return;
}

//Trace Sink for Congestion Window
/*static void
CwndChange (uint32_t oldCwnd, uint32_t newCwnd)
{
  std::ofstream tmpfile ("cwnd1.plotme", std::ios::in |std::ios::out | std::ios::app);
  tmpfile << Simulator::Now ().GetSeconds () << "\t" << (newCwnd/480.0) << "\n";
  tmpfile.close();
}*/

void
IncRate (Ptr<MyApp> app, DataRate rate)
{
  app->ChangeRate (rate);
  return;
}

//Main function
int main (int argc, char *argv[])
{
  //Set point-to-point channel attributes
  std::string delay1 = "1ms";
  std::string rate1 = "10Mbps";

  std::string delay2 = "3ms";
  std::string rate2 = "1Mbps";

  for (int bufSize = 0 * 1500; bufSize <= 33 * 1500;)
    {
      CommandLine cmd;
      cmd.Parse (argc, argv);

      //Create nodes
      NS_LOG_INFO ("Create nodes.");
      NodeContainer c;
      c.Create (4);

      //Create Node Containers for nodes in the point-to-point channel
      NodeContainer n0n1 = NodeContainer (c.Get (0), c.Get (1));
      NodeContainer n1n2 = NodeContainer (c.Get (1), c.Get (2));
      NodeContainer n2n3 = NodeContainer (c.Get (3), c.Get (2));

      // Create point-to-point channel and configure its attributes
      NS_LOG_INFO ("Create channels.");
      PointToPointHelper p2p;
      p2p.SetDeviceAttribute ("DataRate", StringValue (rate1));
      p2p.SetChannelAttribute ("Delay", StringValue (delay1));

      PointToPointHelper p2p2;
      p2p2.SetDeviceAttribute ("DataRate", StringValue (rate2));
      p2p2.SetChannelAttribute ("Delay", StringValue (delay2));

      // Create network device container for nodes
      NetDeviceContainer d0d1 = p2p.Install (n0n1);
      NetDeviceContainer d1d2 = p2p2.Install (n1n2);
      NetDeviceContainer d2d3 = p2p.Install (n2n3);

      // Install Internet Stack
      InternetStackHelper internet;
      internet.Install (c);

      // Assign IP address to every interface in point-to-point network
      NS_LOG_INFO ("Assign IP Addresses.");
      Ipv4AddressHelper ipv4;
      ipv4.SetBase ("10.1.1.0", "255.255.255.0");
      Ipv4InterfaceContainer i0i1 = ipv4.Assign (d0d1);

      ipv4.SetBase ("10.1.2.0", "255.255.255.0");
      Ipv4InterfaceContainer i1i2 = ipv4.Assign (d1d2);

      ipv4.SetBase ("10.1.3.0", "255.255.255.0");
      Ipv4InterfaceContainer i2i3 = ipv4.Assign (d2d3);

      // Initialize routing table for each node
      Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

      //Create a PacketSinkApplication and install it on node 3
      uint16_t sinkPort1 = 8081;
      Address sinkAddress1 (InetSocketAddress (i2i3.GetAddress (0), sinkPort1));
      PacketSinkHelper packetSinkHelper1 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort1));
      ApplicationContainer sinkApps1 = packetSinkHelper1.Install (c.Get (3));
      sinkApps1.Start (Seconds (0.));
      sinkApps1.Stop (Seconds (10.));

      Ptr<Socket> ns3TcpSocket1 = Socket::CreateSocket (c.Get (0), TcpSocketFactory::GetTypeId ());

      //Set the maximum transmit and receive buffer size
      ns3TcpSocket1->SetAttribute ("SndBufSize",  ns3::UintegerValue (bufSize));
      ns3TcpSocket1->SetAttribute ("RcvBufSize",  ns3::UintegerValue (bufSize));

      //Connect the congestion window trace source to sink
      //ns3TcpSocket1->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndChange));

      //Create an object of class MyApp
      Ptr<MyApp> app1 = CreateObject<MyApp> ();
      app1->Setup (ns3TcpSocket1, sinkAddress1, 1040, 1000000, DataRate ("20Mbps"));
      c.Get (0)->AddApplication (app1);
      app1->SetStartTime (Seconds (1.));
      app1->SetStopTime (Seconds (10.));

      // Enable Flowmonitor
      Ptr<FlowMonitor> flowMonitor;
      FlowMonitorHelper flowHelper;
      flowMonitor = flowHelper.InstallAll ();

      NS_LOG_INFO ("Run Simulation.");
      Simulator::Stop (Seconds (15.0));

      //Run the simulation
      Simulator::Run ();

      flowMonitor->CheckForLostPackets ();

      double TPut;
      Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowHelper.GetClassifier ());
      std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats ();

      //Calculation of throughput
      TPut = stats[1].rxBytes * 8.0 / (stats[1].timeLastRxPacket.GetSeconds () - stats[1].timeFirstTxPacket.GetSeconds ());

      //Write values of buffer size and throughput into output file
      std::ofstream tmp1file ("tput.plotme", std::ios::in | std::ios::out | std::ios::app);
      tmp1file << (bufSize / 1000) << "\t" << (TPut / 1000) << "\n";
      tmp1file.close ();

      //Serialize results to a file
      flowMonitor->SerializeToXmlFile ("tcp-performance.flowmon", true, true);

      //Release resources at the end of simulation
      Simulator::Destroy ();

      bufSize += 1 * 1500;
    }

  NS_LOG_INFO ("Done.");
  return 0;
}
