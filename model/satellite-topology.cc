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

#include "satellite-topology.h"

#include <ns3/log.h>

NS_LOG_COMPONENT_DEFINE("SatTopology");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(SatTopology);

TypeId
SatTopology::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::SatTopology").SetParent<Object>().AddConstructor<SatTopology>();
    return tid;
}

SatTopology::SatTopology()
    : m_enableMapPrint(false)
{
    NS_LOG_FUNCTION(this);
}

SatTopology::~SatTopology()
{
    NS_LOG_FUNCTION(this);

    Reset();
}

void
SatTopology::DoDispose()
{
    NS_LOG_FUNCTION(this);

    Reset();

    Object::DoDispose();
}

void
SatTopology::Reset()
{
    NS_LOG_FUNCTION(this);

    m_gws = NodeContainer();
    m_gwIds.clear();
    m_uts = NodeContainer();
    m_orbiters = NodeContainer();
    m_utToGwMap.clear();

    m_enableMapPrint = false;
}

void
SatTopology::AddGwNode(uint32_t gwId, Ptr<Node> gw)
{
    NS_LOG_FUNCTION(this << gw);

    m_gwIds.insert(std::make_pair(gwId, gw));

    m_gws = NodeContainer();
    for (std::map<uint32_t, Ptr<Node>>::const_iterator i = m_gwIds.begin(); i != m_gwIds.end(); i++)
    {
        m_gws.Add(i->second);
    }
}

void
SatTopology::AddUtNode(Ptr<Node> ut)
{
    NS_LOG_FUNCTION(this << ut);

    m_uts.Add(ut);
}

void
SatTopology::AddOrbiterNode(Ptr<Node> orbiter)
{
    NS_LOG_FUNCTION(this << orbiter);

    m_orbiters.Add(orbiter);
}

void
SatTopology::ConnectGwToUt(Ptr<Node> ut, Ptr<Node> gw)
{
    NS_LOG_FUNCTION(this << ut << gw);

    if (m_utToGwMap.count(ut) > 0)
    {
        NS_FATAL_ERROR("UT " << ut << " already in GW to UT map. Connected to GW "
                             << m_utToGwMap[ut]);
    }

    m_utToGwMap[ut] = gw;
}

void
SatTopology::UpdateGwConnectedToUt(Ptr<Node> ut, Ptr<Node> gw)
{
    NS_LOG_FUNCTION(this << ut << gw);

    if (m_utToGwMap.count(ut) == 0)
    {
        NS_FATAL_ERROR("UT " << ut << " not in GW to UT map.");
    }

    m_utToGwMap[ut] = gw;
}

void
SatTopology::DisconnectGwFromUt(Ptr<Node> ut)
{
    NS_LOG_FUNCTION(this << ut);

    const std::map<Ptr<Node>, Ptr<Node>>::iterator it = m_utToGwMap.find(ut);
    if (it == m_utToGwMap.end())
    {
        NS_FATAL_ERROR("UT " << ut << " not in GW to UT map.");
    }
    else
    {
        m_utToGwMap.erase(it);
    }
}

NodeContainer
SatTopology::GetGwNodes() const
{
    NS_LOG_FUNCTION(this);

    return m_gws;
}

NodeContainer
SatTopology::GetUtNodes() const
{
    NS_LOG_FUNCTION(this);

    return m_uts;
}

NodeContainer
SatTopology::GetOrbiterNodes() const
{
    NS_LOG_FUNCTION(this);

    return m_orbiters;
}

uint32_t
SatTopology::GetNGwNodes() const
{
    NS_LOG_FUNCTION(this);

    return m_gws.GetN();
}

uint32_t
SatTopology::GetNUtNodes() const
{
    NS_LOG_FUNCTION(this);

    return m_uts.GetN();
}

uint32_t
SatTopology::GetNOrbiterNodes() const
{
    NS_LOG_FUNCTION(this);

    return m_orbiters.GetN();
}

Ptr<Node>
SatTopology::GetGwNode(uint32_t nodeId) const
{
    NS_LOG_FUNCTION(this << nodeId);

    return m_gws.Get(nodeId);
}

Ptr<Node>
SatTopology::GetUtNode(uint32_t nodeId) const
{
    NS_LOG_FUNCTION(this << nodeId);

    return m_uts.Get(nodeId);
}

