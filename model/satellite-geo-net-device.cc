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
 * Author: Sami Rantanen <sami.rantanen@magister.fi>
 */

#include "satellite-geo-net-device.h"

#include "satellite-address-tag.h"
#include "satellite-channel.h"
#include "satellite-geo-feeder-phy.h"
#include "satellite-geo-user-phy.h"
#include "satellite-ground-station-address-tag.h"
#include "satellite-id-mapper.h"
#include "satellite-mac.h"
#include "satellite-orbiter-feeder-mac.h"
#include "satellite-orbiter-user-mac.h"
#include "satellite-phy-rx.h"
#include "satellite-phy-tx.h"
#include "satellite-phy.h"
#include "satellite-time-tag.h"
#include "satellite-uplink-info-tag.h"

#include <ns3/channel.h>
#include <ns3/error-model.h>
#include <ns3/ipv4-header.h>
#include <ns3/ipv4-l3-protocol.h>
#include <ns3/log.h>
#include <ns3/node.h>
#include <ns3/object-map.h>
#include <ns3/packet.h>
#include <ns3/pointer.h>
#include <ns3/singleton.h>
#include <ns3/trace-source-accessor.h>
#include <ns3/uinteger.h>

NS_LOG_COMPONENT_DEFINE("SatGeoNetDevice");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(SatGeoNetDevice);

