/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Magister Solutions Ltd
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
 * Author: Sami Rantanen <sami.rantanen@magister.fi>
 * Author: Mathias Ettinger <mettinger@toulouse.viveris.com>
 */

#ifndef SAT_BEAM_SCHEDULER_H
#define SAT_BEAM_SCHEDULER_H

#include "satellite-cno-estimator.h"
#include "satellite-enums.h"
#include "satellite-frame-allocator.h"
#include "satellite-gw-mac.h"
#include "satellite-net-device.h"
#include "satellite-orbiter-net-device.h"

#include <ns3/callback.h>
#include <ns3/nstime.h>
#include <ns3/object.h>
#include <ns3/ptr.h>
#include <ns3/simple-ref-count.h>
#include <ns3/traced-callback.h>

#include <list>
#include <map>
#include <vector>

namespace ns3
{

class Address;
class Ipv4Address;
class SatControlMessage;
class SatCrMessage;
class SatTbtpMessage;
class SatTimuMessage;
class SatSuperframeSeq;
class SatSuperframeAllocator;
class SatLowerLayerServiceConf;
class SatDamaEntry;

/**
 * \ingroup satellite
 * \brief class for module Beam Scheduler.
 *
 * This SatBeamScheduler class implements scheduler used to for one specific beam.
 * It's is created and used by NCC.
 *
 * The scheduling process is briefly:
 *   - SatDamaEntry/CR update
 *   - Preliminary resource allocation
 *   - Time slot generation
 *   - SatDamaEntry update
 *   - TBTP signaling (generation)
 *    - Schedule next scheduling time for the next SF
 *
 *    One scheduler per spot-beam is created and utilized.
 *
 */
class SatBeamScheduler : public Object
{
  public:
    /**
     * \enum HandoverInformationForward_t
     * \brief Strategies to exchange informations between beams
     */
    typedef enum
    {
        BASIC,
        CHECK_GATEWAY
    } HandoverInformationForward_t;

    /**
     * \brief Get the type ID
     * \return the object TypeId
     */
    static TypeId GetTypeId(void);

    /**
     * Construct a SatBeamScheduler
     *
     * This is the constructor for the SatBeamScheduler
     *
     */
    SatBeamScheduler();

    /**
     * Destroy a SatBeamScheduler
     *
     * This is the destructor for the SatBeamScheduler.
     */
    ~SatBeamScheduler();

    /**
     * \param msg        the message send
     * \param address    Packet destination address
     * \return Result of sending, true success or false failure
     */
    typedef Callback<bool, Ptr<SatControlMessage>, const Address&> SendCtrlMsgCallback;

    /**
     * \param msg        the TBTP sent
     */
    typedef Callback<void, Ptr<SatTbtpMessage>> SendTbtpCallback;

    /**
     * \param id    Id of the TBTP message to add.
     * \param tbtp  Pointer to the TBTP message to add.
     */
    typedef Callback<void, uint32_t, Ptr<SatTbtpMessage>> TbtpAddCallback;

    /**
     * \param beamId ID of the beam which for callback is set
     * \param satId ID of the satellite using the beam which for callback is set
     * \param gwNetDevice GW NetDevice linked to this beam
     * \param orbiterNetDevice OrbiterNetDevice on satellite linked to this beam
     * \param cb callback to invoke whenever a TBTP is ready for sending and must
     *        be forwarded to the Beam UTs.
     * \param seq Superframe sequence.
     * \param maxFrameSizeInBytes Maximum non fragmented BB frame size with most robust ModCod
     * \param satAddress Mac address of the satellite responsible for this beam
     * \param gwAddress Mac address of the gateway responsible for this beam
     */
    void Initialize(uint32_t satId,
                    uint32_t beamId,
                    Ptr<SatNetDevice> gwNetDevice,
                    Ptr<SatOrbiterNetDevice> orbiterNetDevice,
                    SatBeamScheduler::SendCtrlMsgCallback cb,
                    Ptr<SatSuperframeSeq> seq,
                    uint32_t maxFrameSizeInBytes,
                    Address satAddress,
                    Address gwAddress);

