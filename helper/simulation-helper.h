/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 Magister Solutions
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
 * Author: Lauri Sormunen <lauri.sormunen@magister.fi>
 * Author: Mathias Ettinger <mettinger@viveris.toulouse.fr>
 */

#ifndef SIMULATION_HELPER_H
#define SIMULATION_HELPER_H

#include "satellite-cno-helper.h"
#include "satellite-group-helper.h"
#include "satellite-helper.h"
#include "satellite-traffic-helper.h"

#include <ns3/command-line.h>
#include <ns3/random-variable-stream.h>
#include <ns3/satellite-enums.h>
#include <ns3/satellite-stats-helper-container.h>

#include <map>
#include <set>
#include <string>

namespace ns3
{

/**
 * \ingroup satellite
 * \brief A helper to make it easier to create example simulation cases.
 *
 * Example usage:
 *
 * Ptr<SimulationHelper> simulationHelper = CreateObject<SimulationHelper> ("My satellite
 * simulation");
 *
 * simulationHelper->SetDefaultValues ();
 *
 * simulationHelper->SetBeams ("28 46 48 50 59");
 * simulationHelper->SetUtCountPerBeam (20);
 * simulationHelper->SetUserCountPerUt (1);
 * simulationHelper->SetSimulationTime (300);
 *
 * You may also customize the attribute settings with variety of different
 * public methods grouping several attributes under specific features.
 *
 */
class SimulationHelper : public Object
{
  public:
    /**
     * Default constructor, which is not used.
     */
    SimulationHelper();

    /**
     * \brief Contructor for simulation helper.
     * \param simulationName Name of the simulation.
     */
    SimulationHelper(std::string simulationName);

    /**
     * \brief Destructor.
     */
    virtual ~SimulationHelper();

    /**
     * \brief Disposing.
     */
    void DoDispose(void);

    /**
     * \brief Derived from Object.
     */
    static TypeId GetTypeId(void);

    /**
     * \brief Derived from Object.
     */
    TypeId GetInstanceTypeId(void) const;

    /**
     * \brief Set default values shared by all examples using
     * SimulationHelper. Adjust to your needs.
     */
    void SetDefaultValues();

    /**
     * \brief Set enabled beams (1-72) as a string.
     * \param beamList List of beams.
     * \example simulationHelper->SetBeams ("1 5 20 71")
     *          enables beams 1, 5, 20 and 71.
     */
    void SetBeams(const std::string& beamList);

    /**
     * \brief Set enabled beams (1-72) as a set.
     * \param beamSet List of beams.
     * \example simulationHelper->SetBeams ({1,2,3})
     *          enables beams 1, 2 and 3..
     */
    void SetBeamSet(std::set<uint32_t> beamSet);

    inline std::set<uint32_t> GetBeamSet(void) const
    {
        return m_enabledBeams;
    }

    /**
     * \brief Get enabled beams in integer format.
     * \return const set of integers representing beam ids
     */
    const std::set<uint32_t>& GetBeams();

    /**
     * \brief Set UT count per beam.
     * \param count Number of UTs per beam.
     */
    void SetUtCountPerBeam(uint32_t count);

    /**
     * \brief Set UT count per beam to be taken from a random variable stream.
     * \param rs RandomVariableStream to be used, must implement GetInteger.
     */
    void SetUtCountPerBeam(Ptr<RandomVariableStream> rs);

    /**
     * \brief Set UT count per beam.
     * \param count Number of UTs per beam.
     */
    void SetUtCountPerBeam(uint32_t beamId, uint32_t count);

    /**
     * \brief Set UT count per beam to be taken from a random variable stream.
     * \param rs RandomVariableStream to be used, must implement GetInteger.
     */
    void SetUtCountPerBeam(uint32_t beamId, Ptr<RandomVariableStream> rs);

    /**
     * \brief Set user count per UT.
     * \param count Number of users per UT.
     */
    void SetUserCountPerUt(uint32_t count);

    /**
     * \brief Set UT count per beam to be taken from a random variable stream.
     * \param rs RandomVariableStream to be used, must implement GetInteger.
     */
    void SetUserCountPerUt(Ptr<RandomVariableStream> rs);

    /**
     * \brief Set user count per mobile UT.
     * \param count Number of users per mobile UT.
     */
    void SetUserCountPerMobileUt(uint32_t count);

    /**
     * \brief Set mobile UT count per beam to be taken from a random variable stream.
     * \param rs RandomVariableStream to be used, must implement GetInteger.
     */
    void SetUserCountPerMobileUt(Ptr<RandomVariableStream> rs);

