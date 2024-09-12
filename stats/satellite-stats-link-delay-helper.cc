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

#include "satellite-stats-link-delay-helper.h"

#include <ns3/application-delay-probe.h>
#include <ns3/application.h>
#include <ns3/boolean.h>
#include <ns3/callback.h>
#include <ns3/data-collection-object.h>
#include <ns3/distribution-collector.h>
#include <ns3/enum.h>
#include <ns3/inet-socket-address.h>
#include <ns3/ipv4.h>
#include <ns3/log.h>
#include <ns3/mac48-address.h>
#include <ns3/magister-gnuplot-aggregator.h>
#include <ns3/multi-file-aggregator.h>
#include <ns3/net-device.h>
#include <ns3/node-container.h>
#include <ns3/nstime.h>
#include <ns3/probe.h>
#include <ns3/satellite-helper.h>
#include <ns3/satellite-id-mapper.h>
#include <ns3/satellite-mac.h>
#include <ns3/satellite-net-device.h>
#include <ns3/satellite-orbiter-net-device.h>
#include <ns3/satellite-phy.h>
#include <ns3/satellite-time-tag.h>
#include <ns3/satellite-topology.h>
#include <ns3/scalar-collector.h>
#include <ns3/singleton.h>
#include <ns3/string.h>
#include <ns3/traffic-time-tag.h>
#include <ns3/unit-conversion-collector.h>

#include <sstream>

NS_LOG_COMPONENT_DEFINE("SatStatsLinkDelayHelper");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(SatStatsLinkDelayHelper);

SatStatsLinkDelayHelper::SatStatsLinkDelayHelper(Ptr<const SatHelper> satHelper)
    : SatStatsHelper(satHelper),
      m_averagingMode(false)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsLinkDelayHelper::~SatStatsLinkDelayHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsLinkDelayHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsLinkDelayHelper")
            .SetParent<SatStatsHelper>()
            .AddAttribute("AveragingMode",
                          "If true, all samples will be averaged before passed to aggregator. "
                          "Only affects histogram, PDF, and CDF output types.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&SatStatsLinkDelayHelper::SetAveragingMode,
                                              &SatStatsLinkDelayHelper::GetAveragingMode),
                          MakeBooleanChecker());
    return tid;
}

void
SatStatsLinkDelayHelper::SetAveragingMode(bool averagingMode)
{
    NS_LOG_FUNCTION(this << averagingMode);
    m_averagingMode = averagingMode;
}

bool
SatStatsLinkDelayHelper::GetAveragingMode() const
{
    return m_averagingMode;
}

