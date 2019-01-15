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
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

// Necessary header files
#include <iostream>
#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/applications-module.h"
#include "ns3/packet-sink.h"
#include "ns3/traffic-control-module.h"
#include "ns3/log.h"
#include "ns3/random-variable-stream.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/callback.h"

// Network Topology
//       N1    N6
//        |    |
// N2----+|    |+----N7
//       ||    ||
// N3----R1----R2----N8
//       ||    ||
// N4----+|    |+----N9
//        |    |
//       N5    N10

using namespace ns3;

Ptr<ExponentialRandomVariable> uv = CreateObject<ExponentialRandomVariable> ();
double stopTime = 20.0;

void CheckQueueSize (Ptr<QueueDisc> queue)
{
  uint32_t qSize = queue->GetCurrentSize ().GetValue ();

  // check queue size every 1/100 of a second
  Simulator::Schedule (Seconds (0.001), &CheckQueueSize, queue);

  std::ofstream QSizeFile (std::stringstream ("queuered.plotme").str ().c_str (),   std::ios::out | std::ios::app);
  QSizeFile << Simulator::Now ().GetSeconds () << " " << qSize << std::endl;
  QSizeFile.close ();
}

// Packet Sink Applications on destination nodes
void InstallPacketSink (Ptr<Node> node, uint16_t port)
{
  PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (node);
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (20.0));
}

// Bulk Send Applications on source nodes
void InstallBulkSend (Ptr<Node> node, Ipv4Address address, uint16_t port)
{
  BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (address, port));
  source.SetAttribute ("MaxBytes", UintegerValue (0));
  ApplicationContainer sourceApps = source.Install (node);

  // Start each source application with an interval of 1 second
  double i = 1.0;
  sourceApps.Start (Seconds (10.0 + i));
  i++;
  sourceApps.Stop (Seconds (stopTime));
}