    /**
     * \brief Set the number of GW users in the scenario.
     * Must be called before creation of satellite scenario.
     * \param gwUserCount
     */
    void SetGwUserCount(uint32_t gwUserCount);

    /**
     * \brief Set simulation time in seconds.
     * \param seconds
     */
    void SetSimulationTime(double seconds);

    /**
     * \brief Set simulation time
     * \param time
     */
    inline void SetSimulationTime(Time time)
    {
        m_simTime = time;
    }

    /**
     * \brief Set ideal channel/physical layer parameterization.
     */
    void SetIdealPhyParameterization();

    /**
     * \brief Enable ACM for a simulation direction.
     * \param dir Direction
     */
    void EnableAcm(SatEnums::SatLinkDir_t dir);

    /**
     * \brief Disable ACM for a simulation direction.
     * \param dir Direction
     */
    void DisableAcm(SatEnums::SatLinkDir_t dir);

    /**
     * \brief Disable all capacity allocation categories: CRA/VBDC/RBDC
     */
    void DisableAllCapacityAssignmentCategories();

    /**
     * \brief Enable only CRA for a given RC index
     * \param rcIndex Request class index
     * \param rateKbps CRA rate in kbps
     */
    void EnableOnlyConstantRate(uint32_t rcIndex, double rateKbps);

    /**
     * \brief Enable only RBDC for a given RC index
     * \param rcIndex Request class index
     */
    void EnableOnlyRbdc(uint32_t rcIndex);

    /**
     * \brief Enable only VBDC for a given RC index
     * \param rcIndex Request class index
     */
    void EnableOnlyVbdc(uint32_t rcIndex);

    /**
     * \brief Enable free capacity allocation
     */
    void EnableFca();

    /**
     * \brief Disable free capacity allocation
     */
    void DisableFca();

    /**
     * \brief Enable periodical control slots
     */
    void EnablePeriodicalControlSlots(Time periodicity);

    /**
     * \brief Enable ARQ
     */
    void EnableArq(SatEnums::SatLinkDir_t dir);

    /**
     * \brief Disable random access
     */
    void DisableRandomAccess();

    /**
     * \brief Enable slotted ALOHA random access
     */
    void EnableSlottedAloha();

    /**
     * \brief Enable CRDSA random access
     */
    void EnableCrdsa();

    /**
     * \brief Configure a frame for a certain superframe id.
     * \param superFrameId Superframe id (currently always 0)
     * \param bw Frame bandwidth
     * \param carrierBw Bandwidth of the carriers within frame
     * \param rollOff Roll-off
     * \param carrierSpacing Carrier spacing between frames
     * \param isRandomAccess Is this a RA or DA frame
     */
    void ConfigureFrame(uint32_t superFrameId,
                        double bw,
                        double carrierBw,
                        double rollOff,
                        double carrierSpacing,
                        bool isRandomAccess = false);

    /**
     * \brief Configure the default setting for the forward
     * and return link frequencies.
     */
    void ConfigureFrequencyBands();

    /**
     * \brief Configure the beam hopping functionality for
     * the FWD link. This includes also setup of the proper
     * frequency configuration related to reuse one in
     * FWD link beam hopping.
     */
    void ConfigureFwdLinkBeamHopping();

    /**
     * \brief Enable external fading input.
     */
    void EnableExternalFadingInputTrace();

    /**
     * \brief Enable all output traces
     */
    void EnableOutputTraces();

    /**
     * \brief Configure all link budget related attributes
     */
    void ConfigureLinkBudget();

    /**
     * \brief Set simulation error model.
     * \param em Error model.
     * \param errorRate Static error rate if constant error model used
     */
    void SetErrorModel(SatPhyRxCarrierConf::ErrorModel em, double errorRate = 0.0);

    /**
     * \brief Set simulation interference model.
     * \param ifModel Interference model.
     * \param ifEliminationModel Interference elimination model.
     * \param constantIf Static interference if constant interference model used
     * \param residualSamplingError Sampling error if residual interference elimination model used
     */
    void SetInterferenceModel(SatPhyRxCarrierConf::InterferenceModel ifModel,
                              double constantIf = 0.0);

    /**
     * \brief Enables simulation progress logging.
     * Progress is logged to stdout in form of 'Progress: (current simulation time)/(simulation
     * length)'.
     */
    void EnableProgressLogs();