void
SatStatsLinkDelayHelper::DoInstall()
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
                                        StringValue(GetIdentifierHeading("delay_sec")));

        // Setup collectors.
        m_terminalCollectors.SetType("ns3::ScalarCollector");
        m_terminalCollectors.SetAttribute("InputDataType",
                                          EnumValue(ScalarCollector::INPUT_DATA_TYPE_DOUBLE));
        m_terminalCollectors.SetAttribute(
            "OutputType",
            EnumValue(ScalarCollector::OUTPUT_TYPE_AVERAGE_PER_SAMPLE));
        CreateCollectorPerIdentifier(m_terminalCollectors);
        m_terminalCollectors.ConnectToAggregator("Output",
                                                 m_aggregator,
                                                 &MultiFileAggregator::Write1d);
        break;
    }

    case SatStatsHelper::OUTPUT_SCATTER_FILE: {
        // Setup aggregator.
        m_aggregator = CreateAggregator("ns3::MultiFileAggregator",
                                        "OutputFileName",
                                        StringValue(GetOutputFileName()),
                                        "GeneralHeading",
                                        StringValue(GetTimeHeading("delay_sec")));

        // Setup collectors.
        m_terminalCollectors.SetType("ns3::UnitConversionCollector");
        m_terminalCollectors.SetAttribute("ConversionType",
                                          EnumValue(UnitConversionCollector::TRANSPARENT));
        CreateCollectorPerIdentifier(m_terminalCollectors);
        m_terminalCollectors.ConnectToAggregator("OutputTimeValue",
                                                 m_aggregator,
                                                 &MultiFileAggregator::Write2d);
        break;
    }

    case SatStatsHelper::OUTPUT_HISTOGRAM_FILE:
    case SatStatsHelper::OUTPUT_PDF_FILE:
    case SatStatsHelper::OUTPUT_CDF_FILE: {
        if (m_averagingMode)
        {
            // Setup aggregator.
            m_aggregator = CreateAggregator("ns3::MultiFileAggregator",
                                            "OutputFileName",
                                            StringValue(GetOutputFileName()),
                                            "MultiFileMode",
                                            BooleanValue(false),
                                            "EnableContextPrinting",
                                            BooleanValue(false),
                                            "GeneralHeading",
                                            StringValue(GetDistributionHeading("delay_sec")));
            Ptr<MultiFileAggregator> fileAggregator =
                m_aggregator->GetObject<MultiFileAggregator>();
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

            // Setup collectors.
            m_terminalCollectors.SetType("ns3::ScalarCollector");
            m_terminalCollectors.SetAttribute("InputDataType",
                                              EnumValue(ScalarCollector::INPUT_DATA_TYPE_DOUBLE));
            m_terminalCollectors.SetAttribute(
                "OutputType",
                EnumValue(ScalarCollector::OUTPUT_TYPE_AVERAGE_PER_SAMPLE));
            CreateCollectorPerIdentifier(m_terminalCollectors);
            Callback<void, double> callback =
                MakeCallback(&DistributionCollector::TraceSinkDouble1, m_averagingCollector);
            for (CollectorMap::Iterator it = m_terminalCollectors.Begin();
                 it != m_terminalCollectors.End();
                 ++it)
            {
                it->second->TraceConnectWithoutContext("Output", callback);
            }
        }
        else
        {
            // Setup aggregator.
            m_aggregator = CreateAggregator("ns3::MultiFileAggregator",
                                            "OutputFileName",
                                            StringValue(GetOutputFileName()),
                                            "GeneralHeading",
                                            StringValue(GetDistributionHeading("delay_sec")));

            // Setup collectors.
            m_terminalCollectors.SetType("ns3::DistributionCollector");
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
            m_terminalCollectors.SetAttribute("OutputType", EnumValue(outputType));
            CreateCollectorPerIdentifier(m_terminalCollectors);
            m_terminalCollectors.ConnectToAggregator("Output",
                                                     m_aggregator,
                                                     &MultiFileAggregator::Write2d);
            m_terminalCollectors.ConnectToAggregator("OutputString",
                                                     m_aggregator,
                                                     &MultiFileAggregator::AddContextHeading);
            m_terminalCollectors.ConnectToAggregator("Warning",
                                                     m_aggregator,
                                                     &MultiFileAggregator::EnableContextWarning);
        }

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
        plotAggregator->SetLegend("Time (in seconds)", "Packet delay (in seconds)");
        plotAggregator->Set2dDatasetDefaultStyle(Gnuplot2dDataset::LINES);

        // Setup collectors.
        m_terminalCollectors.SetType("ns3::UnitConversionCollector");
        m_terminalCollectors.SetAttribute("ConversionType",
                                          EnumValue(UnitConversionCollector::TRANSPARENT));
        CreateCollectorPerIdentifier(m_terminalCollectors);
        for (CollectorMap::Iterator it = m_terminalCollectors.Begin();
             it != m_terminalCollectors.End();
             ++it)
        {
            const std::string context = it->second->GetName();
            plotAggregator->Add2dDataset(context, context);
        }
        m_terminalCollectors.ConnectToAggregator("OutputTimeValue",
                                                 m_aggregator,
                                                 &MagisterGnuplotAggregator::Write2d);
        break;
    }

    case SatStatsHelper::OUTPUT_HISTOGRAM_PLOT:
    case SatStatsHelper::OUTPUT_PDF_PLOT:
    case SatStatsHelper::OUTPUT_CDF_PLOT: {
        if (m_averagingMode)
        {
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
            plotAggregator->SetLegend("Packet delay (in seconds)", "Frequency");
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

            // Setup collectors.
            m_terminalCollectors.SetType("ns3::ScalarCollector");
            m_terminalCollectors.SetAttribute("InputDataType",
                                              EnumValue(ScalarCollector::INPUT_DATA_TYPE_DOUBLE));
            m_terminalCollectors.SetAttribute(
                "OutputType",
                EnumValue(ScalarCollector::OUTPUT_TYPE_AVERAGE_PER_SAMPLE));
            CreateCollectorPerIdentifier(m_terminalCollectors);
            Callback<void, double> callback =
                MakeCallback(&DistributionCollector::TraceSinkDouble1, m_averagingCollector);
            for (CollectorMap::Iterator it = m_terminalCollectors.Begin();
                 it != m_terminalCollectors.End();
                 ++it)
            {
                it->second->TraceConnectWithoutContext("Output", callback);
            }
        }
        else
        {
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
            plotAggregator->SetLegend("Packet delay (in seconds)", "Frequency");
            plotAggregator->Set2dDatasetDefaultStyle(Gnuplot2dDataset::LINES);

            // Setup collectors.
            m_terminalCollectors.SetType("ns3::DistributionCollector");
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
            m_terminalCollectors.SetAttribute("OutputType", EnumValue(outputType));
            CreateCollectorPerIdentifier(m_terminalCollectors);
            for (CollectorMap::Iterator it = m_terminalCollectors.Begin();
                 it != m_terminalCollectors.End();
                 ++it)
            {
                const std::string context = it->second->GetName();
                plotAggregator->Add2dDataset(context, context);
            }
            m_terminalCollectors.ConnectToAggregator("Output",
                                                     m_aggregator,
                                                     &MagisterGnuplotAggregator::Write2d);
        }

        break;
    }

    default:
        NS_FATAL_ERROR("SatStatsLinkDelayHelper - Invalid output type");
        break;
    }

    // Setup probes and connect them to the collectors.
    InstallProbes();

} // end of `void DoInstall ();`

void
SatStatsLinkDelayHelper::InstallProbes()
{
    // The method below is supposed to be implemented by the child class.
    DoInstallProbes();
}

void
SatStatsLinkDelayHelper::RxLinkDelayCallback(const Time& delay, const Address& from)
{
    // NS_LOG_FUNCTION (this << delay.GetSeconds () << from);

    if (from.IsInvalid())
    {
        NS_LOG_WARN(this << " discarding a packet delay of " << delay.GetSeconds()
                         << " from statistics collection because of"
                         << " invalid sender address");
    }
    else if (Mac48Address::ConvertFrom(from).IsBroadcast())
    {
        for (std::pair<const Address, uint32_t> item : m_identifierMap)
        {
            PassSampleToCollector(delay, item.second);
        }
    }
    else
    {
        // Determine the identifier associated with the sender address.
        std::map<const Address, uint32_t>::const_iterator it = m_identifierMap.find(from);

        if (it != m_identifierMap.end())
        {
            PassSampleToCollector(delay, it->second);
        }
        else
        {
            NS_LOG_WARN(this << " discarding a packet delay of " << delay.GetSeconds()
                             << " from statistics collection because of"
                             << " unknown sender address " << from);
        }
    }
}

