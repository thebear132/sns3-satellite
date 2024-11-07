/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Magister Solutions Ltd
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
 */

#ifndef SATELLITE_GW_LLC_H_
#define SATELLITE_GW_LLC_H_

#include "satellite-llc.h"

#include <ns3/ptr.h>

#include <vector>

namespace ns3
{

/**
 * \ingroup satellite
 * \brief SatGwLlc holds the GW implementation of LLC layer. SatGwLlc is inherited from
 * SatLlc base class and implements the needed changes from the base class related to
 * GW LLC packet transmissions and receptions.
 */
class SatGwLlc : public SatLlc
{
  public:
    /**
     * \brief Get the type ID
     * \return the object TypeId
     */
    static TypeId GetTypeId(void);

    /**
     * Construct a SatGwLlc, should not be used
     */
    SatGwLlc();

    /**
     * Construct a SatGwLlc
     * \param forwardLinkRegenerationMode Forward link regeneration model
     * \param returnLinkRegenerationMode Return link regeneration model
     */
    SatGwLlc(SatEnums::RegenerationMode_t forwardLinkRegenerationMode,
             SatEnums::RegenerationMode_t returnLinkRegenerationMode);

    /**
     * Destroy a SatGwLlc
     *
     * This is the destructor for the SatGwLlc.
     */
    virtual ~SatGwLlc();

    /**
     * \brief Called from higher layer (SatNetDevice) to enque packet to LLC
     *
     * \param packet packet sent from above down to SatMac
     * \param dest Destination MAC address of the packet
     * \param flowId Flow identifier
     * \return Boolean indicating whether the enque operation succeeded
     */
    virtual bool Enque(Ptr<Packet> packet, Address dest, uint8_t flowId);

    /**
     *  \brief Called from lower layer (MAC) to inform a tx
     *  opportunity of certain amount of bytes
     *
     * \param utAddr MAC address of the UT with tx opportunity
     * \param bytes Size of the Tx opportunity
     * \param flowId Flow identifier
     * \param &bytesLeft Bytes left after TxOpportunity
     * \param &nextMinTxO Minimum TxO after this TxO
     * \return Pointer to packet to be transmitted
     */
    virtual Ptr<Packet> NotifyTxOpportunity(uint32_t bytes,
                                            Mac48Address utAddr,
                                            uint8_t flowId,
                                            uint32_t& bytesLeft,
                                            uint32_t& nextMinTxO);

    /**
     * \brief Create and fill the scheduling objects based on LLC layer information.
     * Scheduling objects may be used at the MAC layer to assist in scheduling.
     * \param output reference to an output vector that will be filled with
     *               pointer to scheduling objects
     */
    virtual void GetSchedulingContexts(std::vector<Ptr<SatSchedulingObject>>& output) const;

    /**
     * \brief Get the number of (new) bytes at LLC queue for a certain UT. Method
     * checks only the SatQueue for packets, thus it does not count possible
     * packets buffered at the encapsulator (e.g. in case of ARQ).
     * \param utAddress the MAC address that identifies a particular UT node.
     * \return Number of bytes currently queued in the encapsulator(s)
     *         associated with the UT.
     */
    virtual uint32_t GetNBytesInQueue(Mac48Address utAddress) const;

    /**
     * \brief Get the number of (new) packets at LLC queues for a certain UT. Method
     * checks only the SatQueue for packets, thus it does not count possible
     * packets buffered at the encapsulator (e.g. in case of ARQ).
     * \param utAddress the MAC address that identifies a particular UT node.
     * \return Number of packets currently queued in the encapsulator(s)
     *         associated with the UT.
     */
    virtual uint32_t GetNPacketsInQueue(Mac48Address utAddress) const;

    /**
     * Remove all packets from the queues. Called when performing handovers.
     */
    virtual void ClearQueues();

  protected:
    /**
     * Dispose of this class instance
     */
    void DoDispose();

    /**
     * \brief Virtual method to create a new encapsulator 'on-a-need-basis' dynamically.
     * \param key Encapsulator key class
     */
    virtual void CreateEncap(Ptr<EncapKey> key);

    /**
     * \brief Virtual method to create a new decapsulator 'on-a-need-basis' dynamically.
     * \param key Encapsulator key class
     */
    virtual void CreateDecap(Ptr<EncapKey> key);

    /**
     * \brief Get the link TX direction. Must be implemented by child clases.
     * \return The link TX direction
     */
    virtual SatEnums::SatLinkDir_t GetSatLinkTxDir();

    /**
     * \brief Get the link RX direction. Must be implemented by child clases.
     * \return The link RX direction
     */
    virtual SatEnums::SatLinkDir_t GetSatLinkRxDir();
};

} // namespace ns3

#endif /* SATELLITE_GW_LLC_H_ */