    /**
     * \brief Disables simulation progress logs.
     */
    void DisableProgressLogs();

    /**
     * \brief Add default command line arguments for the simulation.
     * This method must be called between creation of the CommandLine helper and CommandLine::Parse
     * () call. \param cmd Reference to CommandLine helper instance
     */
    void AddDefaultUiArguments(CommandLine& cmd);

    /**
     * \brief Add default command line arguments for the simulation.
     * This method must be called between creation of the CommandLine helper and CommandLine::Parse
     * () call. \param cmd Reference to CommandLine helper instance \param xmlInputFile Reference to
     * string containing XML input file name
     */
    void AddDefaultUiArguments(CommandLine& cmd, std::string& xmlInputFile);

    /**
     * \brief Run the simulation
     */
    void RunSimulation();

    /**
     * \brief Load a scenario from data submodule.
     *
     * \param name The scenario name. Must be a folder located in data/scenarios
     */
    void LoadScenario(std::string name);

    /**
     * \brief parse scenario folder to load all variables that can be
     */
    void ParseScenarioFolder();

    /**
     * \brief Create the satellite scenario.
     * \param scenario Kind of scenario to create, if any
     * \param mobileUtsFolder Folder from which to load mobile UT traces, if any
     * \return satHelper Satellite helper, which provides e.g. nodes for application installation.
     */
    Ptr<SatHelper> CreateSatScenario(SatHelper::PreDefinedScenario_t scenario = SatHelper::NONE,
                                     const std::string& mobileUtsFolder = "");

    /**
     * \brief Create stats collectors and set default statistics settings
     * for both FWD and RTN links.
     */
    void CreateDefaultStats();

    /**
     * \brief Create stats collectors if needed and set default statistics settings
     * for both FWD link. Adjust this method to your needs.
     */
    void CreateDefaultFwdLinkStats();

    /**
     * \brief Create stats collectors if needed and set default statistics settings
     * for both RTN link. Adjust this method to your needs.
     */
    void CreateDefaultRtnLinkStats();

    /**
     * \brief Set simulation output tag, which is the basename of the directory where
     * output files are stored.
     * \param tag Simulation tag.
     */
    void SetOutputTag(std::string tag);

    /**
     * \brief Force a output file path to this simulation instead of default
     * satellite/data/sims/.
     * \param path Output file path.
     */
    void SetOutputPath(std::string path);

    /**
     * \brief Configure this instance after reading input attributes from XML file
     * \param filePath full path to an Input XML file
     * \param overrideManualConfiguration whether or not to read some configuration (beams,
     * UT count per beam, user count per UT, simulation time) from XML file
     */
    void ConfigureAttributesFromFile(std::string filePath, bool overrideManualConfiguration = true);

    /**
     * \brief Read input attributes from XML file
     * \param filePath full path to an Input XML file
     */
    void ReadInputAttributesFromFile(std::string filePath);

    /**
     * \brief Store all used attributes
     * \param fileName Output filename
     * \param outputAttributes Whether or not to store
     *   individual objects attributes to file
     * \return string Output path
     */
    std::string StoreAttributesToFile(std::string fileName, bool outputAttributes = false);

    /**
     * \brief Get simulation time
     * \return errorRate Simulation time
     */

    inline Time& GetSimTime()
    {
        return m_simTime;
    }

    /**
     * \brief Set common UT position allocator for all beams.
     * The position allocator is used to draw UT position geocoordinates
     * in order of the beam IDs and number of UTs configured.
     * No validation is done.
     * \param posAllocator
     */
    void SetCommonUtPositionAllocator(Ptr<SatListPositionAllocator> posAllocator);

    /**
     * \brief Set a list position allocator for UTs of a specific beam.
     * The position allocator is used to draw UT position geocoordinates
     * when UTs are created for that specific beam-
     * \param beamId Beam ID
     * \param posAllocator List of UT positions, *must* match the number of UTs configured
     */
    void SetUtPositionAllocatorForBeam(uint32_t beamId, Ptr<SatListPositionAllocator> posAllocator);

    /**
     * \brief Enable reading UT list positions from input file
     * \param inputFile List postion file path (starting from data/)
     * \param checkBeam
     */
    void EnableUtListPositionsFromInputFile(std::string inputFile, bool checkBeams = true);

    /**
     * \brief If lower layer API access is required, use this to access SatHelper.
     * You MUST have called CreateSatScenario before calling this method.
     */
    inline Ptr<SatHelper> GetSatelliteHelper()
    {
        NS_ASSERT_MSG(m_satHelper != nullptr,
                      "CreateSatScenario not called before calling GetSatelliteHelper");
        return m_satHelper;
    }

