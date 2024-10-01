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

#include "satellite-on-off-helper.h"
#include "simulation-helper.h"

#include <ns3/cbr-helper.h>
#include <ns3/log.h>
#include <ns3/lora-periodic-sender.h>
#include <ns3/nrtv-helper.h>
#include <ns3/packet-sink-helper.h>
#include <ns3/packet-sink.h>
#include <ns3/satellite-topology.h>
#include <ns3/singleton.h>
#include <ns3/three-gpp-http-satellite-helper.h>
#include <ns3/type-id.h>

NS_LOG_COMPONENT_DEFINE("SatelliteTrafficHelper");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(SatTrafficHelper);

TypeId
SatTrafficHelper::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::SatTrafficHelper")
            .SetParent<Object>()
            .AddConstructor<SatTrafficHelper>()
            .AddAttribute("EnableDefaultStatistics",
                          "Enable outputing values from stats helpers",
                          BooleanValue(true),
                          MakeBooleanAccessor(&SatTrafficHelper::m_enableDefaultStatistics),
                          MakeBooleanChecker());
    return tid;
}

TypeId
SatTrafficHelper::GetInstanceTypeId(void) const
{
    NS_LOG_FUNCTION(this);

    return GetTypeId();
}

SatTrafficHelper::SatTrafficHelper()
    : m_satHelper(nullptr),
      m_satStatsHelperContainer(nullptr)
{
    NS_FATAL_ERROR("Default constructor of SatTrafficHelper cannot be used.");
}

SatTrafficHelper::SatTrafficHelper(Ptr<SatHelper> satHelper,
                                   Ptr<SatStatsHelperContainer> satStatsHelperContainer)
    : m_satHelper(satHelper),
      m_satStatsHelperContainer(satStatsHelperContainer),
      m_enableDefaultStatistics(false)
{
    m_last_custom_application.created = false;
}

void
SatTrafficHelper::AddLoraPeriodicTraffic(Time interval,
                                         uint32_t packetSize,
                                         NodeContainer uts,
                                         Time startTime,
                                         Time stopTime,
                                         Time startDelay)
{
    NS_LOG_FUNCTION(this << interval << packetSize << startTime << stopTime << startDelay);

    if (uts.GetN() == 0)
    {
        NS_LOG_WARN("UT container is empty");
        return;
    }

    PacketSinkHelper sinkHelper("ns3::UdpSocketFactory", Address());
    CbrHelper cbrHelper("ns3::UdpSocketFactory", Address());
    ApplicationContainer sinkContainer;
    ApplicationContainer cbrContainer;

    Ptr<Node> node;

    // create Lora Periodic sender applications from UT users to GW users
    for (uint32_t i = 0; i < uts.GetN(); i++)
    {
        node = uts.Get(i);
        Ptr<LoraPeriodicSender> app = Create<LoraPeriodicSender>();

        app->SetInterval(interval);
        NS_LOG_DEBUG("Created an application with interval = " << interval.GetHours() << " hours");

        app->SetStartTime(startTime + (i + 1) * startDelay);
        app->SetStopTime(stopTime);
        app->SetPacketSize(packetSize);

        app->SetNode(node);
        node->AddApplication(app);
    }
}

void
SatTrafficHelper::AddLoraPeriodicTraffic(Time interval,
                                         uint32_t packetSize,
                                         NodeContainer uts,
                                         Time startTime,
                                         Time stopTime,
                                         Time startDelay,
                                         double percentage)
{
    NS_LOG_FUNCTION(this << interval << packetSize << startTime << stopTime << startDelay
                         << percentage);

    // Filter UTs to keep only a given percentage on which installing the application
    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
    NodeContainer utsUpdated;
    for (uint32_t i = 0; i < uts.GetN(); ++i)
    {
        if (rng->GetValue(0.0, 1.0) < percentage)
        {
            utsUpdated.Add(uts.Get(i));
        }
    }

    AddLoraPeriodicTraffic(interval, packetSize, utsUpdated, startTime, stopTime, startDelay);
}

void
SatTrafficHelper::AddLoraCbrTraffic(Time interval,
                                    uint32_t packetSize,
                                    NodeContainer gwUsers,
                                    NodeContainer utUsers,
                                    Time startTime,
                                    Time stopTime,
                                    Time startDelay)
{
    NS_LOG_FUNCTION(this << interval << packetSize << startTime << stopTime << startDelay);

    if (gwUsers.GetN() == 0)
    {
        NS_LOG_WARN("Gateway users container is empty");
        return;
    }
    if (utUsers.GetN() == 0)
    {
        NS_LOG_WARN("UT users container is empty");
        return;
    }

    uint16_t port = 9;

    PacketSinkHelper sinkHelper("ns3::UdpSocketFactory", Address());
    CbrHelper cbrHelper("ns3::UdpSocketFactory", Address());
    ApplicationContainer sinkContainer;
    ApplicationContainer cbrContainer;

    // create CBR applications from UT users to GW users
    for (uint32_t j = 0; j < gwUsers.GetN(); j++)
    {
        for (uint32_t i = 0; i < utUsers.GetN(); i++)
        {
            InetSocketAddress gwUserAddr =
                InetSocketAddress(m_satHelper->GetUserAddress(gwUsers.Get(j)), port);
            if (!HasSinkInstalled(gwUsers.Get(j), port))
            {
                sinkHelper.SetAttribute("Local", AddressValue(Address(gwUserAddr)));
                sinkContainer.Add(sinkHelper.Install(gwUsers.Get(j)));
            }

            cbrHelper.SetConstantTraffic(interval, packetSize);
            cbrHelper.SetAttribute("Remote", AddressValue(Address(gwUserAddr)));
            auto app = cbrHelper.Install(utUsers.Get(i)).Get(0);
            app->SetStartTime(startTime + (i + j * gwUsers.GetN() + 1) * startDelay);
            cbrContainer.Add(app);
        }
    }

    sinkContainer.Start(startTime);
    sinkContainer.Stop(stopTime);
}