Ptr<Node>
SatTopology::GetOrbiterNode(uint32_t nodeId) const
{
    NS_LOG_FUNCTION(this << nodeId);

    return m_orbiters.Get(nodeId);
}

void
SatTopology::AddGwLayers(Ptr<Node> gw,
                         uint32_t gwSatId,
                         uint32_t gwBeamId,
                         uint32_t utSatId,
                         uint32_t utBeamId,
                         Ptr<SatNetDevice> netDevice,
                         Ptr<SatGwLlc> llc,
                         Ptr<SatGwMac> mac,
                         Ptr<SatGwPhy> phy)
{
    NS_LOG_FUNCTION(this << gw << gwSatId << gwBeamId << utSatId << utBeamId << netDevice << llc
                         << mac << phy);

    std::cout << "SatTopology::AddGwLayers " << gw << " " << gwSatId << " " << gwBeamId << " "
              << utSatId << " " << utBeamId << std::endl;

    std::map<Ptr<Node>, GwLayers_s>::iterator it = m_gwLayers.find(gw);
    GwLayers_s layers;
    if (it != m_gwLayers.end())
    {
        layers = it->second;
        NS_ASSERT_MSG(
            layers.m_satId == gwSatId,
            "Gw has already a different GW satellite ID that the one in argument of this method");
        NS_ASSERT_MSG(
            layers.m_beamId == gwBeamId,
            "Gw has already a different GW beam ID that the one in argument of this method");
        NS_ASSERT_MSG(layers.m_netDevice.find(std::make_pair(utSatId, utBeamId)) ==
                          layers.m_netDevice.end(),
                      "Net device already stored for this GW + UT satellite and beam");
        NS_ASSERT_MSG(layers.m_llc.find(std::make_pair(utSatId, utBeamId)) == layers.m_llc.end(),
                      "LLC already stored for this GW + UT satellite and beam");
        NS_ASSERT_MSG(layers.m_mac.find(std::make_pair(utSatId, utBeamId)) == layers.m_mac.end(),
                      "MAC already stored for this GW + UT satellite and beam");
        NS_ASSERT_MSG(layers.m_phy.find(std::make_pair(utSatId, utBeamId)) == layers.m_phy.end(),
                      "Physical layer already stored for this GW + UT satellite and beam");
    }
    else
    {
        layers.m_satId = gwSatId;
        layers.m_beamId = gwBeamId;
    }

    layers.m_netDevice.insert(std::make_pair(std::make_pair(utSatId, utBeamId), netDevice));
    layers.m_llc.insert(std::make_pair(std::make_pair(utSatId, utBeamId), llc));
    layers.m_mac.insert(std::make_pair(std::make_pair(utSatId, utBeamId), mac));
    layers.m_phy.insert(std::make_pair(std::make_pair(utSatId, utBeamId), phy));

    m_gwLayers[gw] = layers;
}

void
SatTopology::UpdateGwSatAndBeam(Ptr<Node> gw, uint32_t satId, uint32_t beamId)
{
    NS_LOG_FUNCTION(this << gw << satId << beamId);

    NS_ASSERT_MSG(m_gwLayers.find(gw) != m_gwLayers.end(), "Layers do not exist for this GW");

    m_gwLayers[gw].m_satId = satId;
    m_gwLayers[gw].m_beamId = beamId;
}

uint32_t
SatTopology::GetGwSatId(Ptr<Node> gw) const
{
    NS_LOG_FUNCTION(this << gw);

    NS_ASSERT_MSG(m_gwLayers.find(gw) != m_gwLayers.end(), "Layers do not exist for this GW");

    return m_gwLayers.at(gw).m_satId;
}

uint32_t
SatTopology::GetGwBeamId(Ptr<Node> gw) const
{
    NS_LOG_FUNCTION(this << gw);

    NS_ASSERT_MSG(m_gwLayers.find(gw) != m_gwLayers.end(), "Layers do not exist for this GW");

    return m_gwLayers.at(gw).m_beamId;
}

