/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Magister Solutions Ltd
 * Copyright (c) 2020 CNES
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

#ifndef __SATELLITE_TRAFFIC_HELPER_H__
#define __SATELLITE_TRAFFIC_HELPER_H__

#include "satellite-helper.h"

#include <ns3/application-container.h>
#include <ns3/cbr-application.h>
#include <ns3/config.h>
#include <ns3/object.h>
#include <ns3/satellite-stats-helper-container.h>
#include <ns3/string.h>

namespace ns3
{
/**
 * \brief Creates pre-defined trafics.
 */
class SatTrafficHelper : public Object
{
  public:
    /**
     * \brief List of available traffics
     */
    typedef enum
    {
        LORA_PERIODIC, // implemented
        LORA_CBR,      // implemented
        CBR,           // implemented
        ONOFF,         // implemented
        HTTP,          // implemented
        NRTV,          // implemented
        POISSON,       // implemented
        VOIP,          // implemented
        CUSTOM         // implemented
    } TrafficType_t;

    typedef enum
    {
        RTN_LINK,
        FWD_LINK
    } TrafficDirection_t;

    typedef enum
    {
        UDP,
        TCP
    } TransportLayerProtocol_t;

    typedef enum
    {
        G_711_1,
        G_711_2,
        G_723_1,
        G_729_2,
        G_729_3
    } VoipCodec_t;

    /**
     * \brief Get the type ID
     * \return the object TypeId
     */
    static TypeId GetTypeId(void);

    /**
     * \brief Get the type ID of object instance
     * \return the TypeId of object instance
     */
    TypeId GetInstanceTypeId(void) const;

    /**
     * \brief Default constructor. Not used.
     */
    SatTrafficHelper();

    /**
     * \brief Create a base SatTrafficHelper for creating customized traffics.
     */
    SatTrafficHelper(Ptr<SatHelper> satHelper,
                     Ptr<SatStatsHelperContainer> satStatsHelperContainer);

    /**
     * Destructor for SatTrafficHelper
     */
    virtual ~SatTrafficHelper()
    {
    }

    /**
     * Add Lora periodic traffic between chosen GWs and UTs
     * \param interval Wait time between transmission of two packets
     * \param packetSize Packet size in bytes
     * \param uts The User Terminals
     * \param startTime Application Start time
     * \param stopTime Application stop time
     * \param startDelay application start delay between each user
     */
    void AddLoraPeriodicTraffic(Time interval,
                                uint32_t packetSize,
                                NodeContainer uts,
                                Time startTime,
                                Time stopTime,
                                Time startDelay);

    /**
     * Add Lora periodic traffic between chosen GWs and UTs
     * \param interval Wait time between transmission of two packets
     * \param packetSize Packet size in bytes
     * \param uts The User Terminals
     * \param startTime Application Start time
     * \param stopTime Application stop time
     * \param startDelay application start delay between each user
     * \param percentage Percentage of UT users having the traffic installed
     */
    void AddLoraPeriodicTraffic(Time interval,
                                uint32_t packetSize,
                                NodeContainer uts,
                                Time startTime,
                                Time stopTime,
                                Time startDelay,
                                double percentage);

    /**
     * Add Lora CBR traffic between chosen GWs and UTs
     * \param interval Wait time between transmission of two packets
     * \param packetSize Packet size in bytes
     * \param gwUsers The Gateway Users
     * \param utUsers The UT Users
     * \param startTime Application Start time
     * \param stopTime Application stop time
     * \param startDelay application start delay between each user
     */
    void AddLoraCbrTraffic(Time interval,
                           uint32_t packetSize,
                           NodeContainer gwUsers,
                           NodeContainer utUsers,
                           Time startTime,
                           Time stopTime,
                           Time startDelay);

    /**
     * Add Lora CBR traffic between chosen GWs and UTs
     * \param interval Wait time between transmission of two packets
     * \param packetSize Packet size in bytes
     * \param gwUsers The Gateway Users
     * \param utUsers The UT Users
     * \param startTime Application Start time
     * \param stopTime Application stop time
     * \param startDelay application start delay between each user
     * \param percentage Percentage of UT users having the traffic installed
     */
    void AddLoraCbrTraffic(Time interval,
                           uint32_t packetSize,
                           NodeContainer gwUsers,
                           NodeContainer utUsers,
                           Time startTime,
                           Time stopTime,
                           Time startDelay,
                           double percentage);

    /**
     * Add a new CBR traffic between chosen GWs and UTs
     * \param direction Direction of traffic
     * \param protocol Transport layer protocol
     * \param interval Wait time between transmission of two packets
     * \param packetSize Packet size in bytes
     * \param gwUsers The Gateway Users
     * \param utUsers The UT Users
     * \param startTime Application Start time
     * \param stopTime Application stop time
     * \param startDelay application start delay between each user
     */
    void AddCbrTraffic(TrafficDirection_t direction,
                       TransportLayerProtocol_t protocol,
                       Time interval,
                       uint32_t packetSize,
                       NodeContainer gwUsers,
                       NodeContainer utUsers,
                       Time startTime,
                       Time stopTime,
                       Time startDelay);

