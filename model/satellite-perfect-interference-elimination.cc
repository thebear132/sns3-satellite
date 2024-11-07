/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Mathias Ettinger <mettinger@toulouse.viveris.fr>
 */

#include "satellite-perfect-interference-elimination.h"

#include "satellite-signal-parameters.h"

#include <ns3/log.h>
#include <ns3/simulator.h>

#include <cmath>
#include <limits>
#include <utility>
#include <vector>

NS_LOG_COMPONENT_DEFINE("SatPerfectInterferenceElimination");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(SatPerfectInterferenceElimination);

TypeId
SatPerfectInterferenceElimination::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::SatPerfectInterferenceElimination")
                            .SetParent<SatInterferenceElimination>()
                            .AddConstructor<SatPerfectInterferenceElimination>();

    return tid;
}

TypeId
SatPerfectInterferenceElimination::GetInstanceTypeId(void) const
{
    return GetTypeId();
}

SatPerfectInterferenceElimination::SatPerfectInterferenceElimination()
{
    NS_LOG_FUNCTION(this);
}

SatPerfectInterferenceElimination::~SatPerfectInterferenceElimination()
{
    NS_LOG_FUNCTION(this);
}

void
SatPerfectInterferenceElimination::EliminateInterferences(
    Ptr<SatSignalParameters> packetInterferedWith,
    Ptr<SatSignalParameters> processedPacket,
    double EsNo,
    bool isRegenerative)
{
    NS_LOG_FUNCTION(this);

    return EliminateInterferences(packetInterferedWith,
                                  processedPacket,
                                  EsNo,
                                  isRegenerative,
                                  0.0,
                                  1.0);
}

void
SatPerfectInterferenceElimination::EliminateInterferences(
    Ptr<SatSignalParameters> packetInterferedWith,
    Ptr<SatSignalParameters> processedPacket,
    double EsNo,
    bool isRegenerative,
    double startTime,
    double endTime)
{
    NS_LOG_FUNCTION(this);

    NS_LOG_INFO("Removing interference power of packet from Beam[Carrier] "
                << processedPacket->m_beamId << "[" << processedPacket->m_carrierId << "] between "
                << startTime << " and " << endTime);

    double oldIfPower;
    double normalizedTime = 0.0;
    std::vector<std::pair<double, double>> ifPowerPerFragment;
    if (isRegenerative)
    {
        oldIfPower = packetInterferedWith->GetInterferencePower();
        ifPowerPerFragment = packetInterferedWith->GetInterferencePowerPerFragment();
    }
    else
    {
        oldIfPower = packetInterferedWith->GetInterferencePowerInSatellite();
        ifPowerPerFragment = packetInterferedWith->GetInterferencePowerInSatellitePerFragment();
    }

    for (std::pair<double, double>& ifPower : ifPowerPerFragment)
    {
        normalizedTime += ifPower.first;
        if (startTime >= normalizedTime)
        {
            continue;
        }
        else if (endTime < normalizedTime)
        {
            break;
        }

        ifPower.second -= (isRegenerative ? processedPacket->m_rxPower_W
                                          : processedPacket->GetRxPowerInSatellite());
        if (std::abs(ifPower.second) < std::numeric_limits<double>::epsilon())
        {
            ifPower.second = 0.0;
        }

        if (ifPower.second < 0)
        {
            NS_FATAL_ERROR("Negative interference " << ifPower.second);
        }
    }

    if (isRegenerative)
    {
        packetInterferedWith->SetInterferencePower(ifPowerPerFragment);
    }
    else
    {
        packetInterferedWith->SetInterferencePowerInSatellite(ifPowerPerFragment);
    }

    NS_LOG_INFO("Interfered packet ifPower went from "
                << oldIfPower << " to " << SatUtils::ScalarProduct(ifPowerPerFragment));
}

double
SatPerfectInterferenceElimination::GetResidualPower(Ptr<SatSignalParameters> processedPacket,
                                                    double EsNo)
{
    return 0.0;
}

} // namespace ns3