void
SatTrafficHelper::AddLoraCbrTraffic(Time interval,
                                    uint32_t packetSize,
                                    NodeContainer gwUsers,
                                    NodeContainer utUsers,
                                    Time startTime,
                                    Time stopTime,
                                    Time startDelay,
                                    double percentage)
{
    NS_LOG_FUNCTION(this << interval << packetSize << startTime << stopTime << startDelay
                         << percentage);

    // Filter UT users to keep only a given percentage on which installing the application
    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
    NodeContainer utUsersUpdated;
    for (uint32_t i = 0; i < utUsers.GetN(); ++i)
    {
        if (rng->GetValue(0.0, 1.0) < percentage)
        {
            utUsersUpdated.Add(utUsers.Get(i));
        }
    }

    AddLoraCbrTraffic(interval,
                      packetSize,
                      gwUsers,
                      utUsersUpdated,
                      startTime,
                      stopTime,
                      startDelay);
}

void
SatTrafficHelper::AddCbrTraffic(TrafficDirection_t direction,
                                TransportLayerProtocol_t protocol,
                                Time interval,
                                uint32_t packetSize,
                                NodeContainer gwUsers,
                                NodeContainer utUsers,
                                Time startTime,
                                Time stopTime,
                                Time startDelay)
{
    NS_LOG_FUNCTION(this << interval << packetSize << startTime << stopTime << startDelay);

    if (gwUsers.GetN() == 0)
    {
        NS_LOG_WARN("Gateway users container is empty");
        return;
    }
    if (utUsers.GetN() == 0)
    {
        NS_LOG_WARN("UT users container is empty");
        return;
    }

    std::string socketFactory =
        (protocol == SatTrafficHelper::TCP ? "ns3::TcpSocketFactory" : "ns3::UdpSocketFactory");
    uint16_t port = 9;

    PacketSinkHelper sinkHelper(socketFactory, Address());
    CbrHelper cbrHelper(socketFactory, Address());
    ApplicationContainer sinkContainer;
    ApplicationContainer cbrContainer;

    // create CBR applications from GWs to UT users
    for (uint32_t j = 0; j < gwUsers.GetN(); j++)
    {
        for (uint32_t i = 0; i < utUsers.GetN(); i++)
        {
            if (direction == RTN_LINK)
            {
                InetSocketAddress gwUserAddr =
                    InetSocketAddress(m_satHelper->GetUserAddress(gwUsers.Get(j)), port);
                if (!HasSinkInstalled(gwUsers.Get(j), port))
                {
                    sinkHelper.SetAttribute("Local", AddressValue(Address(gwUserAddr)));
                    sinkContainer.Add(sinkHelper.Install(gwUsers.Get(j)));
                }

                cbrHelper.SetConstantTraffic(interval, packetSize);
                cbrHelper.SetAttribute("Remote", AddressValue(Address(gwUserAddr)));
                auto app = cbrHelper.Install(utUsers.Get(i)).Get(0);
                app->SetStartTime(startTime + (i + j * gwUsers.GetN() + 1) * startDelay);
                cbrContainer.Add(app);
            }
            else if (direction == FWD_LINK)
            {
                InetSocketAddress utUserAddr =
                    InetSocketAddress(m_satHelper->GetUserAddress(utUsers.Get(i)), port);
                if (!HasSinkInstalled(utUsers.Get(i), port))
                {
                    sinkHelper.SetAttribute("Local", AddressValue(Address(utUserAddr)));
                    sinkContainer.Add(sinkHelper.Install(utUsers.Get(i)));
                }

                cbrHelper.SetConstantTraffic(interval, packetSize);
                cbrHelper.SetAttribute("Remote", AddressValue(Address(utUserAddr)));
                auto app = cbrHelper.Install(gwUsers.Get(j)).Get(0);
                app->SetStartTime(startTime + (i + j * gwUsers.GetN() + 1) * startDelay);
                cbrContainer.Add(app);
            }
        }
    }

    sinkContainer.Start(startTime);
    sinkContainer.Stop(stopTime);

    if (m_enableDefaultStatistics)
    {
        // Add throuhgput statistics
        if (direction == FWD_LINK)
        {
            // Global scalar
            m_satStatsHelperContainer->AddGlobalFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Global scatter
            m_satStatsHelperContainer->AddGlobalFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per UT scalar
            m_satStatsHelperContainer->AddPerUtFwdAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per UT scatter
            m_satStatsHelperContainer->AddPerUtFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per GW scalar
            m_satStatsHelperContainer->AddPerGwFwdAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per GW scatter
            m_satStatsHelperContainer->AddPerGwFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
        }
        else if (direction == RTN_LINK)
        {
            // Global scalar
            m_satStatsHelperContainer->AddGlobalRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Global scatter
            m_satStatsHelperContainer->AddGlobalRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per UT scalar
            m_satStatsHelperContainer->AddPerUtRtnAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per UT scatter
            m_satStatsHelperContainer->AddPerUtRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per GW scalar
            m_satStatsHelperContainer->AddPerGwRtnAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per GW scatter
            m_satStatsHelperContainer->AddPerGwRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
        }
    }
}

void
SatTrafficHelper::AddCbrTraffic(TrafficDirection_t direction,
                                TransportLayerProtocol_t protocol,
                                Time interval,
                                uint32_t packetSize,
                                NodeContainer gwUsers,
                                NodeContainer utUsers,
                                Time startTime,
                                Time stopTime,
                                Time startDelay,
                                double percentage)
{
    NS_LOG_FUNCTION(this << interval << packetSize << startTime << stopTime << startDelay
                         << percentage);

    // Filter UT users to keep only a given percentage on which installing the application
    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
    NodeContainer utUsersUpdated;
    for (uint32_t i = 0; i < utUsers.GetN(); ++i)
    {
        if (rng->GetValue(0.0, 1.0) < percentage)
        {
            utUsersUpdated.Add(utUsers.Get(i));
        }
    }

    AddCbrTraffic(direction,
                  protocol,
                  interval,
                  packetSize,
                  gwUsers,
                  utUsersUpdated,
                  startTime,
                  stopTime,
                  startDelay);
}