bool
SatStatsLinkDelayHelper::ConnectProbeToCollector(Ptr<Probe> probe, uint32_t identifier)
{
    NS_LOG_FUNCTION(this << probe << probe->GetName() << identifier);

    bool ret = false;
    switch (GetOutputType())
    {
    case SatStatsHelper::OUTPUT_SCALAR_FILE:
    case SatStatsHelper::OUTPUT_SCALAR_PLOT:
        ret = m_terminalCollectors.ConnectWithProbe(probe,
                                                    "OutputSeconds",
                                                    identifier,
                                                    &ScalarCollector::TraceSinkDouble);
        break;

    case SatStatsHelper::OUTPUT_SCATTER_FILE:
    case SatStatsHelper::OUTPUT_SCATTER_PLOT:
        ret = m_terminalCollectors.ConnectWithProbe(probe,
                                                    "OutputSeconds",
                                                    identifier,
                                                    &UnitConversionCollector::TraceSinkDouble);
        break;

    case SatStatsHelper::OUTPUT_HISTOGRAM_FILE:
    case SatStatsHelper::OUTPUT_HISTOGRAM_PLOT:
    case SatStatsHelper::OUTPUT_PDF_FILE:
    case SatStatsHelper::OUTPUT_PDF_PLOT:
    case SatStatsHelper::OUTPUT_CDF_FILE:
    case SatStatsHelper::OUTPUT_CDF_PLOT:
        if (m_averagingMode)
        {
            ret = m_terminalCollectors.ConnectWithProbe(probe,
                                                        "OutputSeconds",
                                                        identifier,
                                                        &ScalarCollector::TraceSinkDouble);
        }
        else
        {
            ret = m_terminalCollectors.ConnectWithProbe(probe,
                                                        "OutputSeconds",
                                                        identifier,
                                                        &DistributionCollector::TraceSinkDouble);
        }
        break;

    default:
        NS_FATAL_ERROR(GetOutputTypeName(GetOutputType())
                       << " is not a valid output type for this statistics.");
        break;
    }

    if (ret)
    {
        NS_LOG_INFO(this << " created probe " << probe->GetName() << ", connected to collector "
                         << identifier);
    }
    else
    {
        NS_LOG_WARN(this << " unable to connect probe " << probe->GetName() << " to collector "
                         << identifier);
    }

    return ret;
}

bool
SatStatsLinkDelayHelper::DisconnectProbeFromCollector(Ptr<Probe> probe, uint32_t identifier)
{
    NS_LOG_FUNCTION(this << probe << probe->GetName() << identifier);

    bool ret = false;
    switch (GetOutputType())
    {
    case SatStatsHelper::OUTPUT_SCALAR_FILE:
    case SatStatsHelper::OUTPUT_SCALAR_PLOT:
        ret = m_terminalCollectors.DisconnectWithProbe(probe,
                                                       "OutputSeconds",
                                                       identifier,
                                                       &ScalarCollector::TraceSinkDouble);
        break;

    case SatStatsHelper::OUTPUT_SCATTER_FILE:
    case SatStatsHelper::OUTPUT_SCATTER_PLOT:
        ret = m_terminalCollectors.DisconnectWithProbe(probe,
                                                       "OutputSeconds",
                                                       identifier,
                                                       &UnitConversionCollector::TraceSinkDouble);
        break;

    case SatStatsHelper::OUTPUT_HISTOGRAM_FILE:
    case SatStatsHelper::OUTPUT_HISTOGRAM_PLOT:
    case SatStatsHelper::OUTPUT_PDF_FILE:
    case SatStatsHelper::OUTPUT_PDF_PLOT:
    case SatStatsHelper::OUTPUT_CDF_FILE:
    case SatStatsHelper::OUTPUT_CDF_PLOT:
        if (m_averagingMode)
        {
            ret = m_terminalCollectors.DisconnectWithProbe(probe,
                                                           "OutputSeconds",
                                                           identifier,
                                                           &ScalarCollector::TraceSinkDouble);
        }
        else
        {
            ret = m_terminalCollectors.DisconnectWithProbe(probe,
                                                           "OutputSeconds",
                                                           identifier,
                                                           &DistributionCollector::TraceSinkDouble);
        }
        break;

    default:
        NS_FATAL_ERROR(GetOutputTypeName(GetOutputType())
                       << " is not a valid output type for this statistics.");
        break;
    }

    if (ret)
    {
        NS_LOG_INFO(this << " probe " << probe->GetName() << ", disconnected from collector "
                         << identifier);
    }
    else
    {
        NS_LOG_WARN(this << " unable to disconnect probe " << probe->GetName() << " from collector "
                         << identifier);
    }

    return ret;
}

void
SatStatsLinkDelayHelper::PassSampleToCollector(const Time& delay, uint32_t identifier)
{
    // NS_LOG_FUNCTION (this << delay.GetSeconds () << identifier);

    Ptr<DataCollectionObject> collector = m_terminalCollectors.Get(identifier);
    NS_ASSERT_MSG(collector != nullptr, "Unable to find collector with identifier " << identifier);

    switch (GetOutputType())
    {
    case SatStatsHelper::OUTPUT_SCALAR_FILE:
    case SatStatsHelper::OUTPUT_SCALAR_PLOT: {
        Ptr<ScalarCollector> c = collector->GetObject<ScalarCollector>();
        NS_ASSERT(c != nullptr);
        c->TraceSinkDouble(0.0, delay.GetSeconds());
        break;
    }

    case SatStatsHelper::OUTPUT_SCATTER_FILE:
    case SatStatsHelper::OUTPUT_SCATTER_PLOT: {
        Ptr<UnitConversionCollector> c = collector->GetObject<UnitConversionCollector>();
        NS_ASSERT(c != nullptr);
        c->TraceSinkDouble(0.0, delay.GetSeconds());
        break;
    }

    case SatStatsHelper::OUTPUT_HISTOGRAM_FILE:
    case SatStatsHelper::OUTPUT_HISTOGRAM_PLOT:
    case SatStatsHelper::OUTPUT_PDF_FILE:
    case SatStatsHelper::OUTPUT_PDF_PLOT:
    case SatStatsHelper::OUTPUT_CDF_FILE:
    case SatStatsHelper::OUTPUT_CDF_PLOT:
        if (m_averagingMode)
        {
            Ptr<ScalarCollector> c = collector->GetObject<ScalarCollector>();
            NS_ASSERT(c != nullptr);
            c->TraceSinkDouble(0.0, delay.GetSeconds());
        }
        else
        {
            Ptr<DistributionCollector> c = collector->GetObject<DistributionCollector>();
            NS_ASSERT(c != nullptr);
            c->TraceSinkDouble(0.0, delay.GetSeconds());
        }
        break;

    default:
        NS_FATAL_ERROR(GetOutputTypeName(GetOutputType())
                       << " is not a valid output type for this statistics.");
        break;

    } // end of `switch (GetOutputType ())`

} // end of `void PassSampleToCollector (Time, uint32_t)`

