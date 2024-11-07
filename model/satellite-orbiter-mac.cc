/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Magister Solutions Ltd.
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
 */

#include "satellite-orbiter-mac.h"

#include "satellite-address-tag.h"
#include "satellite-mac.h"
#include "satellite-signal-parameters.h"
#include "satellite-time-tag.h"
#include "satellite-uplink-info-tag.h"
#include "satellite-utils.h"

#include <ns3/double.h>
#include <ns3/enum.h>
#include <ns3/log.h>
#include <ns3/pointer.h>
#include <ns3/simulator.h>
#include <ns3/uinteger.h>

#include <utility>

NS_LOG_COMPONENT_DEFINE("SatOrbiterMac");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(SatOrbiterMac);

TypeId
SatOrbiterMac::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::SatOrbiterMac")
            .SetParent<SatMac>()
            .AddAttribute(
                "DisableSchedulingIfNoDeviceConnected",
                "If true, the periodic calls of StartTransmission are not called when no "
                "devices are connected to this MAC",
                BooleanValue(false),
                MakeBooleanAccessor(&SatOrbiterMac::m_disableSchedulingIfNoDeviceConnected),
                MakeBooleanChecker())
            .AddTraceSource("BBFrameTxTrace",
                            "Trace for transmitted BB Frames.",
                            MakeTraceSourceAccessor(&SatOrbiterMac::m_bbFrameTxTrace),
                            "ns3::SatBbFrame::BbFrameCallback");

    return tid;
}

TypeId
SatOrbiterMac::GetInstanceTypeId(void) const
{
    NS_LOG_FUNCTION(this);

    return GetTypeId();
}

SatOrbiterMac::SatOrbiterMac(void)
{
    NS_LOG_FUNCTION(this);
    NS_FATAL_ERROR("SatOrbiterMac default constructor is not allowed to use");
}

SatOrbiterMac::SatOrbiterMac(uint32_t satId,
                             uint32_t beamId,
                             SatEnums::RegenerationMode_t forwardLinkRegenerationMode,
                             SatEnums::RegenerationMode_t returnLinkRegenerationMode)
    : SatMac(satId, beamId, forwardLinkRegenerationMode, returnLinkRegenerationMode),
      m_disableSchedulingIfNoDeviceConnected(false),
      m_fwdScheduler(),
      m_guardTime(MicroSeconds(1)),
      m_satId(satId),
      m_beamId(beamId),
      m_periodicTransmissionEnabled(false)
{
    NS_LOG_FUNCTION(this);
}

SatOrbiterMac::~SatOrbiterMac()
{
    NS_LOG_FUNCTION(this);
}

void
SatOrbiterMac::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Object::DoDispose();
}

void
SatOrbiterMac::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    Object::DoInitialize();
}

void
SatOrbiterMac::StartPeriodicTransmissions()
{
    NS_LOG_FUNCTION(this);

    if (m_disableSchedulingIfNoDeviceConnected && !HasPeer())
    {
        NS_LOG_INFO("Do not start beam " << m_beamId << " because no device is connected");
        return;
    }

    if (m_periodicTransmissionEnabled == true)
    {
        NS_LOG_INFO("Beam " << m_beamId << " already enabled");
        return;
    }

    m_periodicTransmissionEnabled = true;

    if (m_fwdScheduler == nullptr)
    {
        NS_FATAL_ERROR("Scheduler not set for orbiter MAC!!!");
    }

    m_llc->ClearQueues();

    Simulator::Schedule(Seconds(0), &SatOrbiterMac::StartTransmission, this, 0);
}

void
SatOrbiterMac::StartTransmission(uint32_t carrierId)
{
    NS_LOG_FUNCTION(this << carrierId);

    Time txDuration;

    if (m_txEnabled && (!m_disableSchedulingIfNoDeviceConnected || m_periodicTransmissionEnabled))
    {
        std::pair<Ptr<SatBbFrame>, const Time> bbFrameInfo = m_fwdScheduler->GetNextFrame();
        Ptr<SatBbFrame> bbFrame = bbFrameInfo.first;
        txDuration = bbFrameInfo.second;

        // trace out BB frames sent
        m_bbFrameTxTrace(bbFrame);

        // Handle both dummy frames and normal frames
        if (bbFrame != nullptr)
        {
            SatSignalParameters::txInfo_s txInfo;
            txInfo.packetType = SatEnums::PACKET_TYPE_DEDICATED_ACCESS;
            txInfo.modCod = bbFrame->GetModcod();
            txInfo.sliceId = bbFrame->GetSliceId();
            txInfo.frameType = bbFrame->GetFrameType();
            txInfo.waveformId = 0;

            /**
             * Decrease a guard time from BB frame duration.
             */
            SendPacket(bbFrame->GetPayload(), carrierId, txDuration - m_guardTime, txInfo);
        }
    }
    else
    {
        /**
         * Orbiter MAC is disabled, thus get the duration of the default BB frame
         * and try again then.
         */

        NS_LOG_INFO("TX is disabled, thus nothing is transmitted!");
        txDuration = m_fwdScheduler->GetDefaultFrameDuration();
    }

    if (m_periodicTransmissionEnabled)
    {
        Simulator::Schedule(txDuration, &SatOrbiterMac::StartTransmission, this, 0);
    }
}