    /**
     * \brief Get the statistics container of this helper. If does not exist, one is created.
     * \return Statistics helper container
     */
    Ptr<SatStatsHelperContainer> GetStatisticsContainer();

    /**
     * \brief Get the traffic helper to create more complex traffics. If does not exist, one is
     * created. \return Traffic Helper
     */
    Ptr<SatTrafficHelper> GetTrafficHelper();

    /**
     * \brief Get the group helper. If does not exist, one is created.
     * \return Group Helper
     */
    Ptr<SatGroupHelper> GetGroupHelper();

    /**
     * \brief Get the C/N0 helper to customize C/N0 on some nodes. If does not exist, one is
     * created. \return C/N0 Helper
     */
    Ptr<SatCnoHelper> GetCnoHelper();

    typedef enum
    {
        CR_NOT_CONFIGURED,
        CR_PERIODIC_CONTROL,
        CR_SLOTTED_ALOHA,
        CR_CRDSA_LOOSE_RC_0,
    } CrTxConf_t;

    void SetCrTxConf(CrTxConf_t crTxConf);

  protected:
    /**
     * \brief Enable random access
     */
    void EnableRandomAccess();

    /**
     * \brief Callback that prints simulation progress to stdout.
     */
    void ProgressCb();

    /**
     * \brief Check if a beam is enabled.
     */
    bool IsBeamEnabled(uint32_t beamId) const;

    /**
     * \brief Get next UT count from internal random variable stream.
     */
    uint32_t GetNextUtCount(uint32_t beamId = 0) const;

    /**
     * \brief Get next UT user count from internal random variable stream.
     */
    inline uint32_t GetNextUtUserCount() const
    {
        NS_ASSERT_MSG(m_utUserCount != nullptr, "User count per UT not set");
        return m_utUserCount->GetInteger();
    }

    /**
     * \brief Check if node has a PacketSink installed at certain port.
     */
    bool HasSinkInstalled(Ptr<Node> node, uint16_t port);

    /**
     * \brief Check if output path has been set. If not, then create a default
     * output directory inside satellite/data/sims/campaign-name/tag-name.
     */
    void SetupOutputPath();

  private:
    Ptr<SatHelper> m_satHelper;
    Ptr<SatStatsHelperContainer> m_statContainer;
    Ptr<SatTrafficHelper> m_trafficHelper;
    Ptr<SatGroupHelper> m_groupHelper;
    Ptr<SatCnoHelper> m_cnoHelper;
    Ptr<SatListPositionAllocator> m_commonUtPositions;
    std::map<uint32_t, Ptr<SatListPositionAllocator>> m_utPositionsByBeam;

    std::string m_scenarioPath;
    std::string m_simulationName;
    std::string m_simulationTag;
    std::string m_enabledBeamsStr;
    std::set<uint32_t> m_enabledBeams;
    std::string m_outputPath;

    std::map<uint32_t, Ptr<RandomVariableStream>> m_utCount;

    Ptr<RandomVariableStream> m_utUserCount;
    Ptr<RandomVariableStream> m_utMobileUserCount;
    Time m_simTime;
    uint32_t m_numberOfConfiguredFrames;
    bool m_randomAccessConfigured;
    std::string m_inputFileUtListPositions;
    bool m_inputFileUtPositionsCheckBeams;

    bool m_progressLoggingEnabled;
    Time m_progressUpdateInterval;
    EventId m_progressReportEvent;
};

class SimulationHelperConf : public Object
{
  public:
    /**
     * Default constructor.
     */
    SimulationHelperConf();

    /**
     * \brief Destructor.
     */
    virtual ~SimulationHelperConf();

    /**
     * \brief Derived from Object.
     */
    static TypeId GetTypeId(void);

    /**
     * \brief Derived from Object.
     */
    TypeId GetInstanceTypeId(void) const;

    Time m_simTime;
    std::string m_enabledBeams;
    Ptr<RandomVariableStream> m_utCount;
    Ptr<RandomVariableStream> m_utUserCount;
    Ptr<RandomVariableStream> m_utMobileUserCount;
    bool m_activateStatistics;
    bool m_activateProgressLogging;
    SimulationHelper::CrTxConf_t m_crTxConf;
    std::string m_mobileUtsFolder;
};

} // namespace ns3

#endif /* TEST_SCRIPT_INPUT_HELPER_H */
