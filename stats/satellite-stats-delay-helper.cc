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

#include "satellite-stats-delay-helper.h"

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

NS_LOG_COMPONENT_DEFINE("SatStatsDelayHelper");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(SatStatsDelayHelper);

SatStatsDelayHelper::SatStatsDelayHelper(Ptr<const SatHelper> satHelper)
    : SatStatsHelper(satHelper),
      m_averagingMode(false)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsDelayHelper::~SatStatsDelayHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsDelayHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SatStatsDelayHelper")
            .SetParent<SatStatsHelper>()
            .AddAttribute("AveragingMode",
                          "If true, all samples will be averaged before passed to aggregator. "
                          "Only affects histogram, PDF, and CDF output types.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&SatStatsDelayHelper::SetAveragingMode,
                                              &SatStatsDelayHelper::GetAveragingMode),
                          MakeBooleanChecker());
    return tid;
}

void
SatStatsDelayHelper::SetAveragingMode(bool averagingMode)
{
    NS_LOG_FUNCTION(this << averagingMode);
    m_averagingMode = averagingMode;
}

bool
SatStatsDelayHelper::GetAveragingMode() const
{
    return m_averagingMode;
}

void
SatStatsDelayHelper::DoInstall()
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
        NS_FATAL_ERROR("SatStatsDelayHelper - Invalid output type");
        break;
    }

    // Setup probes and connect them to the collectors.
    InstallProbes();

} // end of `void DoInstall ();`

void
SatStatsDelayHelper::InstallProbes()
{
    // The method below is supposed to be implemented by the child class.
    DoInstallProbes();
}

void
SatStatsDelayHelper::RxDelayCallback(const Time& delay, const Address& from)
{
    // NS_LOG_FUNCTION (this << delay.GetSeconds () << from);

    if (from.IsInvalid())
    {
        NS_LOG_WARN(this << " discarding a packet delay of " << delay.GetSeconds()
                         << " from statistics collection because of"
                         << " invalid sender address");
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
SatStatsDelayHelper::ConnectProbeToCollector(Ptr<Probe> probe, uint32_t identifier)
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
SatStatsDelayHelper::DisconnectProbeFromCollector(Ptr<Probe> probe, uint32_t identifier)
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
SatStatsDelayHelper::PassSampleToCollector(const Time& delay, uint32_t identifier)
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

// FORWARD LINK APPLICATION-LEVEL /////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsFwdAppDelayHelper);

SatStatsFwdAppDelayHelper::SatStatsFwdAppDelayHelper(Ptr<const SatHelper> satHelper)
    : SatStatsDelayHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsFwdAppDelayHelper::~SatStatsFwdAppDelayHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsFwdAppDelayHelper::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SatStatsFwdAppDelayHelper").SetParent<SatStatsDelayHelper>();
    return tid;
}

void
SatStatsFwdAppDelayHelper::DoInstallProbes()
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
            Ptr<Application> app = (*it)->GetApplication(i);
            bool isConnected = false;

            /*
             * Some applications support RxDelay trace sources, and some other
             * applications support Rx trace sources. Below we support both ways.
             */
            if (app->GetInstanceTypeId().LookupTraceSourceByName("RxDelay") != nullptr)
            {
                NS_LOG_INFO(this << " attempt to connect using RxDelay");

                // Create the probe.
                std::ostringstream probeName;
                probeName << utUserId << "-" << i;
                Ptr<ApplicationDelayProbe> probe = CreateObject<ApplicationDelayProbe>();
                probe->SetName(probeName.str());

                // Connect the object to the probe.
                if (probe->ConnectByObject("RxDelay", app))
                {
                    isConnected = ConnectProbeToCollector(probe, identifier);
                    m_probes.insert(
                        std::make_pair(probe->GetObject<Probe>(), std::make_pair(*it, identifier)));
                }
            }
            else if (app->GetInstanceTypeId().LookupTraceSourceByName("Rx") != nullptr)
            {
                NS_LOG_INFO(this << " attempt to connect using Rx");
                Callback<void, Ptr<const Packet>, const Address&> rxCallback =
                    MakeBoundCallback(&SatStatsFwdAppDelayHelper::RxCallback, this, identifier);
                isConnected = app->TraceConnectWithoutContext("Rx", rxCallback);
            }

            if (isConnected)
            {
                NS_LOG_INFO(this << " successfully connected"
                                 << " with node ID " << (*it)->GetId() << " application #" << i);
            }
            else
            {
                /*
                 * We're being tolerant here by only logging a warning, because
                 * not every kind of Application is equipped with the expected
                 * RxDelay or Rx trace source.
                 */
                NS_LOG_WARN(this << " unable to connect"
                                 << " with node ID " << (*it)->GetId() << " application #" << i);
            }

        } // end of `for (i = 0; i < (*it)->GetNApplications (); i++)`

    } // end of `for (it = utUsers.Begin(); it != utUsers.End (); ++it)`

    /*
     * Some sender applications might need a special attribute to be enabled
     * before delay statistics can be computed. We enable it here.
     */
    NodeContainer gwUsers = GetSatHelper()->GetGwUsers();
    for (NodeContainer::Iterator it = gwUsers.Begin(); it != gwUsers.End(); ++it)
    {
        for (uint32_t i = 0; i < (*it)->GetNApplications(); i++)
        {
            Ptr<Application> app = (*it)->GetApplication(i);

            if (!app->SetAttributeFailSafe("EnableStatisticsTags", BooleanValue(true)))
            {
                NS_LOG_WARN(this << " node ID " << (*it)->GetId() << " application #" << i
                                 << " might not produce the required tags"
                                 << " in the packets it transmits,"
                                 << " thus preventing delay statistics"
                                 << " from this application");
            }

        } // end of `for (i = 0; i < (*it)->GetNApplications (); i++)`

    } // end of `for (it = gwUsers.Begin(); it != gwUsers.End (); ++it)`

} // end of `void DoInstallProbes ();`

