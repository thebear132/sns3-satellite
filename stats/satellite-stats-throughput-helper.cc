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
 * Author: Budiarto Herman <budiarto.herman@magister.fi>
 *
 */

#include "satellite-stats-throughput-helper.h"

#include <ns3/application-packet-probe.h>
#include <ns3/application.h>
#include <ns3/boolean.h>
#include <ns3/callback.h>
#include <ns3/data-collection-object.h>
#include <ns3/distribution-collector.h>
#include <ns3/enum.h>
#include <ns3/inet-socket-address.h>
#include <ns3/interval-rate-collector.h>
#include <ns3/ipv4.h>
#include <ns3/log.h>
#include <ns3/mac48-address.h>
#include <ns3/magister-gnuplot-aggregator.h>
#include <ns3/multi-file-aggregator.h>
#include <ns3/net-device.h>
#include <ns3/node-container.h>
#include <ns3/packet.h>
#include <ns3/probe.h>
#include <ns3/satellite-helper.h>
#include <ns3/satellite-id-mapper.h>
#include <ns3/satellite-mac.h>
#include <ns3/satellite-net-device.h>
#include <ns3/satellite-orbiter-net-device.h>
#include <ns3/satellite-phy.h>
#include <ns3/satellite-topology.h>
#include <ns3/scalar-collector.h>
#include <ns3/singleton.h>
#include <ns3/string.h>
#include <ns3/unit-conversion-collector.h>

#include <sstream>

NS_LOG_COMPONENT_DEFINE("SatStatsThroughputHelper");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(SatStatsThroughputHelper);

SatStatsThroughputHelper::SatStatsThroughputHelper(Ptr<const SatHelper> satHelper)
    : SatStatsHelper(satHelper),
      m_averagingMode(false)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsThroughputHelper::~SatStatsThroughputHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsThroughputHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsThroughputHelper")
            .SetParent<SatStatsHelper>()
            .AddAttribute("AveragingMode",
                          "If true, all samples will be averaged before passed to aggregator. "
                          "Only affects histogram, PDF, and CDF output types.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&SatStatsThroughputHelper::SetAveragingMode,
                                              &SatStatsThroughputHelper::GetAveragingMode),
                          MakeBooleanChecker());
    return tid;
}

void
SatStatsThroughputHelper::SetAveragingMode(bool averagingMode)
{
    NS_LOG_FUNCTION(this << averagingMode);
    m_averagingMode = averagingMode;
}

bool
SatStatsThroughputHelper::GetAveragingMode() const
{
    return m_averagingMode;
}

void
SatStatsThroughputHelper::DoInstall()
{
    NS_LOG_FUNCTION(this);

    switch (GetOutputType())
    {
    case SatStatsHelper::OUTPUT_NONE:
        NS_FATAL_ERROR(GetOutputTypeName(GetOutputType())
                       << " is not a valid output type for this statistics.");
        break;

    case SatStatsHelper::OUTPUT_SCALAR_FILE: {
        // Setup aggregator.
        m_aggregator = CreateAggregator("ns3::MultiFileAggregator",
                                        "OutputFileName",
                                        StringValue(GetOutputFileName()),
                                        "MultiFileMode",
                                        BooleanValue(false),
                                        "EnableContextPrinting",
                                        BooleanValue(true),
                                        "GeneralHeading",
                                        StringValue(GetIdentifierHeading("throughput_kbps")));

        // Setup second-level collectors.
        m_terminalCollectors.SetType("ns3::ScalarCollector");
        m_terminalCollectors.SetAttribute("InputDataType",
                                          EnumValue(ScalarCollector::INPUT_DATA_TYPE_DOUBLE));
        m_terminalCollectors.SetAttribute(
            "OutputType",
            EnumValue(ScalarCollector::OUTPUT_TYPE_AVERAGE_PER_SECOND));
        CreateCollectorPerIdentifier(m_terminalCollectors);
        m_terminalCollectors.ConnectToAggregator("Output",
                                                 m_aggregator,
                                                 &MultiFileAggregator::Write1d);

        // Setup first-level collectors.
        m_conversionCollectors.SetType("ns3::UnitConversionCollector");
        m_conversionCollectors.SetAttribute("ConversionType",
                                            EnumValue(UnitConversionCollector::FROM_BYTES_TO_KBIT));
        CreateCollectorPerIdentifier(m_conversionCollectors);
        m_conversionCollectors.ConnectToCollector("Output",
                                                  m_terminalCollectors,
                                                  &ScalarCollector::TraceSinkDouble);
        break;
    }

    case SatStatsHelper::OUTPUT_SCATTER_FILE: {
        // Setup aggregator.
        m_aggregator = CreateAggregator("ns3::MultiFileAggregator",
                                        "OutputFileName",
                                        StringValue(GetOutputFileName()),
                                        "GeneralHeading",
                                        StringValue(GetTimeHeading("throughput_kbps")));

        // Setup second-level collectors.
        m_terminalCollectors.SetType("ns3::IntervalRateCollector");
        m_terminalCollectors.SetAttribute("InputDataType",
                                          EnumValue(IntervalRateCollector::INPUT_DATA_TYPE_DOUBLE));
        CreateCollectorPerIdentifier(m_terminalCollectors);
        m_terminalCollectors.ConnectToAggregator("OutputWithTime",
                                                 m_aggregator,
                                                 &MultiFileAggregator::Write2d);
        m_terminalCollectors.ConnectToAggregator("OutputString",
                                                 m_aggregator,
                                                 &MultiFileAggregator::AddContextHeading);

        // Setup first-level collectors.
        m_conversionCollectors.SetType("ns3::UnitConversionCollector");
        m_conversionCollectors.SetAttribute("ConversionType",
                                            EnumValue(UnitConversionCollector::FROM_BYTES_TO_KBIT));
        CreateCollectorPerIdentifier(m_conversionCollectors);
        m_conversionCollectors.ConnectToCollector("Output",
                                                  m_terminalCollectors,
                                                  &IntervalRateCollector::TraceSinkDouble);
        break;
    }

    case SatStatsHelper::OUTPUT_HISTOGRAM_FILE:
    case SatStatsHelper::OUTPUT_PDF_FILE:
    case SatStatsHelper::OUTPUT_CDF_FILE: {
        if (!m_averagingMode)
        {
            NS_FATAL_ERROR("This statistics require AveragingMode to be enabled");
        }

        // Setup aggregator.
        m_aggregator = CreateAggregator("ns3::MultiFileAggregator",
                                        "OutputFileName",
                                        StringValue(GetOutputFileName()),
                                        "MultiFileMode",
                                        BooleanValue(false),
                                        "EnableContextPrinting",
                                        BooleanValue(false),
                                        "GeneralHeading",
                                        StringValue(GetDistributionHeading("throughput_kbps")));
        Ptr<MultiFileAggregator> fileAggregator = m_aggregator->GetObject<MultiFileAggregator>();
        NS_ASSERT(fileAggregator != nullptr);

        // Setup the final-level collector.
        m_averagingCollector = CreateObject<DistributionCollector>();
        DistributionCollector::OutputType_t outputType =
            DistributionCollector::OUTPUT_TYPE_HISTOGRAM;
        if (GetOutputType() == SatStatsHelper::OUTPUT_PDF_FILE)
        {
            outputType = DistributionCollector::OUTPUT_TYPE_PROBABILITY;
        }
        else if (GetOutputType() == SatStatsHelper::OUTPUT_CDF_FILE)
        {
            outputType = DistributionCollector::OUTPUT_TYPE_CUMULATIVE;
        }
        m_averagingCollector->SetOutputType(outputType);
        m_averagingCollector->SetName("0");
        m_averagingCollector->TraceConnect(
            "Output",
            "0",
            MakeCallback(&MultiFileAggregator::Write2d, fileAggregator));
        m_averagingCollector->TraceConnect(
            "OutputString",
            "0",
            MakeCallback(&MultiFileAggregator::AddContextHeading, fileAggregator));
        m_averagingCollector->TraceConnect(
            "Warning",
            "0",
            MakeCallback(&MultiFileAggregator::EnableContextWarning, fileAggregator));

        // Setup second-level collectors.
        m_terminalCollectors.SetType("ns3::ScalarCollector");
        m_terminalCollectors.SetAttribute("InputDataType",
                                          EnumValue(ScalarCollector::INPUT_DATA_TYPE_DOUBLE));
        m_terminalCollectors.SetAttribute(
            "OutputType",
            EnumValue(ScalarCollector::OUTPUT_TYPE_AVERAGE_PER_SECOND));
        CreateCollectorPerIdentifier(m_terminalCollectors);
        Callback<void, double> callback =
            MakeCallback(&DistributionCollector::TraceSinkDouble1, m_averagingCollector);
        for (CollectorMap::Iterator it = m_terminalCollectors.Begin();
             it != m_terminalCollectors.End();
             ++it)
        {
            it->second->TraceConnectWithoutContext("Output", callback);
        }

        // Setup first-level collectors.
        m_conversionCollectors.SetType("ns3::UnitConversionCollector");
        m_conversionCollectors.SetAttribute("ConversionType",
                                            EnumValue(UnitConversionCollector::FROM_BYTES_TO_KBIT));
        CreateCollectorPerIdentifier(m_conversionCollectors);
        m_conversionCollectors.ConnectToCollector("Output",
                                                  m_terminalCollectors,
                                                  &ScalarCollector::TraceSinkDouble);
        break;
    }

    case SatStatsHelper::OUTPUT_SCALAR_PLOT:
        /// \todo Add support for boxes in Gnuplot.
        NS_FATAL_ERROR(GetOutputTypeName(GetOutputType())
                       << " is not a valid output type for this statistics.");
        break;

    case SatStatsHelper::OUTPUT_SCATTER_PLOT: {
        // Setup aggregator.
        m_aggregator = CreateAggregator("ns3::MagisterGnuplotAggregator",
                                        "OutputPath",
                                        StringValue(GetOutputPath()),
                                        "OutputFileName",
                                        StringValue(GetName()));
        Ptr<MagisterGnuplotAggregator> plotAggregator =
            m_aggregator->GetObject<MagisterGnuplotAggregator>();
        NS_ASSERT(plotAggregator != nullptr);
        // plot->SetTitle ("");
        plotAggregator->SetLegend("Time (in seconds)",
                                  "Received throughput (in kilobits per second)");
        plotAggregator->Set2dDatasetDefaultStyle(Gnuplot2dDataset::LINES);

        // Setup second-level collectors.
        m_terminalCollectors.SetType("ns3::IntervalRateCollector");
        m_terminalCollectors.SetAttribute("InputDataType",
                                          EnumValue(IntervalRateCollector::INPUT_DATA_TYPE_DOUBLE));
        CreateCollectorPerIdentifier(m_terminalCollectors);
        for (CollectorMap::Iterator it = m_terminalCollectors.Begin();
             it != m_terminalCollectors.End();
             ++it)
        {
            const std::string context = it->second->GetName();
            plotAggregator->Add2dDataset(context, context);
        }
        m_terminalCollectors.ConnectToAggregator("OutputWithTime",
                                                 m_aggregator,
                                                 &MagisterGnuplotAggregator::Write2d);

        // Setup first-level collectors.
        m_conversionCollectors.SetType("ns3::UnitConversionCollector");
        m_conversionCollectors.SetAttribute("ConversionType",
                                            EnumValue(UnitConversionCollector::FROM_BYTES_TO_KBIT));
        CreateCollectorPerIdentifier(m_conversionCollectors);
        m_conversionCollectors.ConnectToCollector("Output",
                                                  m_terminalCollectors,
                                                  &IntervalRateCollector::TraceSinkDouble);
        break;
    }

    case SatStatsHelper::OUTPUT_HISTOGRAM_PLOT:
    case SatStatsHelper::OUTPUT_PDF_PLOT:
    case SatStatsHelper::OUTPUT_CDF_PLOT: {
        if (!m_averagingMode)
        {
            NS_FATAL_ERROR("This statistics require AveragingMode to be enabled");
        }

        // Setup aggregator.
        m_aggregator = CreateAggregator("ns3::MagisterGnuplotAggregator",
                                        "OutputPath",
                                        StringValue(GetOutputPath()),
                                        "OutputFileName",
                                        StringValue(GetName()));
        Ptr<MagisterGnuplotAggregator> plotAggregator =
            m_aggregator->GetObject<MagisterGnuplotAggregator>();
        NS_ASSERT(plotAggregator != nullptr);
        // plot->SetTitle ("");
        plotAggregator->SetLegend("Received throughput (in kilobits per second)", "Frequency");
        plotAggregator->Set2dDatasetDefaultStyle(Gnuplot2dDataset::LINES);
        plotAggregator->Add2dDataset(GetName(), GetName());
        /// \todo Find a better dataset name.

        // Setup the final-level collector.
        m_averagingCollector = CreateObject<DistributionCollector>();
        DistributionCollector::OutputType_t outputType =
            DistributionCollector::OUTPUT_TYPE_HISTOGRAM;
        if (GetOutputType() == SatStatsHelper::OUTPUT_PDF_PLOT)
        {
            outputType = DistributionCollector::OUTPUT_TYPE_PROBABILITY;
        }
        else if (GetOutputType() == SatStatsHelper::OUTPUT_CDF_PLOT)
        {
            outputType = DistributionCollector::OUTPUT_TYPE_CUMULATIVE;
        }
        m_averagingCollector->SetOutputType(outputType);
        m_averagingCollector->SetName("0");
        m_averagingCollector->TraceConnect(
            "Output",
            GetName(),
            MakeCallback(&MagisterGnuplotAggregator::Write2d, plotAggregator));
        /// \todo Find a better dataset name.

        // Setup second-level collectors.
        m_terminalCollectors.SetType("ns3::ScalarCollector");
        m_terminalCollectors.SetAttribute("InputDataType",
                                          EnumValue(ScalarCollector::INPUT_DATA_TYPE_DOUBLE));
        m_terminalCollectors.SetAttribute(
            "OutputType",
            EnumValue(ScalarCollector::OUTPUT_TYPE_AVERAGE_PER_SECOND));
        CreateCollectorPerIdentifier(m_terminalCollectors);
        Callback<void, double> callback =
            MakeCallback(&DistributionCollector::TraceSinkDouble1, m_averagingCollector);
        for (CollectorMap::Iterator it = m_terminalCollectors.Begin();
             it != m_terminalCollectors.End();
             ++it)
        {
            it->second->TraceConnectWithoutContext("Output", callback);
        }

        // Setup first-level collectors.
        m_conversionCollectors.SetType("ns3::UnitConversionCollector");
        m_conversionCollectors.SetAttribute("ConversionType",
                                            EnumValue(UnitConversionCollector::FROM_BYTES_TO_KBIT));
        CreateCollectorPerIdentifier(m_conversionCollectors);
        m_conversionCollectors.ConnectToCollector("Output",
                                                  m_terminalCollectors,
                                                  &ScalarCollector::TraceSinkDouble);
        break;
    }

    default:
        NS_FATAL_ERROR("SatStatsThroughputHelper - Invalid output type");
        break;
    }

    // Setup probes and connect them to conversion collectors.
    InstallProbes();

} // end of `void DoInstall ();`

