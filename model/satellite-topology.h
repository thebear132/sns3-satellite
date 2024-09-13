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
#ifndef SATELLITE_TOPOLOGY_H
#define SATELLITE_TOPOLOGY_H

#include "satellite-gw-llc.h"
#include "satellite-gw-mac.h"
#include "satellite-gw-phy.h"
#include "satellite-net-device.h"
#include "satellite-orbiter-feeder-mac.h"
#include "satellite-orbiter-feeder-phy.h"
#include "satellite-orbiter-net-device.h"
#include "satellite-orbiter-user-mac.h"
#include "satellite-orbiter-user-phy.h"
#include "satellite-ut-llc.h"
#include "satellite-ut-mac.h"
#include "satellite-ut-phy.h"

#include <ns3/node-container.h>
#include <ns3/node.h>
#include <ns3/object.h>

namespace ns3
{

/**
 * \ingroup satellite
 *
 * \brief Class to store topology of the whole system. It has the information of all the nodes of
 * the scenario, and all the links between them.
 */
class SatTopology : public Object
{
  public:
    typedef struct
    {
        uint32_t m_satId;
        uint32_t m_beamId;
        Ptr<SatNetDevice> m_netDevice;
        Ptr<SatGwLlc> m_llc;
        Ptr<SatGwMac> m_mac;
        Ptr<SatGwPhy> m_phy;
    } GwLayers_s;

    typedef struct
    {
        uint32_t m_satId;
        uint32_t m_beamId;
        uint32_t m_groupId;
        Ptr<SatNetDevice> m_netDevice;
        Ptr<SatUtLlc> m_llc;
        Ptr<SatUtMac> m_mac;
        Ptr<SatUtPhy> m_phy;
    } UtLayers_s;

    typedef struct
    {
        uint32_t m_satId;
        Ptr<SatOrbiterNetDevice> m_netDevice;
        std::map<uint32_t, Ptr<SatOrbiterFeederMac>> m_feederMac;
        std::map<uint32_t, Ptr<SatOrbiterUserMac>> m_userMac;
        std::map<uint32_t, Ptr<SatOrbiterFeederPhy>> m_feederPhy;
        std::map<uint32_t, Ptr<SatOrbiterUserPhy>> m_userPhy;
    } OrbiterLayers_s;

    /**
     * \brief Constructor
     */
    SatTopology();

    /**
     * \brief Destructor
     */
    ~SatTopology();

    /**
     * \brief NS-3 type id function
     * \return type id
     */
    static TypeId GetTypeId(void);

    /**
     *  \brief Do needed dispose actions.
     */
    void DoDispose();

    /**
     * \brief Function for resetting the variables
     */
    void Reset();

    /**
     * Add a GW node to the topology
     *
     * \param gwId ID of the GW
     * \param gw The GW node to add
     */
    void AddGwNode(uint32_t gwId, Ptr<Node> gw);

    /**
     * Add a UT node to the topology
     *
     * \param ut The UT node to add
     */
    void AddUtNode(Ptr<Node> ut);

    /**
     * Add an orbiter node to the topology
     *
     * \param orbiter The orbiter node to add
     */
    void AddOrbiterNode(Ptr<Node> orbiter);

    /**
     * Connect a GW to a UT. UT must not have an associated GW yet
     *
     * \param ut The UT to consider
     * \param gw The GW connected to this UT
     */
    void ConnectGwToUt(Ptr<Node> ut, Ptr<Node> gw);

    /**
     * Connect a new GW to a UT. UT must already have an associated GW
     *
     * \param ut The UT to consider
     * \param gw The GW connected to this UT
     */
    void UpdateGwConnectedToUt(Ptr<Node> ut, Ptr<Node> gw);

    /**
     * Disconnect a GW from a UT. UT must already have an associated GW
     *
     * \param ut The UT to consider
     */
    void DisconnectGwFromUt(Ptr<Node> ut);

    /**
     * Get the list of GW nodes
     *
     * \return The list of GW nodes
     */
    NodeContainer GetGwNodes() const;

    /**
     * Get the list of UT nodes
     *
     * \return The list of UT nodes
     */
    NodeContainer GetUtNodes() const;

    /**
     * Get the list of orbiter nodes
     *
     * \return The list of orbiter nodes
     */
    NodeContainer GetOrbiterNodes() const;

    /**
     * Get the number of GW nodes
     *
     * \return The number of GW nodes
     */
    uint32_t GetNGwNodes() const;

    /**
     * Get the number of UT nodes
     *
     * \return The number of UT nodes
     */
    uint32_t GetNUtNodes() const;

    /**
     * Get the number of orbiter nodes
     *
     * \return The number of orbiter nodes
     */
    uint32_t GetNOrbiterNodes() const;

    /**
     * Get the wanted GW node
     *
     * \param nodeId ID of the node needed (index in vector)
     *
     * \return The GW node
     */
    Ptr<Node> GetGwNode(uint32_t nodeId) const;

    /**
     * Get the wanted UT node
     *
     * \param nodeId ID of the node needed
     *
     * \return The UT node
     */
    Ptr<Node> GetUtNode(uint32_t nodeId) const;

