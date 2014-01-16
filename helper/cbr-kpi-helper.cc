/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Magister Solutions
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
 * Author: Jani Puttonen <jani.puttonen@magister.fi>
 *
 */

#include <algorithm>
#include "cbr-kpi-helper.h"
#include <ns3/log.h>
#include <ns3/simulator.h>
#include <ns3/packet.h>
#include <ns3/inet-socket-address.h>
#include <ns3/ipv4.h>
#include <ns3/ipv4-l3-protocol.h>
#include <ns3/flow-monitor.h>
#include <ns3/ipv4-flow-classifier.h>
#include <ns3/ipv4-l3-protocol.h>
#include <iomanip>


NS_LOG_COMPONENT_DEFINE ("CbrKpiHelper");


namespace ns3 {


CbrKpiHelper::CbrKpiHelper ()
{
  NS_LOG_FUNCTION (this);
}


void
CbrKpiHelper::AddServer (ApplicationContainer apps)
{
  NS_LOG_FUNCTION (this << apps.GetN ());

  for (ApplicationContainer::Iterator it = apps.Begin ();
       it != apps.End (); it++)
    {
      Ptr<CbrApplication> cbr = (*it)->GetObject<CbrApplication> ();
      NS_ASSERT (cbr != 0);
      AddServer (cbr);
    }
}

void
CbrKpiHelper::AddServer (Ptr<CbrApplication> cbr)
{
  NS_LOG_FUNCTION (this << cbr);

  // CONNECT TO THE CLIENT'S TRACE SOURCES

  // use the client's IP address as the context
  const Ipv4Address address = InetSocketAddress::ConvertFrom(cbr->GetRemote ()).GetIpv4 ();
  const std::string context = AddressToString (address);
  cbr->TraceConnect ("Tx", context,
                     MakeCallback (&CbrKpiHelper::TxCallback, this));

  // INSTALL FLOW MONITOR ON THIS SERVER
  Ptr<Node> node = cbr->GetNode ();
  NS_ASSERT (node->GetObject<Ipv4L3Protocol> () != 0);
  Ipv4Address serverAddr = GetAddress (node);

  // Install the flow monitor for each server node
  if (m_serverAddresses.empty ())
    {
      m_serverAddresses.push_back (serverAddr);
      m_flowMonitorHelper.Install (node);
    }
  else
    {
      std::vector<Ipv4Address>::const_iterator cit =
          std::find(m_serverAddresses.begin (), m_serverAddresses.end (), serverAddr);

      // If the server address was not found
      if (cit == m_serverAddresses.end ())
        {
          m_flowMonitorHelper.Install (node);
        }
    }
} // end of `void AddClient (Ptr<CbrClient> client)`


void
CbrKpiHelper::AddClient (ApplicationContainer apps)
{
  NS_LOG_FUNCTION (this << apps.GetN ());

  for (ApplicationContainer::Iterator it = apps.Begin ();
       it != apps.End (); it++)
    {
      Ptr<PacketSink> sink = (*it)->GetObject<PacketSink> ();
      NS_ASSERT (sink != 0);
      AddClient (sink);
    }
}

void
CbrKpiHelper::AddClient (Ptr<PacketSink> sink)
{
  NS_LOG_FUNCTION (this << sink);

  // use the client's IP address as the context
  const Ipv4Address address = GetAddress (sink->GetNode ());
  const std::string context = AddressToString (address);
  sink->TraceConnect ("Rx", context,
                      MakeCallback (&CbrKpiHelper::RxCallback, this));

  // INITIALIZE A SET OF COUNTERS FOR THIS CLIENT
  ClientCounter_t counter;
  counter.txBytes = 0;
  counter.txPackets = 0;
  counter.rxBytes = 0;
  counter.rxPackets = 0;
  counter.rxIpLevelPackets = 0;
  counter.sumPacketDelay = Seconds (0.0);
  counter.appStart = Seconds (-1.0);
  counter.appStop = Seconds (-1.0);

  NS_ASSERT_MSG (m_clientCounters.find (address) == m_clientCounters.end (),
                 "Found a client with duplicate address " << address);

  m_clientCounters[address] = counter;

  // INSTALL FLOW MONITOR ON THIS CLIENT

  Ptr<Node> node = sink->GetNode ();
  NS_ASSERT (node->GetObject<Ipv4L3Protocol> () != 0);
  m_flowMonitorHelper.Install (node);
}


void
CbrKpiHelper::Print ()
{
  NS_LOG_FUNCTION (this);

  // this ensures all possibly lost packets are accounted for
  const Ptr<FlowMonitor> flowMonitor = m_flowMonitorHelper.GetMonitor ();
  flowMonitor->CheckForLostPackets ();

  const Ptr<FlowClassifier> baseClassifier = m_flowMonitorHelper.GetClassifier ();
  const Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (baseClassifier);

  const std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats ();
  std::map<FlowId, FlowMonitor::FlowStats>::const_iterator it1;
  for (it1 = stats.begin (); it1 != stats.end (); ++it1)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (it1->first);

      std::vector<Ipv4Address>::const_iterator cit =
          std::find(m_serverAddresses.begin (), m_serverAddresses.end (), t.sourceAddress);

      if (cit != m_serverAddresses.end ()) // only DL direction is considered
        {
          NS_ASSERT (m_clientCounters.find (t.destinationAddress) != m_clientCounters.end ());
          m_clientCounters[t.destinationAddress].rxIpLevelPackets += it1->second.rxPackets;
          m_clientCounters[t.destinationAddress].sumPacketDelay += it1->second.delaySum;
        }
    }

