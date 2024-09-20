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
 * Author: Sami Rantanen <sami.rantanen@magister.fi>
 */
#ifndef __SATELLITE_HELPER_H__
#define __SATELLITE_HELPER_H__

#include "satellite-beam-helper.h"
#include "satellite-beam-user-info.h"
#include "satellite-conf.h"
#include "satellite-group-helper.h"
#include "satellite-user-helper.h"

#include <ns3/csma-helper.h>
#include <ns3/ipv4-address-helper.h>
#include <ns3/node-container.h>
#include <ns3/object.h>
#include <ns3/output-stream-wrapper.h>
#include <ns3/satellite-antenna-gain-pattern-container.h>
#include <ns3/satellite-fading-input-trace-container.h>
#include <ns3/satellite-fading-output-trace-container.h>
#include <ns3/satellite-interference-input-trace-container.h>
#include <ns3/satellite-interference-output-trace-container.h>
#include <ns3/satellite-position-allocator.h>
#include <ns3/satellite-rx-power-input-trace-container.h>
#include <ns3/satellite-rx-power-output-trace-container.h>
#include <ns3/satellite-stats-helper-container.h>
#include <ns3/trace-helper.h>

#include <string>

namespace ns3
{

class SatGroupHelper;

/**
 * \brief Build a satellite network set with needed objects and configuration.
 *        Utilizes SatUserHelper and SatBeamHelper helper objects.
 */
class SatHelper : public Object
{
  public:
    /**
     * definition for beam map key is pair sat ID / beam ID and value is UT/user info.
     */
    typedef std::map<std::pair<uint32_t, uint32_t>, SatBeamUserInfo> BeamUserInfoMap_t;

    /**
     * \brief Values for pre-defined scenarios to be used by helper when building
     *        satellite network topology base.
     */
    typedef enum
    {
        NONE,   //!< NONE Not used.
        SIMPLE, //!< SIMPLE Simple scenario used as base.
        LARGER, //!< LARGER Larger scenario used as base.
        FULL    //!< FULL Full scenario used as base.
    } PreDefinedScenario_t;

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
     * \brief Default constructor. Not in use.
     */
    SatHelper();

    /**
     * \brief Create a base SatHelper for creating customized Satellite topologies.
     *
     * \param scenarioPath Scenario folder path
     */
    SatHelper(std::string scenarioPath);

    /**
     * Destructor for SatHelper
     */
    virtual ~SatHelper()
    {
    }

    /**
     * \brief Get number of Users for a UT
     * \return The number of UT users
     */
    typedef Callback<uint32_t> GetNextUtUserCountCallback;

    /**
     * \brief Create a pre-defined SatHelper to make life easier when creating Satellite topologies.
     */
    void CreatePredefinedScenario(PreDefinedScenario_t scenario);

    /**
     * Creates satellite objects according to user defined scenario.
     *
     * \param info information of the beams, and beam UTs and users in beams
     */
    void CreateUserDefinedScenario(BeamUserInfoMap_t& info);

    /**
     * Creates satellite objects according to user defined scenario.
     * Positions are read from different input files from file set by attribute
     * ns3::SatConf::UtPositionInputFileName.
     *
     * \param satId The ID of the satellite
     * \param info information of the beams, and beam UTs and users in beams
     * \param inputFileUtListPositions Path to the list of UT positions
     * \param checkBeam Check that positions (set through SatConf) match with given beam
     * (the beam is the best according to configured antenna patterns).
     */
    void CreateUserDefinedScenarioFromListPositions(uint32_t satId,
                                                    BeamUserInfoMap_t& info,
                                                    std::string inputFileUtListPositions,
                                                    bool checkBeam);

    /**
     * Load satellite objects according to constellation parameters.
     *
     * \param infoList information of the enabled beams. UT information is given in parameters
     * files. \param getNextUtUserCountCallback Callback to get number of users per UT.
     */
    void LoadConstellationScenario(BeamUserInfoMap_t& info,
                                   GetNextUtUserCountCallback getNextUtUserCountCallback);