void // static
SatStatsFwdAppDelayHelper::RxCallback(Ptr<SatStatsFwdAppDelayHelper> helper,
                                      uint32_t identifier,
                                      Ptr<const Packet> packet,
                                      const Address& from)
{
    NS_LOG_FUNCTION(helper << identifier << packet << packet->GetSize() << from);

    //  bool isTagged = false;
    //  ByteTagIterator it = packet->GetByteTagIterator ();
    //
    //  while (!isTagged && it.HasNext ())
    //    {
    //      ByteTagIterator::Item item = it.Next ();
    //
    //      if (item.GetTypeId () == TrafficTimeTag::GetTypeId ())
    //        {
    //          NS_LOG_DEBUG ("Contains a TrafficTimeTag tag:"
    //                        << " start=" << item.GetStart ()
    //                        << " end=" << item.GetEnd ());
    //          TrafficTimeTag timeTag;
    //          item.GetTag (timeTag);
    //          const Time delay = Simulator::Now () - timeTag.GetSenderTimestamp ();
    //          helper->PassSampleToCollector (delay, identifier);
    //          isTagged = true; // this will exit the while loop.
    //        }
    //    }
    //
    //  if (!isTagged)
    //    {
    //      NS_LOG_WARN ("Discarding a packet of " << packet->GetSize ()
    //                   << " from statistics collection"
    //                   << " because it does not contain any TrafficTimeTag");
    //    }

    TrafficTimeTag timeTag;
    if (packet->PeekPacketTag(timeTag))
    {
        NS_LOG_DEBUG("Contains a TrafficTimeTag tag");
        const Time delay = Simulator::Now() - timeTag.GetSenderTimestamp();
        helper->PassSampleToCollector(delay, identifier);
    }
    else
    {
        NS_LOG_WARN("Discarding a packet of " << packet->GetSize() << " from statistics collection"
                                              << " because it does not contain any TrafficTimeTag");
    }
}

void
SatStatsFwdAppDelayHelper::UpdateIdentifierOnProbes()
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

        identifier = GetIdentifierForUtUser(node);

        if (!ConnectProbeToCollector(probe, identifier))
        {
            NS_FATAL_ERROR("Error connecting trace file on handover");
        }

        it->second.second = identifier;
    }
} // end of `void UpdateIdentifierOnProbes ();`

// FORWARD LINK DEVICE-LEVEL //////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsFwdDevDelayHelper);