void
SatTrafficHelper::AddOnOffTraffic(TrafficDirection_t direction,
                                  TransportLayerProtocol_t protocol,
                                  DataRate dataRate,
                                  uint32_t packetSize,
                                  NodeContainer gwUsers,
                                  NodeContainer utUsers,
                                  std::string onTimePattern,
                                  std::string offTimePattern,
                                  Time startTime,
                                  Time stopTime,
                                  Time startDelay)
{
    NS_LOG_FUNCTION(this << dataRate << packetSize << onTimePattern << offTimePattern << startTime
                         << stopTime << startDelay);

    if (gwUsers.GetN() == 0)
    {
        NS_LOG_WARN("Gateway users container is empty");
        return;
    }
    if (utUsers.GetN() == 0)
    {
        NS_LOG_WARN("UT users container is empty");
        return;
    }

    std::string socketFactory =
        (protocol == SatTrafficHelper::TCP ? "ns3::TcpSocketFactory" : "ns3::UdpSocketFactory");
    uint16_t port = 9;

    PacketSinkHelper sinkHelper(socketFactory, Address());
    SatOnOffHelper onOffHelper(socketFactory, Address());
    ApplicationContainer sinkContainer;
    ApplicationContainer onOffContainer;

    onOffHelper.SetAttribute("OnTime", StringValue(onTimePattern));
    onOffHelper.SetAttribute("OffTime", StringValue(offTimePattern));
    onOffHelper.SetAttribute("DataRate", DataRateValue(dataRate));
    onOffHelper.SetAttribute("PacketSize", UintegerValue(packetSize));

    // create ONOFF applications from GWs to UT users
    for (uint32_t j = 0; j < gwUsers.GetN(); j++)
    {
        for (uint32_t i = 0; i < utUsers.GetN(); i++)
        {
            if (direction == RTN_LINK)
            {
                InetSocketAddress gwUserAddr =
                    InetSocketAddress(m_satHelper->GetUserAddress(gwUsers.Get(j)), port);
                if (!HasSinkInstalled(gwUsers.Get(j), port))
                {
                    sinkHelper.SetAttribute("Local", AddressValue(Address(gwUserAddr)));
                    sinkContainer.Add(sinkHelper.Install(gwUsers.Get(j)));
                }

                onOffHelper.SetAttribute("Remote", AddressValue(Address(gwUserAddr)));
                auto app = onOffHelper.Install(utUsers.Get(i)).Get(0);
                app->SetStartTime(startTime + (i + j * gwUsers.GetN() + 1) * startDelay);
                onOffContainer.Add(app);
            }
            else if (direction == FWD_LINK)
            {
                InetSocketAddress utUserAddr =
                    InetSocketAddress(m_satHelper->GetUserAddress(utUsers.Get(i)), port);
                if (!HasSinkInstalled(utUsers.Get(i), port))
                {
                    sinkHelper.SetAttribute("Local", AddressValue(Address(utUserAddr)));
                    sinkContainer.Add(sinkHelper.Install(utUsers.Get(i)));
                }

                onOffHelper.SetAttribute("Remote", AddressValue(Address(utUserAddr)));
                auto app = onOffHelper.Install(gwUsers.Get(j)).Get(0);
                app->SetStartTime(startTime + (i + j * gwUsers.GetN() + 1) * startDelay);
                onOffContainer.Add(app);
            }
        }
    }

    sinkContainer.Start(startTime);
    sinkContainer.Stop(stopTime);

    if (m_enableDefaultStatistics)
    {
        // Add throuhgput statistics
        if (direction == FWD_LINK)
        {
            // Global scalar
            m_satStatsHelperContainer->AddGlobalFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Global scatter
            m_satStatsHelperContainer->AddGlobalFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per UT scalar
            m_satStatsHelperContainer->AddPerUtFwdAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per UT scatter
            m_satStatsHelperContainer->AddPerUtFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per GW scalar
            m_satStatsHelperContainer->AddPerGwFwdAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per GW scatter
            m_satStatsHelperContainer->AddPerGwFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
        }
        else if (direction == RTN_LINK)
        {
            // Global scalar
            m_satStatsHelperContainer->AddGlobalRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Global scatter
            m_satStatsHelperContainer->AddGlobalRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per UT scalar
            m_satStatsHelperContainer->AddPerUtRtnAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per UT scatter
            m_satStatsHelperContainer->AddPerUtRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per GW scalar
            m_satStatsHelperContainer->AddPerGwRtnAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per GW scatter
            m_satStatsHelperContainer->AddPerGwRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
        }
    }
}

void
SatTrafficHelper::AddOnOffTraffic(TrafficDirection_t direction,
                                  TransportLayerProtocol_t protocol,
                                  DataRate dataRate,
                                  uint32_t packetSize,
                                  NodeContainer gwUsers,
                                  NodeContainer utUsers,
                                  std::string onTimePattern,
                                  std::string offTimePattern,
                                  Time startTime,
                                  Time stopTime,
                                  Time startDelay,
                                  double percentage)
{
    NS_LOG_FUNCTION(this << dataRate << packetSize << onTimePattern << offTimePattern << startTime
                         << stopTime << startDelay << percentage);

    // Filter UT users to keep only a given percentage on which installing the application
    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
    NodeContainer utUsersUpdated;
    for (uint32_t i = 0; i < utUsers.GetN(); ++i)
    {
        if (rng->GetValue(0.0, 1.0) < percentage)
        {
            utUsersUpdated.Add(utUsers.Get(i));
        }
    }

    AddOnOffTraffic(direction,
                    protocol,
                    dataRate,
                    packetSize,
                    gwUsers,
                    utUsersUpdated,
                    onTimePattern,
                    offTimePattern,
                    startTime,
                    stopTime,
                    startDelay);
}