    /**
     * Set the value of GW address for each UT.
     * This method is called when using constellations.
     */
    void SetGwAddressInUts();

    /**
     * Set the value of GW address for a single UT.
     * This method is called when using constellations, and can be called via callbacks after
     * handovers
     *
     * \param utId ID of UT to
     * \return MAC address of GW
     */
    Mac48Address GetGwAddressInSingleUt(uint32_t utId);

    /**
     * Populate the routes, when using constellations.
     */
    void SetBeamRoutingConstellations();

    /**
     * Get closest satellite to a ground station
     * \param position The position of the ground station
     * \return The ID of the closest satellite
     */
    uint32_t GetClosestSat(GeoCoordinate position);

    /**
     * \param  node pointer to user node.
     *
     * \return address of the user.
     */
    Ipv4Address GetUserAddress(Ptr<Node> node);

    /**
     * \return pointer to beam helper.
     */
    Ptr<SatBeamHelper> GetBeamHelper() const;

    /**
     * \return pointer to group helper.
     */
    Ptr<SatGroupHelper> GetGroupHelper() const;

    /**
     * \brief set the group helper.
     */
    void SetGroupHelper(Ptr<SatGroupHelper> groupHelper);

    /**
     * \brief Set the antenna gain patterns.
     * \param antennaGainPattern The pattern to set
     */
    void SetAntennaGainPatterns(Ptr<SatAntennaGainPatternContainer> antennaGainPattern);

    /**
     * \return Get the antenna gain patterns
     */
    Ptr<SatAntennaGainPatternContainer> GetAntennaGainPatterns();

    /**
     * \return pointer to user helper.
     */
    Ptr<SatUserHelper> GetUserHelper() const;

    /**
     * Get count of the beams (configurations).
     *
     * \return beam count
     */
    uint32_t GetBeamCount() const;

    /**
     * \brief Set custom position allocator
     * \param posAllocator
     */
    void SetCustomUtPositionAllocator(Ptr<SatListPositionAllocator> posAllocator);

    /**
     * \brief Set custom position allocator for specific beam.
     * This overrides the custom position allocator for this beam.
     * \param beamId
     * \param posAllocator
     */
    void SetUtPositionAllocatorForBeam(uint32_t beamId, Ptr<SatListPositionAllocator> posAllocator);

    /**
     * \brief Load UTs with a SatTracedMobilityModel associated to them from the
     * files found in the given folder. Each UT will be associated to the beam it
     * is at it's starting position.
     * \param folderName Name of the folder to search for mobility trace files
     * \param utUsers Stream to generate the number of users associated to each loaded UT
     */
    void LoadMobileUTsFromFolder(const std::string& folderName, Ptr<RandomVariableStream> utUsers);

    /**
     * \brief Load an UT with a SatTracedMobilityModel associated to
     * them from the given file.
     * \param filename Name of the trace file containing UT positions
     */
    Ptr<Node> LoadMobileUtFromFile(const std::string& filename);

    /**
     * \brief Load an UT with a SatTracedMobilityModel associated to
     * them from the given file.
     * \param satId Satellite ID
     * \param filename Name of the trace file containing UT positions
     */
    Ptr<Node> LoadMobileUtFromFile(uint32_t satId, const std::string& filename);

    /**
     * Set multicast group to satellite network and IP router. Add needed routes to net devices.
     *
     * \param source Source node of the multicast group (GW or UT connected user node)
     * \param receivers Receiver nodes of the multicast group. (GW or UT connected user nodes)
     * \param sourceAddress Source address of the multicast group.
     * \param groupAddress Address of the multicast group.
     */
    void SetMulticastGroupRoutes(Ptr<Node> source,
                                 NodeContainer receivers,
                                 Ipv4Address sourceAddress,
                                 Ipv4Address groupAddress);

    /**
     * Dispose of this class instance
     */
    void DoDispose();

    /**
     * Create a SatSpotBeamPositionAllocator able to generate
     * random position within the given beam.
     *
     * \param beamId the beam for which the position allocator should be configured
     */
    Ptr<SatSpotBeamPositionAllocator> GetBeamAllocator(uint32_t beamId);