TypeId
SatGeoNetDevice::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::SatGeoNetDevice")
            .SetParent<NetDevice>()
            .AddConstructor<SatGeoNetDevice>()
            .AddAttribute("ReceiveErrorModel",
                          "The receiver error model used to simulate packet loss",
                          PointerValue(),
                          MakePointerAccessor(&SatGeoNetDevice::m_receiveErrorModel),
                          MakePointerChecker<ErrorModel>())
            .AddAttribute("UserPhy",
                          "The User Phy objects attached to this device.",
                          ObjectMapValue(),
                          MakeObjectMapAccessor(&SatGeoNetDevice::m_userPhy),
                          MakeObjectMapChecker<SatPhy>())
            .AddAttribute("FeederPhy",
                          "The Feeder Phy objects attached to this device.",
                          ObjectMapValue(),
                          MakeObjectMapAccessor(&SatGeoNetDevice::m_feederPhy),
                          MakeObjectMapChecker<SatPhy>())
            .AddAttribute("UserMac",
                          "The User MAC objects attached to this device.",
                          ObjectMapValue(),
                          MakeObjectMapAccessor(&SatGeoNetDevice::m_userMac),
                          MakeObjectMapChecker<SatMac>())
            .AddAttribute("FeederMac",
                          "The Feeder MAC objects attached to this device.",
                          ObjectMapValue(),
                          MakeObjectMapAccessor(&SatGeoNetDevice::m_feederMac),
                          MakeObjectMapChecker<SatMac>())
            .AddAttribute("EnableStatisticsTags",
                          "If true, some tags will be added to each transmitted packet to assist "
                          "with statistics computation",
                          BooleanValue(false),
                          MakeBooleanAccessor(&SatGeoNetDevice::m_isStatisticsTagsEnabled),
                          MakeBooleanChecker())
            .AddTraceSource("PacketTrace",
                            "Packet event trace",
                            MakeTraceSourceAccessor(&SatGeoNetDevice::m_packetTrace),
                            "ns3::SatTypedefs::PacketTraceCallback")
            .AddTraceSource("Tx",
                            "A packet to be sent",
                            MakeTraceSourceAccessor(&SatGeoNetDevice::m_txTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("SignallingTx",
                            "A signalling packet to be sent",
                            MakeTraceSourceAccessor(&SatGeoNetDevice::m_signallingTxTrace),
                            "ns3::SatTypedefs::PacketDestinationAddressCallback")
            .AddTraceSource("RxFeeder",
                            "A packet received on feeder",
                            MakeTraceSourceAccessor(&SatGeoNetDevice::m_rxFeederTrace),
                            "ns3::SatTypedefs::PacketSourceAddressCallback")
            .AddTraceSource("RxUser",
                            "A packet received on user",
                            MakeTraceSourceAccessor(&SatGeoNetDevice::m_rxUserTrace),
                            "ns3::SatTypedefs::PacketSourceAddressCallback")
            .AddTraceSource("RxFeederLinkDelay",
                            "A packet is received with feeder link delay information",
                            MakeTraceSourceAccessor(&SatGeoNetDevice::m_rxFeederLinkDelayTrace),
                            "ns3::SatTypedefs::PacketDelayAddressCallback")
            .AddTraceSource("RxFeederLinkJitter",
                            "A packet is received with feeder link jitter information",
                            MakeTraceSourceAccessor(&SatGeoNetDevice::m_rxFeederLinkJitterTrace),
                            "ns3::SatTypedefs::PacketJitterAddressCallback")
            .AddTraceSource("RxUserLinkDelay",
                            "A packet is received with feeder link delay information",
                            MakeTraceSourceAccessor(&SatGeoNetDevice::m_rxUserLinkDelayTrace),
                            "ns3::SatTypedefs::PacketDelayAddressCallback")
            .AddTraceSource("RxUserLinkJitter",
                            "A packet is received with feeder link jitter information",
                            MakeTraceSourceAccessor(&SatGeoNetDevice::m_rxUserLinkJitterTrace),
                            "ns3::SatTypedefs::PacketJitterAddressCallback");
    return tid;
}

SatGeoNetDevice::SatGeoNetDevice()
    : m_node(0),
      m_mtu(0xffff),
      m_ifIndex(0)
{
    NS_LOG_FUNCTION(this);
}

void
SatGeoNetDevice::ReceivePacketUser(Ptr<Packet> packet, const Address& userAddress)
{
    NS_LOG_FUNCTION(this << packet);
    NS_LOG_INFO("Receiving a packet: " << packet->GetUid());

    Mac48Address macUserAddress = Mac48Address::ConvertFrom(userAddress);

    m_packetTrace(Simulator::Now(),
                  SatEnums::PACKET_RECV,
                  SatEnums::NT_SAT,
                  m_nodeId,
                  macUserAddress,
                  SatEnums::LL_ND,
                  SatEnums::LD_RETURN,
                  SatUtils::GetPacketInfo(packet));

    /*
     * Invoke the `Rx` and `RxDelay` trace sources. We look at the packet's tags
     * for information, but cannot remove the tags because the packet is a const.
     */
    if (m_isStatisticsTagsEnabled)
    {
        Address addr = GetRxUtAddress(packet, SatEnums::LD_RETURN);

        m_rxUserTrace(packet, addr);

        SatDevLinkTimeTag linkTimeTag;
        if (packet->RemovePacketTag(linkTimeTag))
        {
            NS_LOG_DEBUG(this << " contains a SatDevLinkTimeTag tag");
            Time delay = Simulator::Now() - linkTimeTag.GetSenderTimestamp();
            m_rxUserLinkDelayTrace(delay, addr);
            if (m_lastDelays[macUserAddress].IsZero() == false)
            {
                Time jitter = Abs(delay - m_lastDelays[macUserAddress]);
                m_rxUserLinkJitterTrace(jitter, addr);
            }
            m_lastDelays[macUserAddress] = delay;
        }
    }

    SatGroundStationAddressTag groundStationAddressTag;
    if (!packet->PeekPacketTag(groundStationAddressTag))
    {
        NS_FATAL_ERROR("SatGroundStationAddressTag not found");
    }
    Mac48Address destination = groundStationAddressTag.GetGroundStationAddress();

    SatUplinkInfoTag satUplinkInfoTag;
    if (!packet->PeekPacketTag(satUplinkInfoTag))
    {
        NS_FATAL_ERROR("SatUplinkInfoTag not found");
    }

    if (m_gwConnected.count(destination))
    {
        if (m_isStatisticsTagsEnabled)
        {
            // Add a SatDevLinkTimeTag tag for packet link delay computation at the receiver end.
            packet->AddPacketTag(SatDevLinkTimeTag(Simulator::Now()));
        }

        DynamicCast<SatOrbiterFeederMac>(m_feederMac[satUplinkInfoTag.GetBeamId()])
            ->EnquePacket(packet);
    }
    else
    {
        if (m_islNetDevices.size() > 0)
        {
            SendToIsl(packet, destination);
        }
    }
}

void
SatGeoNetDevice::ReceivePacketFeeder(Ptr<Packet> packet, const Address& feederAddress)
{
    NS_LOG_FUNCTION(this << packet);
    NS_LOG_INFO("Receiving a packet: " << packet->GetUid());

    Mac48Address macFeederAddress = Mac48Address::ConvertFrom(feederAddress);

    m_packetTrace(Simulator::Now(),
                  SatEnums::PACKET_RECV,
                  SatEnums::NT_SAT,
                  m_nodeId,
                  macFeederAddress,
                  SatEnums::LL_ND,
                  SatEnums::LD_FORWARD,
                  SatUtils::GetPacketInfo(packet));

    /*
     * Invoke the `Rx` and `RxDelay` trace sources. We look at the packet's tags
     * for information, but cannot remove the tags because the packet is a const.
     */
    if (m_isStatisticsTagsEnabled)
    {
        Address addr = GetRxUtAddress(packet, SatEnums::LD_FORWARD);

        m_rxFeederTrace(packet, addr);

        SatDevLinkTimeTag linkTimeTag;
        if (packet->RemovePacketTag(linkTimeTag))
        {
            NS_LOG_DEBUG(this << " contains a SatDevLinkTimeTag tag");
            Time delay = Simulator::Now() - linkTimeTag.GetSenderTimestamp();
            m_rxFeederLinkDelayTrace(delay, addr);
            if (m_lastDelays[macFeederAddress].IsZero() == false)
            {
                Time jitter = Abs(delay - m_lastDelays[macFeederAddress]);
                m_rxFeederLinkJitterTrace(jitter, addr);
            }
            m_lastDelays[macFeederAddress] = delay;
        }
    }

    SatGroundStationAddressTag groundStationAddressTag;
    if (!packet->PeekPacketTag(groundStationAddressTag))
    {
        NS_FATAL_ERROR("SatGroundStationAddressTag not found");
    }
    Mac48Address destination = groundStationAddressTag.GetGroundStationAddress();

    if (destination.IsBroadcast())
    {
        m_broadcastReceived.insert(packet->GetUid());
    }

    SatUplinkInfoTag satUplinkInfoTag;
    if (!packet->PeekPacketTag(satUplinkInfoTag))
    {
        NS_FATAL_ERROR("SatUplinkInfoTag not found");
    }

    if (m_utConnected.count(destination) > 0 || destination.IsBroadcast())
    {
        if (m_isStatisticsTagsEnabled)
        {
            // Add a SatDevLinkTimeTag tag for packet link delay computation at the receiver end.
            packet->AddPacketTag(SatDevLinkTimeTag(Simulator::Now()));
        }

        DynamicCast<SatOrbiterUserMac>(m_userMac[satUplinkInfoTag.GetBeamId()])
            ->EnquePacket(packet);
    }
    if ((m_utConnected.count(destination) == 0 || destination.IsBroadcast()) &&
        m_islNetDevices.size() > 0)
    {
        SendToIsl(packet, destination);
    }
}

void
SatGeoNetDevice::ReceiveUser(SatPhy::PacketContainer_t /*packets*/,
                             Ptr<SatSignalParameters> rxParams)
{
    NS_LOG_FUNCTION(this << rxParams->m_packetsInBurst.size() << rxParams);
    NS_LOG_INFO("Receiving a packet at the satellite from user link");

    switch (m_returnLinkRegenerationMode)
    {
    case SatEnums::TRANSPARENT:
    case SatEnums::REGENERATION_PHY: {
        DynamicCast<SatGeoFeederPhy>(m_feederPhy[rxParams->m_beamId])->SendPduWithParams(rxParams);
        break;
    }
    case SatEnums::REGENERATION_LINK: {
        for (SatPhy::PacketContainer_t::const_iterator it = rxParams->m_packetsInBurst.begin();
             it != rxParams->m_packetsInBurst.end();
             ++it)
        {
            DynamicCast<SatOrbiterFeederMac>(m_feederMac[rxParams->m_beamId])->EnquePacket(*it);
        }

        break;
    }
    case SatEnums::REGENERATION_NETWORK: {
        NS_FATAL_ERROR(
            "SatGeoNetDevice::ReceiveUser should not be used in case of network regeneration");
    }
    default: {
        NS_FATAL_ERROR("Not implemented yet");
    }
    }
}

void
SatGeoNetDevice::ReceiveFeeder(SatPhy::PacketContainer_t /*packets*/,
                               Ptr<SatSignalParameters> rxParams)
{
    NS_LOG_FUNCTION(this << rxParams->m_packetsInBurst.size() << rxParams);
    NS_LOG_INFO("Receiving a packet at the satellite from feeder link");

    switch (m_forwardLinkRegenerationMode)
    {
    case SatEnums::TRANSPARENT:
    case SatEnums::REGENERATION_PHY: {
        DynamicCast<SatGeoUserPhy>(m_userPhy[rxParams->m_beamId])->SendPduWithParams(rxParams);
        break;
    }
    case SatEnums::REGENERATION_NETWORK: {
        NS_FATAL_ERROR(
            "SatGeoNetDevice::ReceiveFeeder should not be used in case of network regeneration");
    }
    default: {
        NS_FATAL_ERROR("Not implemented yet");
    }
    }
}

bool
SatGeoNetDevice::SendControlMsgToFeeder(Ptr<SatControlMessage> msg,
                                        const Address& dest,
                                        Ptr<SatSignalParameters> rxParams)
{
    NS_LOG_FUNCTION(this << msg << dest);

    Ptr<Packet> packet = Create<Packet>(msg->GetSizeInBytes());

    if (m_isStatisticsTagsEnabled)
    {
        // Add a SatAddressTag tag with this device's address as the source address.
        packet->AddByteTag(SatAddressTag(m_address));

        // Add a SatDevLinkTimeTag tag for packet link delay computation at the receiver end.
        packet->AddPacketTag(SatDevLinkTimeTag(Simulator::Now()));
    }

    SatAddressE2ETag addressE2ETag;
    addressE2ETag.SetE2ESourceAddress(m_address);
    addressE2ETag.SetE2EDestAddress(Mac48Address::ConvertFrom(dest));
    packet->AddPacketTag(addressE2ETag);

    SatMacTag macTag;
    macTag.SetSourceAddress(m_address);
    macTag.SetDestAddress(Mac48Address::ConvertFrom(dest));
    packet->AddPacketTag(macTag);

    // Add control tag to message and write msg to container in MAC
    SatControlMsgTag tag;
    tag.SetMsgId(0);
    tag.SetMsgType(msg->GetMsgType());
    packet->AddPacketTag(tag);

    if (m_returnLinkRegenerationMode != SatEnums::TRANSPARENT)
    {
        SatUplinkInfoTag satUplinkInfoTag;
        satUplinkInfoTag.SetSinr(std::numeric_limits<double>::infinity(), 0);
        satUplinkInfoTag.SetBeamId(rxParams->m_beamId);
        satUplinkInfoTag.SetSatId(rxParams->m_satId);
        packet->AddPacketTag(satUplinkInfoTag);
    }

    rxParams->m_packetsInBurst.clear();
    rxParams->m_packetsInBurst.push_back(packet);

    switch (m_returnLinkRegenerationMode)
    {
    case SatEnums::TRANSPARENT:
    case SatEnums::REGENERATION_PHY: {
        DynamicCast<SatGeoFeederPhy>(m_feederPhy[rxParams->m_beamId])->SendPduWithParams(rxParams);
        break;
    }
    case SatEnums::REGENERATION_LINK:
    case SatEnums::REGENERATION_NETWORK: {
        for (SatPhy::PacketContainer_t::const_iterator it = rxParams->m_packetsInBurst.begin();
             it != rxParams->m_packetsInBurst.end();
             ++it)
        {
            DynamicCast<SatOrbiterFeederMac>(m_feederMac[rxParams->m_beamId])->EnquePacket(*it);
        }
        break;
    }
    default: {
        NS_FATAL_ERROR("Not implemented yet");
    }
    }

    return true;
}

void
SatGeoNetDevice::SetReceiveErrorModel(Ptr<ErrorModel> em)
{
    NS_LOG_FUNCTION(this << em);
    m_receiveErrorModel = em;
}

void
SatGeoNetDevice::SetForwardLinkRegenerationMode(
    SatEnums::RegenerationMode_t forwardLinkRegenerationMode)
{
    m_forwardLinkRegenerationMode = forwardLinkRegenerationMode;
}

void
SatGeoNetDevice::SetReturnLinkRegenerationMode(
    SatEnums::RegenerationMode_t returnLinkRegenerationMode)
{
    m_returnLinkRegenerationMode = returnLinkRegenerationMode;
}

void
SatGeoNetDevice::SetNodeId(uint32_t nodeId)
{
    m_nodeId = nodeId;
}

void
SatGeoNetDevice::SetIfIndex(const uint32_t index)
{
    NS_LOG_FUNCTION(this << index);
    m_ifIndex = index;
}

uint32_t
SatGeoNetDevice::GetIfIndex(void) const
{
    NS_LOG_FUNCTION(this);
    return m_ifIndex;
}

void
SatGeoNetDevice::SetAddress(Address address)
{
    NS_LOG_FUNCTION(this << address);
    m_address = Mac48Address::ConvertFrom(address);
}

Address
SatGeoNetDevice::GetAddress(void) const
{
    //
    // Implicit conversion from Mac48Address to Address
    //
    NS_LOG_FUNCTION(this);
    return m_address;
}

bool
SatGeoNetDevice::SetMtu(const uint16_t mtu)
{
    NS_LOG_FUNCTION(this << mtu);
    m_mtu = mtu;
    return true;
}

uint16_t
SatGeoNetDevice::GetMtu(void) const
{
    NS_LOG_FUNCTION(this);
    return m_mtu;
}

bool
SatGeoNetDevice::IsLinkUp(void) const
{
    NS_LOG_FUNCTION(this);
    return true;
}

void
SatGeoNetDevice::AddLinkChangeCallback(Callback<void> callback)
{
    NS_LOG_FUNCTION(this << &callback);
}

bool
SatGeoNetDevice::IsBroadcast(void) const
{
    NS_LOG_FUNCTION(this);
    return true;
}

Address
SatGeoNetDevice::GetBroadcast(void) const
{
    NS_LOG_FUNCTION(this);
    return Mac48Address("ff:ff:ff:ff:ff:ff");
}

bool
SatGeoNetDevice::IsMulticast(void) const
{
    NS_LOG_FUNCTION(this);
    return false;
}

Address
SatGeoNetDevice::GetMulticast(Ipv4Address multicastGroup) const
{
    NS_LOG_FUNCTION(this << multicastGroup);
    return Mac48Address::GetMulticast(multicastGroup);
}

Address
SatGeoNetDevice::GetMulticast(Ipv6Address addr) const
{
    NS_LOG_FUNCTION(this << addr);
    return Mac48Address::GetMulticast(addr);
}

bool
SatGeoNetDevice::IsPointToPoint(void) const
{
    NS_LOG_FUNCTION(this);
    return false;
}

bool
SatGeoNetDevice::IsBridge(void) const
{
    NS_LOG_FUNCTION(this);
    return false;
}

bool
SatGeoNetDevice::Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(this << packet << dest << protocolNumber);

    /**
     * The satellite does not have higher protocol layers which
     * utilize the Send method! Thus, this method should not be used!
     */
    NS_ASSERT(false);
    return false;
}