void
SatTrafficHelper::AddHttpTraffic(TrafficDirection_t direction,
                                 NodeContainer gwUsers,
                                 NodeContainer utUsers,
                                 Time startTime,
                                 Time stopTime,
                                 Time startDelay)
{
    NS_LOG_FUNCTION(this << direction << startTime << stopTime << startDelay);

    if (gwUsers.GetN() == 0)
    {
        NS_LOG_WARN("Gateway users container is empty");
        return;
    }
    if (utUsers.GetN() == 0)
    {
        NS_LOG_WARN("UT users container is empty");
        return;
    }

    ThreeGppHttpHelper httpHelper;
    if (direction == FWD_LINK)
    {
        for (uint32_t j = 0; j < gwUsers.GetN(); j++)
        {
            auto app = httpHelper.InstallUsingIpv4(gwUsers.Get(j), utUsers).Get(1);
            app->SetStartTime(startTime + (j + 1) * startDelay);
            httpHelper.GetServer().Start(startTime);
            httpHelper.GetServer().Stop(stopTime);
        }
    }
    else if (direction == RTN_LINK)
    {
        for (uint32_t i = 0; i < utUsers.GetN(); i++)
        {
            auto app = httpHelper.InstallUsingIpv4(utUsers.Get(i), gwUsers).Get(1);
            app->SetStartTime(startTime + (i + 1) * startDelay);
            httpHelper.GetServer().Start(startTime);
            httpHelper.GetServer().Stop(stopTime);
        }
    }

    if (m_enableDefaultStatistics)
    {
        // Add PLT statistics
        if (direction == FWD_LINK)
        {
            m_satStatsHelperContainer->AddGlobalFwdAppPlt(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalFwdAppPlt(SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtFwdAppPlt(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtFwdAppPlt(SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwFwdAppPlt(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwFwdAppPlt(SatStatsHelper::OUTPUT_SCATTER_FILE);
        }
        else if (direction == RTN_LINK)
        {
            m_satStatsHelperContainer->AddGlobalRtnAppPlt(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalRtnAppPlt(SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtRtnAppPlt(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtRtnAppPlt(SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwRtnAppPlt(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwRtnAppPlt(SatStatsHelper::OUTPUT_SCATTER_FILE);
        }

        // Add throuhgput statistics
        if (direction == FWD_LINK)
        {
            // Global scalar
            m_satStatsHelperContainer->AddGlobalFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Global scatter
            m_satStatsHelperContainer->AddGlobalFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per UT scalar
            m_satStatsHelperContainer->AddPerUtFwdAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per UT scatter
            m_satStatsHelperContainer->AddPerUtFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per GW scalar
            m_satStatsHelperContainer->AddPerGwFwdAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per GW scatter
            m_satStatsHelperContainer->AddPerGwFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
        }
        else if (direction == RTN_LINK)
        {
            // Global scalar
            m_satStatsHelperContainer->AddGlobalRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Global scatter
            m_satStatsHelperContainer->AddGlobalRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per UT scalar
            m_satStatsHelperContainer->AddPerUtRtnAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per UT scatter
            m_satStatsHelperContainer->AddPerUtRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per GW scalar
            m_satStatsHelperContainer->AddPerGwRtnAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per GW scatter
            m_satStatsHelperContainer->AddPerGwRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
        }
    }
}

void
SatTrafficHelper::AddHttpTraffic(TrafficDirection_t direction,
                                 NodeContainer gwUsers,
                                 NodeContainer utUsers,
                                 Time startTime,
                                 Time stopTime,
                                 Time startDelay,
                                 double percentage)
{
    NS_LOG_FUNCTION(this << direction << startTime << stopTime << startDelay << percentage);

    // Filter UT users to keep only a given percentage on which installing the application
    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
    NodeContainer utUsersUpdated;
    for (uint32_t i = 0; i < utUsers.GetN(); ++i)
    {
        if (rng->GetValue(0.0, 1.0) < percentage)
        {
            utUsersUpdated.Add(utUsers.Get(i));
        }
    }

    AddHttpTraffic(direction, gwUsers, utUsersUpdated, startTime, stopTime, startDelay);
}

void
SatTrafficHelper::AddNrtvTraffic(TrafficDirection_t direction,
                                 NodeContainer gwUsers,
                                 NodeContainer utUsers,
                                 Time startTime,
                                 Time stopTime,
                                 Time startDelay)
{
    NS_LOG_FUNCTION(this << direction << startTime << stopTime << startDelay);

    if (gwUsers.GetN() == 0)
    {
        NS_LOG_WARN("Gateway users container is empty");
        return;
    }
    if (utUsers.GetN() == 0)
    {
        NS_LOG_WARN("UT users container is empty");
        return;
    }

    std::string socketFactory = "ns3::TcpSocketFactory";

    NrtvHelper nrtvHelper(TypeId::LookupByName(socketFactory));
    if (direction == FWD_LINK)
    {
        for (uint32_t j = 0; j < gwUsers.GetN(); j++)
        {
            auto app = nrtvHelper.InstallUsingIpv4(gwUsers.Get(j), utUsers).Get(1);
            app->SetStartTime(startTime + (j + 1) * startDelay);
            nrtvHelper.GetServer().Start(startTime);
            nrtvHelper.GetServer().Stop(stopTime);
        }
    }
    else if (direction == RTN_LINK)
    {
        for (uint32_t i = 0; i < utUsers.GetN(); i++)
        {
            auto app = nrtvHelper.InstallUsingIpv4(utUsers.Get(i), gwUsers).Get(1);
            app->SetStartTime(startTime + (i + 1) * startDelay);
            nrtvHelper.GetServer().Start(startTime);
            nrtvHelper.GetServer().Stop(stopTime);
        }
    }

    if (m_enableDefaultStatistics)
    {
        // Add throuhgput statistics
        if (direction == FWD_LINK)
        {
            // Global scalar
            m_satStatsHelperContainer->AddGlobalFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Global scatter
            m_satStatsHelperContainer->AddGlobalFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per UT scalar
            m_satStatsHelperContainer->AddPerUtFwdAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per UT scatter
            m_satStatsHelperContainer->AddPerUtFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per GW scalar
            m_satStatsHelperContainer->AddPerGwFwdAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per GW scatter
            m_satStatsHelperContainer->AddPerGwFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
        }
        else if (direction == RTN_LINK)
        {
            // Global scalar
            m_satStatsHelperContainer->AddGlobalRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Global scatter
            m_satStatsHelperContainer->AddGlobalRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per UT scalar
            m_satStatsHelperContainer->AddPerUtRtnAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per UT scatter
            m_satStatsHelperContainer->AddPerUtRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per GW scalar
            m_satStatsHelperContainer->AddPerGwRtnAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per GW scatter
            m_satStatsHelperContainer->AddPerGwRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
        }
    }
}

void
SatTrafficHelper::AddNrtvTraffic(TrafficDirection_t direction,
                                 NodeContainer gwUsers,
                                 NodeContainer utUsers,
                                 Time startTime,
                                 Time stopTime,
                                 Time startDelay,
                                 double percentage)
{
    NS_LOG_FUNCTION(this << direction << startTime << stopTime << startDelay << percentage);

    // Filter UT users to keep only a given percentage on which installing the application
    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
    NodeContainer utUsersUpdated;
    for (uint32_t i = 0; i < utUsers.GetN(); ++i)
    {
        if (rng->GetValue(0.0, 1.0) < percentage)
        {
            utUsersUpdated.Add(utUsers.Get(i));
        }
    }

    AddNrtvTraffic(direction, gwUsers, utUsersUpdated, startTime, stopTime, startDelay);
}

void
SatTrafficHelper::AddPoissonTraffic(TrafficDirection_t direction,
                                    Time onTime,
                                    Time offTimeExpMean,
                                    DataRate rate,
                                    uint32_t packetSize,
                                    NodeContainer gwUsers,
                                    NodeContainer utUsers,
                                    Time startTime,
                                    Time stopTime,
                                    Time startDelay)
{
    NS_LOG_FUNCTION(this << direction << onTime << offTimeExpMean << rate << packetSize << startTime
                         << stopTime << startDelay);

    if (gwUsers.GetN() == 0)
    {
        NS_LOG_WARN("Gateway users container is empty");
        return;
    }
    if (utUsers.GetN() == 0)
    {
        NS_LOG_WARN("UT users container is empty");
        return;
    }

    std::string socketFactory = "ns3::UdpSocketFactory";

    uint16_t port = 9;

    PacketSinkHelper sinkHelper(socketFactory, Address());
    SatOnOffHelper onOffHelper(socketFactory, Address());
    ApplicationContainer sinkContainer;
    ApplicationContainer onOffContainer;

    // create CBR applications from GWs to UT users
    for (uint32_t j = 0; j < gwUsers.GetN(); j++)
    {
        for (uint32_t i = 0; i < utUsers.GetN(); i++)
        {
            if (direction == RTN_LINK)
            {
                InetSocketAddress gwUserAddr =
                    InetSocketAddress(m_satHelper->GetUserAddress(gwUsers.Get(j)), port);

                if (!HasSinkInstalled(gwUsers.Get(j), port))
                {
                    sinkHelper.SetAttribute("Local", AddressValue(Address(gwUserAddr)));
                    sinkContainer.Add(sinkHelper.Install(gwUsers.Get(j)));
                }

                onOffHelper.SetAttribute("OnTime",
                                         StringValue("ns3::ConstantRandomVariable[Constant=" +
                                                     std::to_string(onTime.GetSeconds()) + "]"));
                onOffHelper.SetAttribute("OffTime",
                                         StringValue("ns3::ExponentialRandomVariable[Mean=" +
                                                     std::to_string(offTimeExpMean.GetSeconds()) +
                                                     "]"));
                onOffHelper.SetAttribute("DataRate", DataRateValue(rate));
                onOffHelper.SetAttribute("PacketSize", UintegerValue(packetSize));
                onOffHelper.SetAttribute("Remote", AddressValue(Address(gwUserAddr)));

                auto app = onOffHelper.Install(utUsers.Get(i)).Get(0);
                app->SetStartTime(startTime + (i + j * gwUsers.GetN() + 1) * startDelay);
                onOffContainer.Add(app);
            }
            else if (direction == FWD_LINK)
            {
                InetSocketAddress utUserAddr =
                    InetSocketAddress(m_satHelper->GetUserAddress(utUsers.Get(i)), port);

                if (!HasSinkInstalled(utUsers.Get(i), port))
                {
                    sinkHelper.SetAttribute("Local", AddressValue(Address(utUserAddr)));
                    sinkContainer.Add(sinkHelper.Install(utUsers.Get(i)));
                }

                onOffHelper.SetAttribute("OnTime",
                                         StringValue("ns3::ConstantRandomVariable[Constant=" +
                                                     std::to_string(onTime.GetSeconds()) + "]"));
                onOffHelper.SetAttribute("OffTime",
                                         StringValue("ns3::ExponentialRandomVariable[Mean=" +
                                                     std::to_string(offTimeExpMean.GetSeconds()) +
                                                     "]"));
                onOffHelper.SetAttribute("DataRate", DataRateValue(rate));
                onOffHelper.SetAttribute("PacketSize", UintegerValue(packetSize));
                onOffHelper.SetAttribute("Remote", AddressValue(Address(utUserAddr)));

                auto app = onOffHelper.Install(gwUsers.Get(j)).Get(0);
                app->SetStartTime(startTime + (i + j * gwUsers.GetN() + 1) * startDelay);
                onOffContainer.Add(app);
            }
        }
    }
    sinkContainer.Start(startTime);
    sinkContainer.Stop(stopTime);

    if (m_enableDefaultStatistics)
    {
        // Add throuhgput statistics
        if (direction == FWD_LINK)
        {
            // Global scalar
            m_satStatsHelperContainer->AddGlobalFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Global scatter
            m_satStatsHelperContainer->AddGlobalFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per UT scalar
            m_satStatsHelperContainer->AddPerUtFwdAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per UT scatter
            m_satStatsHelperContainer->AddPerUtFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per GW scalar
            m_satStatsHelperContainer->AddPerGwFwdAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per GW scatter
            m_satStatsHelperContainer->AddPerGwFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
        }
        else if (direction == RTN_LINK)
        {
            // Global scalar
            m_satStatsHelperContainer->AddGlobalRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Global scatter
            m_satStatsHelperContainer->AddGlobalRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per UT scalar
            m_satStatsHelperContainer->AddPerUtRtnAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per UT scatter
            m_satStatsHelperContainer->AddPerUtRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per GW scalar
            m_satStatsHelperContainer->AddPerGwRtnAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per GW scatter
            m_satStatsHelperContainer->AddPerGwRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
        }
    }
}

void
SatTrafficHelper::AddPoissonTraffic(TrafficDirection_t direction,
                                    Time onTime,
                                    Time offTimeExpMean,
                                    DataRate rate,
                                    uint32_t packetSize,
                                    NodeContainer gwUsers,
                                    NodeContainer utUsers,
                                    Time startTime,
                                    Time stopTime,
                                    Time startDelay,
                                    double percentage)
{
    NS_LOG_FUNCTION(this << direction << onTime << offTimeExpMean << rate << packetSize << startTime
                         << stopTime << startDelay << percentage);

    // Filter UT users to keep only a given percentage on which installing the application
    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
    NodeContainer utUsersUpdated;
    for (uint32_t i = 0; i < utUsers.GetN(); ++i)
    {
        if (rng->GetValue(0.0, 1.0) < percentage)
        {
            utUsersUpdated.Add(utUsers.Get(i));
        }
    }

    AddPoissonTraffic(direction,
                      onTime,
                      offTimeExpMean,
                      rate,
                      packetSize,
                      gwUsers,
                      utUsersUpdated,
                      startTime,
                      stopTime,
                      startDelay);
}

void
SatTrafficHelper::AddVoipTraffic(TrafficDirection_t direction,
                                 VoipCodec_t codec,
                                 NodeContainer gwUsers,
                                 NodeContainer utUsers,
                                 Time startTime,
                                 Time stopTime,
                                 Time startDelay)
{
    NS_LOG_FUNCTION(this << direction << codec << startTime << stopTime << startDelay);

    if (gwUsers.GetN() == 0)
    {
        NS_LOG_WARN("Gateway users container is empty");
        return;
    }
    if (utUsers.GetN() == 0)
    {
        NS_LOG_WARN("UT users container is empty");
        return;
    }

    std::string socketFactory = "ns3::UdpSocketFactory";
    uint16_t port = 9;

    double onTime;
    double offTime;
    std::string rate;
    uint32_t packetSize;

    switch (codec)
    {
    case G_711_1:
        onTime = 0.5;
        offTime = 0.05;
        rate = "70kbps"; // 64kbps globally
        packetSize = 80;
        break;
    case G_711_2:
        onTime = 0.5;
        offTime = 0.05;
        rate = "70kbps"; // 64kbps globally
        packetSize = 160;
        break;
    case G_723_1:
        onTime = 0.5;
        offTime = 0.05;
        rate = "6864bps"; // 6240bps globally
        packetSize = 30;
        break;
    case G_729_2:
        onTime = 0.5;
        offTime = 0.05;
        rate = "8800bps"; // 8kbps globally
        packetSize = 20;
        break;
    case G_729_3:
        onTime = 0.5;
        offTime = 0.05;
        rate = "7920bps"; // 7200bps globally
        packetSize = 30;
        break;
    default:
        NS_FATAL_ERROR("VoIP codec does not exist or is not implemented");
    }

    PacketSinkHelper sinkHelper(socketFactory, Address());
    SatOnOffHelper onOffHelper(socketFactory, Address());
    ApplicationContainer sinkContainer;
    ApplicationContainer onOffContainer;

    // create CBR applications from GWs to UT users
    for (uint32_t j = 0; j < gwUsers.GetN(); j++)
    {
        for (uint32_t i = 0; i < utUsers.GetN(); i++)
        {
            if (direction == RTN_LINK)
            {
                InetSocketAddress gwUserAddr =
                    InetSocketAddress(m_satHelper->GetUserAddress(gwUsers.Get(j)), port);

                if (!HasSinkInstalled(gwUsers.Get(j), port))
                {
                    sinkHelper.SetAttribute("Local", AddressValue(Address(gwUserAddr)));
                    sinkContainer.Add(sinkHelper.Install(gwUsers.Get(j)));
                }

                onOffHelper.SetAttribute("OnTime",
                                         StringValue("ns3::ConstantRandomVariable[Constant=" +
                                                     std::to_string(onTime) + "]"));
                onOffHelper.SetAttribute("OffTime",
                                         StringValue("ns3::ConstantRandomVariable[Constant=" +
                                                     std::to_string(offTime) + "]"));
                onOffHelper.SetAttribute("DataRate", DataRateValue(rate));
                onOffHelper.SetAttribute("PacketSize", UintegerValue(packetSize));
                onOffHelper.SetAttribute("Remote", AddressValue(Address(gwUserAddr)));

                auto app = onOffHelper.Install(utUsers.Get(i)).Get(0);
                app->SetStartTime(startTime + (i + j * gwUsers.GetN() + 1) * startDelay);
                onOffContainer.Add(app);
            }
            else if (direction == FWD_LINK)
            {
                InetSocketAddress utUserAddr =
                    InetSocketAddress(m_satHelper->GetUserAddress(utUsers.Get(i)), port);

                if (!HasSinkInstalled(utUsers.Get(i), port))
                {
                    sinkHelper.SetAttribute("Local", AddressValue(Address(utUserAddr)));
                    sinkContainer.Add(sinkHelper.Install(utUsers.Get(i)));
                }

                onOffHelper.SetAttribute("OnTime",
                                         StringValue("ns3::ConstantRandomVariable[Constant=" +
                                                     std::to_string(onTime) + "]"));
                onOffHelper.SetAttribute("OffTime",
                                         StringValue("ns3::ConstantRandomVariable[Constant=" +
                                                     std::to_string(offTime) + "]"));
                onOffHelper.SetAttribute("DataRate", DataRateValue(rate));
                onOffHelper.SetAttribute("PacketSize", UintegerValue(packetSize));
                onOffHelper.SetAttribute("Remote", AddressValue(Address(utUserAddr)));

                auto app = onOffHelper.Install(gwUsers.Get(j)).Get(0);
                app->SetStartTime(startTime + (i + j * gwUsers.GetN() + 1) * startDelay);
                onOffContainer.Add(app);
            }
        }
    }
    sinkContainer.Start(startTime);
    sinkContainer.Stop(stopTime);

    if (m_enableDefaultStatistics)
    {
        // Add jitter statistics
        if (direction == FWD_LINK)
        {
            // Global
            m_satStatsHelperContainer->AddGlobalFwdAppJitter(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalFwdAppJitter(SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per UT
            m_satStatsHelperContainer->AddPerUtFwdAppJitter(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtFwdAppJitter(SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per GW
            m_satStatsHelperContainer->AddPerGwFwdAppJitter(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwFwdAppJitter(SatStatsHelper::OUTPUT_SCATTER_FILE);
        }
        else if (direction == RTN_LINK)
        {
            // Global
            m_satStatsHelperContainer->AddGlobalRtnAppJitter(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalRtnAppJitter(SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per UT
            m_satStatsHelperContainer->AddPerUtRtnAppJitter(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtRtnAppJitter(SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per GW
            m_satStatsHelperContainer->AddPerGwRtnAppJitter(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwRtnAppJitter(SatStatsHelper::OUTPUT_SCATTER_FILE);
        }

        // Add throuhgput statistics
        if (direction == FWD_LINK)
        {
            // Global scalar
            m_satStatsHelperContainer->AddGlobalFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Global scatter
            m_satStatsHelperContainer->AddGlobalFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per UT scalar
            m_satStatsHelperContainer->AddPerUtFwdAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per UT scatter
            m_satStatsHelperContainer->AddPerUtFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per GW scalar
            m_satStatsHelperContainer->AddPerGwFwdAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per GW scatter
            m_satStatsHelperContainer->AddPerGwFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
        }
        else if (direction == RTN_LINK)
        {
            // Global scalar
            m_satStatsHelperContainer->AddGlobalRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Global scatter
            m_satStatsHelperContainer->AddGlobalRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per UT scalar
            m_satStatsHelperContainer->AddPerUtRtnAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per UT scatter
            m_satStatsHelperContainer->AddPerUtRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per GW scalar
            m_satStatsHelperContainer->AddPerGwRtnAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per GW scatter
            m_satStatsHelperContainer->AddPerGwRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
        }
    }
}

void
SatTrafficHelper::AddVoipTraffic(TrafficDirection_t direction,
                                 VoipCodec_t codec,
                                 NodeContainer gwUsers,
                                 NodeContainer utUsers,
                                 Time startTime,
                                 Time stopTime,
                                 Time startDelay,
                                 double percentage)
{
    NS_LOG_FUNCTION(this << direction << codec << startTime << stopTime << startDelay
                         << percentage);

    // Filter UT users to keep only a given percentage on which installing the application
    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
    NodeContainer utUsersUpdated;
    for (uint32_t i = 0; i < utUsers.GetN(); ++i)
    {
        if (rng->GetValue(0.0, 1.0) < percentage)
        {
            utUsersUpdated.Add(utUsers.Get(i));
        }
    }

    AddVoipTraffic(direction, codec, gwUsers, utUsersUpdated, startTime, stopTime, startDelay);
}

void
SatTrafficHelper::AddCustomTraffic(TrafficDirection_t direction,
                                   std::string interval,
                                   uint32_t packetSize,
                                   NodeContainer gwUsers,
                                   NodeContainer utUsers,
                                   Time startTime,
                                   Time stopTime,
                                   Time startDelay)
{
    NS_LOG_FUNCTION(this << direction << interval << packetSize << startTime << stopTime
                         << startDelay);

    if (gwUsers.GetN() == 0)
    {
        NS_LOG_WARN("Gateway users container is empty");
        return;
    }
    if (utUsers.GetN() == 0)
    {
        NS_LOG_WARN("UT users container is empty");
        return;
    }

    std::string socketFactory = "ns3::UdpSocketFactory";
    uint16_t port = 9;

    PacketSinkHelper sinkHelper(socketFactory, Address());

    ObjectFactory factory;
    factory.SetTypeId("ns3::CbrApplication");
    factory.Set("Protocol", StringValue(socketFactory));
    ApplicationContainer sinkContainer;
    ApplicationContainer cbrContainer;

    // create CBR applications from GWs to UT users
    for (uint32_t j = 0; j < gwUsers.GetN(); j++)
    {
        for (uint32_t i = 0; i < utUsers.GetN(); i++)
        {
            if (direction == RTN_LINK)
            {
                InetSocketAddress gwUserAddr =
                    InetSocketAddress(m_satHelper->GetUserAddress(gwUsers.Get(j)), port);
                if (!HasSinkInstalled(gwUsers.Get(j), port))
                {
                    sinkHelper.SetAttribute("Local", AddressValue(Address(gwUserAddr)));
                    sinkContainer.Add(sinkHelper.Install(gwUsers.Get(j)));
                }

                factory.Set("Interval", TimeValue(Time(interval)));
                factory.Set("PacketSize", UintegerValue(packetSize));
                factory.Set("Remote", AddressValue(Address(gwUserAddr)));
                Ptr<CbrApplication> p_app = factory.Create<CbrApplication>();
                utUsers.Get(i)->AddApplication(p_app);
                auto app = ApplicationContainer(p_app).Get(0);
                app->SetStartTime(startTime + (i + j * gwUsers.GetN() + 1) * startDelay);
                cbrContainer.Add(app);
            }
            else if (direction == FWD_LINK)
            {
                InetSocketAddress utUserAddr =
                    InetSocketAddress(m_satHelper->GetUserAddress(utUsers.Get(i)), port);
                if (!HasSinkInstalled(utUsers.Get(i), port))
                {
                    sinkHelper.SetAttribute("Local", AddressValue(Address(utUserAddr)));
                    sinkContainer.Add(sinkHelper.Install(utUsers.Get(i)));
                }

                factory.Set("Interval", TimeValue(Time(interval)));
                factory.Set("PacketSize", UintegerValue(packetSize));
                factory.Set("Remote", AddressValue(Address(utUserAddr)));
                Ptr<CbrApplication> p_app = factory.Create<CbrApplication>();
                gwUsers.Get(j)->AddApplication(p_app);
                auto app = ApplicationContainer(p_app).Get(0);
                app->SetStartTime(startTime + (i + j * gwUsers.GetN() + 1) * startDelay);
                cbrContainer.Add(app);
            }
        }
    }

    sinkContainer.Start(startTime);
    sinkContainer.Stop(stopTime);

    m_last_custom_application.application = cbrContainer;
    m_last_custom_application.start = startTime;
    m_last_custom_application.stop = stopTime;
    m_last_custom_application.created = true;

    if (m_enableDefaultStatistics)
    {
        // Add throuhgput statistics
        if (direction == FWD_LINK)
        {
            // Global scalar
            m_satStatsHelperContainer->AddGlobalFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Global scatter
            m_satStatsHelperContainer->AddGlobalFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per UT scalar
            m_satStatsHelperContainer->AddPerUtFwdAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per UT scatter
            m_satStatsHelperContainer->AddPerUtFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per GW scalar
            m_satStatsHelperContainer->AddPerGwFwdAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per GW scatter
            m_satStatsHelperContainer->AddPerGwFwdAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwFwdFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwFwdUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
        }
        else if (direction == RTN_LINK)
        {
            // Global scalar
            m_satStatsHelperContainer->AddGlobalRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddGlobalRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Global scatter
            m_satStatsHelperContainer->AddGlobalRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddGlobalRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per UT scalar
            m_satStatsHelperContainer->AddPerUtRtnAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerUtRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per UT scatter
            m_satStatsHelperContainer->AddPerUtRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerUtRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);

            // Per GW scalar
            m_satStatsHelperContainer->AddPerGwRtnAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);
            m_satStatsHelperContainer->AddPerGwRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCALAR_FILE);

            // Per GW scatter
            m_satStatsHelperContainer->AddPerGwRtnAppThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwRtnFeederMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
            m_satStatsHelperContainer->AddPerGwRtnUserMacThroughput(
                SatStatsHelper::OUTPUT_SCATTER_FILE);
        }
    }
}

void
SatTrafficHelper::ChangeCustomTraffic(Time delay, std::string interval, uint32_t packetSize)
{
    NS_LOG_FUNCTION(this << delay << interval << packetSize);

    if (!m_last_custom_application.created)
    {
        NS_FATAL_ERROR("No custom traffic created when calling the method "
                       "SatTrafficHelper::ChangeCustomTraffic for the first time.");
    }
    if (m_last_custom_application.start + delay > m_last_custom_application.stop)
    {
        NS_FATAL_ERROR("Custom traffic updated after its stop time.");
    }
    for (auto i = m_last_custom_application.application.Begin();
         i != m_last_custom_application.application.End();
         ++i)
    {
        Ptr<CbrApplication> app = (dynamic_cast<CbrApplication*>(PeekPointer(*i)));
        Simulator::Schedule(m_last_custom_application.start + delay,
                            &SatTrafficHelper::UpdateAttribute,
                            this,
                            app,
                            interval,
                            packetSize);
    }
}

void
SatTrafficHelper::UpdateAttribute(Ptr<CbrApplication> application,
                                  std::string interval,
                                  uint32_t packetSize)
{
    NS_LOG_FUNCTION(this << application << interval << packetSize);

    application->SetInterval(Time(interval));
    application->SetPacketSize(packetSize);
}

bool
SatTrafficHelper::HasSinkInstalled(Ptr<Node> node, uint16_t port)
{
    NS_LOG_FUNCTION(this << node->GetId() << port);

    for (uint32_t i = 0; i < node->GetNApplications(); i++)
    {
        auto sink = DynamicCast<PacketSink>(node->GetApplication(i));
        if (sink != nullptr)
        {
            AddressValue av;
            sink->GetAttribute("Local", av);
            if (InetSocketAddress::ConvertFrom(av.Get()).GetPort() == port)
            {
                return true;
            }
        }
    }
    return false;
}

} // namespace ns3