Ptr<SatNetDevice>
SatTopology::GetGwNetDevice(Ptr<Node> gw, uint32_t utSatId, uint32_t utBeamId) const
{
    NS_LOG_FUNCTION(this << gw << utSatId << utBeamId);

    std::map<Ptr<Node>, GwLayers_s>::const_iterator it = m_gwLayers.find(gw);
    NS_ASSERT_MSG(it != m_gwLayers.end(),
                  "Layers do not exist for this GW and pair UT satellite+beam");

    GwLayers_s layers = it->second;
    NS_ASSERT_MSG(layers.m_netDevice.find(std::make_pair(utSatId, utBeamId)) !=
                      layers.m_netDevice.end(),
                  "Net device not stored for this GW + UT satellite and beam");

    return layers.m_netDevice.at(std::make_pair(utSatId, utBeamId));
}

Ptr<SatGwLlc>
SatTopology::GetGwLlc(Ptr<Node> gw, uint32_t utSatId, uint32_t utBeamId) const
{
    NS_LOG_FUNCTION(this << gw << utSatId << utBeamId);

    std::map<Ptr<Node>, GwLayers_s>::const_iterator it = m_gwLayers.find(gw);
    NS_ASSERT_MSG(it != m_gwLayers.end(),
                  "Layers do not exist for this GW and pair UT satellite+beam");

    GwLayers_s layers = it->second;
    NS_ASSERT_MSG(layers.m_llc.find(std::make_pair(utSatId, utBeamId)) != layers.m_llc.end(),
                  "LLC not stored for this GW + UT satellite and beam");

    return layers.m_llc.at(std::make_pair(utSatId, utBeamId));
}

Ptr<SatGwMac>
SatTopology::GetGwMac(Ptr<Node> gw, uint32_t utSatId, uint32_t utBeamId) const
{
    NS_LOG_FUNCTION(this << gw << utSatId << utBeamId);

    std::map<Ptr<Node>, GwLayers_s>::const_iterator it = m_gwLayers.find(gw);
    NS_ASSERT_MSG(it != m_gwLayers.end(),
                  "Layers do not exist for this GW and pair UT satellite+beam");

    GwLayers_s layers = it->second;
    NS_ASSERT_MSG(layers.m_mac.find(std::make_pair(utSatId, utBeamId)) != layers.m_mac.end(),
                  "MAC not stored for this GW + UT satellite and beam");

    return layers.m_mac.at(std::make_pair(utSatId, utBeamId));
}

Ptr<SatGwPhy>
SatTopology::GetGwPhy(Ptr<Node> gw, uint32_t utSatId, uint32_t utBeamId) const
{
    NS_LOG_FUNCTION(this << gw << utSatId << utBeamId);

    std::map<Ptr<Node>, GwLayers_s>::const_iterator it = m_gwLayers.find(gw);
    NS_ASSERT_MSG(it != m_gwLayers.end(),
                  "Layers do not exist for this GW and pair UT satellite+beam");

    GwLayers_s layers = it->second;
    NS_ASSERT_MSG(layers.m_phy.find(std::make_pair(utSatId, utBeamId)) != layers.m_phy.end(),
                  "Physical layer not stored for this GW + UT satellite and beam");

    return layers.m_phy.at(std::make_pair(utSatId, utBeamId));
}

void
SatTopology::AddUtLayers(Ptr<Node> ut,
                         uint32_t satId,
                         uint32_t beamId,
                         uint32_t groupId,
                         Ptr<SatNetDevice> netDevice,
                         Ptr<SatUtLlc> llc,
                         Ptr<SatUtMac> mac,
                         Ptr<SatUtPhy> phy)
{
    NS_LOG_FUNCTION(this << ut << satId << beamId << groupId << netDevice << llc << mac << phy);

    NS_ASSERT_MSG(m_utLayers.find(ut) == m_utLayers.end(), "Layers already added to this UT node");

    UtLayers_s layers;
    layers.m_satId = satId;
    layers.m_beamId = beamId;
    layers.m_groupId = groupId;
    layers.m_netDevice = netDevice;
    layers.m_llc = llc;
    layers.m_mac = mac;
    layers.m_phy = phy;

    m_utLayers.insert(std::make_pair(ut, layers));
}

void
SatTopology::UpdateUtSatAndBeam(Ptr<Node> ut, uint32_t satId, uint32_t beamId)
{
    NS_LOG_FUNCTION(this << ut << satId << beamId);

    NS_ASSERT_MSG(m_utLayers.find(ut) != m_utLayers.end(), "Layers do not exist for this UT");

    m_utLayers[ut].m_satId = satId;
    m_utLayers[ut].m_beamId = beamId;
}

