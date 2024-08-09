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
 * \file sat-lora-example.cc
 * \ingroup satellite
 *
 * \brief This file allows to create a scenario with Lora configuration
 */

NS_LOG_COMPONENT_DEFINE("sat-lora-example");

int
main(int argc, char* argv[])
{
    // Variables
    std::string beams = "3 4 5 6";
    // std::string beams = "8";
    uint32_t nbGwUser = 1;
    uint32_t nbUtsPerBeam = 100;
    uint32_t nbEndUsersPerUt = 1;

    Time appStartTime = Seconds(0.001);
    Time simLength = Seconds(15.0);

    uint32_t packetSize = 24;
    Time loraInterval = Seconds(10);
    std::string interval = "10s";

    double frameAllocatedBandwidthHz = 15000;
    double frameCarrierAllocatedBandwidthHz = 15000;
    double frameCarrierRollOff = 0.22;
    double frameCarrierSpacing = 0;
    uint32_t frameSpreadingFactor = 256;

    bool interferenceModePerPacket = true;
    bool displayTraces = true;

    Time firstWindowDelay = MilliSeconds(1500);
    Time secondWindowDelay = Seconds(2);
    Time firstWindowDuration = MilliSeconds(400);
    Time secondWindowDuration = MilliSeconds(400);
    Time firstWindowAnswerDelay = Seconds(1);
    Time secondWindowAnswerDelay = Seconds(2);

    Ptr<SimulationHelper> simulationHelper = CreateObject<SimulationHelper>("example-lora");

    // read command line parameters given by user
    CommandLine cmd;
    cmd.AddValue("modelPP", "interferenceModePerPacket", interferenceModePerPacket);
    cmd.AddValue("traces", "displayTraces", displayTraces);
    cmd.AddValue("utsPerBeam", "Number of UTs per spot-beam", nbUtsPerBeam);
    cmd.AddValue("simLength", "Simulation duration in seconds", simLength);
    cmd.AddValue("packetSize", "Constant packet size in bytes", packetSize);
    cmd.AddValue("loraInterval",
                 "Interval between two transmissions for each UT in seconds",
                 loraInterval);
    cmd.AddValue("frameAllocatedBandwidthHz",
                 "Allocated bandwidth in Hz",
                 frameAllocatedBandwidthHz);
    cmd.AddValue("frameCarrierAllocatedBandwidthHz",
                 "Allocated carrier bandwidth in Hz",
                 frameCarrierAllocatedBandwidthHz);
    cmd.AddValue("frameCarrierRollOff", "Roll-off factor", frameCarrierRollOff);
    cmd.AddValue("frameCarrierSpacing", "Carrier spacing factor", frameCarrierSpacing);
    cmd.AddValue("frameSpreadingFactor", "Carrier spreading factor", frameSpreadingFactor);

    cmd.AddValue("firstWindowDelay",
                 "Delay between end of transmission and opening of first window on End Device",
                 firstWindowDelay);
    cmd.AddValue("secondWindowDelay",
                 "Delay between end of transmission and opening of second window on End Device",
                 secondWindowDelay);
    cmd.AddValue("firstWindowDuration", "First window duration on End Device", firstWindowDuration);
    cmd.AddValue("secondWindowDuration",
                 "Second window duration on End Device",
                 secondWindowDuration);
    cmd.AddValue("firstWindowAnswerDelay",
                 "Delay between end of reception and start of ack on first window on Gateway",
                 firstWindowAnswerDelay);
    cmd.AddValue("secondWindowAnswerDelay",
                 "Delay between end of reception and start of ack on second window on Gateway",
                 secondWindowAnswerDelay);
    simulationHelper->AddDefaultUiArguments(cmd);
    cmd.Parse(argc, argv);

    /// Set regeneration mode
    Config::SetDefault("ns3::SatConf::ForwardLinkRegenerationMode",
                       EnumValue(SatEnums::TRANSPARENT));
    Config::SetDefault("ns3::SatConf::ReturnLinkRegenerationMode",
                       EnumValue(SatEnums::TRANSPARENT));

    // Enable Lora
    Config::SetDefault("ns3::LorawanMacEndDevice::DataRate", UintegerValue(5));
    Config::SetDefault("ns3::LorawanMacEndDevice::MType",
                       EnumValue(LorawanMacHeader::CONFIRMED_DATA_UP));
    Config::SetDefault("ns3::SatLorawanNetDevice::ForwardToUtUsers", BooleanValue(true));

    // Config::SetDefault ("ns3::SatLoraConf::Standard", EnumValue (SatLoraConf::EU863_870));
    Config::SetDefault("ns3::SatLoraConf::Standard", EnumValue(SatLoraConf::SATELLITE));

    Config::SetDefault("ns3::LorawanMacEndDeviceClassA::FirstWindowDelay",
                       TimeValue(firstWindowDelay));
    Config::SetDefault("ns3::LorawanMacEndDeviceClassA::SecondWindowDelay",
                       TimeValue(secondWindowDelay));
    Config::SetDefault("ns3::LorawanMacEndDeviceClassA::FirstWindowDuration",
                       TimeValue(firstWindowDuration));
    Config::SetDefault("ns3::LorawanMacEndDeviceClassA::SecondWindowDuration",
                       TimeValue(secondWindowDuration));
    Config::SetDefault("ns3::LoraNetworkScheduler::FirstWindowAnswerDelay",
                       TimeValue(firstWindowAnswerDelay));
    Config::SetDefault("ns3::LoraNetworkScheduler::SecondWindowAnswerDelay",
                       TimeValue(secondWindowAnswerDelay));

    // Defaults
    Config::SetDefault("ns3::SatEnvVariables::EnableSimulationOutputOverwrite", BooleanValue(true));
    Config::SetDefault("ns3::SatHelper::PacketTraceEnabled", BooleanValue(true));

    // Superframe configuration
    Config::SetDefault("ns3::SatConf::SuperFrameConfForSeq0",
                       EnumValue(SatSuperframeConf::SUPER_FRAME_CONFIG_4));
    Config::SetDefault("ns3::SatSuperframeConf4::FrameConfigType",
                       EnumValue(SatSuperframeConf::CONFIG_TYPE_4));
    Config::SetDefault("ns3::SatSuperframeConf4::Frame0_AllocatedBandwidthHz",
                       DoubleValue(frameAllocatedBandwidthHz));
    Config::SetDefault("ns3::SatSuperframeConf4::Frame0_CarrierAllocatedBandwidthHz",
                       DoubleValue(frameCarrierAllocatedBandwidthHz));
    Config::SetDefault("ns3::SatSuperframeConf4::Frame0_CarrierRollOff",
                       DoubleValue(frameCarrierRollOff));
    Config::SetDefault("ns3::SatSuperframeConf4::Frame0_CarrierSpacing",
                       DoubleValue(frameCarrierSpacing));
    Config::SetDefault("ns3::SatSuperframeConf4::Frame0_SpreadingFactor",
                       UintegerValue(frameSpreadingFactor));

    // CRDSA only
    Config::SetDefault("ns3::SatLowerLayerServiceConf::DaServiceCount", UintegerValue(4));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService0_ConstantAssignmentProvided",
                       BooleanValue(false));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService0_RbdcAllowed",
                       BooleanValue(false));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService0_VolumeAllowed",
                       BooleanValue(false));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService1_ConstantAssignmentProvided",
                       BooleanValue(false));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService1_RbdcAllowed",
                       BooleanValue(false));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService1_VolumeAllowed",
                       BooleanValue(false));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService2_ConstantAssignmentProvided",
                       BooleanValue(false));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService2_RbdcAllowed",
                       BooleanValue(false));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService2_VolumeAllowed",
                       BooleanValue(false));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService3_ConstantAssignmentProvided",
                       BooleanValue(false));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService3_RbdcAllowed",
                       BooleanValue(false));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService3_VolumeAllowed",
                       BooleanValue(false));

    // Configure RA
    Config::SetDefault("ns3::SatOrbiterHelper::FwdLinkErrorModel",
                       EnumValue(SatPhyRxCarrierConf::EM_AVI));
    Config::SetDefault("ns3::SatOrbiterHelper::RtnLinkErrorModel",
                       EnumValue(SatPhyRxCarrierConf::EM_AVI));
    Config::SetDefault("ns3::SatBeamHelper::RandomAccessModel", EnumValue(SatEnums::RA_MODEL_ESSA));
    if (interferenceModePerPacket)
    {
        Config::SetDefault("ns3::SatBeamHelper::RaInterferenceModel",
                           EnumValue(SatPhyRxCarrierConf::IF_PER_PACKET));
    }
    else
    {
        Config::SetDefault("ns3::SatBeamHelper::RaInterferenceModel",
                           EnumValue(SatPhyRxCarrierConf::IF_PER_FRAGMENT));
    }
    Config::SetDefault("ns3::SatBeamHelper::RaInterferenceEliminationModel",
                       EnumValue(SatPhyRxCarrierConf::SIC_RESIDUAL));
    Config::SetDefault("ns3::SatBeamHelper::RaCollisionModel",
                       EnumValue(SatPhyRxCarrierConf::RA_COLLISION_CHECK_AGAINST_SINR));
    Config::SetDefault("ns3::SatBeamHelper::ReturnLinkLinkResults", EnumValue(SatEnums::LR_LORA));

    Config::SetDefault("ns3::SatPhyRxCarrierPerWindow::WindowDuration", StringValue("600ms"));
    Config::SetDefault("ns3::SatPhyRxCarrierPerWindow::WindowStep", StringValue("200ms"));
    Config::SetDefault("ns3::SatPhyRxCarrierPerWindow::WindowDelay", StringValue("0s"));
    Config::SetDefault("ns3::SatPhyRxCarrierPerWindow::FirstWindow", StringValue("0s"));
    Config::SetDefault("ns3::SatPhyRxCarrierPerWindow::WindowSICIterations", UintegerValue(5));
    Config::SetDefault("ns3::SatPhyRxCarrierPerWindow::SpreadingFactor", UintegerValue(1));
    Config::SetDefault("ns3::SatPhyRxCarrierPerWindow::DetectionThreshold", DoubleValue(0));
    Config::SetDefault("ns3::SatPhyRxCarrierPerWindow::EnableSIC", BooleanValue(false));

    // Set random access parameters
    Config::SetDefault("ns3::SatLowerLayerServiceConf::RaService0_MaximumUniquePayloadPerBlock",
                       UintegerValue(3));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::RaService0_MaximumConsecutiveBlockAccessed",
                       UintegerValue(6));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::RaService0_MinimumIdleBlock",
                       UintegerValue(2));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::RaService0_BackOffTimeInMilliSeconds",
                       UintegerValue(50));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::RaService0_BackOffProbability",
                       UintegerValue(1));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::RaService0_HighLoadBackOffProbability",
                       UintegerValue(1));
    Config::SetDefault(
        "ns3::SatLowerLayerServiceConf::RaService0_AverageNormalizedOfferedLoadThreshold",
        DoubleValue(0.99));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::RaService0_NumberOfInstances",
                       UintegerValue(3));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::RaService0_SlottedAlohaAllowed",
                       BooleanValue(false));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::RaService0_CrdsaAllowed",
                       BooleanValue(false));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::RaService0_EssaAllowed", BooleanValue(true));

    // Traffics
    simulationHelper->SetSimulationTime(simLength);

    simulationHelper->SetGwUserCount(nbGwUser);
    simulationHelper->SetUtCountPerBeam(nbUtsPerBeam);
    simulationHelper->SetUserCountPerUt(nbEndUsersPerUt);
    simulationHelper->SetBeams(beams);

    simulationHelper->LoadScenario("geo-33E-lora");

    simulationHelper->CreateSatScenario();

    Config::SetDefault("ns3::CbrApplication::Interval", StringValue(interval));
    Config::SetDefault("ns3::CbrApplication::PacketSize", UintegerValue(packetSize));

    simulationHelper->InstallLoraTrafficModel(SimulationHelper::LORA_CBR,
                                              loraInterval,
                                              packetSize,
                                              appStartTime,
                                              simLength,
                                              Seconds(1));

    /*simulationHelper->InstallLoraTrafficModel (
      SimulationHelper::PERIODIC,
      loraInterval,
      packetSize,
      appStartTime, simLength, Seconds (1));*/

    // Outputs
    simulationHelper->EnableProgressLogs();

    std::string outputPath = Singleton<SatEnvVariables>::Get()->LocateDirectory(
        "contrib/satellite/data/sims/example-lora");
    Config::SetDefault("ns3::ConfigStore::Filename",
                       StringValue(outputPath + "/output-attributes.xml"));
    Config::SetDefault("ns3::ConfigStore::FileFormat", StringValue("Xml"));
    Config::SetDefault("ns3::ConfigStore::Mode", StringValue("Save"));
    ConfigStore outputConfig;
    outputConfig.ConfigureDefaults();

    if (displayTraces)
    {
        Ptr<SatStatsHelperContainer> s = simulationHelper->GetStatisticsContainer();

        s->AddGlobalFeederEssaPacketError(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddGlobalFeederEssaPacketError(SatStatsHelper::OUTPUT_SCATTER_FILE);
        s->AddPerUtFeederEssaPacketError(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddPerUtFeederEssaPacketError(SatStatsHelper::OUTPUT_SCATTER_FILE);

        s->AddGlobalFeederEssaPacketCollision(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddGlobalFeederEssaPacketCollision(SatStatsHelper::OUTPUT_SCATTER_FILE);
        s->AddPerUtFeederEssaPacketCollision(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddPerUtFeederEssaPacketCollision(SatStatsHelper::OUTPUT_SCATTER_FILE);

        s->AddGlobalRtnFeederWindowLoad(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddGlobalRtnFeederWindowLoad(SatStatsHelper::OUTPUT_SCATTER_FILE);
        s->AddPerBeamRtnFeederWindowLoad(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddPerBeamRtnFeederWindowLoad(SatStatsHelper::OUTPUT_SCATTER_FILE);

        s->AddGlobalRtnAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddGlobalRtnFeederMacThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddGlobalRtnAppThroughput(SatStatsHelper::OUTPUT_SCATTER_FILE);
        s->AddGlobalRtnFeederMacThroughput(SatStatsHelper::OUTPUT_SCATTER_FILE);

        s->AddPerUtRtnAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddPerUtRtnFeederMacThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddPerUtRtnAppThroughput(SatStatsHelper::OUTPUT_SCATTER_FILE);
        s->AddPerUtRtnFeederMacThroughput(SatStatsHelper::OUTPUT_SCATTER_FILE);

        s->AddPerUtRtnAppDelay(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddPerUtRtnMacDelay(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddPerUtRtnAppDelay(SatStatsHelper::OUTPUT_SCATTER_FILE);
        s->AddPerUtRtnMacDelay(SatStatsHelper::OUTPUT_SCATTER_FILE);

        s->AddGlobalRtnCompositeSinr(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddGlobalRtnCompositeSinr(SatStatsHelper::OUTPUT_SCATTER_FILE);
        s->AddPerUtRtnCompositeSinr(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddPerUtRtnCompositeSinr(SatStatsHelper::OUTPUT_SCATTER_FILE);

        s->AddGlobalRtnFeederLinkSinr(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddGlobalRtnFeederLinkSinr(SatStatsHelper::OUTPUT_SCATTER_FILE);
        s->AddGlobalRtnUserLinkSinr(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddGlobalRtnUserLinkSinr(SatStatsHelper::OUTPUT_SCATTER_FILE);

        s->AddGlobalRtnFeederLinkRxPower(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddGlobalRtnFeederLinkRxPower(SatStatsHelper::OUTPUT_SCATTER_FILE);
        s->AddGlobalRtnUserLinkRxPower(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddGlobalRtnUserLinkRxPower(SatStatsHelper::OUTPUT_SCATTER_FILE);

        s->AddPerUtFwdAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddPerUtFwdUserMacThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddPerUtFwdAppThroughput(SatStatsHelper::OUTPUT_SCATTER_FILE);
        s->AddPerUtFwdUserMacThroughput(SatStatsHelper::OUTPUT_SCATTER_FILE);

        s->AddGlobalFwdAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddGlobalFwdUserMacThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddGlobalFwdAppThroughput(SatStatsHelper::OUTPUT_SCATTER_FILE);
        s->AddGlobalFwdUserMacThroughput(SatStatsHelper::OUTPUT_SCATTER_FILE);

        s->AddPerUtFwdAppDelay(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddPerUtFwdMacDelay(SatStatsHelper::OUTPUT_SCALAR_FILE);
        s->AddPerUtFwdAppDelay(SatStatsHelper::OUTPUT_SCATTER_FILE);
        s->AddPerUtFwdMacDelay(SatStatsHelper::OUTPUT_SCATTER_FILE);
    }

    simulationHelper->RunSimulation();

    return 0;

} // end of `int main (int argc, char *argv[])`