    inline bool IsSatConstellationEnabled()
    {
        return m_satConstellationEnabled;
    }

    /**
     * Print all the satellite topology
     * \param os output stream in which the data should be printed
     */
    void PrintTopology(std::ostream& os) const;

  private:
    static const uint16_t MIN_ADDRESS_PREFIX_LENGTH = 1;
    static const uint16_t MAX_ADDRESS_PREFIX_LENGTH = 31;

    typedef SatBeamHelper::MulticastBeamInfoItem_t MulticastBeamInfoItem_t;
    typedef SatBeamHelper::MulticastBeamInfo_t MulticastBeamInfo_t;

    /**
     * Scenario folder path
     */
    std::string m_scenarioPath;

    /**
     * Configuration file names as attributes of this class
     */
    std::string m_rtnConfFileName;
    std::string m_fwdConfFileName;
    std::string m_gwPosFileName;
    std::string m_satPosFileName;
    std::string m_utPosFileName;
    std::string m_waveformConfDirectoryName;

    /**
     * Use a constellation of satellites
     */
    bool m_satConstellationEnabled;

    /*
     * The global standard used. Can be either DVB or Lora
     */
    SatEnums::Standard_t m_standard;

    /**
     * User helper
     */
    Ptr<SatUserHelper> m_userHelper;

    /**
     * Beam helper
     */
    Ptr<SatBeamHelper> m_beamHelper;

    /**
     * Group helper
     */
    Ptr<SatGroupHelper> m_groupHelper;

    /**
     * Configuration for satellite network.
     */
    Ptr<SatConf> m_satConf;

    /**
     * Trace callback for creation traces (details)
     */
    TracedCallback<std::string> m_creationDetailsTrace;

    /**
     * Trace callback for creation traces (summary)
     */

    TracedCallback<std::string> m_creationSummaryTrace;

    /**
     * Stream wrapper used for creation traces
     */
    Ptr<OutputStreamWrapper> m_creationTraceStream;

    /**
     * Stream wrapper used for UT position traces
     */
    Ptr<OutputStreamWrapper> m_utTraceStream;

    Ipv4Address
        m_beamNetworkAddress; ///< Initial network number of satellite devices, e.g., 10.1.1.0.
    Ipv4Address
        m_gwNetworkAddress; ///< Initial network number of GW, router, and GW users, e.g., 10.2.1.0.
    Ipv4Address m_utNetworkAddress; ///< Initial network number of UT and UT users, e.g., 10.3.1.0.

    Ipv4Mask m_beamNetworkMask; ///< Network mask number of satellite devices.
    Ipv4Mask m_gwNetworkMask;   ///< Network mask number of GW, router, and GW users.
    Ipv4Mask m_utNetworkMask;   ///< Network mask number of UT and UT users.

    /**
     * Enable handovers for all UTs and GWs. If false, only moving UTs can perform handovers.
     */
    bool m_handoversEnabled;

    /**
     * flag to check if scenario is already created.
     */
    bool m_scenarioCreated;

    /**
     * flag to indicate if creation trace should be enabled for scenario creation.
     */
    bool m_creationTraces;

    /**
     * flag to indicate if detailed creation trace should be enabled for scenario creation.
     */
    bool m_detailedCreationTraces;

    /**
     * flag to indicate if packet trace should be enabled after scenario creation.
     */
    bool m_packetTraces;

    /**
     * Number of UTs created per Beam in full or user-defined scenario
     */
    uint32_t m_utsInBeam;

    /**
     * Number of users created in public network (behind GWs) in full or user-defined scenario
     */
    uint32_t m_gwUsers;

    /**
     * Number of users created in end user network (behind every UT) in full or user-defined
     * scenario
     */
    uint32_t m_utUsers;

    /**
     * Info for beam creation in user defined scenario.
     * first is ID of the beam and second is number of beam created in beam.
     * If second is zero then default number of UTs is created (number set by attribute UtCount)
     *
     * Info is set by attribute BeamInfo
     */
    BeamUserInfoMap_t m_beamUserInfos;

