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
 * Author: Jani Puttonen <jani.puttonen@magister.fi>
 *
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/satellite-module.h"
#include "ns3/traffic-module.h"

using namespace ns3;

/**
 * \file sat-arq-rtn-example.cc
 * \ingroup satellite
 *
 * \brief  An example to test RTN link ARQ functionality
 *
 *         execute command -> ./waf --run "sat-arq-rtn-example --PrintHelp"
 */

NS_LOG_COMPONENT_DEFINE("sat-arq-rtn-example");

int
main(int argc, char* argv[])
{
    uint32_t beamId = 1;
    uint32_t endUsersPerUt(1);
    uint32_t utsPerBeam(3);
    uint32_t packetSize(128);
    Time interval(Seconds(0.3));
    Time simLength(Seconds(100.0));
    Time appStartTime = Seconds(0.1);

    // enable info logs
    // LogComponentEnable ("CbrApplication", LOG_LEVEL_INFO);
    // LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
    // LogComponentEnable ("sat-arq-rtn-example", LOG_LEVEL_INFO);

    /// Set simulation output details
    Config::SetDefault("ns3::SatEnvVariables::EnableSimulationOutputOverwrite", BooleanValue(true));

    /// Enable packet trace
    Config::SetDefault("ns3::SatHelper::PacketTraceEnabled", BooleanValue(true));
    Ptr<SimulationHelper> simulationHelper = CreateObject<SimulationHelper>("example-arq-rtn");

    // read command line parameters given by user
    CommandLine cmd;
    cmd.AddValue("endUsersPerUt", "Number of end users per UT", endUsersPerUt);
    cmd.AddValue("utsPerBeam", "Number of UTs per spot-beam", utsPerBeam);
    simulationHelper->AddDefaultUiArguments(cmd);
    cmd.Parse(argc, argv);

    simulationHelper->SetUtCountPerBeam(utsPerBeam);
    simulationHelper->SetUserCountPerUt(endUsersPerUt);
    simulationHelper->SetSimulationTime(simLength);

    std::stringstream beamsEnabled;
    beamsEnabled << beamId;
    simulationHelper->SetBeams(beamsEnabled.str());

    // Configure error model
    double errorRate(0.1);
    Config::SetDefault("ns3::SatUtHelper::FwdLinkErrorModel",
                       EnumValue(SatPhyRxCarrierConf::EM_NONE));
    Config::SetDefault("ns3::SatGwHelper::RtnLinkErrorModel",
                       EnumValue(SatPhyRxCarrierConf::EM_CONSTANT));
    Config::SetDefault("ns3::SatGwHelper::RtnLinkConstantErrorRate", DoubleValue(errorRate));

    // Enable ARQ
    Config::SetDefault("ns3::SatLlc::RtnLinkArqEnabled", BooleanValue(true));
    Config::SetDefault("ns3::SatLlc::FwdLinkArqEnabled", BooleanValue(false));

    // RTN link ARQ attributes
    Config::SetDefault("ns3::SatReturnLinkEncapsulatorArq::MaxNoOfRetransmissions",
                       UintegerValue(2));
    Config::SetDefault("ns3::SatReturnLinkEncapsulatorArq::WindowSize", UintegerValue(20));
    Config::SetDefault("ns3::SatReturnLinkEncapsulatorArq::RetransmissionTimer",
                       TimeValue(Seconds(0.6)));
    Config::SetDefault("ns3::SatReturnLinkEncapsulatorArq::RxWaitingTime", TimeValue(Seconds(1.8)));

    Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService0_ConstantAssignmentProvided",
                       BooleanValue(true));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService0_ConstantServiceRate",
                       StringValue("ns3::ConstantRandomVariable[Constant=10]"));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService0_RbdcAllowed",
                       BooleanValue(false));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService0_VolumeAllowed",
                       BooleanValue(false));

    Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService3_ConstantAssignmentProvided",
                       BooleanValue(true));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService3_ConstantServiceRate",
                       StringValue("ns3::ConstantRandomVariable[Constant=100]"));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService3_RbdcAllowed", BooleanValue(true));
    Config::SetDefault("ns3::SatLowerLayerServiceConf::DaService3_VolumeAllowed",
                       BooleanValue(false));

    simulationHelper->LoadScenario("geo-33E");

    // Creating the reference system.
    simulationHelper->CreateSatScenario();

    /// Create applicationa on UT users
    simulationHelper->GetTrafficHelper()->AddCbrTraffic(
        SatTrafficHelper::RTN_LINK,
        SatTrafficHelper::UDP,
        interval,
        packetSize,
        Singleton<SatTopology>::Get()->GetGwUserNodes(),
        Singleton<SatTopology>::Get()->GetUtUserNodes(),
        appStartTime,
        simLength,
        Seconds(0.001));

    NS_LOG_INFO("--- sat-arq-rtn-example ---");
    NS_LOG_INFO("  Packet size in bytes: " << packetSize);
    NS_LOG_INFO("  Packet sending interval: " << interval.GetSeconds());
    NS_LOG_INFO("  Simulation length: " << simLength.GetSeconds());
    NS_LOG_INFO("  Number of UTs: " << utsPerBeam);
    NS_LOG_INFO("  Number of end users per UT: " << endUsersPerUt);
    NS_LOG_INFO("  ");

    simulationHelper->EnableProgressLogs();
    simulationHelper->RunSimulation();

    return 0;
}