// FORWARD FEEDER LINK DEV-LEVEL /////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsFwdFeederDevLinkDelayHelper);

SatStatsFwdFeederDevLinkDelayHelper::SatStatsFwdFeederDevLinkDelayHelper(
    Ptr<const SatHelper> satHelper)
    : SatStatsLinkDelayHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsFwdFeederDevLinkDelayHelper::~SatStatsFwdFeederDevLinkDelayHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsFwdFeederDevLinkDelayHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsFwdFeederDevLinkDelayHelper").SetParent<SatStatsLinkDelayHelper>();
    return tid;
}

void
SatStatsFwdFeederDevLinkDelayHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

    NodeContainer sats = Singleton<SatTopology>::Get()->GetOrbiterNodes();
    Callback<void, const Time&, const Address&> callback =
        MakeCallback(&SatStatsFwdFeederDevLinkDelayHelper::RxLinkDelayCallback, this);

    for (NodeContainer::Iterator it = sats.Begin(); it != sats.End(); ++it)
    {
        Ptr<NetDevice> dev = GetSatSatOrbiterNetDevice(*it);
        Ptr<SatOrbiterNetDevice> satOrbiterDev = dev->GetObject<SatOrbiterNetDevice>();
        NS_ASSERT(satOrbiterDev != nullptr);
        satOrbiterDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));

        // Connect the object to the probe.
        if (satOrbiterDev->TraceConnectWithoutContext("RxFeederLinkDelay", callback))
        {
            NS_LOG_INFO(this << " successfully connected with node ID " << (*it)->GetId()
                             << " device #" << satOrbiterDev->GetIfIndex());
        }
        else
        {
            NS_FATAL_ERROR("Error connecting to RxFeederLinkDelay trace source of SatNetDevice"
                           << " at node ID " << (*it)->GetId() << " device #"
                           << satOrbiterDev->GetIfIndex());
        }
    } // end of `for (it = sats.Begin(); it != sats.End (); ++it)`

    NodeContainer uts = Singleton<SatTopology>::Get()->GetUtNodes();

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
    NodeContainer gws = Singleton<SatTopology>::Get()->GetGwNodes();
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

// FORWARD USER LINK DEV-LEVEL /////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsFwdUserDevLinkDelayHelper);

SatStatsFwdUserDevLinkDelayHelper::SatStatsFwdUserDevLinkDelayHelper(Ptr<const SatHelper> satHelper)
    : SatStatsLinkDelayHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsFwdUserDevLinkDelayHelper::~SatStatsFwdUserDevLinkDelayHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsFwdUserDevLinkDelayHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsFwdUserDevLinkDelayHelper").SetParent<SatStatsLinkDelayHelper>();
    return tid;
}

void
SatStatsFwdUserDevLinkDelayHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

    NodeContainer sats = Singleton<SatTopology>::Get()->GetOrbiterNodes();

    for (NodeContainer::Iterator it = sats.Begin(); it != sats.End(); ++it)
    {
        Ptr<NetDevice> dev = GetSatSatOrbiterNetDevice(*it);
        Ptr<SatOrbiterNetDevice> satOrbiterDev = dev->GetObject<SatOrbiterNetDevice>();
        satOrbiterDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
    } // end of `for (it = sats.Begin(); it != sats.End (); ++it)`

    NodeContainer uts = Singleton<SatTopology>::Get()->GetUtNodes();

    for (NodeContainer::Iterator it = uts.Begin(); it != uts.End(); ++it)
    {
        const int32_t utId = GetUtId(*it);
        NS_ASSERT_MSG(utId > 0, "Node " << (*it)->GetId() << " is not a valid UT");
        const uint32_t identifier = GetIdentifierForUt(*it);

        // Create the probe.
        std::ostringstream probeName;
        probeName << utId;
        Ptr<ApplicationDelayProbe> probe = CreateObject<ApplicationDelayProbe>();
        probe->SetName(probeName.str());

        Ptr<NetDevice> dev = GetUtSatNetDevice(*it);
        Ptr<SatNetDevice> satDev = dev->GetObject<SatNetDevice>();
        NS_ASSERT(satDev != nullptr);

        // Connect the object to the probe.
        if (probe->ConnectByObject("RxLinkDelay", satDev) &&
            ConnectProbeToCollector(probe, identifier))
        {
            m_probes.insert(
                std::make_pair(probe->GetObject<Probe>(), std::make_pair(*it, identifier)));

            // Enable statistics-related tags and trace sources on the device.
            satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
        else
        {
            NS_FATAL_ERROR("Error connecting to RxLinkDelay trace source of SatMac"
                           << " at node ID " << (*it)->GetId() << " device #2");
        }

    } // end of `for (it = uts.Begin(); it != uts.End (); ++it)`

    // Enable statistics-related tags on the transmitting device.
    NodeContainer gws = Singleton<SatTopology>::Get()->GetGwNodes();
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

void
SatStatsFwdUserDevLinkDelayHelper::UpdateIdentifierOnProbes()
{
    NS_LOG_FUNCTION(this);

    std::map<Ptr<Probe>, std::pair<Ptr<Node>, uint32_t>>::iterator it;

    for (it = m_probes.begin(); it != m_probes.end(); it++)
    {
        Ptr<Probe> probe = it->first;
        Ptr<Node> node = it->second.first;
        uint32_t identifier = it->second.second;

        if (!DisconnectProbeFromCollector(probe, identifier))
        {
            NS_FATAL_ERROR("Error disconnecting trace file on handover");
        }

        identifier = GetIdentifierForUt(node);

        if (!ConnectProbeToCollector(probe, identifier))
        {
            NS_FATAL_ERROR("Error connecting trace file on handover");
        }

        it->second.second = identifier;
    }
} // end of `void UpdateIdentifierOnProbes ();`

// FORWARD FEEDER LINK MAC-LEVEL /////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsFwdFeederMacLinkDelayHelper);

SatStatsFwdFeederMacLinkDelayHelper::SatStatsFwdFeederMacLinkDelayHelper(
    Ptr<const SatHelper> satHelper)
    : SatStatsLinkDelayHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsFwdFeederMacLinkDelayHelper::~SatStatsFwdFeederMacLinkDelayHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsFwdFeederMacLinkDelayHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsFwdFeederMacLinkDelayHelper").SetParent<SatStatsLinkDelayHelper>();
    return tid;
}