bool
SatGeoNetDevice::SendFrom(Ptr<Packet> packet,
                          const Address& source,
                          const Address& dest,
                          uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(this << packet << source << dest << protocolNumber);

    /**
     * The satellite does not have higher protocol layers which
     * utilize the SendFrom method! Thus, this method should not be used!
     */
    NS_ASSERT(false);
    return false;
}

Ptr<Node>
SatGeoNetDevice::GetNode(void) const
{
    NS_LOG_FUNCTION(this);
    return m_node;
}

void
SatGeoNetDevice::SetNode(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this << node);
    m_node = node;
}

bool
SatGeoNetDevice::NeedsArp(void) const
{
    NS_LOG_FUNCTION(this);
    return false;
}

void
SatGeoNetDevice::SetReceiveCallback(NetDevice::ReceiveCallback cb)
{
    NS_LOG_FUNCTION(this << &cb);
}

void
SatGeoNetDevice::DoDispose(void)
{
    NS_LOG_FUNCTION(this);
    m_node = 0;
    m_receiveErrorModel = 0;
    m_userPhy.clear();
    m_feederPhy.clear();
    m_userMac.clear();
    m_feederMac.clear();
    m_addressMapFeeder.clear();
    m_addressMapUser.clear();
    NetDevice::DoDispose();
}

