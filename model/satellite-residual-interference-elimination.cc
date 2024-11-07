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
 * Author: Mathias Ettinger <mettinger@toulouse.viveris.com>
 */

#include "satellite-residual-interference-elimination.h"

#include "satellite-signal-parameters.h"
#include "satellite-utils.h"

#include <ns3/double.h>
#include <ns3/log.h>
#include <ns3/simulator.h>

#include <cmath>
#include <limits>
#include <utility>
#include <vector>

NS_LOG_COMPONENT_DEFINE("SatResidualInterferenceElimination");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(SatResidualInterferenceElimination);

TypeId
SatResidualInterferenceElimination::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::SatResidualInterferenceElimination")
            .SetParent<SatInterferenceElimination>()
            .AddConstructor<SatResidualInterferenceElimination>()
            .AddAttribute(
                "SamplingError",
                "Residual sampling error corresponding to E[g(τ)]/g(0) for the simulation",
                DoubleValue(0.99),
                MakeDoubleAccessor(&SatResidualInterferenceElimination::m_samplingError),
                MakeDoubleChecker<double>());

    return tid;
}

TypeId
SatResidualInterferenceElimination::GetInstanceTypeId(void) const
{
    return GetTypeId();
}

SatResidualInterferenceElimination::SatResidualInterferenceElimination()
    : SatInterferenceElimination(),
      m_waveformConf(0),
      m_samplingError(0.99)
{
    NS_LOG_FUNCTION(this);

    NS_FATAL_ERROR("SatPhy default constructor is not allowed to use");
}

SatResidualInterferenceElimination::SatResidualInterferenceElimination(
    Ptr<SatWaveformConf> waveformConf)
    : SatInterferenceElimination(),
      m_waveformConf(waveformConf),
      m_samplingError(0.99)
{
    NS_LOG_FUNCTION(this);
}

SatResidualInterferenceElimination::~SatResidualInterferenceElimination()
{
    NS_LOG_FUNCTION(this);
}

void
SatResidualInterferenceElimination::EliminateInterferences(
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
SatResidualInterferenceElimination::EliminateInterferences(
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
    double ifPowerToRemove;
    /// TODO: refactorize this call, so that the residual power is not recalculated
    /// at every iteration of SIC.
    double residualPower = GetResidualPower(processedPacket, EsNo);
    double normalizedTime = 0.0;
    std::vector<std::pair<double, double>> ifPowerPerFragment;

    if (isRegenerative)
    {
        oldIfPower = packetInterferedWith->GetInterferencePower();
        ifPowerToRemove = processedPacket->m_rxPower_W;
        ifPowerPerFragment = packetInterferedWith->GetInterferencePowerPerFragment();
    }
    else
    {
        oldIfPower = packetInterferedWith->GetInterferencePowerInSatellite();
        ifPowerToRemove = processedPacket->GetRxPowerInSatellite();
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

        ifPower.second -= ifPowerToRemove;
        ifPower.second += residualPower;
        if (std::abs(ifPower.second) < 1.0e-30) // std::numeric_limits<double>::epsilon ())
        {
            ifPower.second = 0.0;
        }

        if (ifPower.second < 0)
        {
            NS_FATAL_ERROR("Negative interference");
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
SatResidualInterferenceElimination::GetResidualPower(Ptr<SatSignalParameters> processedPacket,
                                                     double EsNo)
{
    NS_LOG_FUNCTION(this);

    NS_LOG_INFO("SatResidualInterferenceElimination::GetResidualPower");

    double ifPowerToRemove = processedPacket->GetRxPowerInSatellite();
    uint32_t L = GetBurstLengthInSymbols(processedPacket->m_txInfo.waveformId);
    double sigma_lambda_2 = 1.0 / (8.0 * L * EsNo);
    double sigma_phy_2 = 1.0 / (2.0 * L * EsNo);
    return (2.0 + sigma_lambda_2 - (2.0 * m_samplingError * std::exp(-sigma_phy_2 / 2.0))) *
           ifPowerToRemove;
}

} // namespace ns3