void
SatStatsFwdFeederMacLinkDelayHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

    NodeContainer sats = Singleton<SatTopology>::Get()->GetOrbiterNodes();
    Callback<void, const Time&, const Address&> callback =
        MakeCallback(&SatStatsFwdFeederMacLinkDelayHelper::RxLinkDelayCallback, this);

    for (NodeContainer::Iterator it = sats.Begin(); it != sats.End(); ++it)
    {
        Ptr<NetDevice> dev = GetSatSatOrbiterNetDevice(*it);
        Ptr<SatOrbiterNetDevice> satOrbiterDev = dev->GetObject<SatOrbiterNetDevice>();
        NS_ASSERT(satOrbiterDev != nullptr);
        satOrbiterDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        std::map<uint32_t, Ptr<SatMac>> satOrbiterFeederMacs = satOrbiterDev->GetFeederMac();
        Ptr<SatMac> satMac;
        for (std::map<uint32_t, Ptr<SatMac>>::iterator it2 = satOrbiterFeederMacs.begin();
             it2 != satOrbiterFeederMacs.end();
             ++it2)
        {
            satMac = it2->second;
            NS_ASSERT(satMac != nullptr);
            satMac->SetAttribute("EnableStatisticsTags", BooleanValue(true));

            // Connect the object to the probe.
            if (satMac->TraceConnectWithoutContext("RxLinkDelay", callback))
            {
                NS_LOG_INFO(this << " successfully connected with node ID " << (*it)->GetId()
                                 << " device #" << satOrbiterDev->GetIfIndex());

                // Enable statistics-related tags and trace sources on the device.
                satMac->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            }
            else
            {
                NS_FATAL_ERROR("Error connecting to RxLinkDelay trace source of SatNetDevice"
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

    NodeContainer uts = Singleton<SatTopology>::Get()->GetUtNodes();

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
    NodeContainer gws = Singleton<SatTopology>::Get()->GetGwNodes();
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

NS_OBJECT_ENSURE_REGISTERED(SatStatsFwdUserMacLinkDelayHelper);

SatStatsFwdUserMacLinkDelayHelper::SatStatsFwdUserMacLinkDelayHelper(Ptr<const SatHelper> satHelper)
    : SatStatsLinkDelayHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsFwdUserMacLinkDelayHelper::~SatStatsFwdUserMacLinkDelayHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsFwdUserMacLinkDelayHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsFwdUserMacLinkDelayHelper").SetParent<SatStatsLinkDelayHelper>();
    return tid;
}

void
SatStatsFwdUserMacLinkDelayHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

    NodeContainer sats = Singleton<SatTopology>::Get()->GetOrbiterNodes();

    for (NodeContainer::Iterator it = sats.Begin(); it != sats.End(); ++it)
    {
        Ptr<NetDevice> dev = GetSatSatOrbiterNetDevice(*it);
        Ptr<SatOrbiterNetDevice> satOrbiterDev = dev->GetObject<SatOrbiterNetDevice>();
        NS_ASSERT(satOrbiterDev != nullptr);
        satOrbiterDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        std::map<uint32_t, Ptr<SatMac>> satOrbiterFeederMacs = satOrbiterDev->GetFeederMac();
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

    NodeContainer uts = Singleton<SatTopology>::Get()->GetUtNodes();

    for (NodeContainer::Iterator it = uts.Begin(); it != uts.End(); ++it)
    {
        const int32_t utId = GetUtId(*it);
        NS_ASSERT_MSG(utId > 0, "Node " << (*it)->GetId() << " is not a valid UT");
        const uint32_t identifier = GetIdentifierForUt(*it);

        // Create the probe.
        std::ostringstream probeName;
        probeName << utId;
        Ptr<ApplicationDelayProbe> probe = CreateObject<ApplicationDelayProbe>();
        probe->SetName(probeName.str());

        Ptr<NetDevice> dev = GetUtSatNetDevice(*it);
        Ptr<SatNetDevice> satDev = dev->GetObject<SatNetDevice>();
        NS_ASSERT(satDev != nullptr);
        Ptr<SatMac> satMac = satDev->GetMac();
        NS_ASSERT(satMac != nullptr);

        // Connect the object to the probe.
        if (probe->ConnectByObject("RxLinkDelay", satMac) &&
            ConnectProbeToCollector(probe, identifier))
        {
            m_probes.insert(
                std::make_pair(probe->GetObject<Probe>(), std::make_pair(*it, identifier)));

            // Enable statistics-related tags and trace sources on the device.
            satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            satMac->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
        else
        {
            NS_FATAL_ERROR("Error connecting to RxLinkDelay trace source of SatMac"
                           << " at node ID " << (*it)->GetId() << " device #2");
        }

    } // end of `for (it = uts.Begin(); it != uts.End (); ++it)`

    // Enable statistics-related tags on the transmitting device.
    NodeContainer gws = Singleton<SatTopology>::Get()->GetGwNodes();
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
SatStatsFwdUserMacLinkDelayHelper::UpdateIdentifierOnProbes()
{
    NS_LOG_FUNCTION(this);

    std::map<Ptr<Probe>, std::pair<Ptr<Node>, uint32_t>>::iterator it;

    for (it = m_probes.begin(); it != m_probes.end(); it++)
    {
        Ptr<Probe> probe = it->first;
        Ptr<Node> node = it->second.first;
        uint32_t identifier = it->second.second;

        if (!DisconnectProbeFromCollector(probe, identifier))
        {
            NS_FATAL_ERROR("Error disconnecting trace file on handover");
        }

        identifier = GetIdentifierForUt(node);

        if (!ConnectProbeToCollector(probe, identifier))
        {
            NS_FATAL_ERROR("Error connecting trace file on handover");
        }

        it->second.second = identifier;
    }
} // end of `void UpdateIdentifierOnProbes ();`

// FORWARD FEEDER LINK PHY-LEVEL /////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsFwdFeederPhyLinkDelayHelper);

SatStatsFwdFeederPhyLinkDelayHelper::SatStatsFwdFeederPhyLinkDelayHelper(
    Ptr<const SatHelper> satHelper)
    : SatStatsLinkDelayHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsFwdFeederPhyLinkDelayHelper::~SatStatsFwdFeederPhyLinkDelayHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsFwdFeederPhyLinkDelayHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsFwdFeederPhyLinkDelayHelper").SetParent<SatStatsLinkDelayHelper>();
    return tid;
}

void
SatStatsFwdFeederPhyLinkDelayHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

    NodeContainer sats = Singleton<SatTopology>::Get()->GetOrbiterNodes();
    Callback<void, const Time&, const Address&> callback =
        MakeCallback(&SatStatsFwdFeederPhyLinkDelayHelper::RxLinkDelayCallback, this);

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
            if (satPhy->TraceConnectWithoutContext("RxLinkDelay", callback))
            {
                NS_LOG_INFO(this << " successfully connected with node ID " << (*it)->GetId()
                                 << " device #" << satOrbiterDev->GetIfIndex());

                // Enable statistics-related tags and trace sources on the device.
                satPhy->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            }
            else
            {
                NS_FATAL_ERROR("Error connecting to RxLinkDelay trace source of SatNetDevice"
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

    NodeContainer uts = Singleton<SatTopology>::Get()->GetUtNodes();

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
    NodeContainer gws = Singleton<SatTopology>::Get()->GetGwNodes();
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

NS_OBJECT_ENSURE_REGISTERED(SatStatsFwdUserPhyLinkDelayHelper);

SatStatsFwdUserPhyLinkDelayHelper::SatStatsFwdUserPhyLinkDelayHelper(Ptr<const SatHelper> satHelper)
    : SatStatsLinkDelayHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsFwdUserPhyLinkDelayHelper::~SatStatsFwdUserPhyLinkDelayHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsFwdUserPhyLinkDelayHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsFwdUserPhyLinkDelayHelper").SetParent<SatStatsLinkDelayHelper>();
    return tid;
}

void
SatStatsFwdUserPhyLinkDelayHelper::DoInstallProbes()
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

    NodeContainer uts = Singleton<SatTopology>::Get()->GetUtNodes();

    for (NodeContainer::Iterator it = uts.Begin(); it != uts.End(); ++it)
    {
        const int32_t utId = GetUtId(*it);
        NS_ASSERT_MSG(utId > 0, "Node " << (*it)->GetId() << " is not a valid UT");
        const uint32_t identifier = GetIdentifierForUt(*it);

        // Create the probe.
        std::ostringstream probeName;
        probeName << utId;
        Ptr<ApplicationDelayProbe> probe = CreateObject<ApplicationDelayProbe>();
        probe->SetName(probeName.str());

        Ptr<NetDevice> dev = GetUtSatNetDevice(*it);
        Ptr<SatNetDevice> satDev = dev->GetObject<SatNetDevice>();
        NS_ASSERT(satDev != nullptr);
        Ptr<SatPhy> satPhy = satDev->GetPhy();
        NS_ASSERT(satPhy != nullptr);

        // Connect the object to the probe.
        if (probe->ConnectByObject("RxLinkDelay", satPhy) &&
            ConnectProbeToCollector(probe, identifier))
        {
            m_probes.insert(
                std::make_pair(probe->GetObject<Probe>(), std::make_pair(*it, identifier)));

            // Enable statistics-related tags and trace sources on the device.
            satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            satPhy->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
        else
        {
            NS_FATAL_ERROR("Error connecting to RxLinkDelay trace source of SatPhy"
                           << " at node ID " << (*it)->GetId() << " device #2");
        }

    } // end of `for (it = uts.Begin(); it != uts.End (); ++it)`

    // Enable statistics-related tags on the transmitting device.
    NodeContainer gws = Singleton<SatTopology>::Get()->GetGwNodes();
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
SatStatsFwdUserPhyLinkDelayHelper::UpdateIdentifierOnProbes()
{
    NS_LOG_FUNCTION(this);

    std::map<Ptr<Probe>, std::pair<Ptr<Node>, uint32_t>>::iterator it;

    for (it = m_probes.begin(); it != m_probes.end(); it++)
    {
        Ptr<Probe> probe = it->first;
        Ptr<Node> node = it->second.first;
        uint32_t identifier = it->second.second;

        if (!DisconnectProbeFromCollector(probe, identifier))
        {
            NS_FATAL_ERROR("Error disconnecting trace file on handover");
        }

        identifier = GetIdentifierForUt(node);

        if (!ConnectProbeToCollector(probe, identifier))
        {
            NS_FATAL_ERROR("Error connecting trace file on handover");
        }

        it->second.second = identifier;
    }
} // end of `void UpdateIdentifierOnProbes ();`

// RETURN FEEDER LINK DEV-LEVEL //////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsRtnFeederDevLinkDelayHelper);

SatStatsRtnFeederDevLinkDelayHelper::SatStatsRtnFeederDevLinkDelayHelper(
    Ptr<const SatHelper> satHelper)
    : SatStatsLinkDelayHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsRtnFeederDevLinkDelayHelper::~SatStatsRtnFeederDevLinkDelayHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsRtnFeederDevLinkDelayHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsRtnFeederDevLinkDelayHelper").SetParent<SatStatsLinkDelayHelper>();
    return tid;
}

void
SatStatsRtnFeederDevLinkDelayHelper::DoInstallProbes()
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
    } // end of `for (it = sats.Begin(); it != sats.End (); ++it)`

    NodeContainer uts = Singleton<SatTopology>::Get()->GetUtNodes();
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

    NodeContainer gws = Singleton<SatTopology>::Get()->GetGwNodes();
    Callback<void, const Time&, const Address&> callback =
        MakeCallback(&SatStatsRtnFeederDevLinkDelayHelper::RxLinkDelayCallback, this);

    for (NodeContainer::Iterator it = gws.Begin(); it != gws.End(); ++it)
    {
        NetDeviceContainer devs = GetGwSatNetDevice(*it);

        for (NetDeviceContainer::Iterator itDev = devs.Begin(); itDev != devs.End(); ++itDev)
        {
            Ptr<SatNetDevice> satDev = (*itDev)->GetObject<SatNetDevice>();
            NS_ASSERT(satDev != nullptr);

            // Connect the object to the probe.
            if (satDev->TraceConnectWithoutContext("RxLinkDelay", callback))
            {
                NS_LOG_INFO(this << " successfully connected with node ID " << (*it)->GetId()
                                 << " device #" << satDev->GetIfIndex());

                // Enable statistics-related tags and trace sources on the device.
                satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            }
            else
            {
                NS_FATAL_ERROR("Error connecting to RxLinkDelay trace source of SatNetDevice"
                               << " at node ID " << (*it)->GetId() << " device #"
                               << satDev->GetIfIndex());
            }

        } // end of `for (NetDeviceContainer::Iterator itDev = devs)`

    } // end of `for (NodeContainer::Iterator it = gws)`

} // end of `void DoInstallProbes ();`

// RETURN USER LINK DEV-LEVEL //////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsRtnUserDevLinkDelayHelper);

SatStatsRtnUserDevLinkDelayHelper::SatStatsRtnUserDevLinkDelayHelper(Ptr<const SatHelper> satHelper)
    : SatStatsLinkDelayHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsRtnUserDevLinkDelayHelper::~SatStatsRtnUserDevLinkDelayHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsRtnUserDevLinkDelayHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsRtnUserDevLinkDelayHelper").SetParent<SatStatsLinkDelayHelper>();
    return tid;
}

void
SatStatsRtnUserDevLinkDelayHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

    NodeContainer sats = Singleton<SatTopology>::Get()->GetOrbiterNodes();
    Callback<void, const Time&, const Address&> callback =
        MakeCallback(&SatStatsRtnUserDevLinkDelayHelper::RxLinkDelayCallback, this);

    for (NodeContainer::Iterator it = sats.Begin(); it != sats.End(); ++it)
    {
        Ptr<NetDevice> dev = GetSatSatOrbiterNetDevice(*it);
        Ptr<SatOrbiterNetDevice> satOrbiterDev = dev->GetObject<SatOrbiterNetDevice>();
        NS_ASSERT(satOrbiterDev != nullptr);
        satOrbiterDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));

        // Connect the object to the probe.
        if (satOrbiterDev->TraceConnectWithoutContext("RxUserLinkDelay", callback))
        {
            NS_LOG_INFO(this << " successfully connected with node ID " << (*it)->GetId()
                             << " device #" << satOrbiterDev->GetIfIndex());
        }
        else
        {
            NS_FATAL_ERROR("Error connecting to RxUserLinkDelay trace source of SatNetDevice"
                           << " at node ID " << (*it)->GetId() << " device #"
                           << satOrbiterDev->GetIfIndex());
        }
    } // end of `for (it = sats.Begin(); it != sats.End (); ++it)`

    NodeContainer uts = Singleton<SatTopology>::Get()->GetUtNodes();
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

    NodeContainer gws = Singleton<SatTopology>::Get()->GetGwNodes();

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