void
SatGeoNetDevice::SetPromiscReceiveCallback(PromiscReceiveCallback cb)
{
    NS_LOG_FUNCTION(this << &cb);
    m_promiscCallback = cb;
}

bool
SatGeoNetDevice::SupportsSendFrom(void) const
{
    NS_LOG_FUNCTION(this);
    return false;
}

Ptr<Channel>
SatGeoNetDevice::GetChannel(void) const
{
    NS_LOG_FUNCTION(this);
    return nullptr;
}

void
SatGeoNetDevice::AddUserPhy(Ptr<SatPhy> phy, uint32_t beamId)
{
    NS_LOG_FUNCTION(this << phy << beamId);
    m_userPhy.insert(std::pair<uint32_t, Ptr<SatPhy>>(beamId, phy));
}

void
SatGeoNetDevice::AddFeederPhy(Ptr<SatPhy> phy, uint32_t beamId)
{
    NS_LOG_FUNCTION(this << phy << beamId);
    m_feederPhy.insert(std::pair<uint32_t, Ptr<SatPhy>>(beamId, phy));
}

Ptr<SatPhy>
SatGeoNetDevice::GetUserPhy(uint32_t beamId)
{
    if (m_userPhy.count(beamId))
    {
        return m_userPhy[beamId];
    }
    NS_FATAL_ERROR("User Phy does not exist for beam " << beamId);
}