void
SatTopology::UpdateUtGroup(Ptr<Node> ut, uint32_t groupId)
{
    NS_LOG_FUNCTION(this << ut << groupId);

    NS_ASSERT_MSG(m_utLayers.find(ut) != m_utLayers.end(), "Layers do not exist for this UT");

    m_utLayers[ut].m_groupId = groupId;
}

uint32_t
SatTopology::GetUtSatId(Ptr<Node> ut) const
{
    NS_LOG_FUNCTION(this << ut);

    NS_ASSERT_MSG(m_utLayers.find(ut) != m_utLayers.end(), "Layers do not exist for this UT");

    return m_utLayers.at(ut).m_satId;
}

uint32_t
SatTopology::GetUtBeamId(Ptr<Node> ut) const
{
    NS_LOG_FUNCTION(this << ut);

    NS_ASSERT_MSG(m_utLayers.find(ut) != m_utLayers.end(), "Layers do not exist for this UT");

    return m_utLayers.at(ut).m_beamId;
}

uint32_t
SatTopology::GetUtGroupId(Ptr<Node> ut) const
{
    NS_LOG_FUNCTION(this << ut);

    NS_ASSERT_MSG(m_utLayers.find(ut) != m_utLayers.end(), "Layers do not exist for this UT");

    return m_utLayers.at(ut).m_groupId;
}

Ptr<SatNetDevice>
SatTopology::GetUtNetDevice(Ptr<Node> ut) const
{
    NS_LOG_FUNCTION(this << ut);

    NS_ASSERT_MSG(m_utLayers.find(ut) != m_utLayers.end(), "Layers do not exist for this UT");

    return m_utLayers.at(ut).m_netDevice;
}

Ptr<SatUtLlc>
SatTopology::GetUtLlc(Ptr<Node> ut) const
{
    NS_LOG_FUNCTION(this << ut);

    NS_ASSERT_MSG(m_utLayers.find(ut) != m_utLayers.end(), "Layers do not exist for this UT");

    return m_utLayers.at(ut).m_llc;
}

Ptr<SatUtMac>
SatTopology::GetUtMac(Ptr<Node> ut) const
{
    NS_LOG_FUNCTION(this << ut);

    NS_ASSERT_MSG(m_utLayers.find(ut) != m_utLayers.end(), "Layers do not exist for this UT");

    return m_utLayers.at(ut).m_mac;
}

Ptr<SatUtPhy>
SatTopology::GetUtPhy(Ptr<Node> ut) const
{
    NS_LOG_FUNCTION(this << ut);

    NS_ASSERT_MSG(m_utLayers.find(ut) != m_utLayers.end(), "Layers do not exist for this UT");

    return m_utLayers.at(ut).m_phy;
}

void
SatTopology::AddOrbiterFeederLayers(Ptr<Node> orbiter,
                                    uint32_t satId,
                                    uint32_t utSatId,
                                    uint32_t utBeamId,
                                    Ptr<SatOrbiterNetDevice> netDevice,
                                    Ptr<SatOrbiterFeederLlc> llc,
                                    Ptr<SatOrbiterFeederMac> mac,
                                    Ptr<SatOrbiterFeederPhy> phy)
{
    NS_LOG_FUNCTION(this << orbiter << satId << utSatId << utBeamId << netDevice << llc << mac
                         << phy);

    std::map<Ptr<Node>, OrbiterLayers_s>::iterator it = m_orbiterLayers.find(orbiter);
    OrbiterLayers_s layers;
    if (it != m_orbiterLayers.end())
    {
        layers = it->second;
        NS_ASSERT_MSG(
            layers.m_satId == satId,
            "Orbiter has already a different satellite ID that the one in argument of this method");
        NS_ASSERT_MSG(layers.m_netDevice == netDevice,
                      "Orbiter has already a different SatOrbiterNetDevice that the one in "
                      "argument of this method");
        NS_ASSERT_MSG(layers.m_feederLlc.find(std::make_pair(utSatId, utBeamId)) ==
                          layers.m_feederLlc.end(),
                      "Feeder LLC already stored for this pair orbiter/beam");
        NS_ASSERT_MSG(layers.m_feederMac.find(std::make_pair(utSatId, utBeamId)) ==
                          layers.m_feederMac.end(),
                      "Feeder MAC already stored for this pair orbiter/beam");
        NS_ASSERT_MSG(layers.m_feederPhy.find(std::make_pair(utSatId, utBeamId)) ==
                          layers.m_feederPhy.end(),
                      "Feeder physical layer already stored for this pair orbiter/beam");
    }
    else
    {
        layers.m_satId = satId;
        layers.m_netDevice = netDevice;
    }

    layers.m_feederLlc.insert(std::make_pair(std::make_pair(utSatId, utBeamId), llc));
    layers.m_feederMac.insert(std::make_pair(std::make_pair(utSatId, utBeamId), mac));
    layers.m_feederPhy.insert(std::make_pair(std::make_pair(utSatId, utBeamId), phy));

    m_orbiterLayers[orbiter] = layers;
}