    /**
     * Get the wanted orbiter node
     *
     * \param nodeId ID of the node needed
     *
     * \return The orbiter node
     */
    Ptr<Node> GetOrbiterNode(uint32_t nodeId) const;

    /**
     * Add GW layers for given node, associated to chosen satellite and beam
     *
     * \param gw GW node to consider
     * \param satId ID of satellite linked to this stack
     * \param beamId ID of beam linked to this stack
     * \param netDevice SatNetDevice of this node
     * \param llc LLC layer of this node
     * \param mac MAC layer of this node
     * \param phy PHY layer of this node
     */
    void AddGwLayers(Ptr<Node> gw,
                     uint32_t satId,
                     uint32_t beamId,
                     Ptr<SatNetDevice> netDevice,
                     Ptr<SatGwLlc> llc,
                     Ptr<SatGwMac> mac,
                     Ptr<SatGwPhy> phy);

    /**
     * Update satellite and beam associated to a GW
     *
     * \param gw GW node to consider
     * \param satId ID of satellite linked to this stack
     * \param beamId ID of beam linked to this stack
     */
    void UpdateGwSatAndBeam(Ptr<Node> gw, uint32_t satId, uint32_t beamId);

    /**
     * Get ID of satellite linked to a GW
     *
     * \param gw GW node to consider
     *
     * \return ID of satellite linked to this node
     */
    uint32_t GetGwSatId(Ptr<Node> gw) const;

    /**
     * Get ID of beam linked to a GW
     *
     * \param gw GW node to consider
     *
     * \return ID of beam linked to this node
     */
    uint32_t GetGwBeamId(Ptr<Node> gw) const;

    /**
     * Get SatNetDevice instance of a GW
     *
     * \param gw GW node to consider
     *
     * \return SatNetDevice instance of a GW
     */
    Ptr<SatNetDevice> GetGwNetDevice(Ptr<Node> gw) const;

    /**
     * Get SatGwLlc instance of a GW
     *
     * \param gw GW node to consider
     *
     * \return SatGwLlc instance of a GW
     */
    Ptr<SatGwLlc> GetGwLlc(Ptr<Node> gw) const;

    /**
     * Get SatGwMac instance of a GW
     *
     * \param gw GW node to consider
     *
     * \return SatGwMac instance of a GW
     */
    Ptr<SatGwMac> GetGwMac(Ptr<Node> gw) const;

    /**
     * Get SatGwPhy instance of a GW
     *
     * \param gw GW node to consider
     *
     * \return SatGwPhy instance of a GW
     */
    Ptr<SatGwPhy> GetGwPhy(Ptr<Node> gw) const;

    /**
     * Add UT layers for given node, associated to chosen satellite and beam
     *
     * \param ut UT node to consider
     * \param satId ID of satellite linked to this stack
     * \param beamId ID of beam linked to this stack
     * \param groupId ID of group linked to this stack
     * \param netDevice SatNetDevice of this node
     * \param llc LLC layer of this node
     * \param mac MAC layer of this node
     * \param phy PHY layer of this node
     */
    void AddUtLayers(Ptr<Node> ut,
                     uint32_t satId,
                     uint32_t beamId,
                     uint32_t groupId,
                     Ptr<SatNetDevice> netDevice,
                     Ptr<SatUtLlc> llc,
                     Ptr<SatUtMac> mac,
                     Ptr<SatUtPhy> phy);

    /**
     * Update satellite and beam associated to a UT
     *
     * \param ut UT node to consider
     * \param satId ID of satellite linked to this stack
     * \param beamId ID of beam linked to this stack
     * \param groupId ID of group linked to this stack
     */
    void UpdateUtSatAndBeam(Ptr<Node> ut, uint32_t satId, uint32_t beamId, uint32_t groupId);

    /**
     * Get ID of satellite linked to a UT
     *
     * \param ut UT node to consider
     *
     * \return ID of satellite linked to this node
     */
    uint32_t GetUtSatId(Ptr<Node> ut) const;

    /**
     * Get ID of beam linked to a UT
     *
     * \param ut UT node to consider
     *
     * \return ID of beam linked to this node
     */
    uint32_t GetUtBeamId(Ptr<Node> ut) const;

    /**
     * Get ID of group linked to a UT
     *
     * \param ut UT node to consider
     *
     * \return ID of group linked to this node
     */
    uint32_t GetUtGroupId(Ptr<Node> ut) const;

    /**
     * Get SatNetDevice instance of a UT
     *
     * \param ut UT node to consider
     *
     * \return SatNetDevice instance of a UT
     */
    Ptr<SatNetDevice> GetUtNetDevice(Ptr<Node> ut) const;

    /**
     * Get SatUtLlc instance of a UT
     *
     * \param ut UT node to consider
     *
     * \return SatUtLlc instance of a UT
     */
    Ptr<SatUtLlc> GetUtLlc(Ptr<Node> ut) const;

    /**
     * Get SatUtMac instance of a UT
     *
     * \param ut UT node to consider
     *
     * \return SatUtMac instance of a UT
     */
    Ptr<SatUtMac> GetUtMac(Ptr<Node> ut) const;