    /**
     * Add a new CBR traffic between chosen GWs and UTs
     * \param direction Direction of traffic
     * \param protocol Transport layer protocol
     * \param interval Wait time between transmission of two packets
     * \param packetSize Packet size in bytes
     * \param gwUsers The Gateway Users
     * \param utUsers The UT Users
     * \param startTime Application Start time
     * \param stopTime Application stop time
     * \param startDelay application start delay between each user
     * \param percentage Percentage of UT users having the traffic installed
     */
    void AddCbrTraffic(TrafficDirection_t direction,
                       TransportLayerProtocol_t protocol,
                       Time interval,
                       uint32_t packetSize,
                       NodeContainer gwUsers,
                       NodeContainer utUsers,
                       Time startTime,
                       Time stopTime,
                       Time startDelay,
                       double percentage);

    /**
     * Add a new ONOFF traffic between chosen GWs and UTs
     * \param direction Direction of traffic
     * \param protocol Transport layer protocol
     * \param dataRate Data rate in ON state
     * \param packetSize Packet size in bytes
     * \param gwUsers The Gateway Users
     * \param utUsers The UT Users
     * \param onTimePattern Pattern for ON state duration
     * \param offTimePattern Pattern for OFF state duration
     * \param startTime Application Start time
     * \param stopTime Application stop time
     * \param startDelay application start delay between each user
     */
    void AddOnOffTraffic(TrafficDirection_t direction,
                         TransportLayerProtocol_t protocol,
                         DataRate dataRate,
                         uint32_t packetSize,
                         NodeContainer gwUsers,
                         NodeContainer utUsers,
                         std::string onTimePattern,
                         std::string offTimePattern,
                         Time startTime,
                         Time stopTime,
                         Time startDelay);

    /**
     * Add a new ONOFF traffic between chosen GWs and UTs
     * \param direction Direction of traffic
     * \param protocol Transport layer protocol
     * \param dataRate Data rate in ON state
     * \param packetSize Packet size in bytes
     * \param gwUsers The Gateway Users
     * \param utUsers The UT Users
     * \param onTimePattern Pattern for ON state duration
     * \param offTimePattern Pattern for OFF state duration
     * \param startTime Application Start time
     * \param stopTime Application stop time
     * \param startDelay application start delay between each user
     * \param percentage Percentage of UT users having the traffic installed
     */
    void AddOnOffTraffic(TrafficDirection_t direction,
                         TransportLayerProtocol_t protocol,
                         DataRate dataRate,
                         uint32_t packetSize,
                         NodeContainer gwUsers,
                         NodeContainer utUsers,
                         std::string onTimePattern,
                         std::string offTimePattern,
                         Time startTime,
                         Time stopTime,
                         Time startDelay,
                         double percentage);

    /**
     * Add a new TCP/HTTP traffic between chosen GWs and UTs
     * \param direction Direction of traffic
     * \param gwUsers The Gateway Users
     * \param utUsers The UT Users
     * \param startTime Application Start time
     * \param stopTime Application stop time
     * \param startDelay application start delay between each user
     */
    void AddHttpTraffic(TrafficDirection_t direction,
                        NodeContainer gwUsers,
                        NodeContainer utUsers,
                        Time startTime,
                        Time stopTime,
                        Time startDelay);

    /**
     * Add a new TCP/HTTP traffic between chosen GWs and UTs
     * \param direction Direction of traffic
     * \param gwUsers The Gateway Users
     * \param utUsers The UT Users
     * \param startTime Application Start time
     * \param stopTime Application stop time
     * \param startDelay application start delay between each user
     * \param percentage Percentage of UT users having the traffic installed
     */
    void AddHttpTraffic(TrafficDirection_t direction,
                        NodeContainer gwUsers,
                        NodeContainer utUsers,
                        Time startTime,
                        Time stopTime,
                        Time startDelay,
                        double percentage);

    /**
     * Add a new TCP/NRTV traffic between chosen GWs and UTs
     * \param direction Direction of traffic
     * \param gwUsers The Gateway Users
     * \param utUsers The UT Users
     * \param startTime Application Start time
     * \param stopTime Application stop time
     * \param startDelay application start delay between each user
     */
    void AddNrtvTraffic(TrafficDirection_t direction,
                        NodeContainer gwUsers,
                        NodeContainer utUsers,
                        Time startTime,
                        Time stopTime,
                        Time startDelay);

    /**
     * Add a new TCP/NRTV traffic between chosen GWs and UTs
     * \param direction Direction of traffic
     * \param gwUsers The Gateway Users
     * \param utUsers The UT Users
     * \param startTime Application Start time
     * \param stopTime Application stop time
     * \param startDelay application start delay between each user
     * \param percentage Percentage of UT users having the traffic installed
     */
    void AddNrtvTraffic(TrafficDirection_t direction,
                        NodeContainer gwUsers,
                        NodeContainer utUsers,
                        Time startTime,
                        Time stopTime,
                        Time startDelay,
                        double percentage);