    /**
     * File name for scenario creation trace output
     */
    std::string m_scenarioCreationFileName;

    /**
     * File name for UT creation trace output
     */
    std::string m_utCreationFileName;

    /**
     * File name for Waveform configurations file
     */
    std::string m_wfConfigFileName;

    /**
     * Antenna gain patterns for all spot-beams. Used for beam selection.
     */
    Ptr<SatAntennaGainPatternContainer> m_antennaGainPatterns;

    /**
     * User defined UT positions by beam ID. This is preferred to m_utPositions,
     * which is a common list for all UTs.
     */
    std::map<uint32_t, Ptr<SatListPositionAllocator>> m_utPositionsByBeam;

    /**
     * User defined UT positions from SatConf (or manually set)
     */
    Ptr<SatListPositionAllocator> m_utPositions;

    /**
     * List of mobile UTs by beam ID.
     */
    std::map<uint32_t, NodeContainer> m_mobileUtsByBeam;

    /**
     * List of users by mobile UT by beam ID.
     */
    std::multimap<uint32_t, uint32_t> m_mobileUtsUsersByBeam;

    /**
     * Map of closest satellite for each GW
     */
    std::map<uint32_t, uint32_t> m_gwSats;

    /**
     * Map indicating all UT NetDevices associated to each GW NetDevice
     */
    std::map<Ptr<NetDevice>, NetDeviceContainer> m_utsDistribution;

    /**
     * Enables creation traces to be written in given file
     */
    void EnableCreationTraces();

    /**
     * Enables creation traces in sub-helpers.
     */
    void EnableDetailedCreationTraces();

    /**
     * Enable packet traces
     */
    void EnablePacketTrace();

    /**
     * Load a constellation topology.
     * \param tles vector to store read TLEs
     * \param isls vector to store read ISLs
     */
    void LoadConstellationTopology(std::vector<std::string>& tles,
                                   std::vector<std::pair<uint32_t, uint32_t>>& isls);

    /**
     * Sink for creation details traces
     * \param stream stream for traces
     * \param context context for traces
     * \param info creation info
     */
    static void CreationDetailsSink(Ptr<OutputStreamWrapper> stream,
                                    std::string context,
                                    std::string info);
    /**
     * Sink for creation summary traces
     * \param title creation summary title
     */
    void CreationSummarySink(std::string title);
    /**
     * Creates satellite objects according to simple scenario.
     */
    void CreateSimpleScenario();
    /**
     * Creates satellite objects according to larger scenario.
     */
    void CreateLargerScenario();
    /**
     * Creates satellite objects according to full scenario.
     */
    void CreateFullScenario();

    /**
     * Creates satellite objects according to given beam info.
     * \param infoList information of the sats and beam to create (and beams which are given in map)
     * \param gwUsers number of the users in GW(s) side
     */
    void DoCreateScenario(BeamUserInfoMap_t& info, uint32_t gwUsers);

    /**
     * Creates trace summary starting with give title.
     * \param title title for summary
     * \returns std::string as summary
     */
    std::string CreateCreationSummary(std::string title);

    /**
     * Sets mobilities to created GW nodes.
     *
     * \param gwNodes node container of UTs to set mobility
     */
    void SetGwMobility(NodeContainer gwNodes);

    /**
     * Sets mobility to created Sat node.
     *
     * \param node node pointer of Satellite to set mobility
     */
    void SetSatMobility(Ptr<Node> node);

    /**
     * Sets SGP4 mobility to created Sat node.
     *
     * \param node node pointer of Satellite to set mobility
     * \param tle the TLE linked to this satellite
     */
    void SetSatMobility(Ptr<Node> node, std::string tle);

    /**
     * Sets mobility to created UT nodes.
     *
     * \param uts node container of UTs to set mobility
     * \param satId the satellite id, where the UTs should be placed
     * \param beamId the spot-beam id, where the UTs should be placed
     *
     */
    void SetUtMobility(NodeContainer uts, uint32_t satId, uint32_t beamId);

