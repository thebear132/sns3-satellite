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
     * \param gw The GW node to add
     */
    void AddGwNode(Ptr<Node> gw);

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
     * Get the wanted GW node
     *
     * \param nodeId ID of the node needed
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
    NodeContainer m_gws;      // List of GW nodes
    NodeContainer m_uts;      // List of UT nodes
    NodeContainer m_orbiters; // List of orbiter nodes

    std::map<Ptr<Node>, Ptr<Node>> m_utToGwMap; // Map of GW connected for each UT

    bool m_enableMapPrint; // Is map printing enabled or not
};

} // namespace ns3

#endif /* SATELLITE_TOPOLOGY_H */