    /**
     * Add UT to scheduler.
     *
     * \param utId ID (mac address) of the UT to be added
     * \param llsConf Lower layer service configuration for the UT.
     * \return Index of the RA channel allocated to added UT
     */
    uint32_t AddUt(Address utId, Ptr<SatLowerLayerServiceConf> llsConf);

    /**
     * Check whether an UT is handled by this scheduler
     *
     * \param utId ID (mac address) of the UT to be searched for
     * \return Whether or not this UT is in this beam
     */
    bool HasUt(Address utId);

    /**
     * Check whether an UT is handled by this scheduler
     *
     * \return Whether or not at least one UT is handled by this beam
     */
    bool HasUt();

    /**
     * Update UT C/N0 info with the latest value.
     *
     * \param utId Id of the UT (address).
     * \param cno C/N0 value
     */
    void UpdateUtCno(Address utId, double cno);

    /**
     * Update satellite C/N0 info with the latest value.
     *
     * \param satelliteMac MAC of the SAT (address).
     * \param cno C/N0 value
     */
    void UpdateSatelliteCno(Address satelliteMac, double cno);

    /**
     * Receive capacity requests from UTs.
     *
     * \param utId Id of the UT (address).
     * \param crMsg Pointer to the CR message
     */
    void UtCrReceived(Address utId, Ptr<SatCrMessage> crMsg);

    /**
     * Send control messages to the beam.
     *
     * \param message Pointer or control message to send.
     * \return true if sending is success, false otherwise.
     */
    bool Send(Ptr<SatControlMessage> message);

    /**
     * Send control message to an UT into the beam.
     *
     * \param message Pointer of control message to send.
     * \param utId Address of the UT to send the message to.
     * \return true if sending is success, false otherwise.
     */
    bool SendTo(Ptr<SatControlMessage> message, Address utId);

    /**
     * Send control message to the satellite.
     *
     * \param message Pointer of control message to send.
     * \param satelliteMac Address of the feeder SAT to send the message to.
     * \return true if sending is success, false otherwise.
     */
    bool SendToSatellite(Ptr<SatControlMessage> msg, Address satelliteMac);

    /**
     * Set the callback to inform NCC a TBTP has been sent.
     */
    void SetSendTbtpCallback(SendTbtpCallback cb);

    /**
     * Callback signature for `BacklogRequestsTrace` trace source.
     *
     * \param trace A string containing the following information:
     *                 - the current simulation time (in seconds),
     *                 - beam ID,
     *                 - UT ID,
     *                 - type (RBDC or VBDC), and
     *                 - request size (in kbps for RBDC or in bytes for VBDC).
     */
    typedef void (*BacklogRequestsTraceCallback)(std::string trace);

    /**
     * Callback signature for `WaveformTrace` trace source.
     *
     * \param waveformId The first waveform scheduled for a UT.
     */
    typedef void (*WaveformTraceCallback)(uint32_t waveformId);

    /**
     * Callback signature for the `UsableCapacityTrace` trace source.
     *
     * \param usableCapacity The amount of capacity allocated, in kbps.
     */
    typedef void (*UsableCapacityTraceCallback)(uint32_t usableCapacity);

    /**
     * Callback signature for the `UnmetCapacityTrace` trace source.
     *
     * \param unmetCapacity The amount of capacity requested but not used,
     *                      in kbps.
     */
    typedef void (*UnmetCapacityTraceCallback)(uint32_t unmetCapacity);

    /**
     * Callback signature for the `ExceedingCapacityTrace` trace source.
     *
     * \param exceedingCapacity The amount capacity offered exceeds capacity
     *                          requested, in kbps.
     */
    typedef void (*ExceedingCapacityTraceCallback)(uint32_t exceedingCapacity);