void
SatTopology::AddOrbiterUserLayers(Ptr<Node> orbiter,
                                  uint32_t satId,
                                  uint32_t beamId,
                                  Ptr<SatOrbiterNetDevice> netDevice,
                                  Ptr<SatOrbiterUserLlc> llc,
                                  Ptr<SatOrbiterUserMac> mac,
                                  Ptr<SatOrbiterUserPhy> phy)
{
    NS_LOG_FUNCTION(this << orbiter << satId << beamId << netDevice << llc << mac << phy);

    std::map<Ptr<Node>, OrbiterLayers_s>::iterator it = m_orbiterLayers.find(orbiter);
    OrbiterLayers_s layers;
    if (it != m_orbiterLayers.end())
    {
        layers = it->second;
        NS_ASSERT_MSG(
            layers.m_satId == satId,
            "Orbiter has already a different satellite ID that the one in argument of this method");
        NS_ASSERT_MSG(layers.m_netDevice == netDevice,
                      "Orbiter has already a different SatOrbiterNetDevice that the one in "
                      "argument of this method");
        NS_ASSERT_MSG(layers.m_userLlc.find(beamId) == layers.m_userLlc.end(),
                      "User LLC already stored for this pair orbiter/beam");
        NS_ASSERT_MSG(layers.m_userMac.find(beamId) == layers.m_userMac.end(),
                      "User MAC already stored for this pair orbiter/beam");
        NS_ASSERT_MSG(layers.m_userPhy.find(beamId) == layers.m_userPhy.end(),
                      "User physical layer already stored for this pair orbiter/beam");
    }
    else
    {
        layers.m_satId = satId;
        layers.m_netDevice = netDevice;
    }

    layers.m_userLlc.insert(std::make_pair(beamId, llc));
    layers.m_userMac.insert(std::make_pair(beamId, mac));
    layers.m_userPhy.insert(std::make_pair(beamId, phy));

    m_orbiterLayers[orbiter] = layers;
}

uint32_t
SatTopology::GetOrbiterSatId(Ptr<Node> orbiter) const
{
    NS_LOG_FUNCTION(this << orbiter);

    NS_ASSERT_MSG(m_orbiterLayers.find(orbiter) != m_orbiterLayers.end(),
                  "Layers do not exist for this UT");

    return m_orbiterLayers.at(orbiter).m_satId;
}

Ptr<SatOrbiterNetDevice>
SatTopology::GetOrbiterNetDevice(Ptr<Node> orbiter) const
{
    NS_LOG_FUNCTION(this << orbiter);

    NS_ASSERT_MSG(m_orbiterLayers.find(orbiter) != m_orbiterLayers.end(),
                  "Layers do not exist for this UT");

    return m_orbiterLayers.at(orbiter).m_netDevice;
}

Ptr<SatOrbiterFeederLlc>
SatTopology::GetOrbiterFeederLlc(Ptr<Node> orbiter, uint32_t utSatId, uint32_t utBeamId) const
{
    NS_LOG_FUNCTION(this << orbiter << utSatId << utBeamId);

    std::map<Ptr<Node>, OrbiterLayers_s>::const_iterator it = m_orbiterLayers.find(orbiter);
    NS_ASSERT_MSG(it != m_orbiterLayers.end(), "Layers do not exist for this UT");

    OrbiterLayers_s layers = it->second;
    NS_ASSERT_MSG(layers.m_feederLlc.find(std::make_pair(utSatId, utBeamId)) !=
                      layers.m_feederLlc.end(),
                  "Feeder LLC not stored for this pair orbiter/beam");

    return layers.m_feederLlc.at(std::make_pair(utSatId, utBeamId));
}

