/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 Magister Solutions
 * Copyright (c) 2020 CNES
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
 * Author: Bastien Tauran <bastien.tauran@viveris.fr>
 */

#include "satellite-traffic-helper.h"

#include <ns3/type-id.h>
#include <ns3/log.h>

#include <ns3/simulation-helper.h>
#include <ns3/packet-sink.h>
#include <ns3/packet-sink-helper.h>
#include <ns3/cbr-helper.h>
#include <ns3/satellite-on-off-helper.h>
#include <ns3/nrtv-helper.h>
#include <ns3/three-gpp-http-satellite-helper.h>

NS_LOG_COMPONENT_DEFINE ("SatelliteTrafficHelper");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SatTrafficHelper);

TypeId
SatTrafficHelper::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SatTrafficHelper")
    .SetParent<Object> ()
    .AddConstructor<SatTrafficHelper> ();
  return tid;
}

TypeId
SatTrafficHelper::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);

  return GetTypeId ();
}

SatTrafficHelper::SatTrafficHelper ()
  : m_satHelper (NULL)
{
  m_last_custom_application.created = false;
}

SatTrafficHelper::SatTrafficHelper (Ptr<SatHelper> satHelper)
  : m_satHelper (satHelper)
{
  m_last_custom_application.created = false;
}

void
SatTrafficHelper::AddCbrTraffic (TrafficDirection_t direction,
                                 std::string interval,
                                 uint32_t packetSize,
                                 NodeContainer gws,
                                 NodeContainer uts,
                                 Time startTime,
                                 Time stopTime,
                                 Time startDelay)
{
  NS_LOG_FUNCTION (this << interval << packetSize << startTime << stopTime << startDelay);

  std::string socketFactory = "ns3::UdpSocketFactory";
  uint16_t port = 9;

  PacketSinkHelper sinkHelper (socketFactory, Address ());
  CbrHelper cbrHelper (socketFactory, Address ());
  ApplicationContainer sinkContainer;
  ApplicationContainer cbrContainer;

  // create CBR applications from GWs to UT users
  for (uint32_t j = 0; j < gws.GetN (); j++)
    {
      for (uint32_t i = 0; i < uts.GetN (); i++)
        {
          if (direction == RTN_LINK)
            {
              InetSocketAddress gwUserAddr = InetSocketAddress (m_satHelper->GetUserAddress (gws.Get (j)), port);
              if (!HasSinkInstalled (gws.Get (j), port))
                {
                  sinkHelper.SetAttribute ("Local", AddressValue (Address (gwUserAddr)));
                  sinkContainer.Add (sinkHelper.Install (gws.Get (j)));
                }

              cbrHelper.SetConstantTraffic (Time (interval), packetSize);
              cbrHelper.SetAttribute ("Remote", AddressValue (Address (gwUserAddr)));
              auto app = cbrHelper.Install (uts.Get (i)).Get (0);
              app->SetStartTime (startTime + (i + j*gws.GetN () + 1) * startDelay);
              cbrContainer.Add (app);
            }
          else if (direction == FWD_LINK)
            {
              InetSocketAddress utUserAddr = InetSocketAddress (m_satHelper->GetUserAddress (uts.Get (i)), port);
              if (!HasSinkInstalled (uts.Get (i), port))
                {
                  sinkHelper.SetAttribute ("Local", AddressValue (Address (utUserAddr)));
                  sinkContainer.Add (sinkHelper.Install (uts.Get (i)));
                }

              cbrHelper.SetConstantTraffic (Time (interval), packetSize);
              cbrHelper.SetAttribute ("Remote", AddressValue (Address (utUserAddr)));
              auto app = cbrHelper.Install (gws.Get (j)).Get (0);
              app->SetStartTime (startTime + (i + j*gws.GetN () + 1) * startDelay);
              cbrContainer.Add (app);
            }
        }
    }

  sinkContainer.Start (startTime);
  sinkContainer.Stop (stopTime);
}

void
SatTrafficHelper::AddHttpTraffic (TrafficDirection_t direction,
                                  NodeContainer gws,
                                  NodeContainer uts,
                                  Time startTime,
                                  Time stopTime,
                                  Time startDelay)
{
  ThreeGppHttpHelper httpHelper;
  if (direction == FWD_LINK)
    {
      for (uint32_t j = 0; j < gws.GetN (); j++)
        {
          auto app = httpHelper.InstallUsingIpv4 (gws.Get (j), uts).Get (1);
          app->SetStartTime (startTime + (j + 1) * startDelay);
          httpHelper.GetServer ().Start (startTime);
          httpHelper.GetServer ().Stop (stopTime);
        }
    }
  else if (direction == RTN_LINK)
    {
      for (uint32_t i = 0; i < uts.GetN (); i++)
        {
          auto app = httpHelper.InstallUsingIpv4 (uts.Get (i), gws).Get (1);
          app->SetStartTime (startTime + (i + 1) * startDelay);
          httpHelper.GetServer ().Start (startTime);
          httpHelper.GetServer ().Stop (stopTime);
        }
    }
}

