/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Magister Solutions Ltd
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
 * Author: Frans Laakso <frans.laakso@magister.fi>
 */

#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/satellite-module.h"

using namespace ns3;

/**
 * \file sat-ra-sim-tn9.cc
 * \ingroup satellite
 *
 * \brief Simulation script to run example simulation results related to satellite RTN
 * link performance. Currently only one beam is simulated with variable amount of users
 * and RA-DAMA configuration. The script supports three different setups: SA + VBDC,
 * CRDSA + VBDC and CRDSA only. As output, the example provides statistics about RA
 * collision and error rate, throughput, packet delay, SINR, resources granted, frame
 * load and waveform usage. The random access results for TN9 were obtained by using
 * this script.
 *
 * execute command -> ./waf --run "sat-ra-sim-tn9 --PrintHelp"
 */

NS_LOG_COMPONENT_DEFINE("sat-ra-sim-tn9");

int
main(int argc, char* argv[])
{
    uint32_t beamId = 1;
    uint32_t endUsersPerUt(1);
    uint32_t raMode(3);
    uint32_t utsPerBeam(1);
    uint32_t packetSize(64);
    std::string dataRate = "5kb/s";
    std::string onTime = "0.2";
    std::string offTime = "0.8";

    double simLength(300.0); // in seconds

    /// Set simulation output details
    auto simulationHelper = CreateObject<SimulationHelper>("example-ra-sim-tn9");

    // To read attributes from file
    std::string inputFileNameWithPath =
        Singleton<SatEnvVariables>::Get()->LocateDirectory("contrib/satellite/examples") +
        "/tn9-ra-input-attributes.xml";

    // read command line parameters given by user
    CommandLine cmd;
    cmd.AddValue("utsPerBeam", "Number of UTs per spot-beam", utsPerBeam);
    cmd.AddValue("raMode", "RA mode", raMode);
    cmd.AddValue("simLength", "Simulation duration in seconds", simLength);
    cmd.AddValue("packetSize", "Constant packet size in bytes", packetSize);
    cmd.AddValue("dataRate", "Data rate (e.g. 500kb/s)", dataRate);
    cmd.AddValue("onTime", "Time for packet sending is on in seconds", onTime);
    cmd.AddValue("offTime", "Time for packet sending is off in seconds", offTime);
    simulationHelper->AddDefaultUiArguments(cmd, inputFileNameWithPath);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::ConfigStore::Filename", StringValue(inputFileNameWithPath));
    Config::SetDefault("ns3::ConfigStore::Mode", StringValue("Load"));
    Config::SetDefault("ns3::ConfigStore::FileFormat", StringValue("Xml"));
    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    // Enable Random Access with all available modules
    Config::SetDefault("ns3::SatBeamHelper::RandomAccessModel",
                       EnumValue(SatEnums::RA_MODEL_RCS2_SPECIFICATION));

    // Set Random Access interference model
    Config::SetDefault("ns3::SatBeamHelper::RaInterferenceModel",
                       EnumValue(SatPhyRxCarrierConf::IF_PER_PACKET));

    // Set Random Access collision model
    Config::SetDefault("ns3::SatBeamHelper::RaCollisionModel",
                       EnumValue(SatPhyRxCarrierConf::RA_COLLISION_CHECK_AGAINST_SINR));

    // Set dynamic load control parameters
    Config::SetDefault("ns3::SatPhyRxCarrierConf::EnableRandomAccessDynamicLoadControl",
                       BooleanValue(false));
    Config::SetDefault(
        "ns3::SatPhyRxCarrierConf::RandomAccessAverageNormalizedOfferedLoadMeasurementWindowSize",
        UintegerValue(10));

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

    switch (raMode)
    {
    // CRDSA + VBDC
    case 0: {
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService0_ConstantAssignmentProvided",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService1_ConstantAssignmentProvided",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService2_ConstantAssignmentProvided",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService3_ConstantAssignmentProvided",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService0_RbdcAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService1_RbdcAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService2_RbdcAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService3_RbdcAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService0_VolumeAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService1_VolumeAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService2_VolumeAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService3_VolumeAllowed",
                           BooleanValue(true));

        // Disable periodic control slots
        Config::SetDefault("ns3::SatBeamScheduler::ControlSlotsEnabled", BooleanValue(false));
        break;
    }
    // SA + VBDC
    case 1: {
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService0_ConstantAssignmentProvided",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService1_ConstantAssignmentProvided",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService2_ConstantAssignmentProvided",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService3_ConstantAssignmentProvided",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService0_RbdcAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService1_RbdcAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService2_RbdcAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService3_RbdcAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService0_VolumeAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService1_VolumeAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService2_VolumeAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService3_VolumeAllowed",
                           BooleanValue(true));

        Config::SetDefault("ns3::SatLowerLayerServiceConf::RaService0_NumberOfInstances",
                           UintegerValue(1));

        // Disable periodic control slots
        Config::SetDefault("ns3::SatBeamScheduler::ControlSlotsEnabled", BooleanValue(false));
        break;
    }
    // Periodic control slots + VBDC
    case 2: {
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService0_ConstantAssignmentProvided",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService1_ConstantAssignmentProvided",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService2_ConstantAssignmentProvided",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService3_ConstantAssignmentProvided",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService0_RbdcAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService1_RbdcAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService2_RbdcAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService3_RbdcAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService0_VolumeAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService1_VolumeAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService2_VolumeAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService3_VolumeAllowed",
                           BooleanValue(true));

        Config::SetDefault("ns3::SatBeamHelper::RandomAccessModel",
                           EnumValue(SatEnums::RA_MODEL_OFF));

        Config::SetDefault("ns3::SatBeamScheduler::ControlSlotsEnabled", BooleanValue(true));
        break;
    }
    // CRDSA only
    case 3: {
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService0_ConstantAssignmentProvided",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService1_ConstantAssignmentProvided",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService2_ConstantAssignmentProvided",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService3_ConstantAssignmentProvided",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService0_RbdcAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService1_RbdcAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService2_RbdcAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService3_RbdcAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService0_VolumeAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService1_VolumeAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService2_VolumeAllowed",
                           BooleanValue(false));
        Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService3_VolumeAllowed",
                           BooleanValue(false));

        // Disable periodic control slots
        Config::SetDefault("ns3::SatBeamScheduler::ControlSlotsEnabled", BooleanValue(false));
        break;
    }
    default: {
        NS_FATAL_ERROR("Unsupported raMode: " << raMode);
        break;
    }
    }

    // Creating the reference system.
    simulationHelper->SetSimulationTime(simLength);
    simulationHelper->SetUserCountPerUt(endUsersPerUt);
    simulationHelper->SetUtCountPerBeam(utsPerBeam);
    simulationHelper->SetBeamSet({beamId});

    simulationHelper->LoadScenario("geo-33E");

    simulationHelper->CreateSatScenario();

    /**
     * Set-up On-Off traffic
     */
    simulationHelper->GetTrafficHelper()->AddOnOffTraffic(
        SatTrafficHelper::RTN_LINK,
        SatTrafficHelper::UDP,
        dataRate,
        packetSize,
        Singleton<SatTopology>::Get()->GetGwUserNodes(),
        Singleton<SatTopology>::Get()->GetUtUserNodes(),
        "ns3::ConstantRandomVariable[Constant=" + onTime + "]",
        "ns3::ConstantRandomVariable[Constant=" + offTime + "]",
        Seconds(0),
        Seconds(simLength - 2.0),
        Seconds(0));

    /**
     * Set-up statistics
     */
    Ptr<SatStatsHelperContainer> s = simulationHelper->GetStatisticsContainer();

    s->AddPerBeamRtnAppThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddPerBeamRtnFeederDevThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddPerBeamRtnFeederMacThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddPerBeamRtnFeederPhyThroughput(SatStatsHelper::OUTPUT_SCALAR_FILE);

    s->AddAverageUtUserRtnAppThroughput(SatStatsHelper::OUTPUT_CDF_FILE);
    s->AddAverageUtUserRtnAppThroughput(SatStatsHelper::OUTPUT_CDF_PLOT);

    s->AddPerBeamRtnAppDelay(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddPerBeamRtnDevDelay(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddPerBeamRtnPhyDelay(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddPerBeamRtnMacDelay(SatStatsHelper::OUTPUT_SCALAR_FILE);

    s->AddPerBeamRtnAppDelay(SatStatsHelper::OUTPUT_CDF_FILE);
    s->AddPerBeamRtnDevDelay(SatStatsHelper::OUTPUT_CDF_FILE);
    s->AddPerBeamRtnPhyDelay(SatStatsHelper::OUTPUT_CDF_FILE);
    s->AddPerBeamRtnMacDelay(SatStatsHelper::OUTPUT_CDF_FILE);

    // s->AddPerUtUserRtnAppThroughput (SatStatsHelper::OUTPUT_SCALAR_FILE);
    // s->AddPerUtUserRtnAppThroughput (SatStatsHelper::OUTPUT_SCATTER_FILE);
    // s->AddPerUtUserRtnAppThroughput (SatStatsHelper::OUTPUT_SCATTER_PLOT);
    // s->AddPerUtUserRtnDevThroughput (SatStatsHelper::OUTPUT_SCATTER_FILE);
    // s->AddPerUtUserRtnDevThroughput (SatStatsHelper::OUTPUT_SCATTER_PLOT);

    // s->AddPerUtUserRtnAppDelay (SatStatsHelper::OUTPUT_CDF_FILE);
    // s->AddPerUtUserRtnAppDelay (SatStatsHelper::OUTPUT_CDF_PLOT);
    // s->AddPerUtRtnDevDelay (SatStatsHelper::OUTPUT_CDF_FILE);
    // s->AddPerUtRtnDevDelay (SatStatsHelper::OUTPUT_CDF_PLOT);

    s->AddPerBeamRtnCompositeSinr(SatStatsHelper::OUTPUT_CDF_FILE);
    s->AddPerBeamRtnCompositeSinr(SatStatsHelper::OUTPUT_CDF_PLOT);

    s->AddPerBeamResourcesGranted(SatStatsHelper::OUTPUT_CDF_FILE);
    s->AddPerBeamResourcesGranted(SatStatsHelper::OUTPUT_CDF_PLOT);

    s->AddPerBeamFrameSymbolLoad(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddPerBeamWaveformUsage(SatStatsHelper::OUTPUT_SCALAR_FILE);

    s->AddPerBeamRtnFeederDaPacketError(SatStatsHelper::OUTPUT_SCALAR_FILE);

    s->AddPerBeamFeederCrdsaPacketCollision(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddPerBeamFeederCrdsaPacketError(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddPerBeamFeederSlottedAlohaPacketCollision(SatStatsHelper::OUTPUT_SCALAR_FILE);
    s->AddPerBeamFeederSlottedAlohaPacketError(SatStatsHelper::OUTPUT_SCALAR_FILE);

    // s->AddPerUtFeederCrdsaPacketCollision (SatStatsHelper::OUTPUT_SCALAR_FILE);
    // s->AddPerUtFeederCrdsaPacketError (SatStatsHelper::OUTPUT_SCALAR_FILE);
    // s->AddPerUtFeederSlottedAlohaPacketCollision (SatStatsHelper::OUTPUT_SCALAR_FILE);
    // s->AddPerUtFeederSlottedAlohaPacketError (SatStatsHelper::OUTPUT_SCALAR_FILE);

    NS_LOG_INFO("--- sat-ra-sim-tn9 ---");
    NS_LOG_INFO("  Packet size: " << packetSize);
    NS_LOG_INFO("  Simulation length: " << simLength);
    NS_LOG_INFO("  Number of UTs: " << utsPerBeam);
    NS_LOG_INFO("  Number of end users per UT: " << endUsersPerUt);
    NS_LOG_INFO("  ");

    /**
     * Store attributes into XML output
     */
    // std::stringstream filename;
    // filename << "tn9-ra-output-attributes-ut" << utsPerBeam
    //          << "-mode" << raMode << ".xml";
    //
    // Config::SetDefault ("ns3::ConfigStore::Filename", StringValue (filename.str ()));
    // Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("Xml"));
    // Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Save"));
    // ConfigStore outputConfig;
    // outputConfig.ConfigureDefaults ();

    /**
     * Run simulation
     */
    simulationHelper->RunSimulation();

    return 0;
}