void
SatStatsThroughputHelper::InstallProbes()
{
    NS_LOG_FUNCTION(this);

    // The method below is supposed to be implemented by the child class.
    DoInstallProbes();
}

void
SatStatsThroughputHelper::RxCallback(Ptr<const Packet> packet, const Address& from)
{
    NS_LOG_FUNCTION(this << packet->GetSize() << from);

    if (from.IsInvalid())
    {
        NS_LOG_WARN(this << " discarding packet " << packet << " (" << packet->GetSize()
                         << " bytes)"
                         << " from statistics collection because of"
                         << " invalid sender address");
    }
    else
    {
        // Determine the identifier associated with the sender address.
        std::map<const Address, uint32_t>::const_iterator it = m_identifierMap.find(from);

        if (it == m_identifierMap.end())
        {
            NS_LOG_WARN(this << " discarding packet " << packet << " (" << packet->GetSize()
                             << " bytes)"
                             << " from statistics collection because of"
                             << " unknown sender address " << from);
        }
        else
        {
            // Find the first-level collector with the right identifier.
            Ptr<DataCollectionObject> collector = m_conversionCollectors.Get(it->second);
            NS_ASSERT_MSG(collector != nullptr,
                          "Unable to find collector with identifier " << it->second);
            Ptr<UnitConversionCollector> c = collector->GetObject<UnitConversionCollector>();
            NS_ASSERT(c != nullptr);

            // Pass the sample to the collector.
            c->TraceSinkUinteger32(0, packet->GetSize());
        }
    }

} // end of `void RxCallback (Ptr<const Packet>, const Address);`

// FORWARD LINK APPLICATION-LEVEL /////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsFwdAppThroughputHelper);

SatStatsFwdAppThroughputHelper::SatStatsFwdAppThroughputHelper(Ptr<const SatHelper> satHelper)
    : SatStatsThroughputHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsFwdAppThroughputHelper::~SatStatsFwdAppThroughputHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsFwdAppThroughputHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsFwdAppThroughputHelper").SetParent<SatStatsThroughputHelper>();
    return tid;
}

void
SatStatsFwdAppThroughputHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);
    NodeContainer utUsers = GetSatHelper()->GetUtUsers();

    for (NodeContainer::Iterator it = utUsers.Begin(); it != utUsers.End(); ++it)
    {
        const int32_t utUserId = GetUtUserId(*it);
        NS_ASSERT_MSG(utUserId > 0, "Node " << (*it)->GetId() << " is not a valid UT user");
        const uint32_t identifier = GetIdentifierForUtUser(*it);

        for (uint32_t i = 0; i < (*it)->GetNApplications(); i++)
        {
            // Create the probe.
            std::ostringstream probeName;
            probeName << utUserId << "-" << i;
            Ptr<ApplicationPacketProbe> probe = CreateObject<ApplicationPacketProbe>();
            probe->SetName(probeName.str());

            // Connect the object to the probe.
            if (probe->ConnectByObject("Rx", (*it)->GetApplication(i)))
            {
                // Connect the probe to the right collector.
                if (m_conversionCollectors.ConnectWithProbe(
                        probe->GetObject<Probe>(),
                        "OutputBytes",
                        identifier,
                        &UnitConversionCollector::TraceSinkUinteger32))
                {
                    NS_LOG_INFO(this << " created probe " << probeName.str()
                                     << ", connected to collector " << identifier);
                    m_probes.insert(
                        std::make_pair(probe->GetObject<Probe>(), std::make_pair(*it, identifier)));
                }
                else
                {
                    NS_LOG_WARN(this << " unable to connect probe " << probeName.str()
                                     << " to collector " << identifier);
                }
            }
            else
            {
                /*
                 * We're being tolerant here by only logging a warning, because
                 * not every kind of Application is equipped with the expected
                 * Rx trace source.
                 */
                NS_LOG_WARN(this << " unable to connect probe " << probeName.str()
                                 << " with node ID " << (*it)->GetId() << " application #" << i);
            }

        } // end of `for (i = 0; i < (*it)->GetNApplications (); i++)`

    } // end of `for (it = utUsers.Begin(); it != utUsers.End (); ++it)`

} // end of `void DoInstallProbes ();`