    /**
     * Get SatUtPhy instance of a UT
     *
     * \param ut UT node to consider
     *
     * \return SatUtPhy instance of a UT
     */
    Ptr<SatUtPhy> GetUtPhy(Ptr<Node> ut) const;

    /**
     * Add orbiter feeder layers for given satellite and beam ID
     *
     * \param orbiter UT node to consider
     * \param satId ID of satellite linked to this stack
     * \param beamId ID of beam linked to this stack
     * \param netDevice SatNetDevice of this node
     * \param mac MAC layer of this node
     * \param phy PHY layer of this node
     */
    void AddOrbiterFeederLayers(Ptr<Node> orbiter,
                                uint32_t satId,
                                uint32_t beamId,
                                Ptr<SatOrbiterNetDevice> netDevice,
                                Ptr<SatOrbiterFeederMac> mac,
                                Ptr<SatOrbiterFeederPhy> phy);

    /**
     * Add orbiter user layers for given satellite and beam ID
     *
     * \param orbiter UT node to consider
     * \param satId ID of satellite linked to this stack
     * \param beamId ID of beam linked to this stack
     * \param netDevice SatNetDevice of this node
     * \param mac MAC layer of this node
     * \param phy PHY layer of this node
     */
    void AddOrbiterUserLayers(Ptr<Node> orbiter,
                              uint32_t satId,
                              uint32_t beamId,
                              Ptr<SatOrbiterNetDevice> netDevice,
                              Ptr<SatOrbiterUserMac> mac,
                              Ptr<SatOrbiterUserPhy> phy);

    /**
     * Get ID of a given orbiter
     *
     * \param orbiter Orbiter node to consider
     *
     * \return ID of orbiter linked to this node
     */
    uint32_t GetOrbiterSatId(Ptr<Node> orbiter) const;

    /**
     * Get SatOrbiterNetDevice instance of an orbiter
     *
     * \param orbiter Orbiter node to consider
     *
     * \return SatOrbiterNetDevice instance of an orbiter
     */
    Ptr<SatOrbiterNetDevice> GetOrbiterNetDevice(Ptr<Node> orbiter) const;

    /**
     * Get SatOrbiterFeederMac instance of an orbiter serving wanted beam ID
     *
     * \param orbiter Orbiter node to consider
     * \param beamId Beam ID served by this feeder MAC layer
     *
     * \return SatOrbiterFeederMac instance of an orbiter
     */
    Ptr<SatOrbiterFeederMac> GetOrbiterFeederMac(Ptr<Node> orbiter, uint32_t beamId) const;

    /**
     * Get SatOrbiterUserMac instance of an orbiter serving wanted beam ID
     *
     * \param orbiter Orbiter node to consider
     * \param beamId Beam ID served by this user MAC layer
     *
     * \return SatOrbiterUserMac instance of an orbiter
     */
    Ptr<SatOrbiterUserMac> GetOrbiterUserMac(Ptr<Node> orbiter, uint32_t beamId) const;

    /**
     * Get SatOrbiterFeederPhy instance of an orbiter serving wanted beam ID
     *
     * \param orbiter Orbiter node to consider
     * \param beamId Beam ID served by this feeder physical layer
     *
     * \return SatOrbiterFeederPhy instance of an orbiter
     */
    Ptr<SatOrbiterFeederPhy> GetOrbiterFeederPhy(Ptr<Node> orbiter, uint32_t beamId) const;

    /**
     * Get SatOrbiterUserPhy instance of an orbiter serving wanted beam ID
     *
     * \param orbiter Orbiter node to consider
     * \param beamId Beam ID served by this user physical layer
     *
     * \return SatOrbiterUserPhy instance of an orbiter
     */
    Ptr<SatOrbiterUserPhy> GetOrbiterUserPhy(Ptr<Node> orbiter, uint32_t beamId) const;

    /**
     * \brief Function for printing out the topology
     */
    void PrintTopology() const;

    /**
     * \brief Function for enabling the map prints
     */
    void EnableMapPrint(bool enableMapPrint)
    {
        m_enableMapPrint = enableMapPrint;
    }

  private:
    std::map<uint32_t, Ptr<Node>> m_gwIds; // List of GW nodes
    NodeContainer m_gws;                   // List of GW nodes
    NodeContainer m_uts;                   // List of UT nodes
    NodeContainer m_orbiters;              // List of orbiter nodes

    std::map<Ptr<Node>, GwLayers_s> m_gwLayers; // Map giving protocol layers for all GWs
    std::map<Ptr<Node>, UtLayers_s> m_utLayers; // Map giving protocol layers for all UTs
    std::map<Ptr<Node>, OrbiterLayers_s>
        m_orbiterLayers; // Map giving protocol layers for all orbiters

    std::map<Ptr<Node>, Ptr<Node>> m_utToGwMap; // Map of GW connected for each UT

    bool m_enableMapPrint; // Is map printing enabled or not
};

} // namespace ns3

#endif /* SATELLITE_TOPOLOGY_H */