Ptr<SatPhy>
SatGeoNetDevice::GetFeederPhy(uint32_t beamId)
{
    if (m_userPhy.count(beamId))
    {
        return m_feederPhy[beamId];
    }
    NS_FATAL_ERROR("User Phy does not exist for beam " << beamId);
}

std::map<uint32_t, Ptr<SatPhy>>
SatGeoNetDevice::GetUserPhy()
{
    return m_userPhy;
}

std::map<uint32_t, Ptr<SatPhy>>
SatGeoNetDevice::GetFeederPhy()
{
    return m_feederPhy;
}

void
SatGeoNetDevice::AddUserMac(Ptr<SatMac> mac, uint32_t beamId)
{
    NS_LOG_FUNCTION(this << mac << beamId);
    m_userMac.insert(std::pair<uint32_t, Ptr<SatMac>>(beamId, mac));
}

void
SatGeoNetDevice::AddFeederMac(Ptr<SatMac> mac, Ptr<SatMac> macUsed, uint32_t beamId)
{
    NS_LOG_FUNCTION(this << mac << macUsed << beamId);
    m_feederMac.insert(std::pair<uint32_t, Ptr<SatMac>>(beamId, macUsed));
    m_allFeederMac.insert(std::pair<uint32_t, Ptr<SatMac>>(beamId, mac));
}