void
SatTrafficHelper::AddNrtvTraffic (TrafficDirection_t direction,
                                  NodeContainer gws,
                                  NodeContainer uts,
                                  Time startTime,
                                  Time stopTime,
                                  Time startDelay)
{
  std::string socketFactory = "ns3::TcpSocketFactory";

  NrtvHelper nrtvHelper (TypeId::LookupByName (socketFactory));
  if (direction == FWD_LINK)
    {
      for (uint32_t j = 0; j < gws.GetN (); j++)
        {
          auto app = nrtvHelper.InstallUsingIpv4 (gws.Get (j), uts).Get (1);
          app->SetStartTime (startTime + (j + 1) * startDelay);
          nrtvHelper.GetServer ().Start (startTime);
          nrtvHelper.GetServer ().Stop (stopTime);
        }
    }
  else if (direction == RTN_LINK)
    {
      for (uint32_t i = 0; i < uts.GetN (); i++)
        {
          auto app = nrtvHelper.InstallUsingIpv4 (uts.Get (i), gws).Get (1);
          app->SetStartTime (startTime + (i + 1) * startDelay);
          nrtvHelper.GetServer ().Start (startTime);
          nrtvHelper.GetServer ().Stop (stopTime);
        }
    }
}

void
SatTrafficHelper::AddPoissonTraffic (TrafficDirection_t direction,
                                     Time onTime,
                                     Time offTimeExpMean,
                                     std::string rate,
                                     uint32_t packetSize,
                                     NodeContainer gws,
                                     NodeContainer uts,
                                     Time startTime,
                                     Time stopTime,
                                     Time startDelay)
{
  NS_LOG_FUNCTION (this << direction << onTime << offTimeExpMean << rate << packetSize << startTime << stopTime << startDelay);

  std::string socketFactory = "ns3::UdpSocketFactory";

  uint16_t port = 9;

  PacketSinkHelper sinkHelper (socketFactory, Address ());
  SatOnOffHelper onOffHelper (socketFactory, Address ());
  ApplicationContainer sinkContainer;
  ApplicationContainer onOffContainer;

  // create CBR applications from GWs to UT users
  for (uint32_t j = 0; j < gws.GetN (); j++)
    {
      for (uint32_t i = 0; i < uts.GetN (); i++)
        {
          if (direction == RTN_LINK)
            {
              InetSocketAddress gwUserAddr = InetSocketAddress (m_satHelper->GetUserAddress (gws.Get (j)), port);

              if (!HasSinkInstalled (gws.Get (j), port))
                {
                  sinkHelper.SetAttribute ("Local", AddressValue (Address (gwUserAddr)));
                  sinkContainer.Add (sinkHelper.Install (gws.Get (j)));
                }

              onOffHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=" + std::to_string (onTime.GetSeconds ()) + "]"));
              onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ExponentialRandomVariable[Mean=" + std::to_string (offTimeExpMean.GetSeconds ()) + "]"));
              onOffHelper.SetAttribute ("DataRate", DataRateValue (rate));
              onOffHelper.SetAttribute ("PacketSize", UintegerValue (packetSize));
              onOffHelper.SetAttribute ("Remote", AddressValue (Address (gwUserAddr)));

              auto app = onOffHelper.Install (uts.Get (i)).Get (0);
              app->SetStartTime (startTime + (i + j*gws.GetN () + 1) * startDelay);
              onOffContainer.Add (app);
            }
          else if (direction == FWD_LINK)
            {
              InetSocketAddress utUserAddr = InetSocketAddress (m_satHelper->GetUserAddress (uts.Get (i)), port);

              if (!HasSinkInstalled (uts.Get (i), port))
                {
                  sinkHelper.SetAttribute ("Local", AddressValue (Address (utUserAddr)));
                  sinkContainer.Add (sinkHelper.Install (uts.Get (i)));
                }

              onOffHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=" + std::to_string (onTime.GetSeconds ()) + "]"));
              onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ExponentialRandomVariable[Mean=" + std::to_string (offTimeExpMean.GetSeconds ()) + "]"));
              onOffHelper.SetAttribute ("DataRate", DataRateValue (rate));
              onOffHelper.SetAttribute ("PacketSize", UintegerValue (packetSize));
              onOffHelper.SetAttribute ("Remote", AddressValue (Address (utUserAddr)));

              auto app = onOffHelper.Install (gws.Get (j)).Get (0);
              app->SetStartTime (startTime + (i + j*gws.GetN () + 1) * startDelay);
              onOffContainer.Add (app);
            }
        }
    }
  sinkContainer.Start (startTime);
  sinkContainer.Stop (stopTime);
}

