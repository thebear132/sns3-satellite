/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Padova
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
 * Author: Davide Magrin <magrinda@dei.unipd.it>
 *
 * Modified by: Bastien Tauran <bastien.tauran@viveris.fr>
 */

#ifndef LORAWAN_MAC_H
#define LORAWAN_MAC_H

#include "lora-logical-channel-helper.h"
#include "satellite-lora-phy-rx.h"
#include "satellite-mac.h"
#include "satellite-phy.h"

#include <ns3/object.h>
#include <ns3/packet.h>

#include <array>
#include <vector>

namespace ns3
{

/**
 * Class representing the LoRaWAN MAC layer.
 *
 * This class is meant to be extended differently based on whether the layer
 * belongs to an End Device or a Gateway, while holding some functionality that
 * is common to both.
 */
class LorawanMac : public SatMac
{
  public:
    static TypeId GetTypeId(void);

    LorawanMac();
    LorawanMac(uint32_t satId, uint32_t beamId);
    virtual ~LorawanMac();

    typedef std::array<std::array<uint8_t, 6>, 8> ReplyDataRateMatrix;

    /**
     * Get the underlying PHY layer
     *
     * \return The PHY layer that this MAC is connected to.
     */
    Ptr<SatPhy> GetPhy(void);

    /**
     * Set the underlying PHY layer
     *
     * \param phy the phy layer
     */
    void SetPhy(Ptr<SatPhy> phy);

    /**
     * Get the underlying PHY TX layer
     *
     * \return The PHY TX layer that this MAC is connected to.
     */
    Ptr<SatLoraPhyTx> GetPhyTx(void);

    /**
     * Set the underlying PHY TX layer
     *
     * \param phy the phy tx layer
     */
    void SetPhyTx(Ptr<SatLoraPhyTx> phyTx);

    /**
     * Send a packet.
     *
     * \param packet The packet to send.
     */
    virtual void Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber);

    /**
     * Send a packet.
     *
     * \param packet The packet to send.
     */
    virtual void Send(Ptr<Packet> packet) = 0;

    /**
     * Receive a packet from the lower layer.
     *
     * \param packets the received packets
     */
    virtual void Receive(SatPhy::PacketContainer_t packets,
                         Ptr<SatSignalParameters> /*rxParams*/) = 0;

    /**
     * Function called by lower layers to inform this layer that reception of a
     * packet we were locked on failed.
     *
     * \param packet the packet we failed to receive
     */
    virtual void FailedReception(Ptr<const Packet> packet) = 0;

    /**
     * Perform actions after sending a packet.
     */
    virtual void TxFinished() = 0;

    /**
     * Set the device this MAC layer is installed on.
     *
     * \param device The NetDevice this MAC layer will refer to.
     */
    void SetDevice(Ptr<NetDevice> device);

    /**
     * Get the device this MAC layer is installed on.
     *
     * \return The NetDevice this MAC layer will refer to.
     */
    Ptr<NetDevice> GetDevice(void);

    /**
     * Get the logical lora channel helper associated with this MAC.
     *
     * \return The instance of LoraLogicalChannelHelper that this MAC is using.
     */
    LoraLogicalChannelHelper GetLoraLogicalChannelHelper(void);

    /**
     * Set the LoraLogicalChannelHelper this MAC instance will use.
     *
     * \param helper The instance of the helper to use.
     */
    void SetLoraLogicalChannelHelper(LoraLogicalChannelHelper helper);

    /**
     * Get the SF corresponding to a data rate, based on this MAC's region.
     *
     * \param dataRate The Data Rate we need to convert to a Spreading Factor
     * value.
     * \return The SF that corresponds to a Data Rate in this MAC's region, or 0
     * if the dataRate is not valid.
     */
    uint8_t GetSfFromDataRate(uint8_t dataRate);

    /**
     * Get the BW corresponding to a data rate, based on this MAC's region
     *
     * \param dataRate The Data Rate we need to convert to a bandwidth value.
     * \return The bandwidth that corresponds to the parameter Data Rate in this
     * MAC's region, or 0 if the dataRate is not valid.
     */
    double GetBandwidthFromDataRate(uint8_t dataRate);

    /**
     * Get the transmission power in dBm that corresponds, in this region, to the
     * encoded 8-bit txPower.
     *
     * \param txPower The 8-bit encoded txPower to convert.
     *
     * \return The corresponding transmission power in dBm, or 0 if the encoded
     * power was not recognized as valid.
     */
    double GetDbmForTxPower(uint8_t txPower);