    /**
     * \brief Create a TIM unicast message containing enough data for a
     * terminal to connect to the beam handled by this SatBeamScheduler
     */
    Ptr<SatTimuMessage> CreateTimu() const;

    /**
     * \brief Transfer ownership of a terminal to the given SatBeamScheduler
     * \param utId the terminal that is leaving this beam
     * \param destination the beam that should accept the terminal
     */
    void TransferUtToBeam(Address utId, Ptr<SatBeamScheduler> destination);

    /**
     * Connect a new UT address to this scheduler
     *
     * \param address The UT address to connect
     */
    void ConnectUt(Mac48Address address);

    /**
     * Disconnect a new UT address from this scheduler
     *
     * \param address The UT address to disconnect
     */
    void DisconnectUt(Mac48Address address);

    /**
     * Connect a new GW address to this scheduler
     *
     * \param address The GW address to connect
     */
    void ConnectGw(Mac48Address address);

    /**
     * Disconnect a new GW address from this scheduler
     *
     * \param address The GW address to disconnect
     */
    void DisconnectGw(Mac48Address address);

    /**
     * \brief Remove a UT from its SatBeamScheduler
     * \param utId the terminal that is leaving this beam
     */
    void RemoveUt(Address utId);

    void ReserveLogonChannel(uint32_t logonChannelId);

    /**
     * \brief Return the address of the satellite responsible of this beam
     */
    inline Address GetSatAddress(void) const
    {
        return m_satAddress;
    }

    /**
     * \brief Return the address of the gateway responsible of this beam
     */
    inline Address GetGwAddress(void) const
    {
        return m_gwAddress;
    }

  private:
    /**
     * \brief UT information helper class for SatBeamScheduler.
     *
     * Stores capacity request and C/N0 estimation information of the UT.
     */
    class SatUtInfo : public SimpleRefCount<SatUtInfo>
    {
      public:
        /**
         * Construct SatUtInfo.
         *
         * \param damaEntry DamaEntry for created UT info.
         * \param cnoEstimator C/N0 estimator for the UT info.
         * \param controlSlotOffset Offset of the current moment to generate control slot.
         * \param controlSlotsEnabled Flag to tell if control slots generation is enabled according
         * to controlSlotOffset
         */
        SatUtInfo(Ptr<SatDamaEntry> damaEntry,
                  Ptr<SatCnoEstimator> cnoEstimator,
                  Time controlSlotOffset,
                  bool controlSlotsEnabled);

        /**
         * Get damaEntry of the UT info.
         *
         * \return Pointer to DamEntry of the UT info.
         */
        Ptr<SatDamaEntry> GetDamaEntry();

        /**
         * Update DamaEntry with information of the received CR messages.
         */
        void UpdateDamaEntryFromCrs();

        /**
         * Add C/N0 sample to UT info's estimator.
         *
         * \param sample C/N0 sample value to add.
         */
        void AddCnoSample(double sample);

        /**
         * Get estimated C/N0 value based on added samples.
         *
         * \return C/N0 estimation.
         */
        double GetCnoEstimation();

        /**
         * Add CR message to UT info to be used when capacity request is calculated
         * next time (method UpdateDamaEntryFromCrs is called).
         *
         * \param crMsg
         */
        void AddCrMsg(Ptr<SatCrMessage> crMsg);

        /**
         * Remove all CR messages from UT info to reset capacity requests in
         * case of handover.
         */
        void ClearCrMsgs();

        /**
         * Check if time is expired to send control slot.
         *
         * \return Status of control slot generation time.
         */
        bool IsControlSlotGenerationTime() const;

        /**
         * Set time for next time slot generation for this UT.
         *
         * \param offset Offset of the current moment to generate control slot.
         */
        void SetControlSlotGenerationTime(Time offset);

      private:
        /**
         * Container to store received CR messages.
         */
        typedef std::vector<Ptr<SatCrMessage>> CrMsgContainer_t;

        /**
         * DamaEntry of this UT info.
         */
        Ptr<SatDamaEntry> m_damaEntry;