Ptr<SatOrbiterUserLlc>
SatTopology::GetOrbiterUserLlc(Ptr<Node> orbiter, uint32_t beamId) const
{
    NS_LOG_FUNCTION(this << orbiter << beamId);

    std::map<Ptr<Node>, OrbiterLayers_s>::const_iterator it = m_orbiterLayers.find(orbiter);
    NS_ASSERT_MSG(it != m_orbiterLayers.end(), "Layers do not exist for this UT");

    OrbiterLayers_s layers = it->second;
    NS_ASSERT_MSG(layers.m_userLlc.find(beamId) != layers.m_userLlc.end(),
                  "Feeder LLC not stored for this pair orbiter/beam");

    return layers.m_userLlc.at(beamId);
}

Ptr<SatOrbiterFeederMac>
SatTopology::GetOrbiterFeederMac(Ptr<Node> orbiter, uint32_t utSatId, uint32_t utBeamId) const
{
    NS_LOG_FUNCTION(this << orbiter << utSatId << utBeamId);

    std::map<Ptr<Node>, OrbiterLayers_s>::const_iterator it = m_orbiterLayers.find(orbiter);
    NS_ASSERT_MSG(it != m_orbiterLayers.end(), "Layers do not exist for this UT");

    OrbiterLayers_s layers = it->second;
    NS_ASSERT_MSG(layers.m_feederMac.find(std::make_pair(utSatId, utBeamId)) !=
                      layers.m_feederMac.end(),
                  "Feeder MAC not stored for this pair orbiter/beam");

    return layers.m_feederMac.at(std::make_pair(utSatId, utBeamId));
}

Ptr<SatOrbiterUserMac>
SatTopology::GetOrbiterUserMac(Ptr<Node> orbiter, uint32_t beamId) const
{
    NS_LOG_FUNCTION(this << orbiter << beamId);

    std::map<Ptr<Node>, OrbiterLayers_s>::const_iterator it = m_orbiterLayers.find(orbiter);
    NS_ASSERT_MSG(it != m_orbiterLayers.end(), "Layers do not exist for this UT");

    OrbiterLayers_s layers = it->second;
    NS_ASSERT_MSG(layers.m_userMac.find(beamId) != layers.m_userMac.end(),
                  "Feeder MAC not stored for this pair orbiter/beam");

    return layers.m_userMac.at(beamId);
}

Ptr<SatOrbiterFeederPhy>
SatTopology::GetOrbiterFeederPhy(Ptr<Node> orbiter, uint32_t utSatId, uint32_t utBeamId) const
{
    NS_LOG_FUNCTION(this << orbiter << utSatId << utBeamId);

    std::map<Ptr<Node>, OrbiterLayers_s>::const_iterator it = m_orbiterLayers.find(orbiter);
    NS_ASSERT_MSG(it != m_orbiterLayers.end(), "Layers do not exist for this UT");

    OrbiterLayers_s layers = it->second;
    NS_ASSERT_MSG(layers.m_feederPhy.find(std::make_pair(utSatId, utBeamId)) !=
                      layers.m_feederPhy.end(),
                  "Feeder MAC not stored for this pair orbiter/beam");

    return layers.m_feederPhy.at(std::make_pair(utSatId, utBeamId));
}

Ptr<SatOrbiterUserPhy>
SatTopology::GetOrbiterUserPhy(Ptr<Node> orbiter, uint32_t beamId) const
{
    NS_LOG_FUNCTION(this << orbiter << beamId);

    std::map<Ptr<Node>, OrbiterLayers_s>::const_iterator it = m_orbiterLayers.find(orbiter);
    NS_ASSERT_MSG(it != m_orbiterLayers.end(), "Layers do not exist for this UT");

    OrbiterLayers_s layers = it->second;
    NS_ASSERT_MSG(layers.m_userPhy.find(beamId) != layers.m_userPhy.end(),
                  "Feeder MAC not stored for this pair orbiter/beam");

    return layers.m_userPhy.at(beamId);
}

} // namespace ns3