void
SatStatsFwdAppThroughputHelper::UpdateIdentifierOnProbes()
{
    NS_LOG_FUNCTION(this);

    std::map<Ptr<Probe>, std::pair<Ptr<Node>, uint32_t>>::iterator it;

    for (it = m_probes.begin(); it != m_probes.end(); it++)
    {
        Ptr<Probe> probe = it->first;
        Ptr<Node> node = it->second.first;
        uint32_t identifier = it->second.second;
        m_conversionCollectors.DisconnectWithProbe(probe->GetObject<Probe>(),
                                                   "OutputBytes",
                                                   identifier,
                                                   &UnitConversionCollector::TraceSinkUinteger32);

        identifier = GetIdentifierForUtUser(node);

        m_conversionCollectors.ConnectWithProbe(probe->GetObject<Probe>(),
                                                "OutputBytes",
                                                identifier,
                                                &UnitConversionCollector::TraceSinkUinteger32);

        it->second.second = identifier;
    }
} // end of `void UpdateIdentifierOnProbes ();`

// FORWARD FEEDER LINK DEVICE-LEVEL //////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsFwdFeederDevThroughputHelper);

SatStatsFwdFeederDevThroughputHelper::SatStatsFwdFeederDevThroughputHelper(
    Ptr<const SatHelper> satHelper)
    : SatStatsThroughputHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsFwdFeederDevThroughputHelper::~SatStatsFwdFeederDevThroughputHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsFwdFeederDevThroughputHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsFwdFeederDevThroughputHelper").SetParent<SatStatsThroughputHelper>();
    return tid;
}

void
SatStatsFwdFeederDevThroughputHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

    NodeContainer sats = Singleton<SatTopology>::Get()->GetOrbiterNodes();
    Callback<void, Ptr<const Packet>, const Address&> callback =
        MakeCallback(&SatStatsFwdFeederDevThroughputHelper::RxCallback, this);

    for (NodeContainer::Iterator it = sats.Begin(); it != sats.End(); ++it)
    {
        Ptr<NetDevice> dev = GetSatSatOrbiterNetDevice(*it);
        Ptr<SatOrbiterNetDevice> satOrbiterDev = dev->GetObject<SatOrbiterNetDevice>();
        NS_ASSERT(satOrbiterDev != nullptr);
        satOrbiterDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));

        if (satOrbiterDev->TraceConnectWithoutContext("RxFeeder", callback))
        {
            NS_LOG_INFO(this << " successfully connected with node ID " << (*it)->GetId()
                             << " device #" << satOrbiterDev->GetIfIndex());

            // Enable statistics-related tags and trace sources on the device.
            satOrbiterDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
        else
        {
            NS_FATAL_ERROR("Error connecting to Rx trace source of SatNetDevice"
                           << " at node ID " << (*it)->GetId() << " device #"
                           << satOrbiterDev->GetIfIndex());
        }
    } // end of `for (it = sats.Begin(); it != sats.End (); ++it)`

    NodeContainer uts = GetSatHelper()->GetBeamHelper()->GetUtNodes();

    for (NodeContainer::Iterator it = uts.Begin(); it != uts.End(); ++it)
    {
        // Create a map of UT addresses and identifiers.
        SaveAddressAndIdentifier(*it);

        Ptr<NetDevice> dev = GetUtSatNetDevice(*it);
        Ptr<SatNetDevice> satDev = dev->GetObject<SatNetDevice>();
        NS_ASSERT(satDev != nullptr);

        satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));

    } // end of `for (it = uts.Begin(); it != uts.End (); ++it)`

    // Enable statistics-related tags on the transmitting device.
    NodeContainer gws = GetSatHelper()->GetBeamHelper()->GetGwNodes();
    for (NodeContainer::Iterator it = gws.Begin(); it != gws.End(); ++it)
    {
        NetDeviceContainer devs = GetGwSatNetDevice(*it);

        for (NetDeviceContainer::Iterator itDev = devs.Begin(); itDev != devs.End(); ++itDev)
        {
            Ptr<SatNetDevice> satDev = (*itDev)->GetObject<SatNetDevice>();
            NS_ASSERT(satDev != nullptr);

            satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
    }
} // end of `void DoInstallProbes ();`

// FORWARD USER LINK DEVICE-LEVEL //////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsFwdUserDevThroughputHelper);

SatStatsFwdUserDevThroughputHelper::SatStatsFwdUserDevThroughputHelper(
    Ptr<const SatHelper> satHelper)
    : SatStatsThroughputHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsFwdUserDevThroughputHelper::~SatStatsFwdUserDevThroughputHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsFwdUserDevThroughputHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsFwdUserDevThroughputHelper").SetParent<SatStatsThroughputHelper>();
    return tid;
}

void
SatStatsFwdUserDevThroughputHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);
    NodeContainer uts = GetSatHelper()->GetBeamHelper()->GetUtNodes();

    for (NodeContainer::Iterator it = uts.Begin(); it != uts.End(); ++it)
    {
        const int32_t utId = GetUtId(*it);
        NS_ASSERT_MSG(utId > 0, "Node " << (*it)->GetId() << " is not a valid UT");
        const uint32_t identifier = GetIdentifierForUt(*it);

        // Create the probe.
        std::ostringstream probeName;
        probeName << utId;
        Ptr<ApplicationPacketProbe> probe = CreateObject<ApplicationPacketProbe>();
        probe->SetName(probeName.str());

        Ptr<NetDevice> dev = GetUtSatNetDevice(*it);

        // Connect the object to the probe.
        if (probe->ConnectByObject("Rx", dev))
        {
            // Connect the probe to the right collector.
            if (m_conversionCollectors.ConnectWithProbe(
                    probe->GetObject<Probe>(),
                    "OutputBytes",
                    identifier,
                    &UnitConversionCollector::TraceSinkUinteger32))
            {
                NS_LOG_INFO(this << " created probe " << probeName.str()
                                 << ", connected to collector " << identifier);
                m_probes.insert(
                    std::make_pair(probe->GetObject<Probe>(), std::make_pair(*it, identifier)));

                // Enable statistics-related tags and trace sources on the device.
                dev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            }
            else
            {
                NS_LOG_WARN(this << " unable to connect probe " << probeName.str()
                                 << " to collector " << identifier);
            }

        } // end of `if (probe->ConnectByObject ("Rx", dev))`
        else
        {
            NS_FATAL_ERROR("Error connecting to Rx trace source of SatNetDevice"
                           << " at node ID " << (*it)->GetId() << " device #2");
        }

    } // end of `for (it = uts.Begin(); it != uts.End (); ++it)`

    // Enable statistics-related tags on the transmitting device.
    NodeContainer gws = GetSatHelper()->GetBeamHelper()->GetGwNodes();
    for (NodeContainer::Iterator it = gws.Begin(); it != gws.End(); ++it)
    {
        NetDeviceContainer devs = GetGwSatNetDevice(*it);

        for (NetDeviceContainer::Iterator itDev = devs.Begin(); itDev != devs.End(); ++itDev)
        {
            NS_ASSERT((*itDev)->GetObject<SatNetDevice>() != nullptr);
            (*itDev)->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
    }

} // end of `void DoInstallProbes ();`

void
SatStatsFwdUserDevThroughputHelper::UpdateIdentifierOnProbes()
{
    NS_LOG_FUNCTION(this);

    std::map<Ptr<Probe>, std::pair<Ptr<Node>, uint32_t>>::iterator it;

    for (it = m_probes.begin(); it != m_probes.end(); it++)
    {
        Ptr<Probe> probe = it->first;
        Ptr<Node> node = it->second.first;
        uint32_t identifier = it->second.second;
        m_conversionCollectors.DisconnectWithProbe(probe->GetObject<Probe>(),
                                                   "OutputBytes",
                                                   identifier,
                                                   &UnitConversionCollector::TraceSinkUinteger32);

        identifier = GetIdentifierForUt(node);

        m_conversionCollectors.ConnectWithProbe(probe->GetObject<Probe>(),
                                                "OutputBytes",
                                                identifier,
                                                &UnitConversionCollector::TraceSinkUinteger32);

        it->second.second = identifier;
    }
} // end of `void UpdateIdentifierOnProbes ();`

// FORWARD FEEDER LINK MAC-LEVEL /////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsFwdFeederMacThroughputHelper);

SatStatsFwdFeederMacThroughputHelper::SatStatsFwdFeederMacThroughputHelper(
    Ptr<const SatHelper> satHelper)
    : SatStatsThroughputHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsFwdFeederMacThroughputHelper::~SatStatsFwdFeederMacThroughputHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsFwdFeederMacThroughputHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsFwdFeederMacThroughputHelper").SetParent<SatStatsThroughputHelper>();
    return tid;
}