    /**
     * Sets mobility to created UT nodes when position is known.
     *
     * \param uts node container of UTs to set mobility
     * \param satId the satellite id, where the UTs should be placed
     * \param beamId the spot-beam id, where the UTs should be placed
     * \param positionsAndGroupId the list of known positions, associated to a group ID
     *
     */
    void SetUtMobilityFromPosition(
        NodeContainer uts,
        uint32_t satId,
        uint32_t beamId,
        std::vector<std::pair<GeoCoordinate, uint32_t>> positionsAndGroupId);

    /**
     * Install Satellite Mobility Observer to nodes, if observer doesn't exist already in a node
     *
     * \param satId ID of the satellite.
     * \param nodes Nodecontainer of nodes to install mobility observer.
     */
    void InstallMobilityObserver(uint32_t satId, NodeContainer nodes) const;

    /**
     * Find given device's counterpart (device belonging to same network) device from given node.
     *
     * \param devA Pointer to the device whose counterpart device is found from given node.
     * \param nodeB Pointer to node where given device's counterpart device is searched.
     * \return Pointer to device belonging to same network with given device in given node.
     *         NULL in cast that counterpart device is not found.
     */
    Ptr<NetDevice> FindMatchingDevice(Ptr<NetDevice> devA, Ptr<Node> nodeB);

    /// \return The device belonging to same network with given device on given node.
    /**
     * Find counterpart (device belonging to same network) devices from given nodes.
     *
     * @param nodeA Pointer to node A where given device's counterpart device is searched.
     * @param nodeB Pointer to node A where given device's counterpart device is searched.
     * @param matchingDevices Pair consisting pointers to found devices. first belongs to nodeA
     *                        and second to nodeB.
     * @return true when counterpart devices are found from given nodes, false in other cases.
     */
    bool FindMatchingDevices(Ptr<Node> nodeA,
                             Ptr<Node> nodeB,
                             std::pair<Ptr<NetDevice>, Ptr<NetDevice>>& matchingDevices);

    /**
     * Set multicast traffic to source's nwtwork by finding source network utilizing given
     * destination node.
     *
     * Note that all multicast traffic is routed by source through selected device in source node
     * to found network.
     *
     * \param source Pointer to source node of the multicast traffic.
     * \param destination Pointer to destination node where to find matching source network
     */
    void SetMulticastRouteToSourceNetwork(Ptr<Node> source, Ptr<Node> destination);

    /**
     * Construct multicast information from source UT node and group receivers.
     *
     * \param sourceUtNode Pointer to UT source node. When NULL source node is not UT.
     * \param receivers Container of the multicast group receivers.
     * \param beamInfo Beam information to be filled in for multicast group.
     * \param routerUserOutputDev Pointer to router output device for backbone network (GW users).
     * Set to NULL when traffic is not needed to route backbone network. \return true when multicast
     * traffic shall be routed to source's network.
     */
    bool ConstructMulticastInfo(Ptr<Node> sourceUtNode,
                                NodeContainer receivers,
                                MulticastBeamInfo_t& beamInfo,
                                Ptr<NetDevice>& routerUserOutputDev);

    /**
     * Set configured network addresses to user and beam helpers.
     */
    void SetNetworkAddresses(BeamUserInfoMap_t& info, uint32_t gwUsers) const;

    /**
     * Check validity of the configured network space.
     *
     * \param networkName Name string of the network to check. To be used when printing out possible
     * errors. \param firstNetwork Address of the first network. \param mask The mask of the
     * networks. \param networkAddresses The container of first address values of the networks used
     * all together. \param networkCount The number of the networks created in network. \param
     * hostCount The maximum number of the hosts created in a network.
     */
    void CheckNetwork(std::string networkName,
                      const Ipv4Address& firstNetwork,
                      const Ipv4Mask& mask,
                      const std::set<uint32_t>& networkAddresses,
                      uint32_t networkCount,
                      uint32_t hostCount) const;

    /**
     * Read to standard use from file given in path
     *
     * \param pathName Path of file containing the standard used
     */
    void ReadStandard(std::string pathName);
};

} // namespace ns3

#endif /* __SATELLITE_HELPER_H__ */