    /**
     * Add a new Poisson traffic between chosen GWs and UTs
     * \param direction Direction of traffic
     * \param onTime On time duration in seconds
     * \param offTimeExpMean Off time mean in seconds. The off time follows an exponential law of
     * mean offTimeExpMean \param rate The rate with the unit \param packetSize Packet size in bytes
     * \param gwUsers The Gateway Users
     * \param utUsers The UT Users
     * \param startTime Application Start time
     * \param stopTime Application stop time
     * \param startDelay application start delay between each user
     */
    void AddPoissonTraffic(TrafficDirection_t direction,
                           Time onTime,
                           Time offTimeExpMean,
                           DataRate rate,
                           uint32_t packetSize,
                           NodeContainer gwUsers,
                           NodeContainer utUsers,
                           Time startTime,
                           Time stopTime,
                           Time startDelay);

    /**
     * Add a new Poisson traffic between chosen GWs and UTs
     * \param direction Direction of traffic
     * \param onTime On time duration in seconds
     * \param offTimeExpMean Off time mean in seconds. The off time follows an exponential law of
     * mean offTimeExpMean \param rate The rate with the unit \param packetSize Packet size in bytes
     * \param gwUsers The Gateway Users
     * \param utUsers The UT Users
     * \param startTime Application Start time
     * \param stopTime Application stop time
     * \param startDelay application start delay between each user
     * \param percentage Percentage of UT users having the traffic installed
     */
    void AddPoissonTraffic(TrafficDirection_t direction,
                           Time onTime,
                           Time offTimeExpMean,
                           DataRate rate,
                           uint32_t packetSize,
                           NodeContainer gwUsers,
                           NodeContainer utUsers,
                           Time startTime,
                           Time stopTime,
                           Time startDelay,
                           double percentage);

    /**
     * Add a new Poisson traffic between chosen GWs and UTs
     * \param direction Direction of traffic
     * \param codec the Codec used
     * \param gwUsers The Gateway Users
     * \param utUsers The UT Users
     * \param startTime Application Start time
     * \param stopTime Application stop time
     * \param startDelay application start delay between each user
     */
    void AddVoipTraffic(TrafficDirection_t direction,
                        VoipCodec_t codec,
                        NodeContainer gwUsers,
                        NodeContainer utUsers,
                        Time startTime,
                        Time stopTime,
                        Time startDelay);

    /**
     * Add a new Poisson traffic between chosen GWs and UTs
     * \param direction Direction of traffic
     * \param codec the Codec used
     * \param gwUsers The Gateway Users
     * \param utUsers The UT Users
     * \param startTime Application Start time
     * \param stopTime Application stop time
     * \param startDelay application start delay between each user
     * \param percentage Percentage of UT users having the traffic installed
     */
    void AddVoipTraffic(TrafficDirection_t direction,
                        VoipCodec_t codec,
                        NodeContainer gwUsers,
                        NodeContainer utUsers,
                        Time startTime,
                        Time stopTime,
                        Time startDelay,
                        double percentage);

    /**
     * Add a new CBR traffic between chosen GWs and UTs that can be customized
     * \param direction Direction of traffic
     * \param interval Initial wait time between transmission of two packets
     * \param packetSize Packet size in bytes
     * \param gwUsers The Gateway Users
     * \param utUsers The UT Users
     * \param startTime Application Start time
     * \param stopTime Application stop time
     * \param startDelay application start delay between each user
     */
    void AddCustomTraffic(TrafficDirection_t direction,
                          std::string interval,
                          uint32_t packetSize,
                          NodeContainer gwUsers,
                          NodeContainer utUsers,
                          Time startTime,
                          Time stopTime,
                          Time startDelay);

    /**
     * Change the parameters of the last custom traffic created
     * \param delay Delay after traffic launch to apply the changes
     * \param interval New wait time between transmission of two packets
     * \param packetSize New packet size in bytes
     */
    void ChangeCustomTraffic(Time delay, std::string interval, uint32_t packetSize);

  private:
    /**
     * \brief Struct for info on last custom trafic created
     */
    typedef struct
    {
        ApplicationContainer application;
        Time start;
        Time stop;
        bool created;
    } CustomTrafficInfo_s;

    Ptr<SatHelper> m_satHelper; // Pointer to the SatHelper objet
    Ptr<SatStatsHelperContainer>
        m_satStatsHelperContainer; // Pointer to the SatStatsHelperContainer objet

    CustomTrafficInfo_s m_last_custom_application; // Last application container of custom traffic

    bool m_enableDefaultStatistics;

    /**
     * Update the chosen attribute of a custom traffic
     * \param application The CBR application to update
     * \param interval The new interval
     * \param packetSize the new packet size
     */
    void UpdateAttribute(Ptr<CbrApplication> application,
                         std::string interval,
                         uint32_t packetSize);

    /**
     * \brief Check if node has a PacketSink installed at certain port.
     */
    bool HasSinkInstalled(Ptr<Node> node, uint16_t port);
};

} // namespace ns3

#endif /* __SATELLITE_TRAFFIC_HELPER_H__ */

// More generic
// call functons AddVoipTraffic, AddPoissonTraffic, etc.
// each one call a generic function (with for loops). Has a additionnal function -> subfunction, for
// traffic with parameters Subfunction (private) call GW*UT times