void
SatStatsFwdFeederMacThroughputHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

    NodeContainer sats = Singleton<SatTopology>::Get()->GetOrbiterNodes();
    Callback<void, Ptr<const Packet>, const Address&> callback =
        MakeCallback(&SatStatsFwdFeederMacThroughputHelper::RxCallback, this);

    for (NodeContainer::Iterator it = sats.Begin(); it != sats.End(); ++it)
    {
        Ptr<NetDevice> dev = GetSatSatOrbiterNetDevice(*it);
        Ptr<SatOrbiterNetDevice> satOrbiterDev = dev->GetObject<SatOrbiterNetDevice>();
        NS_ASSERT(satOrbiterDev != nullptr);
        satOrbiterDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        std::map<uint32_t, Ptr<SatMac>> satOrbiterFeederMacs = satOrbiterDev->GetAllFeederMac();
        Ptr<SatMac> satMac;
        for (std::map<uint32_t, Ptr<SatMac>>::iterator it2 = satOrbiterFeederMacs.begin();
             it2 != satOrbiterFeederMacs.end();
             ++it2)
        {
            satMac = it2->second;
            NS_ASSERT(satMac != nullptr);
            satMac->SetAttribute("EnableStatisticsTags", BooleanValue(true));

            // Connect the object to the probe.
            if (satMac->TraceConnectWithoutContext("Rx", callback))
            {
                NS_LOG_INFO(this << " successfully connected with node ID " << (*it)->GetId()
                                 << " device #" << satOrbiterDev->GetIfIndex());
            }
            else
            {
                NS_FATAL_ERROR("Error connecting to Rx trace source of SatMac"
                               << " at node ID " << (*it)->GetId() << " device #"
                               << satOrbiterDev->GetIfIndex());
            }
        }
        std::map<uint32_t, Ptr<SatMac>> satOrbiterUserMacs = satOrbiterDev->GetUserMac();
        for (std::map<uint32_t, Ptr<SatMac>>::iterator it2 = satOrbiterUserMacs.begin();
             it2 != satOrbiterUserMacs.end();
             ++it2)
        {
            satMac = it2->second;
            NS_ASSERT(satMac != nullptr);
            satMac->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
    } // end of `for (it = sats.Begin(); it != sats.End (); ++it)`

    NodeContainer uts = GetSatHelper()->GetBeamHelper()->GetUtNodes();

    for (NodeContainer::Iterator it = uts.Begin(); it != uts.End(); ++it)
    {
        // Create a map of UT addresses and identifiers.
        SaveAddressAndIdentifier(*it);

        Ptr<NetDevice> dev = GetUtSatNetDevice(*it);
        Ptr<SatNetDevice> satDev = dev->GetObject<SatNetDevice>();
        NS_ASSERT(satDev != nullptr);
        Ptr<SatMac> satMac = satDev->GetMac();
        NS_ASSERT(satMac != nullptr);

        satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        satMac->SetAttribute("EnableStatisticsTags", BooleanValue(true));

    } // end of `for (it = uts.Begin(); it != uts.End (); ++it)`

    // Enable statistics-related tags on the transmitting device.
    NodeContainer gws = GetSatHelper()->GetBeamHelper()->GetGwNodes();
    for (NodeContainer::Iterator it = gws.Begin(); it != gws.End(); ++it)
    {
        NetDeviceContainer devs = GetGwSatNetDevice(*it);

        for (NetDeviceContainer::Iterator itDev = devs.Begin(); itDev != devs.End(); ++itDev)
        {
            Ptr<SatNetDevice> satDev = (*itDev)->GetObject<SatNetDevice>();
            NS_ASSERT(satDev != nullptr);
            Ptr<SatMac> satMac = satDev->GetMac();
            NS_ASSERT(satMac != nullptr);

            satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            satMac->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
    }

} // end of `void DoInstallProbes ();`

// FORWARD USER LINK MAC-LEVEL /////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsFwdUserMacThroughputHelper);

SatStatsFwdUserMacThroughputHelper::SatStatsFwdUserMacThroughputHelper(
    Ptr<const SatHelper> satHelper)
    : SatStatsThroughputHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsFwdUserMacThroughputHelper::~SatStatsFwdUserMacThroughputHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsFwdUserMacThroughputHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsFwdUserMacThroughputHelper").SetParent<SatStatsThroughputHelper>();
    return tid;
}