    /**
     * Compute the time that a packet with certain characteristics will take to be
     * transmitted.
     *
     * Besides from the ones saved in LoraTxParameters, the packet's payload
     * (obtained through a GetSize () call to accout for the presence of Headers
     * and Trailers, too) also influences the packet transmit time.
     *
     * \param packet The packet that needs to be transmitted.
     * \param txParams The set of parameters that will be used for transmission.
     * \return The time necessary to transmit the packet.
     */
    Time GetOnAirTime(Ptr<Packet> packet, LoraTxParameters txParams);

    /**
     * Set the vector to use to check up correspondence between SF and DataRate.
     *
     * \param sfForDataRate A vector that contains at position i the SF that
     * should correspond to DR i.
     */
    void SetSfForDataRate(std::vector<uint8_t> sfForDataRate);

    /**
     * Set the vector to use to check up correspondence between bandwidth and
     * DataRate.
     *
     * \param bandwidthForDataRate A vector that contains at position i the
     * bandwidth that should correspond to DR i in this MAC's region.
     */
    void SetBandwidthForDataRate(std::vector<double> bandwidthForDataRate);

    /**
     * Set the maximum App layer payload for a set DataRate.
     *
     * \param maxAppPayloadForDataRate A vector that contains at position i the
     * maximum Application layer payload that should correspond to DR i in this
     * MAC's region.
     */
    void SetMaxAppPayloadForDataRate(std::vector<uint32_t> maxAppPayloadForDataRate);

    /**
     * Set the vector to use to check up which transmission power in Dbm
     * corresponds to a certain TxPower value in this MAC's region.
     *
     * \param txDbmForTxPower A vector that contains at position i the
     * transmission power in dBm that should correspond to a TXPOWER value of i in
     * this MAC's region.
     */
    void SetTxDbmForTxPower(std::vector<double> txDbmForTxPower);

    /**
     * Set the matrix to use when deciding with which DataRate to respond. Region
     * based.
     *
     * \param replyDataRateMatrix A matrix containing the reply DataRates, based
     * on the sending DataRate and on the value of the RX1DROffset parameter.
     */
    void SetReplyDataRateMatrix(ReplyDataRateMatrix replyDataRateMatrix);

    /**
     * Set the number of PHY preamble symbols this MAC is set to use.
     *
     * \param nPreambleSymbols The number of preamble symbols to use (typically 8).
     */
    void SetNPreambleSymbols(int nPreambleSymbols);

    /**
     * Get the number of PHY preamble symbols this MAC is set to use.
     *
     * \return The number of preamble symbols to use (typically 8).
     */
    int GetNPreambleSymbols(void);

    /**
     * \brief Indicates if the satellite is regenerative on the link this layer is sending packets.
     */
    void setRegenerative(bool isRegenerative);

  protected:
    /**
     * The trace source that is fired when a packet cannot be sent because of duty
     * cycle limitations.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_cannotSendBecauseDutyCycle;

    /**
     * Trace source that is fired when a packet reaches the MAC layer.
     */
    TracedCallback<Ptr<const Packet>> m_receivedPacket;

    /**
     * Trace source that is fired when a new APP layer packet arrives at the MAC
     * layer.
     */
    TracedCallback<Ptr<const Packet>> m_sentNewPacket;

    /**
     * The PHY instance that sits under this MAC layer.
     */
    Ptr<SatPhy> m_phy;

    /**
     * The device this MAC layer is installed on.
     */
    Ptr<NetDevice> m_device;

    /**
     * The LoraLogicalChannelHelper instance that is assigned to this MAC.
     */
    LoraLogicalChannelHelper m_channelHelper;

    /**
     * A vector holding the SF each Data Rate corresponds to.
     */
    std::vector<uint8_t> m_sfForDataRate;

    /**
     * A vector holding the bandwidth each Data Rate corresponds to.
     */
    std::vector<double> m_bandwidthForDataRate;

    /**
     * A vector holding the maximum app payload size that corresponds to a
     * certain DataRate.
     */
    std::vector<uint32_t> m_maxAppPayloadForDataRate;

    /**
     * The number of symbols to use in the PHY preamble.
     */
    int m_nPreambleSymbols;

    /**
     * A vector holding the power that corresponds to a certain TxPower value.
     */
    std::vector<double> m_txDbmForTxPower;

    /**
     * The matrix that decides the DR the GW will use in a reply based on the ED's
     * sending DR and on the value of the RX1DROffset parameter.
     */
    ReplyDataRateMatrix m_replyDataRateMatrix;

    /**
     * ID of beam for UT
     */
    uint32_t m_beamId;

    /**
     * Indicates if satellite is regenerative on the link where this layer is sending packets.
     */
    bool m_isRegenerative;
};

} /* namespace ns3 */

#endif /* LORAWAN_MAC_H */