Ptr<SatMac>
SatGeoNetDevice::GetUserMac(uint32_t beamId)
{
    if (m_userMac.count(beamId))
    {
        return m_userMac[beamId];
    }
    NS_FATAL_ERROR("User MAC does not exist for beam " << beamId);
}

Ptr<SatMac>
SatGeoNetDevice::GetFeederMac(uint32_t beamId)
{
    if (m_feederMac.count(beamId))
    {
        return m_feederMac[beamId];
    }
    NS_FATAL_ERROR("User MAC does not exist for beam " << beamId);
}

std::map<uint32_t, Ptr<SatMac>>
SatGeoNetDevice::GetUserMac()
{
    return m_userMac;
}

std::map<uint32_t, Ptr<SatMac>>
SatGeoNetDevice::GetFeederMac()
{
    return m_feederMac;
}

std::map<uint32_t, Ptr<SatMac>>
SatGeoNetDevice::GetAllFeederMac()
{
    return m_allFeederMac;
}

void
SatGeoNetDevice::AddFeederPair(uint32_t beamId, Mac48Address satelliteFeederAddress)
{
    NS_LOG_FUNCTION(this << beamId << satelliteFeederAddress);

    m_addressMapFeeder.insert(std::pair<uint32_t, Mac48Address>(beamId, satelliteFeederAddress));
}

void
SatGeoNetDevice::AddUserPair(uint32_t beamId, Mac48Address satelliteUserAddress)
{
    NS_LOG_FUNCTION(this << beamId << satelliteUserAddress);

    m_addressMapUser.insert(std::pair<uint32_t, Mac48Address>(beamId, satelliteUserAddress));
}

Mac48Address
SatGeoNetDevice::GetSatelliteFeederAddress(uint32_t beamId)
{
    NS_LOG_FUNCTION(this << beamId);

    if (m_addressMapFeeder.count(beamId))
    {
        return m_addressMapFeeder[beamId];
    }
    NS_FATAL_ERROR("Satellite MAC does not exist for beam " << beamId);
}

Mac48Address
SatGeoNetDevice::GetSatelliteUserAddress(uint32_t beamId)
{
    NS_LOG_FUNCTION(this << beamId);

    if (m_addressMapUser.count(beamId))
    {
        return m_addressMapUser[beamId];
    }
    NS_FATAL_ERROR("Satellite MAC does not exist for beam " << beamId);
}