void
SatStatsFwdUserMacThroughputHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

    NodeContainer sats = Singleton<SatTopology>::Get()->GetOrbiterNodes();

    for (NodeContainer::Iterator it = sats.Begin(); it != sats.End(); ++it)
    {
        Ptr<NetDevice> dev = GetSatSatOrbiterNetDevice(*it);
        Ptr<SatOrbiterNetDevice> satOrbiterDev = dev->GetObject<SatOrbiterNetDevice>();
        NS_ASSERT(satOrbiterDev != nullptr);
        satOrbiterDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        std::map<uint32_t, Ptr<SatMac>> satOrbiterFeederMacs = satOrbiterDev->GetAllFeederMac();
        Ptr<SatMac> satMac;
        for (std::map<uint32_t, Ptr<SatMac>>::iterator it2 = satOrbiterFeederMacs.begin();
             it2 != satOrbiterFeederMacs.end();
             ++it2)
        {
            satMac = it2->second;
            NS_ASSERT(satMac != nullptr);
            satMac->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
        std::map<uint32_t, Ptr<SatMac>> satOrbiterUserMacs = satOrbiterDev->GetUserMac();
        for (std::map<uint32_t, Ptr<SatMac>>::iterator it2 = satOrbiterUserMacs.begin();
             it2 != satOrbiterUserMacs.end();
             ++it2)
        {
            satMac = it2->second;
            NS_ASSERT(satMac != nullptr);
            satMac->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
    } // end of `for (it = sats.Begin(); it != sats.End (); ++it)`

    NodeContainer uts = GetSatHelper()->GetBeamHelper()->GetUtNodes();

    for (NodeContainer::Iterator it = uts.Begin(); it != uts.End(); ++it)
    {
        const int32_t utId = GetUtId(*it);
        NS_ASSERT_MSG(utId > 0, "Node " << (*it)->GetId() << " is not a valid UT");
        const uint32_t identifier = GetIdentifierForUt(*it);

        // Create the probe.
        std::ostringstream probeName;
        probeName << utId;
        Ptr<ApplicationPacketProbe> probe = CreateObject<ApplicationPacketProbe>();
        probe->SetName(probeName.str());

        Ptr<NetDevice> dev = GetUtSatNetDevice(*it);
        Ptr<SatNetDevice> satDev = dev->GetObject<SatNetDevice>();
        NS_ASSERT(satDev != nullptr);
        Ptr<SatMac> satMac = satDev->GetMac();
        NS_ASSERT(satMac != nullptr);

        // Connect the object to the probe.
        if (probe->ConnectByObject("Rx", satMac))
        {
            // Connect the probe to the right collector.
            if (m_conversionCollectors.ConnectWithProbe(
                    probe->GetObject<Probe>(),
                    "OutputBytes",
                    identifier,
                    &UnitConversionCollector::TraceSinkUinteger32))
            {
                m_probes.insert(
                    std::make_pair(probe->GetObject<Probe>(), std::make_pair(*it, identifier)));

                // Enable statistics-related tags and trace sources on the device.
                satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
                satMac->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            }
            else
            {
                NS_LOG_WARN(this << " unable to connect probe " << probeName.str()
                                 << " to collector " << identifier);
            }
        }
        else
        {
            NS_FATAL_ERROR("Error connecting to Rx trace source of SatMac"
                           << " at node ID " << (*it)->GetId() << " device #2");
        }
    } // end of `for (it = uts.Begin(); it != uts.End (); ++it)`

    // Enable statistics-related tags on the transmitting device.
    NodeContainer gws = GetSatHelper()->GetBeamHelper()->GetGwNodes();
    for (NodeContainer::Iterator it = gws.Begin(); it != gws.End(); ++it)
    {
        NetDeviceContainer devs = GetGwSatNetDevice(*it);

        for (NetDeviceContainer::Iterator itDev = devs.Begin(); itDev != devs.End(); ++itDev)
        {
            Ptr<SatNetDevice> satDev = (*itDev)->GetObject<SatNetDevice>();
            NS_ASSERT(satDev != nullptr);
            Ptr<SatMac> satMac = satDev->GetMac();
            NS_ASSERT(satMac != nullptr);

            satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            satMac->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
    }

} // end of `void DoInstallProbes ();`

void
SatStatsFwdUserMacThroughputHelper::UpdateIdentifierOnProbes()
{
    NS_LOG_FUNCTION(this);

    std::map<Ptr<Probe>, std::pair<Ptr<Node>, uint32_t>>::iterator it;

    for (it = m_probes.begin(); it != m_probes.end(); it++)
    {
        Ptr<Probe> probe = it->first;
        Ptr<Node> node = it->second.first;
        uint32_t identifier = it->second.second;
        m_conversionCollectors.DisconnectWithProbe(probe->GetObject<Probe>(),
                                                   "OutputBytes",
                                                   identifier,
                                                   &UnitConversionCollector::TraceSinkUinteger32);

        identifier = GetIdentifierForUt(node);

        m_conversionCollectors.ConnectWithProbe(probe->GetObject<Probe>(),
                                                "OutputBytes",
                                                identifier,
                                                &UnitConversionCollector::TraceSinkUinteger32);

        it->second.second = identifier;
    }
} // end of `void UpdateIdentifierOnProbes ();`

// FORWARD FEEDER LINK PHY-LEVEL /////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsFwdFeederPhyThroughputHelper);

SatStatsFwdFeederPhyThroughputHelper::SatStatsFwdFeederPhyThroughputHelper(
    Ptr<const SatHelper> satHelper)
    : SatStatsThroughputHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsFwdFeederPhyThroughputHelper::~SatStatsFwdFeederPhyThroughputHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsFwdFeederPhyThroughputHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsFwdFeederPhyThroughputHelper").SetParent<SatStatsThroughputHelper>();
    return tid;
}

void
SatStatsFwdFeederPhyThroughputHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

    NodeContainer sats = Singleton<SatTopology>::Get()->GetOrbiterNodes();
    Callback<void, Ptr<const Packet>, const Address&> callback =
        MakeCallback(&SatStatsFwdFeederPhyThroughputHelper::RxCallback, this);

    for (NodeContainer::Iterator it = sats.Begin(); it != sats.End(); ++it)
    {
        Ptr<NetDevice> dev = GetSatSatOrbiterNetDevice(*it);
        Ptr<SatOrbiterNetDevice> satOrbiterDev = dev->GetObject<SatOrbiterNetDevice>();
        NS_ASSERT(satOrbiterDev != nullptr);
        satOrbiterDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        std::map<uint32_t, Ptr<SatPhy>> satOrbiterFeederPhys = satOrbiterDev->GetFeederPhy();
        Ptr<SatPhy> satPhy;
        for (std::map<uint32_t, Ptr<SatPhy>>::iterator it2 = satOrbiterFeederPhys.begin();
             it2 != satOrbiterFeederPhys.end();
             ++it2)
        {
            satPhy = it2->second;
            NS_ASSERT(satPhy != nullptr);
            satPhy->SetAttribute("EnableStatisticsTags", BooleanValue(true));

            // Connect the object to the probe.
            if (satPhy->TraceConnectWithoutContext("Rx", callback))
            {
                NS_LOG_INFO(this << " successfully connected with node ID " << (*it)->GetId()
                                 << " device #" << satOrbiterDev->GetIfIndex());

                // Enable statistics-related tags and trace sources on the device.
                satPhy->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            }
            else
            {
                NS_FATAL_ERROR("Error connecting to Rx trace source of SatPhy"
                               << " at node ID " << (*it)->GetId() << " device #"
                               << satOrbiterDev->GetIfIndex());
            }
        }
        std::map<uint32_t, Ptr<SatPhy>> satOrbiterUserPhys = satOrbiterDev->GetUserPhy();
        for (std::map<uint32_t, Ptr<SatPhy>>::iterator it2 = satOrbiterUserPhys.begin();
             it2 != satOrbiterUserPhys.end();
             ++it2)
        {
            satPhy = it2->second;
            NS_ASSERT(satPhy != nullptr);
            satPhy->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
    } // end of `for (it = sats.Begin(); it != sats.End (); ++it)`

    NodeContainer uts = GetSatHelper()->GetBeamHelper()->GetUtNodes();

    for (NodeContainer::Iterator it = uts.Begin(); it != uts.End(); ++it)
    {
        // Create a map of UT addresses and identifiers.
        SaveAddressAndIdentifier(*it);

        Ptr<NetDevice> dev = GetUtSatNetDevice(*it);
        Ptr<SatNetDevice> satDev = dev->GetObject<SatNetDevice>();
        NS_ASSERT(satDev != nullptr);
        Ptr<SatPhy> satPhy = satDev->GetPhy();
        NS_ASSERT(satPhy != nullptr);

        satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        satPhy->SetAttribute("EnableStatisticsTags", BooleanValue(true));

    } // end of `for (it = uts.Begin(); it != uts.End (); ++it)`

    // Enable statistics-related tags on the transmitting device.
    NodeContainer gws = GetSatHelper()->GetBeamHelper()->GetGwNodes();
    for (NodeContainer::Iterator it = gws.Begin(); it != gws.End(); ++it)
    {
        NetDeviceContainer devs = GetGwSatNetDevice(*it);

        for (NetDeviceContainer::Iterator itDev = devs.Begin(); itDev != devs.End(); ++itDev)
        {
            Ptr<SatNetDevice> satDev = (*itDev)->GetObject<SatNetDevice>();
            NS_ASSERT(satDev != nullptr);
            Ptr<SatPhy> satPhy = satDev->GetPhy();
            NS_ASSERT(satPhy != nullptr);

            satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            satPhy->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
    }

} // end of `void DoInstallProbes ();`

// FORWARD USER LINK PHY-LEVEL /////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsFwdUserPhyThroughputHelper);

SatStatsFwdUserPhyThroughputHelper::SatStatsFwdUserPhyThroughputHelper(
    Ptr<const SatHelper> satHelper)
    : SatStatsThroughputHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsFwdUserPhyThroughputHelper::~SatStatsFwdUserPhyThroughputHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsFwdUserPhyThroughputHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsFwdUserPhyThroughputHelper").SetParent<SatStatsThroughputHelper>();
    return tid;
}

void
SatStatsFwdUserPhyThroughputHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

    NodeContainer sats = Singleton<SatTopology>::Get()->GetOrbiterNodes();

    for (NodeContainer::Iterator it = sats.Begin(); it != sats.End(); ++it)
    {
        Ptr<NetDevice> dev = GetSatSatOrbiterNetDevice(*it);
        Ptr<SatOrbiterNetDevice> satOrbiterDev = dev->GetObject<SatOrbiterNetDevice>();
        NS_ASSERT(satOrbiterDev != nullptr);
        satOrbiterDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        std::map<uint32_t, Ptr<SatPhy>> satOrbiterFeederPhys = satOrbiterDev->GetFeederPhy();
        Ptr<SatPhy> satPhy;
        for (std::map<uint32_t, Ptr<SatPhy>>::iterator it2 = satOrbiterFeederPhys.begin();
             it2 != satOrbiterFeederPhys.end();
             ++it2)
        {
            satPhy = it2->second;
            NS_ASSERT(satPhy != nullptr);
            satPhy->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
        std::map<uint32_t, Ptr<SatPhy>> satOrbiterUserPhys = satOrbiterDev->GetUserPhy();
        for (std::map<uint32_t, Ptr<SatPhy>>::iterator it2 = satOrbiterUserPhys.begin();
             it2 != satOrbiterUserPhys.end();
             ++it2)
        {
            satPhy = it2->second;
            NS_ASSERT(satPhy != nullptr);
            satPhy->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
    } // end of `for (it = sats.Begin(); it != sats.End (); ++it)`

    NodeContainer uts = GetSatHelper()->GetBeamHelper()->GetUtNodes();

    for (NodeContainer::Iterator it = uts.Begin(); it != uts.End(); ++it)
    {
        const int32_t utId = GetUtId(*it);
        NS_ASSERT_MSG(utId > 0, "Node " << (*it)->GetId() << " is not a valid UT");
        const uint32_t identifier = GetIdentifierForUt(*it);

        // Create the probe.
        std::ostringstream probeName;
        probeName << utId;
        Ptr<ApplicationPacketProbe> probe = CreateObject<ApplicationPacketProbe>();
        probe->SetName(probeName.str());

        Ptr<NetDevice> dev = GetUtSatNetDevice(*it);
        Ptr<SatNetDevice> satDev = dev->GetObject<SatNetDevice>();
        NS_ASSERT(satDev != nullptr);
        Ptr<SatPhy> satPhy = satDev->GetPhy();
        NS_ASSERT(satPhy != nullptr);

        // Connect the object to the probe.
        if (probe->ConnectByObject("Rx", satPhy))
        { // Connect the probe to the right collector.
            if (m_conversionCollectors.ConnectWithProbe(
                    probe->GetObject<Probe>(),
                    "OutputBytes",
                    identifier,
                    &UnitConversionCollector::TraceSinkUinteger32))
            {
                m_probes.insert(
                    std::make_pair(probe->GetObject<Probe>(), std::make_pair(*it, identifier)));

                // Enable statistics-related tags and trace sources on the device.
                satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
                satPhy->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            }
            else
            {
                NS_LOG_WARN(this << " unable to connect probe " << probeName.str()
                                 << " to collector " << identifier);
            }
        }
        else
        {
            NS_FATAL_ERROR("Error connecting to Rx trace source of SatPhy"
                           << " at node ID " << (*it)->GetId() << " device #2");
        }
    } // end of `for (it = uts.Begin(); it != uts.End (); ++it)`

    // Enable statistics-related tags on the transmitting device.
    NodeContainer gws = GetSatHelper()->GetBeamHelper()->GetGwNodes();
    for (NodeContainer::Iterator it = gws.Begin(); it != gws.End(); ++it)
    {
        NetDeviceContainer devs = GetGwSatNetDevice(*it);

        for (NetDeviceContainer::Iterator itDev = devs.Begin(); itDev != devs.End(); ++itDev)
        {
            Ptr<SatNetDevice> satDev = (*itDev)->GetObject<SatNetDevice>();
            NS_ASSERT(satDev != nullptr);
            Ptr<SatPhy> satPhy = satDev->GetPhy();
            NS_ASSERT(satPhy != nullptr);

            satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            satPhy->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
    }

} // end of `void DoInstallProbes ();`

void
SatStatsFwdUserPhyThroughputHelper::UpdateIdentifierOnProbes()
{
    NS_LOG_FUNCTION(this);

    std::map<Ptr<Probe>, std::pair<Ptr<Node>, uint32_t>>::iterator it;

    for (it = m_probes.begin(); it != m_probes.end(); it++)
    {
        Ptr<Probe> probe = it->first;
        Ptr<Node> node = it->second.first;
        uint32_t identifier = it->second.second;
        m_conversionCollectors.DisconnectWithProbe(probe->GetObject<Probe>(),
                                                   "OutputBytes",
                                                   identifier,
                                                   &UnitConversionCollector::TraceSinkUinteger32);

        identifier = GetIdentifierForUt(node);

        m_conversionCollectors.ConnectWithProbe(probe->GetObject<Probe>(),
                                                "OutputBytes",
                                                identifier,
                                                &UnitConversionCollector::TraceSinkUinteger32);

        it->second.second = identifier;
    }
} // end of `void UpdateIdentifierOnProbes ();`

// RETURN LINK APPLICATION-LEVEL //////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsRtnAppThroughputHelper);

SatStatsRtnAppThroughputHelper::SatStatsRtnAppThroughputHelper(Ptr<const SatHelper> satHelper)
    : SatStatsThroughputHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsRtnAppThroughputHelper::~SatStatsRtnAppThroughputHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsRtnAppThroughputHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsRtnAppThroughputHelper").SetParent<SatStatsThroughputHelper>();
    return tid;
}

void
SatStatsRtnAppThroughputHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

    // Create a map of UT user addresses and identifiers.
    NodeContainer utUsers = GetSatHelper()->GetUtUsers();
    for (NodeContainer::Iterator it = utUsers.Begin(); it != utUsers.End(); ++it)
    {
        SaveIpv4AddressAndIdentifier(*it);
    }

    // Connect to trace sources at GW user node's applications.

    NodeContainer gwUsers = GetSatHelper()->GetGwUsers();
    Callback<void, Ptr<const Packet>, const Address&> callback =
        MakeCallback(&SatStatsRtnAppThroughputHelper::Ipv4Callback, this);

    for (NodeContainer::Iterator it = gwUsers.Begin(); it != gwUsers.End(); ++it)
    {
        for (uint32_t i = 0; i < (*it)->GetNApplications(); i++)
        {
            Ptr<Application> app = (*it)->GetApplication(i);

            if (app->TraceConnectWithoutContext("Rx", callback))
            {
                NS_LOG_INFO(this << " successfully connected with node ID " << (*it)->GetId()
                                 << " application #" << i);
            }
            else
            {
                /*
                 * We're being tolerant here by only logging a warning, because
                 * not every kind of Application is equipped with the expected
                 * Rx trace source.
                 */
                NS_LOG_WARN(this << " unable to connect with node ID " << (*it)->GetId()
                                 << " application #" << i);
            }
        }
    }

} // end of `void DoInstallProbes ();`

void
SatStatsRtnAppThroughputHelper::Ipv4Callback(Ptr<const Packet> packet, const Address& from)
{
    // NS_LOG_FUNCTION (this << packet->GetSize () << from);

    if (InetSocketAddress::IsMatchingType(from))
    {
        // Determine the identifier associated with the sender address.
        const Address ipv4Addr = InetSocketAddress::ConvertFrom(from).GetIpv4();
        std::map<const Address, uint32_t>::const_iterator it1 = m_identifierMap.find(ipv4Addr);

        if (it1 == m_identifierMap.end())
        {
            NS_LOG_WARN(this << " discarding packet " << packet << " (" << packet->GetSize()
                             << " bytes)"
                             << " from statistics collection because of"
                             << " unknown sender IPv4 address " << ipv4Addr);
        }
        else
        {
            // Find the collector with the right identifier.
            Ptr<DataCollectionObject> collector = m_conversionCollectors.Get(it1->second);
            NS_ASSERT_MSG(collector != nullptr,
                          "Unable to find collector with identifier " << it1->second);
            Ptr<UnitConversionCollector> c = collector->GetObject<UnitConversionCollector>();
            NS_ASSERT(c != nullptr);

            // Pass the sample to the collector.
            c->TraceSinkUinteger32(0, packet->GetSize());
        }
    }
    else
    {
        NS_LOG_WARN(
            this << " discarding packet " << packet << " (" << packet->GetSize() << " bytes)"
                 << " from statistics collection"
                 << " because it comes from sender " << from << " without valid InetSocketAddress");
    }

} // end of `void Ipv4Callback (Ptr<const Packet>, const Address);`

void
SatStatsRtnAppThroughputHelper::SaveIpv4AddressAndIdentifier(Ptr<Node> utUserNode)
{
    NS_LOG_FUNCTION(this << utUserNode->GetId());

    Ptr<Ipv4> ipv4 = utUserNode->GetObject<Ipv4>();

    if (ipv4 == nullptr)
    {
        NS_LOG_INFO(this << " Node " << utUserNode->GetId() << " does not support IPv4 protocol");
    }
    else if (ipv4->GetNInterfaces() >= 2)
    {
        const uint32_t identifier = GetIdentifierForUtUser(utUserNode);

        /*
         * Assuming that #0 is for loopback interface and #1 is for subscriber
         * network interface.
         */
        for (uint32_t i = 0; i < ipv4->GetNAddresses(1); i++)
        {
            const Address addr = ipv4->GetAddress(1, i).GetLocal();
            m_identifierMap[addr] = identifier;
            NS_LOG_INFO(this << " associated address " << addr << " with identifier "
                             << identifier);
        }
    }
    else
    {
        NS_LOG_WARN(this << " Node " << utUserNode->GetId() << " is not a valid UT user");
    }
}

// RETURN FEEDER LINK DEVICE-LEVEL ///////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsRtnFeederDevThroughputHelper);

SatStatsRtnFeederDevThroughputHelper::SatStatsRtnFeederDevThroughputHelper(
    Ptr<const SatHelper> satHelper)
    : SatStatsThroughputHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsRtnFeederDevThroughputHelper::~SatStatsRtnFeederDevThroughputHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsRtnFeederDevThroughputHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsRtnFeederDevThroughputHelper").SetParent<SatStatsThroughputHelper>();
    return tid;
}