  // PRINT HEADER

  std::cout << "CBR/sink applications' client specific statistics:" << std::endl;
  std::cout << "-------------------------------------------------------------------------" << std::endl;
  std::cout << std::setw (16) << "address"
            << std::setw (16) << "Tx packet"
            << std::setw (16) << "Tx bytes"
            << std::setw (16) << "Rx packet"
            << std::setw (16) << "Rx bytes"
            << std::setw (16) << "Rx IP packets"
            << std::setw (16) << "User throughput [kbps]" << std::endl;
  std::cout << "-------------------------------------------------------------------------" << std::endl;

  // PRINT ONE LINE FOR EACH CLIENT

  uint64_t sumTxBytes (0);
  uint64_t sumRxBytes (0);
  uint64_t sumTxPackets (0);
  uint64_t sumRxPackets (0);
  uint32_t sumRxIpLevelPackets = 0;
  Time sumPacketDelaySecond = Seconds (0.0);
  double sumUserTroughputs (0.0);

  std::map<Ipv4Address, ClientCounter_t>::const_iterator it2;
  for (it2 = m_clientCounters.begin (); it2 != m_clientCounters.end (); ++it2)
    {
      NS_ASSERT (it2->second.appStop.GetSeconds () > 0);
      NS_ASSERT (it2->second.appStart.GetSeconds () > 0);
      NS_ASSERT (it2->second.appStop.GetSeconds () > it2->second.appStart.GetSeconds ());

      Time length = it2->second.appStop - it2->second.appStart;
      const double userThroughput = GetKbps (it2->second.rxBytes, length);
      sumUserTroughputs += userThroughput;
      std::cout << std::setw (16) << AddressToString (it2->first)
                << std::setw (16) << it2->second.txPackets
                << std::setw (16) << it2->second.txBytes
                << std::setw (16) << it2->second.rxPackets
                << std::setw (16) << it2->second.rxBytes
                << std::setw (16) << it2->second.rxIpLevelPackets
                << std::setw (16) << userThroughput
                << std::endl;
      sumTxBytes += it2->second.txBytes;
      sumRxBytes += it2->second.rxBytes;
      sumTxPackets += it2->second.txPackets;
      sumRxPackets += it2->second.rxPackets;
      sumRxIpLevelPackets += it2->second.rxIpLevelPackets;
      sumPacketDelaySecond += it2->second.sumPacketDelay;
    }

  // PRINT FOOTER
  const double sumThroughput = GetKbps (sumRxBytes, Simulator::Now ());
  const double avgErrorRatio = (sumTxPackets - sumRxPackets) / (double)(sumTxPackets);
  const double avgDelaySecond = sumPacketDelaySecond.GetSeconds () / sumRxIpLevelPackets;
  std::cout << "-------------------------------------------------------------------------" << std::endl;
  std::cout << std::setw (16) << "SumTxBytes [B]"
            << std::setw (16) << "SumRxBytes [B]"
            << std::setw (16) << "SumTput [kbps]"
            << std::setw (16) << "AvgTput [kbps]"
            << std::setw (16) << "AvgErrorRatio [-]"
            << std::setw (16) << "AvgDelay [s]"
            << std::endl;

  std::cout << std::setw (16) << sumTxBytes
            << std::setw (16) << sumRxBytes
            << std::setw (16) << sumThroughput
            << std::setw (16) << sumUserTroughputs / m_clientCounters.size ()
            << std::setw (16) << avgErrorRatio
            << std::setw (16) << avgDelaySecond
            << std::endl;

  std::cout << "-------------------------------------------------------------------------" << std::endl;

} // end of `Print()`


void
CbrKpiHelper::TxCallback (std::string context, Ptr<const Packet> packet)
{
  NS_LOG_LOGIC ("TxCallback: " << context << " size: " << packet->GetSize ());

  Ipv4Address address (context.c_str ());
  NS_ASSERT (m_clientCounters.find (address) != m_clientCounters.end ());

  if (m_clientCounters[address].appStart < 0)
    {
      m_clientCounters[address].appStart = Simulator::Now ();
    }
  m_clientCounters[address].txPackets++;
  m_clientCounters[address].txBytes += packet->GetSize ();

}

void
CbrKpiHelper::RxCallback (std::string context, Ptr<const Packet> packet, const Address &from)
{
  NS_LOG_LOGIC ("RxCallback: " << context << " size: " << packet->GetSize ());

  Ipv4Address address (context.c_str ());
  NS_ASSERT (m_clientCounters.find (address) != m_clientCounters.end ());

  m_clientCounters[address].appStop = Simulator::Now ();
  m_clientCounters[address].rxPackets++;
  m_clientCounters[address].rxBytes += packet->GetSize ();

}

Ipv4Address
CbrKpiHelper::GetAddress (Ptr<Node> node)
{
  const Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  NS_ASSERT_MSG (ipv4 != 0,
                 "No IPv4 object is found within node " << node);
  /// \todo Still unclear if the hard-coded indices below will work in any possible cases.
  const Ipv4InterfaceAddress interfaceAddress = ipv4->GetAddress (1, 0);
  return interfaceAddress.GetLocal ();
}


std::string
CbrKpiHelper::AddressToString (const Ipv4Address address)
{
  std::ostringstream oss;
  address.Print (oss);
  return oss.str ();
}


double
CbrKpiHelper::GetKbps (uint64_t bytes, Time duration)
{
  return static_cast<double> (bytes * 8) / 1000.0 / duration.GetSeconds ();
}


} // end of `namespace ns3`
