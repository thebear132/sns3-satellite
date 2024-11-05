/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Magister Solutions
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
 *
 */

#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/satellite-module.h"
#include "ns3/traffic-module.h"

using namespace ns3;

/**
 * \file sat-mobility-example.cc
 * \ingroup satellite
 *
 * \brief Simulation script to run example simulation with a moving satellite.
 *
 * execute command -> ./waf --run "sat-mobility-example --PrintHelp"
 */

NS_LOG_COMPONENT_DEFINE("sat-mobility-example");

int
main(int argc, char* argv[])
{
    // Enable info logs
    LogComponentEnable("sat-mobility-example", LOG_LEVEL_INFO);

    // Variables
    uint32_t beamId = 17;
    uint32_t endUsersPerUt(1);
    uint32_t utsPerBeam(1);

    uint32_t packetSize(100);
    Time interval(Seconds(10.0));

    bool updatePositionEachRequest(false);
    Time updatePositionPeriod(Seconds(1));

    Time appStartTime = Seconds(0.1);
    Time simLength(Seconds(7200.0));

    Ptr<SimulationHelper> simulationHelper = CreateObject<SimulationHelper>("sat-mobility-example");

    // Parse command-line
    CommandLine cmd;
    cmd.AddValue("PacketSize", "UDP packet size (in bytes)", packetSize);
    cmd.AddValue("Interval", "CBR interval (in seconds, or add unit)", interval);
    cmd.AddValue("SimLength", "Simulation length (in seconds, or add unit)", simLength);
    cmd.AddValue("UpdatePositionEachRequest",
                 "Enable position computation each time a packet is sent",
                 updatePositionEachRequest);
    cmd.AddValue("UpdatePositionPeriod",
                 "Period of satellite position refresh, if not update on each request (in seconds, "
                 "or add unit)",
                 updatePositionPeriod);
    simulationHelper->AddDefaultUiArguments(cmd);
    cmd.Parse(argc, argv);

    /// Set default values
    Config::SetDefault("ns3::SatEnvVariables::EnableSimulationOutputOverwrite", BooleanValue(true));
    Config::SetDefault("ns3::SatHelper::PacketTraceEnabled", BooleanValue(true));

    Config::SetDefault("ns3::SatSGP4MobilityModel::UpdatePositionEachRequest",
                       BooleanValue(updatePositionEachRequest));
    Config::SetDefault("ns3::SatSGP4MobilityModel::UpdatePositionPeriod",
                       TimeValue(updatePositionPeriod));

    simulationHelper->SetSimulationTime(simLength);
    simulationHelper->SetUserCountPerUt(endUsersPerUt);
    simulationHelper->SetUtCountPerBeam(utsPerBeam);

    // Set beam ID
    std::stringstream beamsEnabled;
    beamsEnabled << beamId;
    simulationHelper->SetBeams(beamsEnabled.str());

    simulationHelper->LoadScenario("leo-iss");

    // Create reference system
    simulationHelper->CreateSatScenario();

    // setup CBR traffic
    simulationHelper->GetTrafficHelper()->AddCbrTraffic(
        SatTrafficHelper::FWD_LINK,
        SatTrafficHelper::UDP,
        interval,
        packetSize,
        NodeContainer(Singleton<SatTopology>::Get()->GetGwUserNode(0)),
        Singleton<SatTopology>::Get()->GetUtUserNodes(),
        appStartTime,
        simLength,
        MilliSeconds(50));

    simulationHelper->GetTrafficHelper()->AddCbrTraffic(
        SatTrafficHelper::RTN_LINK,
        SatTrafficHelper::UDP,
        interval,
        packetSize,
        NodeContainer(Singleton<SatTopology>::Get()->GetGwUserNode(0)),
        Singleton<SatTopology>::Get()->GetUtUserNodes(),
        appStartTime,
        simLength,
        MilliSeconds(50));

    NS_LOG_INFO("--- sat-mobility-example ---");
    NS_LOG_INFO("  Packet size in bytes: " << packetSize);
    NS_LOG_INFO("  Packet sending interval: " << interval.GetSeconds());
    NS_LOG_INFO("  Simulation length: " << simLength.GetSeconds());
    NS_LOG_INFO("  Number of UTs: " << utsPerBeam);
    NS_LOG_INFO("  Number of end users per UT: " << endUsersPerUt);
    NS_LOG_INFO("  ");

    // Set statistics
    Ptr<SatStatsHelperContainer> s = simulationHelper->GetStatisticsContainer();
    simulationHelper->EnableProgressLogs();

    Config::SetDefault("ns3::ConfigStore::Filename", StringValue("output-attributes.xml"));
    Config::SetDefault("ns3::ConfigStore::FileFormat", StringValue("Xml"));
    Config::SetDefault("ns3::ConfigStore::Mode", StringValue("Save"));
    ConfigStore outputConfig;
    outputConfig.ConfigureDefaults();

    s->AddGlobalFwdPhyDelay(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddGlobalFwdPhyDelay(SatStatsHelper::OUTPUT_SCATTER_FILE);
    s->AddGlobalRtnPhyDelay(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddGlobalRtnPhyDelay(SatStatsHelper::OUTPUT_SCATTER_FILE);

    s->AddGlobalFwdMacDelay(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddGlobalFwdMacDelay(SatStatsHelper::OUTPUT_SCATTER_FILE);
    s->AddGlobalRtnMacDelay(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddGlobalRtnMacDelay(SatStatsHelper::OUTPUT_SCATTER_FILE);

    s->AddGlobalFwdAppDelay(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddGlobalFwdAppDelay(SatStatsHelper::OUTPUT_SCATTER_FILE);
    s->AddGlobalRtnAppDelay(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddGlobalRtnAppDelay(SatStatsHelper::OUTPUT_SCATTER_FILE);

    simulationHelper->RunSimulation();

    return 0;
}