void
SatStatsRtnFeederDevThroughputHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

    NodeContainer uts = GetSatHelper()->GetBeamHelper()->GetUtNodes();
    for (NodeContainer::Iterator it = uts.Begin(); it != uts.End(); ++it)
    {
        // Create a map of UT addresses and identifiers.
        SaveAddressAndIdentifier(*it);

        // Enable statistics-related tags and trace sources on the device.
        Ptr<NetDevice> dev = GetUtSatNetDevice(*it);
        dev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
    }

    // Connect to trace sources at GW nodes.

    NodeContainer gws = GetSatHelper()->GetBeamHelper()->GetGwNodes();
    Callback<void, Ptr<const Packet>, const Address&> callback =
        MakeCallback(&SatStatsRtnFeederDevThroughputHelper::RxCallback, this);

    for (NodeContainer::Iterator it = gws.Begin(); it != gws.End(); ++it)
    {
        NetDeviceContainer devs = GetGwSatNetDevice(*it);

        for (NetDeviceContainer::Iterator itDev = devs.Begin(); itDev != devs.End(); ++itDev)
        {
            NS_ASSERT((*itDev)->GetObject<SatNetDevice>() != nullptr);

            if ((*itDev)->TraceConnectWithoutContext("Rx", callback))
            {
                NS_LOG_INFO(this << " successfully connected with node ID " << (*it)->GetId()
                                 << " device #" << (*itDev)->GetIfIndex());

                // Enable statistics-related tags and trace sources on the device.
                (*itDev)->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            }
            else
            {
                NS_FATAL_ERROR("Error connecting to Rx trace source of SatNetDevice"
                               << " at node ID " << (*it)->GetId() << " device #"
                               << (*itDev)->GetIfIndex());
            }

        } // end of `for (NetDeviceContainer::Iterator itDev = devs)`

    } // end of `for (NodeContainer::Iterator it = gws)`

} // end of `void DoInstallProbes ();`

// RETURN USER LINK DEVICE-LEVEL ///////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsRtnUserDevThroughputHelper);

SatStatsRtnUserDevThroughputHelper::SatStatsRtnUserDevThroughputHelper(
    Ptr<const SatHelper> satHelper)
    : SatStatsThroughputHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsRtnUserDevThroughputHelper::~SatStatsRtnUserDevThroughputHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsRtnUserDevThroughputHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsRtnUserDevThroughputHelper").SetParent<SatStatsThroughputHelper>();
    return tid;
}

void
SatStatsRtnUserDevThroughputHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

    NodeContainer sats = Singleton<SatTopology>::Get()->GetOrbiterNodes();
    Callback<void, Ptr<const Packet>, const Address&> callback =
        MakeCallback(&SatStatsRtnUserMacThroughputHelper::RxCallback, this);

    for (NodeContainer::Iterator it = sats.Begin(); it != sats.End(); ++it)
    {
        Ptr<NetDevice> dev = GetSatSatOrbiterNetDevice(*it);
        Ptr<SatOrbiterNetDevice> satOrbiterDev = dev->GetObject<SatOrbiterNetDevice>();
        NS_ASSERT(satOrbiterDev != nullptr);
        satOrbiterDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));

        // Connect the object to the probe.
        if (satOrbiterDev->TraceConnectWithoutContext("RxUser", callback))
        {
            NS_LOG_INFO(this << " successfully connected with node ID " << (*it)->GetId()
                             << " device #" << satOrbiterDev->GetIfIndex());
        }
        else
        {
            NS_FATAL_ERROR("Error connecting to Rx trace source of SatMac"
                           << " at node ID " << (*it)->GetId() << " device #"
                           << satOrbiterDev->GetIfIndex());
        }
    } // end of `for (it = sats.Begin(); it != sats.End (); ++it)`

    NodeContainer uts = GetSatHelper()->GetBeamHelper()->GetUtNodes();
    for (NodeContainer::Iterator it = uts.Begin(); it != uts.End(); ++it)
    {
        // Create a map of UT addresses and identifiers.
        SaveAddressAndIdentifier(*it);

        // Enable statistics-related tags and trace sources on the device.
        Ptr<NetDevice> dev = GetUtSatNetDevice(*it);
        Ptr<SatNetDevice> satDev = dev->GetObject<SatNetDevice>();
        NS_ASSERT(satDev != nullptr);
        satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
    }

    // Connect to trace sources at GW nodes.

    NodeContainer gws = GetSatHelper()->GetBeamHelper()->GetGwNodes();

    for (NodeContainer::Iterator it = gws.Begin(); it != gws.End(); ++it)
    {
        NetDeviceContainer devs = GetGwSatNetDevice(*it);

        for (NetDeviceContainer::Iterator itDev = devs.Begin(); itDev != devs.End(); ++itDev)
        {
            Ptr<SatNetDevice> satDev = (*itDev)->GetObject<SatNetDevice>();
            NS_ASSERT(satDev != nullptr);

            satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        } // end of `for (NetDeviceContainer::Iterator itDev = devs)`

    } // end of `for (NodeContainer::Iterator it = gws)`

} // end of `void DoInstallProbes ();`