SatStatsFwdDevDelayHelper::SatStatsFwdDevDelayHelper(Ptr<const SatHelper> satHelper)
    : SatStatsDelayHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsFwdDevDelayHelper::~SatStatsFwdDevDelayHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsFwdDevDelayHelper::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SatStatsFwdDevDelayHelper").SetParent<SatStatsDelayHelper>();
    return tid;
}

void
SatStatsFwdDevDelayHelper::DoInstallProbes()
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
        Ptr<ApplicationDelayProbe> probe = CreateObject<ApplicationDelayProbe>();
        probe->SetName(probeName.str());

        Ptr<NetDevice> dev = GetUtSatNetDevice(*it);

        // Connect the object to the probe.
        if (probe->ConnectByObject("RxDelay", dev) && ConnectProbeToCollector(probe, identifier))
        {
            m_probes.insert(
                std::make_pair(probe->GetObject<Probe>(), std::make_pair(*it, identifier)));

            // Enable statistics-related tags and trace sources on the device.
            dev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
        else
        {
            NS_FATAL_ERROR("Error connecting to RxDelay trace source of SatNetDevice"
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
SatStatsFwdDevDelayHelper::UpdateIdentifierOnProbes()
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

// FORWARD LINK MAC-LEVEL /////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsFwdMacDelayHelper);

SatStatsFwdMacDelayHelper::SatStatsFwdMacDelayHelper(Ptr<const SatHelper> satHelper)
    : SatStatsDelayHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsFwdMacDelayHelper::~SatStatsFwdMacDelayHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsFwdMacDelayHelper::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SatStatsFwdMacDelayHelper").SetParent<SatStatsDelayHelper>();
    return tid;
}

void
SatStatsFwdMacDelayHelper::DoInstallProbes()
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
        Ptr<ApplicationDelayProbe> probe = CreateObject<ApplicationDelayProbe>();
        probe->SetName(probeName.str());

        Ptr<NetDevice> dev = GetUtSatNetDevice(*it);
        Ptr<SatNetDevice> satDev = dev->GetObject<SatNetDevice>();
        NS_ASSERT(satDev != nullptr);
        Ptr<SatMac> satMac = satDev->GetMac();
        NS_ASSERT(satMac != nullptr);

        // Connect the object to the probe.
        if (probe->ConnectByObject("RxDelay", satMac) && ConnectProbeToCollector(probe, identifier))
        {
            m_probes.insert(
                std::make_pair(probe->GetObject<Probe>(), std::make_pair(*it, identifier)));

            // Enable statistics-related tags and trace sources on the device.
            satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            satMac->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
        else
        {
            NS_FATAL_ERROR("Error connecting to RxDelay trace source of satMac"
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
SatStatsFwdMacDelayHelper::UpdateIdentifierOnProbes()
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

// FORWARD LINK PHY-LEVEL /////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsFwdPhyDelayHelper);

SatStatsFwdPhyDelayHelper::SatStatsFwdPhyDelayHelper(Ptr<const SatHelper> satHelper)
    : SatStatsDelayHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsFwdPhyDelayHelper::~SatStatsFwdPhyDelayHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsFwdPhyDelayHelper::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SatStatsFwdPhyDelayHelper").SetParent<SatStatsDelayHelper>();
    return tid;
}

void
SatStatsFwdPhyDelayHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

    NodeContainer sats = Singleton<SatTopology>::Get()->GetOrbiterNodes();

    for (NodeContainer::Iterator it = sats.Begin(); it != sats.End(); ++it)
    {
        Ptr<NetDevice> dev = GetSatSatOrbiterNetDevice(*it);
        Ptr<SatOrbiterNetDevice> satOrbiterDev = dev->GetObject<SatOrbiterNetDevice>();
        NS_ASSERT(satOrbiterDev != nullptr);
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
        Ptr<ApplicationDelayProbe> probe = CreateObject<ApplicationDelayProbe>();
        probe->SetName(probeName.str());

        Ptr<NetDevice> dev = GetUtSatNetDevice(*it);
        Ptr<SatNetDevice> satDev = dev->GetObject<SatNetDevice>();
        NS_ASSERT(satDev != nullptr);
        Ptr<SatPhy> satPhy = satDev->GetPhy();
        NS_ASSERT(satPhy != nullptr);

        // Connect the object to the probe.
        if (probe->ConnectByObject("RxDelay", satPhy) && ConnectProbeToCollector(probe, identifier))
        {
            m_probes.insert(
                std::make_pair(probe->GetObject<Probe>(), std::make_pair(*it, identifier)));

            // Enable statistics-related tags and trace sources on the device.
            satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            satPhy->SetAttribute("EnableStatisticsTags", BooleanValue(true));
        }
        else
        {
            NS_FATAL_ERROR("Error connecting to RxDelay trace source of SatPhy"
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
SatStatsFwdPhyDelayHelper::UpdateIdentifierOnProbes()
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

// RETURN LINK APPLICATION-LEVEL //////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsRtnAppDelayHelper);

SatStatsRtnAppDelayHelper::SatStatsRtnAppDelayHelper(Ptr<const SatHelper> satHelper)
    : SatStatsDelayHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsRtnAppDelayHelper::~SatStatsRtnAppDelayHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsRtnAppDelayHelper::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SatStatsRtnAppDelayHelper").SetParent<SatStatsDelayHelper>();
    return tid;
}

void
SatStatsRtnAppDelayHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

    NodeContainer utUsers = GetSatHelper()->GetUtUsers();
    for (NodeContainer::Iterator it = utUsers.Begin(); it != utUsers.End(); ++it)
    {
        // Create a map of UT user addresses and identifiers.
        SaveIpv4AddressAndIdentifier(*it);

        /*
         * Some sender applications might need a special attribute to be enabled
         * before delay statistics can be computed. We enable it here.
         */
        for (uint32_t i = 0; i < (*it)->GetNApplications(); i++)
        {
            Ptr<Application> app = (*it)->GetApplication(i);

            if (!app->SetAttributeFailSafe("EnableStatisticsTags", BooleanValue(true)))
            {
                NS_LOG_WARN(this << " node ID " << (*it)->GetId() << " application #" << i
                                 << " might not produce the required tags"
                                 << " in the transmitted packets,"
                                 << " thus preventing delay statistics"
                                 << " from this sender application");
            }

        } // end of `for (i = 0; i < (*it)->GetNApplications (); i++)`

    } // end of `for (NodeContainer::Iterator it: utUsers)`

    // Connect to trace sources at GW user node's applications.

    NodeContainer gwUsers = GetSatHelper()->GetGwUsers();
    Callback<void, const Time&, const Address&> rxDelayCallback =
        MakeCallback(&SatStatsRtnAppDelayHelper::Ipv4Callback, this);
    Callback<void, Ptr<const Packet>, const Address&> rxCallback =
        MakeCallback(&SatStatsRtnAppDelayHelper::RxCallback, this);

    for (NodeContainer::Iterator it = gwUsers.Begin(); it != gwUsers.End(); ++it)
    {
        for (uint32_t i = 0; i < (*it)->GetNApplications(); i++)
        {
            Ptr<Application> app = (*it)->GetApplication(i);
            bool isConnected = false;

            /*
             * Some applications support RxDelay trace sources, and some other
             * applications support Rx trace sources. Below we support both ways.
             */
            if (app->GetInstanceTypeId().LookupTraceSourceByName("RxDelay") != nullptr)
            {
                isConnected = app->TraceConnectWithoutContext("RxDelay", rxDelayCallback);
            }
            else if (app->GetInstanceTypeId().LookupTraceSourceByName("Rx") != nullptr)
            {
                isConnected = app->TraceConnectWithoutContext("Rx", rxCallback);
            }

            if (isConnected)
            {
                NS_LOG_INFO(this << " successfully connected"
                                 << " with node ID " << (*it)->GetId() << " application #" << i);
            }
            else
            {
                /*
                 * We're being tolerant here by only logging a warning, because
                 * not every kind of Application is equipped with the expected
                 * RxDelay or Rx trace source.
                 */
                NS_LOG_WARN(this << " unable to connect"
                                 << " with node ID " << (*it)->GetId() << " application #" << i);
            }

        } // end of `for (i = 0; i < (*it)->GetNApplications (); i++)`

    } // end of `for (NodeContainer::Iterator it: gwUsers)`

} // end of `void DoInstallProbes ();`

void
SatStatsRtnAppDelayHelper::RxCallback(Ptr<const Packet> packet, const Address& from)
{
    // NS_LOG_FUNCTION (this << packet << packet->GetSize () << from);

    //  bool isTagged = false;
    //  ByteTagIterator it = packet->GetByteTagIterator ();
    //
    //  while (!isTagged && it.HasNext ())
    //    {
    //      ByteTagIterator::Item item = it.Next ();
    //
    //      if (item.GetTypeId () == TrafficTimeTag::GetTypeId ())
    //        {
    //          NS_LOG_DEBUG ("Contains a TrafficTimeTag tag:"
    //                        << " start=" << item.GetStart ()
    //                        << " end=" << item.GetEnd ());
    //          TrafficTimeTag timeTag;
    //          item.GetTag (timeTag);
    //          const Time delay = Simulator::Now () - timeTag.GetSenderTimestamp ();
    //          helper->PassSampleToCollector (delay, identifier);
    //          isTagged = true; // this will exit the while loop.
    //        }
    //    }
    //
    //  if (!isTagged)
    //    {
    //      NS_LOG_WARN ("Discarding a packet of " << packet->GetSize ()
    //                   << " from statistics collection"
    //                   << " because it does not contain any TrafficTimeTag");
    //    }

    TrafficTimeTag timeTag;
    if (packet->PeekPacketTag(timeTag))
    {
        NS_LOG_DEBUG(this << " contains a TrafficTimeTag tag");
        Ipv4Callback(Simulator::Now() - timeTag.GetSenderTimestamp(), from);
    }
    else
    {
        NS_LOG_WARN(this << " discarding a packet of " << packet->GetSize()
                         << " from statistics collection"
                         << " because it does not contain any TrafficTimeTag");
    }

} // end of `void RxCallback (Ptr<const Packet>, const Address);`

void
SatStatsRtnAppDelayHelper::Ipv4Callback(const Time& delay, const Address& from)
{
    // NS_LOG_FUNCTION (this << Time.GetSeconds () << from);

    if (InetSocketAddress::IsMatchingType(from))
    {
        // Determine the identifier associated with the sender address.
        const Address ipv4Addr = InetSocketAddress::ConvertFrom(from).GetIpv4();
        std::map<const Address, uint32_t>::const_iterator it1 = m_identifierMap.find(ipv4Addr);

        if (it1 == m_identifierMap.end())
        {
            NS_LOG_WARN(this << " discarding a packet delay of " << delay.GetSeconds()
                             << " from statistics collection because of"
                             << " unknown sender IPV4 address " << ipv4Addr);
        }
        else
        {
            PassSampleToCollector(delay, it1->second);
        }
    }
    else
    {
        NS_LOG_WARN(this << " discarding a packet delay of " << delay.GetSeconds()
                         << " from statistics collection"
                         << " because it comes from sender " << from
                         << " without valid InetSocketAddress");
    }
}

void
SatStatsRtnAppDelayHelper::SaveIpv4AddressAndIdentifier(Ptr<Node> utUserNode)
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

// RETURN LINK DEVICE-LEVEL ///////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsRtnDevDelayHelper);

SatStatsRtnDevDelayHelper::SatStatsRtnDevDelayHelper(Ptr<const SatHelper> satHelper)
    : SatStatsDelayHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsRtnDevDelayHelper::~SatStatsRtnDevDelayHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsRtnDevDelayHelper::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SatStatsRtnDevDelayHelper").SetParent<SatStatsDelayHelper>();
    return tid;
}

void
SatStatsRtnDevDelayHelper::DoInstallProbes()
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
    Callback<void, const Time&, const Address&> callback =
        MakeCallback(&SatStatsRtnDevDelayHelper::RxDelayCallback, this);

    for (NodeContainer::Iterator it = gws.Begin(); it != gws.End(); ++it)
    {
        NetDeviceContainer devs = GetGwSatNetDevice(*it);

        for (NetDeviceContainer::Iterator itDev = devs.Begin(); itDev != devs.End(); ++itDev)
        {
            NS_ASSERT((*itDev)->GetObject<SatNetDevice>() != nullptr);

            if ((*itDev)->TraceConnectWithoutContext("RxDelay", callback))
            {
                NS_LOG_INFO(this << " successfully connected with node ID " << (*it)->GetId()
                                 << " device #" << (*itDev)->GetIfIndex());

                // Enable statistics-related tags and trace sources on the device.
                (*itDev)->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            }
            else
            {
                NS_FATAL_ERROR("Error connecting to RxDelay trace source of SatNetDevice"
                               << " at node ID " << (*it)->GetId() << " device #"
                               << (*itDev)->GetIfIndex());
            }

        } // end of `for (NetDeviceContainer::Iterator itDev = devs)`

    } // end of `for (NodeContainer::Iterator it = gws)`

} // end of `void DoInstallProbes ();`

// RETURN LINK MAC-LEVEL //////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsRtnMacDelayHelper);

SatStatsRtnMacDelayHelper::SatStatsRtnMacDelayHelper(Ptr<const SatHelper> satHelper)
    : SatStatsDelayHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsRtnMacDelayHelper::~SatStatsRtnMacDelayHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsRtnMacDelayHelper::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SatStatsRtnMacDelayHelper").SetParent<SatStatsDelayHelper>();
    return tid;
}

void
SatStatsRtnMacDelayHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

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
    Callback<void, const Time&, const Address&> callback =
        MakeCallback(&SatStatsRtnMacDelayHelper::RxDelayCallback, this);

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
            if (satMac->TraceConnectWithoutContext("RxDelay", callback))
            {
                NS_LOG_INFO(this << " successfully connected with node ID " << (*it)->GetId()
                                 << " device #" << satDev->GetIfIndex());

                // Enable statistics-related tags and trace sources on the device.
                satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
                satMac->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            }
            else
            {
                NS_FATAL_ERROR("Error connecting to RxDelay trace source of SatNetDevice"
                               << " at node ID " << (*it)->GetId() << " device #"
                               << satDev->GetIfIndex());
            }

        } // end of `for (NetDeviceContainer::Iterator itDev = devs)`

    } // end of `for (NodeContainer::Iterator it = gws)`

} // end of `void DoInstallProbes ();`