NS_OBJECT_ENSURE_REGISTERED(SatStatsRtnFeederMacLinkDelayHelper);

SatStatsRtnFeederMacLinkDelayHelper::SatStatsRtnFeederMacLinkDelayHelper(
    Ptr<const SatHelper> satHelper)
    : SatStatsLinkDelayHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsRtnFeederMacLinkDelayHelper::~SatStatsRtnFeederMacLinkDelayHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsRtnFeederMacLinkDelayHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsRtnFeederMacLinkDelayHelper").SetParent<SatStatsLinkDelayHelper>();
    return tid;
}

void
SatStatsRtnFeederMacLinkDelayHelper::DoInstallProbes()
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
        std::map<uint32_t, Ptr<SatMac>> satOrbiterFeederMacs = satOrbiterDev->GetFeederMac();
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

    NodeContainer uts = Singleton<SatTopology>::Get()->GetUtNodes();
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

    NodeContainer gws = Singleton<SatTopology>::Get()->GetGwNodes();
    Callback<void, const Time&, const Address&> callback =
        MakeCallback(&SatStatsRtnFeederMacLinkDelayHelper::RxLinkDelayCallback, this);

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
            if (satMac->TraceConnectWithoutContext("RxLinkDelay", callback))
            {
                NS_LOG_INFO(this << " successfully connected with node ID " << (*it)->GetId()
                                 << " device #" << satDev->GetIfIndex());

                // Enable statistics-related tags and trace sources on the device.
                satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
                satMac->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            }
            else
            {
                NS_FATAL_ERROR("Error connecting to RxLinkDelay trace source of SatNetDevice"
                               << " at node ID " << (*it)->GetId() << " device #"
                               << satDev->GetIfIndex());
            }

        } // end of `for (NetDeviceContainer::Iterator itDev = devs)`

    } // end of `for (NodeContainer::Iterator it = gws)`

} // end of `void DoInstallProbes ();`

