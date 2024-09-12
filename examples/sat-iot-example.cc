/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
 * \file sat-iot-example.cc
 * \ingroup satellite
 *
 * \brief This file allows to create an IoT scenario
 */

NS_LOG_COMPONENT_DEFINE("sat-iot-example");

int
main(int argc, char* argv[])
{
    // Variables
    std::string beam = "8";
    uint32_t nbGw = 1;
    uint32_t nbUtsPerBeam = 1;
    uint32_t nbEndUsersPerUt = 1;

    Time appStartTime = Seconds(0.001);
    Time simLength = Seconds(60.0);

    uint32_t queueSize = 50;
    double maxPowerTerminalW = 0.3;

    double rtnFeederLinkBaseFrequency = 1.77e+10;
    double rtnUserLinkBaseFrequency = 2.95e+10;
    double rtnFeederLinkBandwidth = 4.6848e+6;
    double frame0_AllocatedBandwidthHz = 2.928e+05;
    double frame0_CarrierAllocatedBandwidthHz = 2.928e+05;
    double frame0_CarrierRollOff = 0.22;
    double frame0_CarrierSpacing = 0;

    std::string modcodsUsed =
        "QPSK_1_TO_2 QPSK_3_TO_5 QPSK_2_TO_3 QPSK_3_TO_4 QPSK_4_TO_5 QPSK_5_TO_6 QPSK_8_TO_9 "
        "QPSK_9_TO_10 "
        "8PSK_3_TO_5 8PSK_2_TO_3 8PSK_3_TO_4 8PSK_5_TO_6 8PSK_8_TO_9 8PSK_9_TO_10 "
        "16APSK_2_TO_3 16APSK_3_TO_4 16APSK_4_TO_5 16APSK_5_TO_6 16APSK_8_TO_9 16APSK_9_TO_10 "
        "32APSK_3_TO_4 32APSK_4_TO_5 32APSK_5_TO_6 32APSK_8_TO_9";

    Ptr<SimulationHelper> simulationHelper = CreateObject<SimulationHelper>("sat-iot-example");

    // Read command line parameters given by user
    CommandLine cmd;
    cmd.AddValue("Beam", "Id of beam used (cannot use multiple beams)", beam);
    cmd.AddValue("NbGw", "Number of GWs", nbGw);
    cmd.AddValue("NbUtsPerBeam", "Number of UTs per spot-beam", nbUtsPerBeam);
    cmd.AddValue("NbEndUsersPerUt", "Number of end users per UT", nbEndUsersPerUt);
    cmd.AddValue("QueueSize", "Satellite queue sizes in packets", queueSize);
    cmd.AddValue("AppStartTime", "Applications start time (in seconds, or add unit)", appStartTime);
    cmd.AddValue("SimLength", "Simulation length (in seconds, or add unit)", simLength);
    cmd.AddValue("MaxPowerTerminalW", "Maximum power of terminals in W", maxPowerTerminalW);
    cmd.AddValue("RtnFeederLinkBaseFrequency",
                 "Base frequency of the return feeder link band",
                 rtnFeederLinkBaseFrequency);
    cmd.AddValue("RtnUserLinkBaseFrequency",
                 "Base frequency of the return user link band",
                 rtnUserLinkBaseFrequency);
    cmd.AddValue("RtnFeederLinkBandwidth",
                 "Bandwidth of the return feeder link band",
                 rtnFeederLinkBandwidth);
    cmd.AddValue("Frame0_AllocatedBandwidthHz",
                 "The allocated bandwidth [Hz] for frame",
                 frame0_AllocatedBandwidthHz);
    cmd.AddValue("Frame0_CarrierAllocatedBandwidthHz",
                 "The allocated carrier bandwidth [Hz] for frame",
                 frame0_CarrierAllocatedBandwidthHz);
    cmd.AddValue("Frame0_CarrierRollOff", "The roll-off factor for frame", frame0_CarrierRollOff);
    cmd.AddValue("Frame0_CarrierSpacing",
                 "The carrier spacing factor for frame",
                 frame0_CarrierSpacing);
    simulationHelper->AddDefaultUiArguments(cmd);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::SatEnvVariables::EnableSimulationOutputOverwrite", BooleanValue(true));
    Config::SetDefault("ns3::SatHelper::PacketTraceEnabled", BooleanValue(true));

    /*
     * FWD link
     */
    // Set defaults
    Config::SetDefault("ns3::SatConf::FwdUserLinkBandwidth", DoubleValue(2e+08));
    Config::SetDefault("ns3::SatConf::FwdFeederLinkBandwidth", DoubleValue(8e+08));
    Config::SetDefault("ns3::SatConf::FwdCarrierAllocatedBandwidth", DoubleValue(50e+06));
    Config::SetDefault("ns3::SatConf::FwdCarrierRollOff", DoubleValue(0.05));

    // ModCods selection
    Config::SetDefault("ns3::SatBeamHelper::DvbVersion", StringValue("DVB_S2"));
    Config::SetDefault("ns3::SatBbFrameConf::ModCodsUsed", StringValue(modcodsUsed));
    Config::SetDefault("ns3::SatBbFrameConf::DefaultModCod", StringValue("QPSK_1_TO_2"));

    // Queue size
    Config::SetDefault("ns3::SatQueue::MaxPackets", UintegerValue(queueSize));

    // Power limitation
    Config::SetDefault("ns3::SatUtPhy::TxMaxPowerDbw",
                       DoubleValue(SatUtils::LinearToDb(maxPowerTerminalW)));

    /*
     * RTN link
     */
    // Default plan
    Config::SetDefault("ns3::SatSuperframeConf0::FrameCount", UintegerValue(1));
    Config::SetDefault("ns3::SatConf::SuperFrameConfForSeq0",
                       EnumValue(SatSuperframeConf::SUPER_FRAME_CONFIG_0));
    Config::SetDefault("ns3::SatSuperframeConf0::FrameConfigType",
                       EnumValue(SatSuperframeConf::CONFIG_TYPE_0));

    Config::SetDefault("ns3::SatConf::RtnFeederLinkBaseFrequency",
                       DoubleValue(rtnFeederLinkBaseFrequency)); // Default value
    Config::SetDefault("ns3::SatConf::RtnUserLinkBaseFrequency",
                       DoubleValue(rtnUserLinkBaseFrequency)); // Default value
    Config::SetDefault("ns3::SatConf::RtnFeederLinkBandwidth", DoubleValue(rtnFeederLinkBandwidth));
    Config::SetDefault("ns3::SatConf::RtnUserLinkBandwidth",
                       DoubleValue(rtnFeederLinkBandwidth / 4));

    Config::SetDefault("ns3::SatSuperframeConf0::Frame0_AllocatedBandwidthHz",
                       DoubleValue(frame0_AllocatedBandwidthHz));
    Config::SetDefault("ns3::SatSuperframeConf0::Frame0_CarrierAllocatedBandwidthHz",
                       DoubleValue(frame0_CarrierAllocatedBandwidthHz));
    Config::SetDefault("ns3::SatSuperframeConf0::Frame0_CarrierRollOff",
                       DoubleValue(frame0_CarrierRollOff));
    Config::SetDefault("ns3::SatSuperframeConf0::Frame0_CarrierSpacing",
                       DoubleValue(frame0_CarrierSpacing));

    /*
     * Traffics
     */
    simulationHelper->SetSimulationTime(simLength);

    simulationHelper->SetGwUserCount(nbGw);
    simulationHelper->SetUtCountPerBeam(nbUtsPerBeam);
    simulationHelper->SetUserCountPerUt(nbEndUsersPerUt);
    simulationHelper->SetBeams(beam);

    simulationHelper->LoadScenario("geo-33E");

    simulationHelper->CreateSatScenario();

    Ptr<SatHelper> satHelper = simulationHelper->GetSatelliteHelper();
    Ptr<SatTrafficHelper> trafficHelper = simulationHelper->GetTrafficHelper();
    trafficHelper->AddPoissonTraffic(SatTrafficHelper::RTN_LINK,
                                     Seconds(1),
                                     Seconds(0.1),
                                     "200kb/s",
                                     300,
                                     satHelper->GetGwUsers(),
                                     satHelper->GetUtUsers(),
                                     appStartTime,
                                     simLength,
                                     Seconds(0.001)); // 200kb/s == 100kBaud
    trafficHelper->AddCbrTraffic(SatTrafficHelper::RTN_LINK,
                                 "8.5ms",
                                 300,
                                 satHelper->GetGwUsers(),
                                 satHelper->GetUtUsers(),
                                 appStartTime,
                                 simLength,
                                 Seconds(0.001)); // 280kb/s == 140kBaud

    // Link results
    // Uncomment to use custom C/N0 traces or constants for some links
    /*
    Ptr<SatCnoHelper> satCnoHelper = simulationHelper->GetCnoHelper ();
    satCnoHelper->UseTracesForDefault (false);
    for (uint32_t i = 0; i < Singleton<SatTopology>::Get()->GetUtNodes().GetN (); i++)
      {
        satCnoHelper->SetUtNodeCnoFile (Singleton<SatTopology>::Get()->GetUtNode(i),
    SatEnums::FORWARD_USER_CH, "path_to_cno_file"); // For input trace file
        // or
        satCnoHelper->SetGwNodeCno (Singleton<SatTopology>::Get()->GetUtNode(i),
    SatEnums::FORWARD_USER_CH, 1e10); // For constant value
      }
    */

    /*
     * Outputs
     * Note: some outputs are automatically generated by traffic helper
     */
    simulationHelper->EnableProgressLogs();

    Config::SetDefault("ns3::ConfigStore::Filename", StringValue("output-attributes.xml"));
    Config::SetDefault("ns3::ConfigStore::FileFormat", StringValue("Xml"));
    Config::SetDefault("ns3::ConfigStore::Mode", StringValue("Save"));
    ConfigStore outputConfig;
    outputConfig.ConfigureDefaults();

    Ptr<SatStatsHelperContainer> s = simulationHelper->GetStatisticsContainer();

    // Link SINR
    s->AddGlobalFwdFeederLinkSinr(SatStatsHelper::OUTPUT_SCATTER_FILE);
    s->AddGlobalFwdUserLinkSinr(SatStatsHelper::OUTPUT_SCATTER_FILE);
    s->AddGlobalRtnFeederLinkSinr(SatStatsHelper::OUTPUT_SCATTER_FILE);
    s->AddGlobalRtnUserLinkSinr(SatStatsHelper::OUTPUT_SCATTER_FILE);

    s->AddGlobalFwdFeederLinkSinr(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddGlobalFwdUserLinkSinr(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddGlobalRtnFeederLinkSinr(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddGlobalRtnUserLinkSinr(SatStatsHelper::OUTPUT_SCALAR_FILE);

    // SINR
    s->AddGlobalFwdCompositeSinr(SatStatsHelper::OUTPUT_CDF_FILE);
    s->AddGlobalFwdCompositeSinr(SatStatsHelper::OUTPUT_SCATTER_FILE);
    s->AddPerUtFwdCompositeSinr(SatStatsHelper::OUTPUT_CDF_FILE);
    s->AddPerUtFwdCompositeSinr(SatStatsHelper::OUTPUT_SCATTER_FILE);
    s->AddPerUtFwdCompositeSinr(SatStatsHelper::OUTPUT_CDF_PLOT);
    s->AddGlobalRtnCompositeSinr(SatStatsHelper::OUTPUT_CDF_FILE);
    s->AddGlobalRtnCompositeSinr(SatStatsHelper::OUTPUT_SCATTER_FILE);
    s->AddPerBeamRtnCompositeSinr(SatStatsHelper::OUTPUT_CDF_FILE);
    s->AddPerBeamRtnCompositeSinr(SatStatsHelper::OUTPUT_CDF_PLOT);
    s->AddPerUtRtnCompositeSinr(SatStatsHelper::OUTPUT_CDF_FILE);
    s->AddPerUtRtnCompositeSinr(SatStatsHelper::OUTPUT_SCATTER_FILE);
    s->AddPerUtRtnCompositeSinr(SatStatsHelper::OUTPUT_CDF_PLOT);

    // Link RX Power
    s->AddGlobalFwdFeederLinkRxPower(SatStatsHelper::OUTPUT_SCATTER_FILE);
    s->AddGlobalFwdUserLinkRxPower(SatStatsHelper::OUTPUT_SCATTER_FILE);
    s->AddGlobalRtnFeederLinkRxPower(SatStatsHelper::OUTPUT_SCATTER_FILE);
    s->AddGlobalRtnUserLinkRxPower(SatStatsHelper::OUTPUT_SCATTER_FILE);

    s->AddGlobalFwdFeederLinkRxPower(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddGlobalFwdUserLinkRxPower(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddGlobalRtnFeederLinkRxPower(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddGlobalRtnUserLinkRxPower(SatStatsHelper::OUTPUT_SCALAR_FILE);

    // Return link load
    s->AddGlobalFrameUserLoad(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddPerGwFrameUserLoad(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddPerBeamFrameUserLoad(SatStatsHelper::OUTPUT_SCALAR_FILE);

    s->AddGlobalFrameUserLoad(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddPerGwFrameUserLoad(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddPerBeamFrameUserLoad(SatStatsHelper::OUTPUT_SCALAR_FILE);

    // Frame type usage
    s->AddGlobalFrameTypeUsage(SatStatsHelper::OUTPUT_SCALAR_FILE);

    simulationHelper->RunSimulation();

    return 0;

} // end of `int main (int argc, char *argv[])`