// Main function
int main (int argc, char *argv[])
{
  uint32_t stream = 1;
  std::string transport_prot = "TcpNewReno";
  std::string queue_disc_type = "RedQueueDisc";

  CommandLine cmd;
  cmd.AddValue ("stream", "Seed value for random variable", stream);
  cmd.AddValue ("queue_disc_type", "Queue disc type for gateway (e.g. ns3::FifoQueueDisc or ns3::RedQueueDisc)", queue_disc_type);
  cmd.AddValue ("stopTime", "Stop time for applications / simulation time will be stopTime", stopTime);
  cmd.Parse (argc,argv);

  uv->SetStream (stream);
  transport_prot = std::string ("ns3::") + transport_prot;
  queue_disc_type = std::string ("ns3::") + queue_disc_type;

  TypeId qdTid;
  NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (queue_disc_type, &qdTid), "TypeId " << queue_disc_type << " not found");

  // Create nodes
  NodeContainer leftNodes, rightNodes, routers;
  routers.Create (2);
  leftNodes.Create (5);
  rightNodes.Create (5);

  // Create the point-to-point link helpers
  PointToPointHelper pointToPointRouter;
  pointToPointRouter.SetDeviceAttribute  ("DataRate", StringValue ("50Mbps"));
  pointToPointRouter.SetChannelAttribute ("Delay", StringValue ("10ms"));
  NetDeviceContainer r1r2ND = pointToPointRouter.Install (routers.Get (0), routers.Get (1));

  // Separate Network device containers for left and right nodes
  std::vector <NetDeviceContainer> leftToRouter;
  std::vector <NetDeviceContainer> routerToRight;
  PointToPointHelper pointToPointLeaf;
  pointToPointLeaf.SetDeviceAttribute    ("DataRate", StringValue ("100Mbps"));

  // Node 1
  pointToPointLeaf.SetChannelAttribute   ("Delay", StringValue ("1ms"));
  leftToRouter.push_back (pointToPointLeaf.Install (leftNodes.Get (0), routers.Get (0)));
  routerToRight.push_back (pointToPointLeaf.Install (routers.Get (1), rightNodes.Get (0)));

  // Node 2
  leftToRouter.push_back (pointToPointLeaf.Install (leftNodes.Get (1), routers.Get (0)));
  routerToRight.push_back (pointToPointLeafstream.Install (routers.Get (1), rightNodes.Get (1)));

  // Node 3
  leftToRouter.push_back (pointToPointLeaf.Install (leftNodes.Get (2), routers.Get (0)));
  routerToRight.push_back (pointToPointLeaf.Install (routers.Get (1), rightNodes.Get (2)));

  // Node 4
  leftToRouter.push_back (pointToPointLeaf.Install (leftNodes.Get (3), routers.Get (0)));
  routerToRight.push_back (pointToPointLeaf.Install (routers.Get (1), rightNodes.Get (3)));

  // Node 5
  leftToRouter.push_back (pointToPointLeaf.Install (leftNodes.Get (4), routers.Get (0)));
  routerToRight.push_back (pointToPointLeaf.Install (routers.Get (1), rightNodes.Get (4)));

  // Install Internet Stack on all nodes
  InternetStackHelper stack;
  stack.Install (routers);
  stack.Install (leftNodes);
  stack.Install (rightNodes);

  // Assign IP addresses
  Ipv4AddressHelper ipAddresses ("10.0.0.0", "255.255.255.0");

  Ipv4InterfaceContainer r1r2IPAddress = ipAddresses.Assign (r1r2ND);
  ipAddresses.NewNetwork ();

  std::vector <Ipv4InterfaceContainer> leftToRouterIPAddress;
  leftToRouterIPAddress.push_back (ipAddresses.Assign (leftToRouter [0]));
  ipAddresses.NewNetwork ();
  leftToRouterIPAddress.push_back (ipAddresses.Assign (leftToRouter [1]));
  ipAddresses.NewNetwork ();
  leftToRouterIPAddress.push_back (ipAddresses.Assign (leftToRouter [2]));
  ipAddresses.NewNetwork ();
  leftToRouterIPAddress.push_back (ipAddresses.Assign (leftToRouter [3]));
  ipAddresses.NewNetwork ();
  leftToRouterIPAddress.push_back (ipAddresses.Assign (leftToRouter [4]));
  ipAddresses.NewNetwork ();

  std::vector <Ipv4InterfaceContainer> routerToRightIPAddress;
  routerToRightIPAddress.push_back (ipAddresses.Assign (routerToRight [0]));
  ipAddresses.NewNetwork ();
  routerToRightIPAddress.push_back (ipAddresses.Assign (routerToRight [1]));
  ipAddresses.NewNetwork ();
  routerToRightIPAddress.push_back (ipAddresses.Assign (routerToRight [2]));
  ipAddresses.NewNetwork ();
  routerToRightIPAddress.push_back (ipAddresses.Assign (routerToRight [3]));
  ipAddresses.NewNetwork ();
  routerToRightIPAddress.push_back (ipAddresses.Assign (routerToRight [4]));

  // Set values for Red Queue Disc attributes
  Config::SetDefault ("ns3::RedQueueDisc::ARED", BooleanValue (false));
  Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (false));
  Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (512));
  Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (20));
  Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (80));
  Config::SetDefault ("ns3::RedQueueDisc::LInterm", DoubleValue (10));
  Config::SetDefault (queue_disc_type + "::MaxSize", QueueSizeValue (QueueSize ("100p")));

  // Create traffic control helper to install queue disc on the net device container
  TrafficControlHelper tchBottleneck;
  tchBottleneck.SetRootQueueDisc (queue_disc_type);
  QueueDiscContainer qd;
  tchBottleneck.Uninstall (routers.Get (0)->GetDevice (0));
  qd.Add (tchBottleneck.Install (routers.Get (0)->GetDevice (0)).Get (0));
  Simulator::ScheduleNow (&CheckQueueSize, qd.Get (0));

  // Install Packet Sink Application on all right side nodes
  uint16_t port = 50000;
  for (int i = 0; i < 5; i++)
    {
      InstallPacketSink (rightNodes.Get (i), port);
    }

  // Install Bulk Send Application on all left side nodes
  for (int i = 0; i < 5; i++)
    {
      InstallBulkSend (leftNodes.Get (i), routerToRightIPAddress [i].GetAddress (1), port);
    }

  // Initialize routing table for each node
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Run the simulation
  Simulator::Stop (Seconds (stopTime));
  Simulator::Run ();

  // Release resources at the end of simulation
  Simulator::Destroy ();
  return 0;
}
