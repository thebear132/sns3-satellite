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
SatTopology::AddGwLayers(Ptr<Node> gw, uint32_t satId, uint32_t beamId, Ptr<SatNetDevice> netDevice, Ptr<SatGwLlc> llc, Ptr<SatGwMac> mac, Ptr<SatGwPhy> phy)
{
    NS_LOG_FUNCTION(this << gw << satId << beamId << netDevice << llc << mac << phy);

    NS_ASSERT_MSG(m_gwLayers.find(gw) == m_gwLayers.end(), "Layers already added to this GW node");

    GwLayers_s layers;
    layers.m_satId = satId;
    layers.m_beamId = beamId;
    layers.m_netDevice = netDevice;
    layers.m_llc = llc;
    layers.m_mac = mac;
    layers.m_phy = phy;

    m_gwLayers.insert(std::make_pair(gw, layers));
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
SatTopology::GetGwNetDevice(Ptr<Node> gw) const
{
    NS_LOG_FUNCTION(this << gw);

    NS_ASSERT_MSG(m_gwLayers.find(gw) != m_gwLayers.end(), "Layers do not exist for this GW");

    return m_gwLayers.at(gw).m_netDevice;
}

Ptr<SatGwLlc>
SatTopology::GetGwLlc(Ptr<Node> gw) const
{
    NS_LOG_FUNCTION(this << gw);

    NS_ASSERT_MSG(m_gwLayers.find(gw) != m_gwLayers.end(), "Layers do not exist for this GW");

    return m_gwLayers.at(gw).m_llc;
}

Ptr<SatGwMac>
SatTopology::GetGwMac(Ptr<Node> gw) const
{
    NS_LOG_FUNCTION(this << gw);

    NS_ASSERT_MSG(m_gwLayers.find(gw) != m_gwLayers.end(), "Layers do not exist for this GW");

    return m_gwLayers.at(gw).m_mac;
}

Ptr<SatGwPhy>
SatTopology::GetGwPhy(Ptr<Node> gw) const
{
    NS_LOG_FUNCTION(this << gw);

    NS_ASSERT_MSG(m_gwLayers.find(gw) != m_gwLayers.end(), "Layers do not exist for this GW");

    return m_gwLayers.at(gw).m_phy;
}

} // namespace ns3