void
SatTrafficHelper::AddVoipTraffic (TrafficDirection_t direction,
                                  VoipCodec_t codec,
                                  NodeContainer gws,
                                  NodeContainer uts,
                                  Time startTime,
                                  Time stopTime,
                                  Time startDelay)
{
  NS_LOG_FUNCTION (this << direction << codec << startTime << stopTime << startDelay);

  std::string socketFactory = "ns3::UdpSocketFactory";
  uint16_t port = 9;

  double onTime;
  double offTime;
  std::string rate;
  uint32_t packetSize;

  switch(codec)
    {
      case G_711_1:
        onTime = 0.5;
        offTime = 0.05;
        rate = "64kbps";
        packetSize = 210;
        break;
      default:
        NS_FATAL_ERROR ("Not implemented yet");
        // TODO do it
    }

  PacketSinkHelper sinkHelper (socketFactory, Address ());
  SatOnOffHelper onOffHelper (socketFactory, Address ());
  ApplicationContainer sinkContainer;
  ApplicationContainer onOffContainer;

  // create CBR applications from GWs to UT users
  for (uint32_t j = 0; j < gws.GetN (); j++)
    {
      for (uint32_t i = 0; i < uts.GetN (); i++)
        {
          if (direction == RTN_LINK)
            {
              InetSocketAddress gwUserAddr = InetSocketAddress (m_satHelper->GetUserAddress (gws.Get (j)), port);

              if (!HasSinkInstalled (gws.Get (j), port))
                {
                  sinkHelper.SetAttribute ("Local", AddressValue (Address (gwUserAddr)));
                  sinkContainer.Add (sinkHelper.Install (gws.Get (j)));
                }

              onOffHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=" + std::to_string(onTime) + "]"));
              onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=" + std::to_string(offTime) + "]"));
              onOffHelper.SetAttribute ("DataRate", DataRateValue (rate));
              onOffHelper.SetAttribute ("PacketSize", UintegerValue (packetSize));
              onOffHelper.SetAttribute ("Remote", AddressValue (Address (gwUserAddr)));

              auto app = onOffHelper.Install (uts.Get (i)).Get (0);
              app->SetStartTime (startTime + (i + j*gws.GetN () + 1) * startDelay);
              onOffContainer.Add (app);
            }
          else if (direction == FWD_LINK)
            {
              InetSocketAddress utUserAddr = InetSocketAddress (m_satHelper->GetUserAddress (uts.Get (i)), port);

              if (!HasSinkInstalled (uts.Get (i), port))
                {
                  sinkHelper.SetAttribute ("Local", AddressValue (Address (utUserAddr)));
                  sinkContainer.Add (sinkHelper.Install (uts.Get (i)));
                }

              onOffHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=" + std::to_string(onTime) + "]"));
              onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=" + std::to_string(offTime) + "]"));
              onOffHelper.SetAttribute ("DataRate", DataRateValue (rate));
              onOffHelper.SetAttribute ("PacketSize", UintegerValue (packetSize));
              onOffHelper.SetAttribute ("Remote", AddressValue (Address (utUserAddr)));

              auto app = onOffHelper.Install (gws.Get (j)).Get (0);
              app->SetStartTime (startTime + (i + j*gws.GetN () + 1) * startDelay);
              onOffContainer.Add (app);
            }
        }
    }
  sinkContainer.Start (startTime);
  sinkContainer.Stop (stopTime);
}