Address
SatGeoNetDevice::GetRxUtAddress(Ptr<Packet> packet, SatEnums::SatLinkDir_t ld)
{
    NS_LOG_FUNCTION(this << packet);

    Address utAddr; // invalid address.

    SatAddressE2ETag addressE2ETag;
    if (packet->PeekPacketTag(addressE2ETag))
    {
        NS_LOG_DEBUG(this << " contains a SatE2E tag");
        if (ld == SatEnums::LD_FORWARD)
        {
            utAddr = addressE2ETag.GetE2EDestAddress();
        }
        else if (ld == SatEnums::LD_RETURN)
        {
            utAddr = addressE2ETag.GetE2ESourceAddress();
        }
    }

    return utAddr;
}

void
SatGeoNetDevice::ConnectGw(Mac48Address gwAddress, uint32_t beamId)
{
    NS_LOG_FUNCTION(this << gwAddress << beamId);

    NS_ASSERT_MSG(m_gwConnected.find(gwAddress) == m_gwConnected.end(),
                  "Cannot add same GW twice to map");

    m_gwConnected.insert({gwAddress, beamId});
    Singleton<SatIdMapper>::Get()->AttachMacToSatIdIsl(gwAddress, m_nodeId);

    if (m_feederMac.find(beamId) != m_feederMac.end())
    {
        Ptr<SatOrbiterFeederMac> geoFeederMac =
            DynamicCast<SatOrbiterFeederMac>(GetFeederMac(beamId));
        NS_ASSERT(geoFeederMac != nullptr);
        {
            geoFeederMac->AddPeer(gwAddress);
        }
    }
}

void
SatGeoNetDevice::DisconnectGw(Mac48Address gwAddress, uint32_t beamId)
{
    NS_LOG_FUNCTION(this << gwAddress << beamId);

    NS_ASSERT_MSG(m_gwConnected.find(gwAddress) != m_gwConnected.end(), "GW not in map");

    m_gwConnected.erase(gwAddress);
    Singleton<SatIdMapper>::Get()->RemoveMacToSatIdIsl(gwAddress);

    if (m_feederMac.find(beamId) != m_feederMac.end())
    {
        Ptr<SatOrbiterFeederMac> geoFeederMac =
            DynamicCast<SatOrbiterFeederMac>(GetFeederMac(beamId));
        NS_ASSERT(geoFeederMac != nullptr);
        {
            geoFeederMac->RemovePeer(gwAddress);
        }
    }
}

std::set<Mac48Address>
SatGeoNetDevice::GetGwConnected()
{
    NS_LOG_FUNCTION(this);

    std::set<Mac48Address> gws;
    std::map<Mac48Address, uint32_t>::iterator it;
    for (it = m_gwConnected.begin(); it != m_gwConnected.end(); it++)
    {
        gws.insert(it->first);
    }

    return gws;
}

void
SatGeoNetDevice::ConnectUt(Mac48Address utAddress, uint32_t beamId)
{
    NS_LOG_FUNCTION(this << utAddress << beamId);

    NS_ASSERT_MSG(m_utConnected.find(utAddress) == m_utConnected.end(),
                  "Cannot add same UT twice to map");

    m_utConnected.insert({utAddress, beamId});
    Singleton<SatIdMapper>::Get()->AttachMacToSatIdIsl(utAddress, m_nodeId);

    if (m_userMac.find(beamId) != m_userMac.end())
    {
        Ptr<SatOrbiterUserMac> geoUserMac = DynamicCast<SatOrbiterUserMac>(GetUserMac(beamId));
        NS_ASSERT(geoUserMac != nullptr);
        {
            geoUserMac->AddPeer(utAddress);
        }
    }
}

void
SatGeoNetDevice::DisconnectUt(Mac48Address utAddress, uint32_t beamId)
{
    NS_LOG_FUNCTION(this << utAddress << beamId);

    NS_ASSERT_MSG(m_utConnected.find(utAddress) != m_utConnected.end(), "UT not in map");

    m_utConnected.erase(utAddress);
    Singleton<SatIdMapper>::Get()->RemoveMacToSatIdIsl(utAddress);

    if (m_userMac.find(beamId) != m_userMac.end())
    {
        Ptr<SatOrbiterUserMac> geoUserMac = DynamicCast<SatOrbiterUserMac>(GetUserMac(beamId));
        NS_ASSERT(geoUserMac != nullptr);
        {
            geoUserMac->RemovePeer(utAddress);
        }
    }
}