// RETURN FEEDER LINK MAC-LEVEL //////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsRtnFeederMacThroughputHelper);

SatStatsRtnFeederMacThroughputHelper::SatStatsRtnFeederMacThroughputHelper(
    Ptr<const SatHelper> satHelper)
    : SatStatsThroughputHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsRtnFeederMacThroughputHelper::~SatStatsRtnFeederMacThroughputHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsRtnFeederMacThroughputHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsRtnFeederMacThroughputHelper").SetParent<SatStatsThroughputHelper>();
    return tid;
}

void
SatStatsRtnFeederMacThroughputHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

    NodeContainer sats = Singleton<SatTopology>::Get()->GetOrbiterNodes();

    for (NodeContainer::Iterator it = sats.Begin(); it != sats.End(); ++it)
    {
        Ptr<SatMac> satMac;
        Ptr<NetDevice> dev = GetSatSatOrbiterNetDevice(*it);
        Ptr<SatOrbiterNetDevice> satOrbiterDev = dev->GetObject<SatOrbiterNetDevice>();
        NS_ASSERT(satOrbiterDev != nullptr);
        satOrbiterDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        std::map<uint32_t, Ptr<SatMac>> satOrbiterFeederMacs = satOrbiterDev->GetAllFeederMac();
        for (std::map<uint32_t, Ptr<SatMac>>::iterator it2 = satOrbiterFeederMacs.begin();
             it2 != satOrbiterFeederMacs.end();
             ++it2)
        {
            satMac = it2->second;
            NS_ASSERT(satMac != nullptr);
            satMac->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
        std::map<uint32_t, Ptr<SatMac>> satOrbiterUserMacs = satOrbiterDev->GetUserMac();
        for (std::map<uint32_t, Ptr<SatMac>>::iterator it2 = satOrbiterUserMacs.begin();
             it2 != satOrbiterUserMacs.end();
             ++it2)
        {
            satMac = it2->second;
            NS_ASSERT(satMac != nullptr);
            satMac->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
    } // end of `for (it = sats.Begin(); it != sats.End (); ++it)`

    NodeContainer uts = GetSatHelper()->GetBeamHelper()->GetUtNodes();
    for (NodeContainer::Iterator it = uts.Begin(); it != uts.End(); ++it)
    {
        // Create a map of UT addresses and identifiers.
        SaveAddressAndIdentifier(*it);

        // Enable statistics-related tags and trace sources on the device.
        Ptr<NetDevice> dev = GetUtSatNetDevice(*it);
        Ptr<SatNetDevice> satDev = dev->GetObject<SatNetDevice>();
        NS_ASSERT(satDev != nullptr);
        Ptr<SatMac> satMac = satDev->GetMac();
        NS_ASSERT(satMac != nullptr);
        satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        satMac->SetAttribute("EnableStatisticsTags", BooleanValue(true));
    }

    // Connect to trace sources at GW nodes.

    NodeContainer gws = GetSatHelper()->GetBeamHelper()->GetGwNodes();
    Callback<void, Ptr<const Packet>, const Address&> callback =
        MakeCallback(&SatStatsRtnFeederMacThroughputHelper::RxCallback, this);

    for (NodeContainer::Iterator it = gws.Begin(); it != gws.End(); ++it)
    {
        NetDeviceContainer devs = GetGwSatNetDevice(*it);

        for (NetDeviceContainer::Iterator itDev = devs.Begin(); itDev != devs.End(); ++itDev)
        {
            Ptr<SatNetDevice> satDev = (*itDev)->GetObject<SatNetDevice>();
            NS_ASSERT(satDev != nullptr);
            Ptr<SatMac> satMac = satDev->GetMac();
            NS_ASSERT(satMac != nullptr);

            // Connect the object to the probe.
            if (satMac->TraceConnectWithoutContext("Rx", callback))
            {
                NS_LOG_INFO(this << " successfully connected with node ID " << (*it)->GetId()
                                 << " device #" << satDev->GetIfIndex());

                // Enable statistics-related tags and trace sources on the device.
                satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
                satMac->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            }
            else
            {
                NS_FATAL_ERROR("Error connecting to Rx trace source of SatMac"
                               << " at node ID " << (*it)->GetId() << " device #"
                               << satDev->GetIfIndex());
            }

        } // end of `for (NetDeviceContainer::Iterator itDev = devs)`

    } // end of `for (NodeContainer::Iterator it = gws)`

} // end of `void DoInstallProbes ();`

// RETURN USER LINK MAC-LEVEL //////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsRtnUserMacThroughputHelper);

SatStatsRtnUserMacThroughputHelper::SatStatsRtnUserMacThroughputHelper(
    Ptr<const SatHelper> satHelper)
    : SatStatsThroughputHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsRtnUserMacThroughputHelper::~SatStatsRtnUserMacThroughputHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsRtnUserMacThroughputHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsRtnUserMacThroughputHelper").SetParent<SatStatsThroughputHelper>();
    return tid;
}

void
SatStatsRtnUserMacThroughputHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

    NodeContainer sats = Singleton<SatTopology>::Get()->GetOrbiterNodes();
    Callback<void, Ptr<const Packet>, const Address&> callback =
        MakeCallback(&SatStatsRtnUserMacThroughputHelper::RxCallback, this);

    for (NodeContainer::Iterator it = sats.Begin(); it != sats.End(); ++it)
    {
        Ptr<NetDevice> dev = GetSatSatOrbiterNetDevice(*it);
        Ptr<SatOrbiterNetDevice> satOrbiterDev = dev->GetObject<SatOrbiterNetDevice>();
        NS_ASSERT(satOrbiterDev != nullptr);
        satOrbiterDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        std::map<uint32_t, Ptr<SatMac>> satOrbiterFeederMacs = satOrbiterDev->GetAllFeederMac();
        Ptr<SatMac> satMac;
        for (std::map<uint32_t, Ptr<SatMac>>::iterator it2 = satOrbiterFeederMacs.begin();
             it2 != satOrbiterFeederMacs.end();
             ++it2)
        {
            satMac = it2->second;
            NS_ASSERT(satMac != nullptr);
            satMac->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
        std::map<uint32_t, Ptr<SatMac>> satOrbiterUserMacs = satOrbiterDev->GetUserMac();
        for (std::map<uint32_t, Ptr<SatMac>>::iterator it2 = satOrbiterUserMacs.begin();
             it2 != satOrbiterUserMacs.end();
             ++it2)
        {
            satMac = it2->second;
            NS_ASSERT(satMac != nullptr);
            satMac->SetAttribute("EnableStatisticsTags", BooleanValue(true));

            // Connect the object to the probe.
            if (satMac->TraceConnectWithoutContext("Rx", callback))
            {
                NS_LOG_INFO(this << " successfully connected with node ID " << (*it)->GetId()
                                 << " device #" << satOrbiterDev->GetIfIndex());
            }
            else
            {
                NS_FATAL_ERROR("Error connecting to Rx trace source of SatMac"
                               << " at node ID " << (*it)->GetId() << " device #"
                               << satOrbiterDev->GetIfIndex());
            }
        }
    } // end of `for (it = sats.Begin(); it != sats.End (); ++it)`

    NodeContainer uts = GetSatHelper()->GetBeamHelper()->GetUtNodes();
    for (NodeContainer::Iterator it = uts.Begin(); it != uts.End(); ++it)
    {
        // Create a map of UT addresses and identifiers.
        SaveAddressAndIdentifier(*it);

        // Enable statistics-related tags and trace sources on the device.
        Ptr<NetDevice> dev = GetUtSatNetDevice(*it);
        Ptr<SatNetDevice> satDev = dev->GetObject<SatNetDevice>();
        NS_ASSERT(satDev != nullptr);
        Ptr<SatMac> satMac = satDev->GetMac();
        NS_ASSERT(satMac != nullptr);
        satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        satMac->SetAttribute("EnableStatisticsTags", BooleanValue(true));
    }

    // Connect to trace sources at GW nodes.

    NodeContainer gws = GetSatHelper()->GetBeamHelper()->GetGwNodes();

    for (NodeContainer::Iterator it = gws.Begin(); it != gws.End(); ++it)
    {
        NetDeviceContainer devs = GetGwSatNetDevice(*it);

        for (NetDeviceContainer::Iterator itDev = devs.Begin(); itDev != devs.End(); ++itDev)
        {
            Ptr<SatNetDevice> satDev = (*itDev)->GetObject<SatNetDevice>();
            NS_ASSERT(satDev != nullptr);
            Ptr<SatMac> satMac = satDev->GetMac();
            NS_ASSERT(satMac != nullptr);

            satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            satMac->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        } // end of `for (NetDeviceContainer::Iterator itDev = devs)`

    } // end of `for (NodeContainer::Iterator it = gws)`

} // end of `void DoInstallProbes ();`