void
SatTrafficHelper::AddCustomTraffic (TrafficDirection_t direction,
                                    std::string interval,
                                    uint32_t packetSize,
                                    NodeContainer gws,
                                    NodeContainer uts,
                                    Time startTime,
                                    Time stopTime,
                                    Time startDelay)
{
  NS_LOG_FUNCTION (this << direction << interval << packetSize << startTime << stopTime << startDelay);

  std::string socketFactory = "ns3::UdpSocketFactory";
  uint16_t port = 9;

  PacketSinkHelper sinkHelper (socketFactory, Address ());

  ObjectFactory factory;
  factory.SetTypeId ("ns3::CbrApplication");
  factory.Set ("Protocol", StringValue (socketFactory));
  ApplicationContainer sinkContainer;
  ApplicationContainer cbrContainer;

  // create CBR applications from GWs to UT users
  for (uint32_t j = 0; j < gws.GetN (); j++)
    {
      for (uint32_t i = 0; i < uts.GetN (); i++)
        {
          if (direction == RTN_LINK)
            {
              InetSocketAddress gwUserAddr = InetSocketAddress (m_satHelper->GetUserAddress (gws.Get (j)), port);
              if (!HasSinkInstalled (gws.Get (j), port))
                {
                  sinkHelper.SetAttribute ("Local", AddressValue (Address (gwUserAddr)));
                  sinkContainer.Add (sinkHelper.Install (gws.Get (j)));
                }

              factory.Set ("Interval", TimeValue (Time (interval)));
              factory.Set ("PacketSize", UintegerValue (packetSize));
              factory.Set ("Remote", AddressValue (Address (gwUserAddr)));
              Ptr<CbrApplication> p_app = factory.Create<CbrApplication> ();
              uts.Get (i)->AddApplication (p_app);
              auto app = ApplicationContainer (p_app).Get (0);
              app->SetStartTime (startTime + (i + j*gws.GetN () + 1) * startDelay);
              cbrContainer.Add (app);
            }
          else if (direction == FWD_LINK)
            {
              InetSocketAddress utUserAddr = InetSocketAddress (m_satHelper->GetUserAddress (uts.Get (i)), port);
              if (!HasSinkInstalled (uts.Get (i), port))
                {
                  sinkHelper.SetAttribute ("Local", AddressValue (Address (utUserAddr)));
                  sinkContainer.Add (sinkHelper.Install (uts.Get (i)));
                }

              factory.Set ("Interval", TimeValue (Time (interval)));
              factory.Set ("PacketSize", UintegerValue (packetSize));
              factory.Set ("Remote", AddressValue (Address (utUserAddr)));
              Ptr<CbrApplication> p_app = factory.Create<CbrApplication> ();
              gws.Get (j)->AddApplication (p_app);
              auto app = ApplicationContainer (p_app).Get (0);
              app->SetStartTime (startTime + (i + j*gws.GetN () + 1) * startDelay);
              cbrContainer.Add (app);
            }
        }
    }

  sinkContainer.Start (startTime);
  sinkContainer.Stop (stopTime);

  m_last_custom_application.application = cbrContainer;
  m_last_custom_application.start = startTime;
  m_last_custom_application.stop = stopTime;
  m_last_custom_application.created = true;
}

  /**
   * Change the parameters of the last traffic created
   * \param time Time to apply the changes
   * \param interval New wait time between transmission of two packets
   * \param packetSize New packet size in bytes
   */
void
SatTrafficHelper::ChangeCustomTraffic (Time delay,
                                       std::string interval,
                                       uint32_t packetSize)
{
  NS_LOG_FUNCTION (this << delay << interval << packetSize);

  if (!m_last_custom_application.created)
    {
      NS_FATAL_ERROR ("No custom traffic created when calling the method SatTrafficHelper::ChangeCustomTraffic for the first time.");
    }
  if (m_last_custom_application.start + delay > m_last_custom_application.stop)
    {
      NS_FATAL_ERROR ("Custom traffic updated after its stop time.");
    }
  for (auto i = m_last_custom_application.application.Begin (); i != m_last_custom_application.application.End (); ++i)
    {
      Ptr<CbrApplication> app = (dynamic_cast<CbrApplication*> (PeekPointer (*i)));
      Simulator::Schedule (m_last_custom_application.start + delay, &SatTrafficHelper::UpdateAttribute, this, app, interval, packetSize);
    }
}

void
SatTrafficHelper::UpdateAttribute (Ptr<CbrApplication> application, std::string interval, uint32_t packetSize)
{
  NS_LOG_FUNCTION (this << application << interval << packetSize);

  application->SetInterval (Time (interval));
  application->SetPacketSize (packetSize);
}

//TODO same with add/remove nodes ?

bool
SatTrafficHelper::HasSinkInstalled (Ptr<Node> node, uint16_t port)
{
  NS_LOG_FUNCTION (this << node->GetId () << port);

  for (uint32_t i = 0; i < node->GetNApplications (); i++)
    {
      auto sink = DynamicCast<PacketSink> (node->GetApplication (i));
      if (sink != NULL)
        {
          AddressValue av;
          sink->GetAttribute ("Local", av);
          if (InetSocketAddress::ConvertFrom (av.Get ()).GetPort () == port)
            {
              return true;
            }
        }
    }
  return false;
}

} // namespace ns3