// RETURN USER LINK MAC-LEVEL //////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsRtnUserMacLinkDelayHelper);

SatStatsRtnUserMacLinkDelayHelper::SatStatsRtnUserMacLinkDelayHelper(Ptr<const SatHelper> satHelper)
    : SatStatsLinkDelayHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsRtnUserMacLinkDelayHelper::~SatStatsRtnUserMacLinkDelayHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsRtnUserMacLinkDelayHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsRtnUserMacLinkDelayHelper").SetParent<SatStatsLinkDelayHelper>();
    return tid;
}

void
SatStatsRtnUserMacLinkDelayHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

    NodeContainer sats = Singleton<SatTopology>::Get()->GetOrbiterNodes();
    Callback<void, const Time&, const Address&> callback =
        MakeCallback(&SatStatsRtnUserMacLinkDelayHelper::RxLinkDelayCallback, this);

    for (NodeContainer::Iterator it = sats.Begin(); it != sats.End(); ++it)
    {
        Ptr<NetDevice> dev = GetSatSatOrbiterNetDevice(*it);
        Ptr<SatOrbiterNetDevice> satOrbiterDev = dev->GetObject<SatOrbiterNetDevice>();
        NS_ASSERT(satOrbiterDev != nullptr);
        satOrbiterDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        std::map<uint32_t, Ptr<SatMac>> satOrbiterFeederMacs = satOrbiterDev->GetFeederMac();
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
            if (satMac->TraceConnectWithoutContext("RxLinkDelay", callback))
            {
                NS_LOG_INFO(this << " successfully connected with node ID " << (*it)->GetId()
                                 << " device #" << satOrbiterDev->GetIfIndex());
            }
            else
            {
                NS_FATAL_ERROR("Error connecting to RxLinkDelay trace source of SatNetDevice"
                               << " at node ID " << (*it)->GetId() << " device #"
                               << satOrbiterDev->GetIfIndex());
            }
        }
    } // end of `for (it = sats.Begin(); it != sats.End (); ++it)`

    NodeContainer uts = Singleton<SatTopology>::Get()->GetUtNodes();
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

    NodeContainer gws = Singleton<SatTopology>::Get()->GetGwNodes();

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

