/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Magister Solutions Ltd
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
 * Inspired and adapted from Hypatia: https://github.com/snkas/hypatia
 *
 * Author: Bastien Tauran <bastien.tauran@viveris.fr>
 */

#ifndef SATELLITE_ISL_ARBITER_UNICAST_H
#define SATELLITE_ISL_ARBITER_UNICAST_H

#include "satellite-isl-arbiter.h"

#include <map>
#include <string>

namespace ns3
{

class SatIslArbiterUnicast : public SatIslArbiter
{
  public:
    static TypeId GetTypeId(void);

    /**
     * Default constructor.
     */
    SatIslArbiterUnicast();

    /**
     * Constructor, without initializing the map of next hops
     * \param node The satellite node this unicast arbiter is attached
     */
    SatIslArbiterUnicast(Ptr<Node> node);

    /**
     * Constructor.
     * \param node The satellite node this unicast arbiter is attached
     * \param nextHop The next hop (interface ID) for each destination possible (satellite ID)
     */
    SatIslArbiterUnicast(Ptr<Node> node, std::map<uint32_t, uint32_t> nextHopMap);

    /**
     * Decide how to forward. Implemented in subclasses
     *
     * \param sourceSatId                       Satellite ID where the packet originated from
     * \param targetSatId                       Satellite ID where the packet has to go to
     * \param pkt                               Packet
     *
     * \return ISL interface index, or -1 if routing failed
     */
    int32_t Decide(int32_t sourceSatId, int32_t targetSatId, Ptr<Packet> pkt);

    /**
     * Unicast routing table
     *
     * \return string representation of the table
     */
    std::string StringReprOfForwardingState();

    /**
     * Add an entry on arbiter
     * \param destinationId Node ID of destination satellite
     * \param netDeviceIndex ISL Net Device index
     */
    void AddNextHopEntry(uint32_t destinationId, uint32_t netDeviceIndex);

  private:
    std::map<uint32_t, uint32_t>
        m_nextHopMap; // Map indicating next hops. Key = satellite destination ID, value =
                      // IslNetDevice index to send packet
};

} // namespace ns3

#endif // SATELLITE_ISL_ARBITER_UNICAST_H