        /**
         *  Estimator for the C/N0.
         */
        Ptr<SatCnoEstimator> m_cnoEstimator;

        /**
         *  Received CRs since last update round (call of the method UpdateDamaEntryFromCrs).
         */
        CrMsgContainer_t m_crContainer;

        /**
         * Time to send next control time slot.
         */
        Time m_controlSlotGenerationTime;

        /**
         * Flag to indicated if control time slots generation is enabled.
         */
        bool m_controlSlotsEnabled;
    };

    /**
     * Pair to store capacity request information for the UT
     */
    typedef std::pair<Address, SatFrameAllocator::SatFrameAllocReq> UtReqInfoItem_t;

    /**
     * Map container to store UT information.
     */
    typedef std::map<Address, Ptr<SatUtInfo>> UtInfoMap_t;

    /**
     * Container to store capacity request information for the UTs.
     */
    typedef std::list<UtReqInfoItem_t> UtReqInfoContainer_t;

    /**
     * \brief CnoCompare class to sort UT request according to C/N0 information
     */
    class CnoCompare
    {
      public:
        /**
         * Construct CnoCompare object
         *
         * \param utInfoMap Reference to map container for the UT information
         */
        CnoCompare(const UtInfoMap_t& utInfoMap)
            : m_utInfoMap(utInfoMap)
        {
        }

        /**
         * Compare operator to compare request information of the two UTs.
         *
         * \param utReqInfo1 Request information for UT 1
         * \param utReqInfo2 Request information for UT 2
         * \return true if first UT's C/N0 is more robust than second UT's
         */
        bool operator()(UtReqInfoItem_t utReqInfo1, UtReqInfoItem_t utReqInfo2)
        {
            double result = false;

            double cnoFirst = m_utInfoMap.at(utReqInfo1.first)->GetCnoEstimation();
            double cnoSecond = m_utInfoMap.at(utReqInfo2.first)->GetCnoEstimation();

            if (!std::isnan(cnoFirst))
            {
                if (std::isnan(cnoSecond))
                {
                    result = false;
                }
                else
                {
                    result = (cnoFirst < cnoSecond);
                }
            }

            return result;
        }

      private:
        /**
         * Reference to map container for the UT information
         */
        const UtInfoMap_t& m_utInfoMap;
    };

    /**
     * ID of the satellite using this beam
     */
    uint32_t m_satId;

    /**
     * ID of the beam
     */
    uint32_t m_beamId;

    /**
     * GW MAC linked to this beam
     */
    Ptr<SatGwMac> m_gwMac;

    /**
     * OrbiterNetDevice on satellite linked to this beam
     */
    Ptr<SatOrbiterNetDevice> m_orbiterNetDevice;

    /**
     * Pointer to super frame sequence.
     */
    Ptr<SatSuperframeSeq> m_superframeSeq;

    /**
     * Counter for super frame sequence.
     */
    uint32_t m_superFrameCounter;

    /**
     * The control message send callback.
     */
    SatBeamScheduler::SendCtrlMsgCallback m_txCallback;

    /**
     * The TBTP send callback to inform GW Mac.
     */
    SatBeamScheduler::SendTbtpCallback m_txTbtpCallback;

    /**
     * Map to store UT information in beam for updating purposes.
     */
    UtInfoMap_t m_utInfos;

    /**
     * Container including every UT's allocation requests.
     */
    UtReqInfoContainer_t m_utRequestInfos;

    /**
     * Random variable stream to select RA channel for a UT.
     */
    Ptr<RandomVariableStream> m_raChRandomIndex;

    /**
     * Logon channel ID to exclude it from the RA channel selection
     */
    uint32_t m_logonChannelIndex;

    /**
     * Mode used for C/N0 estimator.
     */
    SatCnoEstimator::EstimationMode_t m_cnoEstimatorMode;

    /**
     * Time window for C/N0 estimation.
     */
    Time m_cnoEstimationWindow;