NS_OBJECT_ENSURE_REGISTERED(SatStatsRtnFeederPhyLinkDelayHelper);

SatStatsRtnFeederPhyLinkDelayHelper::SatStatsRtnFeederPhyLinkDelayHelper(
    Ptr<const SatHelper> satHelper)
    : SatStatsLinkDelayHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsRtnFeederPhyLinkDelayHelper::~SatStatsRtnFeederPhyLinkDelayHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsRtnFeederPhyLinkDelayHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsRtnFeederPhyLinkDelayHelper").SetParent<SatStatsLinkDelayHelper>();
    return tid;
}

void
SatStatsRtnFeederPhyLinkDelayHelper::DoInstallProbes()
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

    NodeContainer uts = Singleton<SatTopology>::Get()->GetUtNodes();
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

    NodeContainer gws = Singleton<SatTopology>::Get()->GetGwNodes();
    Callback<void, const Time&, const Address&> callback =
        MakeCallback(&SatStatsRtnFeederPhyLinkDelayHelper::RxLinkDelayCallback, this);

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
            if (satPhy->TraceConnectWithoutContext("RxLinkDelay", callback))
            {
                NS_LOG_INFO(this << " successfully connected with node ID " << (*it)->GetId()
                                 << " device #" << satDev->GetIfIndex());

                // Enable statistics-related tags and trace sources on the device.
                satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
                satPhy->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            }
            else
            {
                NS_FATAL_ERROR("Error connecting to RxLinkDelay trace source of SatNetDevice"
                               << " at node ID " << (*it)->GetId() << " device #"
                               << satDev->GetIfIndex());
            }

        } // end of `for (NetDeviceContainer::Iterator itDev = devs)`

    } // end of `for (NodeContainer::Iterator it = gws)`

} // end of `void DoInstallProbes ();`

// RETURN USER LINK PHY-LEVEL //////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsRtnUserPhyLinkDelayHelper);

SatStatsRtnUserPhyLinkDelayHelper::SatStatsRtnUserPhyLinkDelayHelper(Ptr<const SatHelper> satHelper)
    : SatStatsLinkDelayHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsRtnUserPhyLinkDelayHelper::~SatStatsRtnUserPhyLinkDelayHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsRtnUserPhyLinkDelayHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsRtnUserPhyLinkDelayHelper").SetParent<SatStatsLinkDelayHelper>();
    return tid;
}

void
SatStatsRtnUserPhyLinkDelayHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

    NodeContainer sats = Singleton<SatTopology>::Get()->GetOrbiterNodes();
    Callback<void, const Time&, const Address&> callback =
        MakeCallback(&SatStatsRtnUserPhyLinkDelayHelper::RxLinkDelayCallback, this);

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
            if (satPhy->TraceConnectWithoutContext("RxLinkDelay", callback))
            {
                NS_LOG_INFO(this << " successfully connected with node ID " << (*it)->GetId()
                                 << " device #" << satOrbiterDev->GetIfIndex());
            }
            else
            {
                NS_FATAL_ERROR("Error connecting to RxLinkDelay trace source of SatNetDevice"
                               << " at node ID " << (*it)->GetId() << " device #"
                               << satOrbiterDev->GetIfIndex());
            }
        }
    } // end of `for (it = sats.Begin(); it != sats.End (); ++it)`

    NodeContainer uts = Singleton<SatTopology>::Get()->GetUtNodes();
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

    NodeContainer gws = Singleton<SatTopology>::Get()->GetGwNodes();

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