// RETURN FEEDER LINK PHY-LEVEL //////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsRtnFeederPhyThroughputHelper);

SatStatsRtnFeederPhyThroughputHelper::SatStatsRtnFeederPhyThroughputHelper(
    Ptr<const SatHelper> satHelper)
    : SatStatsThroughputHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsRtnFeederPhyThroughputHelper::~SatStatsRtnFeederPhyThroughputHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsRtnFeederPhyThroughputHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsRtnFeederPhyThroughputHelper").SetParent<SatStatsThroughputHelper>();
    return tid;
}

void
SatStatsRtnFeederPhyThroughputHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

    NodeContainer sats = Singleton<SatTopology>::Get()->GetOrbiterNodes();

    for (NodeContainer::Iterator it = sats.Begin(); it != sats.End(); ++it)
    {
        Ptr<SatPhy> satPhy;
        Ptr<NetDevice> dev = GetSatSatOrbiterNetDevice(*it);
        Ptr<SatOrbiterNetDevice> satOrbiterDev = dev->GetObject<SatOrbiterNetDevice>();
        NS_ASSERT(satOrbiterDev != nullptr);
        satOrbiterDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        std::map<uint32_t, Ptr<SatPhy>> satOrbiterFeederPhys = satOrbiterDev->GetFeederPhy();
        for (std::map<uint32_t, Ptr<SatPhy>>::iterator it2 = satOrbiterFeederPhys.begin();
             it2 != satOrbiterFeederPhys.end();
             ++it2)
        {
            satPhy = it2->second;
            NS_ASSERT(satPhy != nullptr);
            satPhy->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
        std::map<uint32_t, Ptr<SatPhy>> satOrbiterUserPhys = satOrbiterDev->GetUserPhy();
        for (std::map<uint32_t, Ptr<SatPhy>>::iterator it2 = satOrbiterUserPhys.begin();
             it2 != satOrbiterUserPhys.end();
             ++it2)
        {
            satPhy = it2->second;
            NS_ASSERT(satPhy != nullptr);
            satPhy->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
    } // end of `for (it = sats.Begin(); it != sats.End (); ++it)`

    NodeContainer uts = GetSatHelper()->GetBeamHelper()->GetUtNodes();
    for (NodeContainer::Iterator it = uts.Begin(); it != uts.End(); ++it)
    {
        // Create a map of UT addresses and identifiers.
        SaveAddressAndIdentifier(*it);

        // Enable statistics-related tags and trace sources on the device.
        Ptr<NetDevice> dev = GetUtSatNetDevice(*it);
        Ptr<SatNetDevice> satDev = dev->GetObject<SatNetDevice>();
        NS_ASSERT(satDev != nullptr);
        Ptr<SatPhy> satPhy = satDev->GetPhy();
        NS_ASSERT(satPhy != nullptr);
        satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        satPhy->SetAttribute("EnableStatisticsTags", BooleanValue(true));
    }

    // Connect to trace sources at GW nodes.

    NodeContainer gws = GetSatHelper()->GetBeamHelper()->GetGwNodes();
    Callback<void, Ptr<const Packet>, const Address&> callback =
        MakeCallback(&SatStatsRtnFeederPhyThroughputHelper::RxCallback, this);

    for (NodeContainer::Iterator it = gws.Begin(); it != gws.End(); ++it)
    {
        NetDeviceContainer devs = GetGwSatNetDevice(*it);

        for (NetDeviceContainer::Iterator itDev = devs.Begin(); itDev != devs.End(); ++itDev)
        {
            Ptr<SatNetDevice> satDev = (*itDev)->GetObject<SatNetDevice>();
            NS_ASSERT(satDev != nullptr);
            Ptr<SatPhy> satPhy = satDev->GetPhy();
            NS_ASSERT(satPhy != nullptr);

            // Connect the object to the probe.
            if (satPhy->TraceConnectWithoutContext("Rx", callback))
            {
                NS_LOG_INFO(this << " successfully connected with node ID " << (*it)->GetId()
                                 << " device #" << satDev->GetIfIndex());

                // Enable statistics-related tags and trace sources on the device.
                satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
                satPhy->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            }
            else
            {
                NS_FATAL_ERROR("Error connecting to Rx trace source of SatPhy"
                               << " at node ID " << (*it)->GetId() << " device #"
                               << satDev->GetIfIndex());
            }

        } // end of `for (NetDeviceContainer::Iterator itDev = devs)`

    } // end of `for (NodeContainer::Iterator it = gws)`

} // end of `void DoInstallProbes ();`

// RETURN USER LINK PHY-LEVEL //////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsRtnUserPhyThroughputHelper);

SatStatsRtnUserPhyThroughputHelper::SatStatsRtnUserPhyThroughputHelper(
    Ptr<const SatHelper> satHelper)
    : SatStatsThroughputHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsRtnUserPhyThroughputHelper::~SatStatsRtnUserPhyThroughputHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsRtnUserPhyThroughputHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsRtnUserPhyThroughputHelper").SetParent<SatStatsThroughputHelper>();
    return tid;
}

void
SatStatsRtnUserPhyThroughputHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

    NodeContainer sats = Singleton<SatTopology>::Get()->GetOrbiterNodes();
    Callback<void, Ptr<const Packet>, const Address&> callback =
        MakeCallback(&SatStatsRtnUserPhyThroughputHelper::RxCallback, this);

    for (NodeContainer::Iterator it = sats.Begin(); it != sats.End(); ++it)
    {
        Ptr<NetDevice> dev = GetSatSatOrbiterNetDevice(*it);
        Ptr<SatOrbiterNetDevice> satOrbiterDev = dev->GetObject<SatOrbiterNetDevice>();
        NS_ASSERT(satOrbiterDev != nullptr);
        satOrbiterDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        std::map<uint32_t, Ptr<SatPhy>> satOrbiterFeederPhys = satOrbiterDev->GetFeederPhy();
        Ptr<SatPhy> satPhy;
        for (std::map<uint32_t, Ptr<SatPhy>>::iterator it2 = satOrbiterFeederPhys.begin();
             it2 != satOrbiterFeederPhys.end();
             ++it2)
        {
            satPhy = it2->second;
            NS_ASSERT(satPhy != nullptr);
            satPhy->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
        std::map<uint32_t, Ptr<SatPhy>> satOrbiterUserPhys = satOrbiterDev->GetUserPhy();
        for (std::map<uint32_t, Ptr<SatPhy>>::iterator it2 = satOrbiterUserPhys.begin();
             it2 != satOrbiterUserPhys.end();
             ++it2)
        {
            satPhy = it2->second;
            NS_ASSERT(satPhy != nullptr);
            satPhy->SetAttribute("EnableStatisticsTags", BooleanValue(true));

            // Connect the object to the probe.
            if (satPhy->TraceConnectWithoutContext("Rx", callback))
            {
                NS_LOG_INFO(this << " successfully connected with node ID " << (*it)->GetId()
                                 << " device #" << satOrbiterDev->GetIfIndex());
            }
            else
            {
                NS_FATAL_ERROR("Error connecting to Rx trace source of SatPhy"
                               << " at node ID " << (*it)->GetId() << " device #"
                               << satOrbiterDev->GetIfIndex());
            }
        }
    } // end of `for (it = sats.Begin(); it != sats.End (); ++it)`

    NodeContainer uts = GetSatHelper()->GetBeamHelper()->GetUtNodes();
    for (NodeContainer::Iterator it = uts.Begin(); it != uts.End(); ++it)
    {
        // Create a map of UT addresses and identifiers.
        SaveAddressAndIdentifier(*it);

        // Enable statistics-related tags and trace sources on the device.
        Ptr<NetDevice> dev = GetUtSatNetDevice(*it);
        Ptr<SatNetDevice> satDev = dev->GetObject<SatNetDevice>();
        NS_ASSERT(satDev != nullptr);
        Ptr<SatPhy> satPhy = satDev->GetPhy();
        NS_ASSERT(satPhy != nullptr);
        satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        satPhy->SetAttribute("EnableStatisticsTags", BooleanValue(true));
    }

    // Connect to trace sources at GW nodes.

    NodeContainer gws = GetSatHelper()->GetBeamHelper()->GetGwNodes();

    for (NodeContainer::Iterator it = gws.Begin(); it != gws.End(); ++it)
    {
        NetDeviceContainer devs = GetGwSatNetDevice(*it);

        for (NetDeviceContainer::Iterator itDev = devs.Begin(); itDev != devs.End(); ++itDev)
        {
            Ptr<SatNetDevice> satDev = (*itDev)->GetObject<SatNetDevice>();
            NS_ASSERT(satDev != nullptr);
            Ptr<SatPhy> satPhy = satDev->GetPhy();
            NS_ASSERT(satPhy != nullptr);

            satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            satPhy->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        } // end of `for (NetDeviceContainer::Iterator itDev = devs)`

    } // end of `for (NodeContainer::Iterator it = gws)`

} // end of `void DoInstallProbes ();`

} // end of namespace ns3