    /**
     * Superframe allocator to maintain load information of the frames and their configurations.
     */
    Ptr<SatSuperframeAllocator> m_superframeAllocator;

    /**
     * Maximum two-way propagation delay estimate between GW-SAT-UT-SAT-GW.
     * This is used to estimate how much time into the future the scheduler
     * has to schedule the super frames.
     */
    Time m_maxTwoWayPropagationDelay;

    /**
     * Maximum TBTP tx and processing delay estimate at the GW (scheduler).
     * This is used to estimate how much time into the future the scheduler
     * has to schedule the super frames.
     */
    Time m_maxTbtpTxAndProcessingDelay;

    /**
     * Maximum size of the BB frame.
     */
    uint32_t m_maxBbFrameSize;

    /**
     * Interval to generate control time slots.
     */
    Time m_controlSlotInterval;

    /**
     * Flag to indicated if control time slots generation is enabled.
     */
    bool m_controlSlotsEnabled;

    /**
     * Trace for backlog requests done to beam scheduler.
     */
    TracedCallback<std::string> m_backlogRequestsTrace;

    /**
     * Trace first wave form scheduled for the UT.
     */
    TracedCallback<uint32_t> m_waveformTrace;

    /**
     * Trace count of UTs scheduled per Frame.
     */
    TracedCallback<uint32_t, uint32_t> m_frameUtLoadTrace;

    /**
     * Trace frame load ratio.
     */
    TracedCallback<uint32_t, double> m_frameLoadTrace;

    /**
     * Trace usable capacity.
     */
    TracedCallback<uint32_t> m_usableCapacityTrace;

    /**
     * Trace unmet capacity.
     */
    TracedCallback<uint32_t> m_unmetCapacityTrace;

    /**
     * Trace exceeding capacity.
     */
    TracedCallback<uint32_t> m_exceedingCapacityTrace;

    /**
     * Dispose actions for SatBeamScheduler.
     */
    void DoDispose(void);

    /**
     * Schedule UTs added (registered) to scheduler.
     */
    void Schedule();

    /**
     * Update dama entries with received requests at beginning of the scheduling.
     */
    uint32_t UpdateDamaEntriesWithReqs();

    /**
     * Update dama entries with given allocations at end of the scheduling.
     *
     * \param utAllocContainer Reference to container including granted allocations per UT.
     */
    uint32_t UpdateDamaEntriesWithAllocs(
        SatFrameAllocator::UtAllocInfoContainer_t& utAllocContainer);

    /**
     * Do pre-allocation of the symbols per UT/RC, before time slot generation.
     */
    void DoPreResourceAllocation();

    /**
     * Add RA channel information to TBTP(s).
     * \param tbtpContainer Reference to container including TBTPs already and to be added.
     */
    void AddRaChannels(std::vector<Ptr<SatTbtpMessage>>& tbtpContainer);

    void AddUtInfo(Address utId, Ptr<SatUtInfo> utInfo);
    void RemoveUtInfo(UtInfoMap_t::iterator iterator);

    /**
     * Create estimator for the UT according to set attributes.
     * \return pointer to created estimator
     */
    Ptr<SatCnoEstimator> CreateCnoEstimator();

    /**
     * Send an estimation of cno to satellite, if samples have been received
     */
    void SendCnoToSatellite();

    Address m_satAddress;

    Address m_gwAddress;

    HandoverInformationForward_t m_handoverStrategy;

    /**
     * Type of SatSuperframeAllocator class to use
     */
    SatEnums::SuperframeAllocatorType_t m_superframeAllocatorType;

    /**
     * Estimator for the C/N0 from satellite.
     */
    Ptr<SatCnoEstimator> m_satelliteCnoEstimator;

    /**
     * MAC address of the satellite (used when regenerative)
     */
    Address m_satelliteMac;

    /**
     * Indicates if Cno sample have been received since last C/N0 control message sent
     */
    bool m_receivedSatelliteCnoSample;
};

} // namespace ns3

#endif /* SAT_BEAM_SCHEDULER_H */
