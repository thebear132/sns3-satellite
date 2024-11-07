/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Magister Solutions Ltd.
 * Copyright (c) 2018 CNES
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
 * Author: Mathias Ettinger <mettinger@toulouse.viveris.fr>
 */

#include "satellite-per-packet-interference.h"

#include <ns3/log.h>
#include <ns3/simulator.h>
#include <ns3/singleton.h>

#include <algorithm>
#include <cmath>
#include <istream>
#include <limits>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

NS_LOG_COMPONENT_DEFINE("SatPerPacketInterference");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(SatPerPacketInterference);

TypeId
SatPerPacketInterference::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::SatPerPacketInterference")
                            .SetParent<SatInterference>()
                            .AddConstructor<SatPerPacketInterference>();

    return tid;
}

TypeId
SatPerPacketInterference::GetInstanceTypeId(void) const
{
    NS_LOG_FUNCTION(this);

    return GetTypeId();
}

SatPerPacketInterference::SatPerPacketInterference()
    : m_residualPowerW(0.0),
      m_rxing(false),
      m_nextEventId(0),
      m_enableTraceOutput(false),
      m_channelType(),
      m_rxBandwidth_Hz()
{
    NS_LOG_FUNCTION(this);
}

SatPerPacketInterference::SatPerPacketInterference(SatEnums::ChannelType_t channelType,
                                                   double rxBandwidthHz)
    : m_residualPowerW(0.0),
      m_rxing(false),
      m_nextEventId(0),
      m_enableTraceOutput(true),
      m_channelType(channelType),
      m_rxBandwidth_Hz(rxBandwidthHz)
{
    NS_LOG_FUNCTION(this << channelType << rxBandwidthHz);

    if (m_rxBandwidth_Hz <= std::numeric_limits<double>::epsilon())
    {
        NS_FATAL_ERROR("SatPerPacketInterference::SatPerPacketInterference - Invalid value");
    }
}

SatPerPacketInterference::~SatPerPacketInterference()
{
    NS_LOG_FUNCTION(this);

    Reset();
}

Ptr<SatInterference::InterferenceChangeEvent>
SatPerPacketInterference::DoAdd(Time duration, double power, Address rxAddress)
{
    NS_LOG_FUNCTION(this << duration << power << rxAddress);

    Ptr<SatInterference::InterferenceChangeEvent> event;
    event = Create<SatInterference::InterferenceChangeEvent>(m_nextEventId++,
                                                             duration,
                                                             power,
                                                             rxAddress);
    Time now = event->GetStartTime();

    NS_LOG_INFO("Add change: Duration= " << duration << ", Power= " << power << ", Time: " << now);

    // do update and clean-ups, if we are not receiving
    if (!m_rxing)
    {
        InterferenceChanges::iterator nowIterator = m_interferenceChanges.upper_bound(now);

        for (InterferenceChanges::iterator i = m_interferenceChanges.begin(); i != nowIterator; i++)
        {
            uint32_t eventID;
            long double powerValue;
            std::tie(eventID, powerValue, std::ignore) = i->second;

            NS_LOG_INFO("Change to erase: Time= " << i->first << ", Id= " << eventID
                                                  << ", PowerValue= " << powerValue);

            m_residualPowerW += powerValue;

            NS_LOG_INFO("First power after erase: " << m_residualPowerW);
        }
        m_interferenceChanges.erase(m_interferenceChanges.begin(), nowIterator);
    }

    NS_LOG_INFO("Change count before addition: " << m_interferenceChanges.size());

    // if no changes in future, first power should be zero
    if (m_interferenceChanges.size() == 0)
    {
        if ((m_residualPowerW != 0) &&
            std::fabs(m_residualPowerW) < std::numeric_limits<long double>::epsilon())
        {
            // if we end up here,
            // reset first power (this probably due to roundin problem with very small values)
            m_residualPowerW = 0;
        }
    }

    m_interferenceChanges.insert(
        std::make_pair(now, InterferenceChange(event->GetId(), power, false)));
    m_interferenceChanges.insert(
        std::make_pair(event->GetEndTime(), InterferenceChange(event->GetId(), -power, true)));

    NS_LOG_INFO("Change count after addition: " << m_interferenceChanges.size());

    if (m_residualPowerW < 0)
    {
        // First power should never leak negative
        NS_FATAL_ERROR("First power negative!!!");
    }

    return event;
}