std::set<Mac48Address>
SatGeoNetDevice::GetUtConnected()
{
    NS_LOG_FUNCTION(this);

    std::set<Mac48Address> uts;
    std::map<Mac48Address, uint32_t>::iterator it;
    for (it = m_utConnected.begin(); it != m_utConnected.end(); it++)
    {
        uts.insert(it->first);
    }

    return uts;
}

void
SatGeoNetDevice::AddIslsNetDevice(Ptr<PointToPointIslNetDevice> islNetDevices)
{
    NS_LOG_FUNCTION(this);

    m_islNetDevices.push_back(islNetDevices);
}

std::vector<Ptr<PointToPointIslNetDevice>>
SatGeoNetDevice::GetIslsNetDevices()
{
    NS_LOG_FUNCTION(this);

    return m_islNetDevices;
}

void
SatGeoNetDevice::SetArbiter(Ptr<SatIslArbiter> arbiter)
{
    NS_LOG_FUNCTION(this << arbiter);

    m_arbiter = arbiter;
}

Ptr<SatIslArbiter>
SatGeoNetDevice::GetArbiter()
{
    NS_LOG_FUNCTION(this);

    return m_arbiter;
}

void
SatGeoNetDevice::SendToIsl(Ptr<Packet> packet, Mac48Address destination)
{
    NS_LOG_FUNCTION(this << packet << destination);

    // If ISLs, arbiter must be set
    NS_ASSERT_MSG(m_arbiter != nullptr, "Arbiter not set");

    if (destination.IsBroadcast())
    {
        // Send to all interfaces
        for (uint32_t i = 0; i < m_islNetDevices.size(); i++)
        {
            m_islNetDevices[i]->Send(packet->Copy(), Address(), 0x0800);
        }
        return;
    }

    int32_t islInterfaceIndex = m_arbiter->BaseDecide(packet, destination);

    if (islInterfaceIndex < 0)
    {
        NS_LOG_INFO("Cannot route packet form node " << m_nodeId << " to " << destination);
    }
    else if (uint32_t(islInterfaceIndex) >= m_islNetDevices.size())
    {
        NS_FATAL_ERROR("Incorrect interface index from arbiter: "
                       << islInterfaceIndex << ". Max is " << m_islNetDevices.size() - 1);
    }
    else
    {
        m_islNetDevices[islInterfaceIndex]->Send(packet, Address(), 0x0800);
    }
}

void
SatGeoNetDevice::ReceiveFromIsl(Ptr<Packet> packet, Mac48Address destination)
{
    NS_LOG_FUNCTION(this << packet << destination);

    if (destination.IsBroadcast())
    {
        if (m_broadcastReceived.count(packet->GetUid()) > 0)
        {
            // Packet already received, drop it
            return;
        }
        else
        {
            // Insert in list of receuived broadcast
            m_broadcastReceived.insert(packet->GetUid());
        }
    }

    if (m_gwConnected.count(destination) > 0)
    {
        SatUplinkInfoTag satUplinkInfoTag;
        if (!packet->PeekPacketTag(satUplinkInfoTag))
        {
            NS_FATAL_ERROR("SatUplinkInfoTag not found");
        }

        if (m_isStatisticsTagsEnabled)
        {
            // Add a SatDevLinkTimeTag tag for packet link delay computation at the receiver end.
            packet->AddPacketTag(SatDevLinkTimeTag(Simulator::Now()));
        }

        DynamicCast<SatOrbiterFeederMac>(m_feederMac[satUplinkInfoTag.GetBeamId()])
            ->EnquePacket(packet);
    }
    else
    {
        if (m_utConnected.count(destination) > 0 || destination.IsBroadcast())
        {
            SatUplinkInfoTag satUplinkInfoTag;
            if (!packet->PeekPacketTag(satUplinkInfoTag))
            {
                NS_FATAL_ERROR("SatUplinkInfoTag not found");
            }

            if (m_isStatisticsTagsEnabled && !destination.IsBroadcast())
            {
                // Add a SatDevLinkTimeTag tag for packet link delay computation at the receiver
                // end.
                packet->AddPacketTag(SatDevLinkTimeTag(Simulator::Now()));
            }

            DynamicCast<SatOrbiterUserMac>(m_userMac[satUplinkInfoTag.GetBeamId()])
                ->EnquePacket(packet);
        }
        if ((m_utConnected.count(destination) == 0 || destination.IsBroadcast()) &&
            m_islNetDevices.size() > 0)
        {
            SendToIsl(packet, destination);
        }
    }
}

} // namespace ns3