// RETURN LINK PHY-LEVEL //////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(SatStatsRtnPhyDelayHelper);

SatStatsRtnPhyDelayHelper::SatStatsRtnPhyDelayHelper(Ptr<const SatHelper> satHelper)
    : SatStatsDelayHelper(satHelper)
{
    NS_LOG_FUNCTION(this << satHelper);
}

SatStatsRtnPhyDelayHelper::~SatStatsRtnPhyDelayHelper()
{
    NS_LOG_FUNCTION(this);
}

TypeId // static
SatStatsRtnPhyDelayHelper::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SatStatsRtnPhyDelayHelper").SetParent<SatStatsDelayHelper>();
    return tid;
}

void
SatStatsRtnPhyDelayHelper::DoInstallProbes()
{
    NS_LOG_FUNCTION(this);

    NodeContainer sats = Singleton<SatTopology>::Get()->GetOrbiterNodes();

    for (NodeContainer::Iterator it = sats.Begin(); it != sats.End(); ++it)
    {
        Ptr<SatPhy> satPhy;
        Ptr<NetDevice> dev = GetSatSatOrbiterNetDevice(*it);
        Ptr<SatOrbiterNetDevice> satOrbiterDev = dev->GetObject<SatOrbiterNetDevice>();
        NS_ASSERT(satOrbiterDev != nullptr);
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
    Callback<void, const Time&, const Address&> callback =
        MakeCallback(&SatStatsRtnPhyDelayHelper::RxDelayCallback, this);

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
            if (satPhy->TraceConnectWithoutContext("RxDelay", callback))
            {
                NS_LOG_INFO(this << " successfully connected with node ID " << (*it)->GetId()
                                 << " device #" << satDev->GetIfIndex());

                // Enable statistics-related tags and trace sources on the device.
                satDev->SetAttribute("EnableStatisticsTags", BooleanValue(true));
                satPhy->SetAttribute("EnableStatisticsTags", BooleanValue(true));
            }
            else
            {
                NS_FATAL_ERROR("Error connecting to RxDelay trace source of SatNetDevice"
                               << " at node ID " << (*it)->GetId() << " device #"
                               << satDev->GetIfIndex());
            }

        } // end of `for (NetDeviceContainer::Iterator itDev = devs)`

    } // end of `for (NodeContainer::Iterator it = gws)`

} // end of `void DoInstallProbes ();`

} // end of namespace ns3