std::vector<std::pair<double, double>>
SatPerPacketInterference::DoCalculate(Ptr<SatInterference::InterferenceChangeEvent> event)
{
    NS_LOG_FUNCTION(this);

    if (m_rxing == false)
    {
        NS_FATAL_ERROR("Receiving is not set on!!!");
    }

    double ifPowerW = m_residualPowerW;
    double rxDuration = event->GetDuration().GetDouble();
    double rxEndTime = event->GetEndTime().GetDouble();
    bool ownStartReached = false;

    NS_LOG_INFO("Calculate: IfPower (W)= " << ifPowerW << ", Event ID= " << event->GetId()
                                           << ", Duration= " << event->GetDuration()
                                           << ", StartTime= " << event->GetStartTime()
                                           << ", EndTime= " << event->GetEndTime());

    InterferenceChanges::iterator currentItem = m_interferenceChanges.begin();

    // calculate power values until own "stop" event found (own negative power event)
    while (currentItem != m_interferenceChanges.end())
    {
        uint32_t eventID;
        long double powerValue;
        bool isEndEvent;
        std::tie(eventID, powerValue, isEndEvent) = currentItem->second;

        if (event->GetId() == eventID)
        {
            if (isEndEvent)
            {
                NS_LOG_INFO("IfPower after end event: " << ifPowerW);
                break;
            }
            // stop increasing power value fully, when own 'start' event is reached
            // needed to support multiple simultaneous receiving (currently not supported)
            // own event is not updated to ifPower
            ownStartReached = true;
            onOwnStartReached(ifPowerW);
        }
        else if (ownStartReached)
        {
            // increase/decrease interference power with relative part of duration of power change
            // in list
            double itemTime = currentItem->first.GetDouble();
            onInterferentEvent(((rxEndTime - itemTime) / rxDuration), powerValue, ifPowerW);

            NS_LOG_INFO("Update (partial): ID: " << eventID << ", Power (W)= " << powerValue
                                                 << ", Time= " << currentItem->first
                                                 << ", DeltaTime= " << (rxEndTime - itemTime));

            NS_LOG_INFO("IfPower after update: " << ifPowerW);
        }
        else
        {
            // increase/decrease interference power with full power change in list
            ifPowerW += powerValue;

            NS_LOG_INFO("Update (full): ID: " << eventID << ", Power (W)= " << powerValue);
            NS_LOG_INFO("IfPower after update: " << ifPowerW);
        }

        currentItem++;
    }

    if (m_enableTraceOutput)
    {
        std::vector<double> tempVector;
        tempVector.push_back(Now().GetSeconds());
        tempVector.push_back(ifPowerW / m_rxBandwidth_Hz);
        Singleton<SatInterferenceOutputTraceContainer>::Get()->AddToContainer(
            std::make_pair(event->GetSatEarthStationAddress(), m_channelType),
            tempVector);
    }

    std::vector<std::pair<double, double>> ifPowerPerFragment;
    ifPowerPerFragment.emplace_back(1.0, ifPowerW);

    return ifPowerPerFragment;
}

void
SatPerPacketInterference::onOwnStartReached(double ifPowerW)
{
    // do nothing, meant for subclasses to override
}

void
SatPerPacketInterference::onInterferentEvent(long double timeRatio,
                                             double interferenceValue,
                                             double& ifPowerW)
{
    ifPowerW += timeRatio * interferenceValue;
}

void
SatPerPacketInterference::DoReset(void)
{
    NS_LOG_FUNCTION(this);

    m_interferenceChanges.clear();
    m_rxing = false;
    m_residualPowerW = 0.0;
}

void
SatPerPacketInterference::DoNotifyRxStart(Ptr<SatInterference::InterferenceChangeEvent> event)
{
    NS_LOG_FUNCTION(this);

    std::pair<std::set<uint32_t>::iterator, bool> result = m_rxEventIds.insert(event->GetId());

    NS_ASSERT(result.second);
    m_rxing = true;
}

void
SatPerPacketInterference::DoNotifyRxEnd(Ptr<SatInterference::InterferenceChangeEvent> event)
{
    NS_LOG_FUNCTION(this);

    m_rxEventIds.erase(event->GetId());

    if (m_rxEventIds.empty())
    {
        m_rxing = false;
    }
}

void
SatPerPacketInterference::DoDispose()
{
    NS_LOG_FUNCTION(this);

    SatInterference::DoDispose();
}

void
SatPerPacketInterference::SetRxBandwidth(double rxBandwidth)
{
    NS_LOG_FUNCTION(this << rxBandwidth);

    if (rxBandwidth <= std::numeric_limits<double>::epsilon())
    {
        NS_FATAL_ERROR("SatPerPacketInterference::SetRxBandwidth - Invalid value");
    }

    m_rxBandwidth_Hz = rxBandwidth;
}

} // namespace ns3

// namespace ns3