void
SatOrbiterMac::SendPacket(SatPhy::PacketContainer_t packets,
                          uint32_t carrierId,
                          Time duration,
                          SatSignalParameters::txInfo_s txInfo)
{
    NS_LOG_FUNCTION(this);

    // Add a SatMacTimeTag tag for packet delay computation at the receiver end.
    SetTimeTag(packets);

    // Add packet trace entry:
    m_packetTrace(Simulator::Now(),
                  SatEnums::PACKET_SENT,
                  m_nodeInfo->GetNodeType(),
                  m_nodeInfo->GetNodeId(),
                  m_nodeInfo->GetMacAddress(),
                  SatEnums::LL_MAC,
                  GetSatLinkTxDir(),
                  SatUtils::GetPacketInfo(packets));

    Ptr<SatSignalParameters> txParams = Create<SatSignalParameters>();
    txParams->m_duration = duration;
    txParams->m_packetsInBurst = packets;
    txParams->m_satId = m_satId;
    txParams->m_beamId = m_beamId;
    txParams->m_carrierId = carrierId;
    txParams->m_txInfo = txInfo;

    // Use call back to send packet to lower layer
    m_txCallback(txParams);
}

void
SatOrbiterMac::RxTraces(SatPhy::PacketContainer_t packets)
{
    NS_LOG_FUNCTION(this);

    if (m_isStatisticsTagsEnabled)
    {
        for (SatPhy::PacketContainer_t::const_iterator it1 = packets.begin(); it1 != packets.end();
             ++it1)
        {
            // Remove packet tag
            SatMacTag macTag;
            bool mSuccess = (*it1)->PeekPacketTag(macTag);
            if (!mSuccess)
            {
                NS_FATAL_ERROR("MAC tag was not found from the packet!");
            }

            // If the packet is intended for this receiver
            Mac48Address destAddress = macTag.GetDestAddress();

            if (destAddress == m_nodeInfo->GetMacAddress())
            {
                Address addr = GetRxUtAddress(*it1);

                m_rxTrace(*it1, addr);

                SatMacLinkTimeTag linkTimeTag;
                if ((*it1)->RemovePacketTag(linkTimeTag))
                {
                    NS_LOG_DEBUG(this << " contains a SatMacLinkTimeTag tag");
                    Time delay = Simulator::Now() - linkTimeTag.GetSenderLinkTimestamp();
                    m_rxLinkDelayTrace(delay, addr);
                    if (m_lastLinkDelay.IsZero() == false)
                    {
                        Time jitter = Abs(delay - m_lastLinkDelay);
                        m_rxLinkJitterTrace(jitter, addr);
                    }
                    m_lastLinkDelay = delay;
                }
            } // end of `if (destAddress == m_nodeInfo->GetMacAddress () || destAddress.IsBroadcast
              // ())`
        }     // end of `for it1 = packets.begin () -> packets.end ()`
    }         // end of `if (m_isStatisticsTagsEnabled)`
}

void
SatOrbiterMac::SetFwdScheduler(Ptr<SatFwdLinkScheduler> fwdScheduler)
{
    m_fwdScheduler = fwdScheduler;
}

void
SatOrbiterMac::SetLlc(Ptr<SatOrbiterLlc> llc)
{
    m_llc = llc;
}

Time
SatOrbiterMac::GetGuardTime() const
{
    return m_guardTime;
}

void
SatOrbiterMac::SetGuardTime(Time guardTime)
{
    m_guardTime = guardTime;
}

void
SatOrbiterMac::SetTransmitCallback(TransmitCallback cb)
{
    NS_LOG_FUNCTION(this << &cb);
    m_txCallback = cb;
}

void
SatOrbiterMac::SetReceiveNetDeviceCallback(ReceiveNetDeviceCallback cb)
{
    NS_LOG_FUNCTION(this << &cb);
    m_rxNetDeviceCallback = cb;
}

void
SatOrbiterMac::StopPeriodicTransmissions()
{
    NS_LOG_FUNCTION(this);

    m_periodicTransmissionEnabled = false;

    m_llc->ClearQueues();
}

} // namespace ns3
