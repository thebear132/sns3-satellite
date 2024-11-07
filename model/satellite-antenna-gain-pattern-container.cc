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
 * Author: Jani Puttonen <jani.puttonen@magister.fi>
 */

#include "satellite-antenna-gain-pattern-container.h"

#include "satellite-sgp4-mobility-model.h"

#include "ns3/log.h"
#include "ns3/satellite-env-variables.h"
#include "ns3/singleton.h"
#include "ns3/string.h"

#include <cmath>
#include <cstddef>
#include <dirent.h>
#include <errno.h>
#include <fstream>
#include <limits>
#include <map>
#include <sstream>
#include <string.h>
#include <string>
#include <utility>

NS_LOG_COMPONENT_DEFINE("SatAntennaGainPatternContainer");

const std::string numbers{"0123456789"};

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(SatAntennaGainPatternContainer);

TypeId
SatAntennaGainPatternContainer::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::SatAntennaGainPatternContainer")
                            .SetParent<Object>()
                            .AddConstructor<SatAntennaGainPatternContainer>();
    return tid;
}

TypeId
SatAntennaGainPatternContainer::GetInstanceTypeId() const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId();
}

SatAntennaGainPatternContainer::SatAntennaGainPatternContainer()
{
    NS_LOG_FUNCTION(this);

    NS_FATAL_ERROR("Constructor not in use");
}

SatAntennaGainPatternContainer::SatAntennaGainPatternContainer(uint32_t nbSats,
                                                               std::string patternsFolder)
{
    NS_LOG_FUNCTION(this << nbSats << patternsFolder);

    m_patternsFolder = patternsFolder;

    GeoCoordinate geoPos = GetDefaultGeoPosition();

    NS_LOG_INFO(this << " directory for antenna patterns set to " << m_patternsFolder);

    if (!Singleton<SatEnvVariables>::Get()->IsValidDirectory(m_patternsFolder))
    {
        NS_FATAL_ERROR("SatAntennaGainPatternContainer::SatAntennaGainPatternContainer directory "
                       << m_patternsFolder << " not found in antennapatterns folder");
    }

    DIR* dir;
    struct dirent* ent;
    std::string prefix;
    if ((dir = opendir(m_patternsFolder.c_str())) != nullptr)
    {
        /* process all the files and directories within m_patternsFolder */
        while ((ent = readdir(dir)) != nullptr)
        {
            std::string filename{ent->d_name};
            std::size_t pathLength = filename.length();
            if (pathLength > 4)
            {
                pathLength -= 4; // Size of .txt extention
                if (filename.substr(pathLength) == ".txt")
                {
                    std::string num, stem = filename.substr(0, pathLength);
                    std::size_t found = stem.find_last_not_of(numbers);
                    if (found == std::string::npos)
                    {
                        num = stem;
                        stem.erase(0);
                    }
                    else
                    {
                        num = stem.substr(found + 1);
                        stem.erase(found + 1);
                    }

                    if (prefix.empty())
                    {
                        prefix = stem;
                    }

                    if (prefix != stem)
                    {
                        NS_FATAL_ERROR(
                            "SatAntennaGainPatternContainer::SatAntennaGainPatternContainer mixing "
                            "different prefix for antenna pattern names: "
                            << prefix << " and " << stem);
                    }

                    std::string filePath = m_patternsFolder + "/" + filename;
                    std::istringstream ss{num};
                    uint32_t beamId;
                    ss >> beamId;
                    if (ss.bad())
                    {
                        NS_FATAL_ERROR(
                            "SatAntennaGainPatternContainer::SatAntennaGainPatternContainer unable "
                            "to find beam number in "
                            << filePath << " file name");
                    }

                    Ptr<SatAntennaGainPattern> gainPattern =
                        CreateObject<SatAntennaGainPattern>(filePath, geoPos);
                    std::pair<std::map<uint32_t, Ptr<SatAntennaGainPattern>>::iterator, bool> ret;
                    ret = m_antennaPatternMap.insert(std::make_pair(beamId, gainPattern));

                    if (ret.second == false)
                    {
                        NS_FATAL_ERROR("SatAntennaGainPatternContainer::"
                                       "SatAntennaGainPatternContainer an antenna pattern for beam "
                                       << beamId << " already exists!");
                    }
                }
            }
        }
        closedir(dir);
    }
    else
    {
        /* could not open directory */
        const char* error = strerror(errno);
        NS_FATAL_ERROR("SatAntennaGainPatternContainer::SatAntennaGainPatternContainer unable to "
                       "open directory "
                       << m_patternsFolder << ": " << error);
    }
}

SatAntennaGainPatternContainer::~SatAntennaGainPatternContainer()
{
    NS_LOG_FUNCTION(this);
}

GeoCoordinate
SatAntennaGainPatternContainer::GetDefaultGeoPosition()
{
    NS_LOG_FUNCTION(this);

    std::string geoPosFilename = m_patternsFolder + "/GeoPos.in";

    // READ FROM THE SPECIFIED INPUT FILE
    std::ifstream* ifs = new std::ifstream(geoPosFilename, std::ifstream::in);

    if (!ifs->is_open())
    {
        NS_FATAL_ERROR("The file " << geoPosFilename << " is not found.");
    }

    double lat, lon, alt;
    *ifs >> lat >> lon >> alt;

    // Store the values
    GeoCoordinate coord(lat, lon, alt);

    ifs->close();
    delete ifs;

    return coord;
}

Ptr<SatAntennaGainPattern>
SatAntennaGainPatternContainer::GetAntennaGainPattern(uint32_t beamId) const
{
    NS_LOG_FUNCTION(this << beamId);

    std::map<uint32_t, Ptr<SatAntennaGainPattern>>::const_iterator agp =
        m_antennaPatternMap.find(beamId);
    if (agp == m_antennaPatternMap.end())
    {
        NS_FATAL_ERROR(
            "SatAntennaGainPatternContainer::GetAntennaGainPattern - unvalid beam id: " << beamId);
    }

    return agp->second;
}

Ptr<SatMobilityModel>
SatAntennaGainPatternContainer::GetAntennaMobility(uint32_t satelliteId) const
{
    NS_LOG_FUNCTION(this << satelliteId);

    std::map<uint32_t, Ptr<SatMobilityModel>>::const_iterator mm =
        m_mobilityModelMap.find(satelliteId);
    if (mm == m_mobilityModelMap.end())
    {
        NS_FATAL_ERROR(
            "SatAntennaGainPatternContainer::GetAntennaGainPattern - unvalid satellite id: "
            << satelliteId);
    }

    return mm->second;
}

uint32_t
SatAntennaGainPatternContainer::GetBestBeamId(uint32_t satelliteId,
                                              GeoCoordinate coord,
                                              bool ignoreNan)
{
    NS_LOG_FUNCTION(this << satelliteId << coord.GetLatitude() << coord.GetLongitude()
                         << ignoreNan);

    double bestGain(-100.0);
    uint32_t bestId(0);

    Ptr<SatMobilityModel> mobility = m_mobilityModelMap[satelliteId];

    for (const auto& entry : m_antennaPatternMap)
    {
        uint32_t i = entry.first;
        double gain = entry.second->GetAntennaGain_lin(coord, mobility);

        // The antenna pattern has returned a NAN gain. This means
        // that this position is not valid. Return 0, which is not a valid beam id.
        if (std::isnan(gain))
        {
            if (ignoreNan)
            {
                NS_LOG_WARN("SatAntennaGainPatternContainer::GetBestBeamId - Beam "
                            << i << " returned a NAN antenna gain value!");
            }
            else
            {
                NS_FATAL_ERROR("SatAntennaGainPatternContainer::GetBestBeamId - Beam "
                               << i << " returned a NAN antenna gain value!");
            }
        }
        else if (gain > bestGain)
        {
            bestGain = gain;
            bestId = i;
        }
    }

    if (bestId == 0 && ignoreNan)
    {
        NS_LOG_WARN(
            "SatAntennaGainPatternContainer::GetBestBeamId - did not find any good beam! The "
            "ground station is probably too far from the satellite. Return 0 by default.");
    }

    return bestId;
}

double
SatAntennaGainPatternContainer::GetBeamGain(uint32_t satelliteId,
                                            uint32_t beamId,
                                            GeoCoordinate coord)
{
    NS_LOG_FUNCTION(this << satelliteId << beamId << coord.GetLatitude() << coord.GetLongitude());

    Ptr<SatMobilityModel> mobility = m_mobilityModelMap[satelliteId];

    if (m_antennaPatternMap.find(beamId) == m_antennaPatternMap.end())
    {
        return std::numeric_limits<double>::quiet_NaN();
    }
    Ptr<SatAntennaGainPattern> pattern = m_antennaPatternMap.at(beamId);

    return pattern->GetAntennaGain_lin(coord, mobility);
}

uint32_t
SatAntennaGainPatternContainer::GetNAntennaGainPatterns() const
{
    NS_LOG_FUNCTION(this);

    // Note, that now we assume that all the antenna patterns are created
    // regardless of how many beams are actually simulated.
    return m_antennaPatternMap.size();
}

void
SatAntennaGainPatternContainer::ConfigureBeamsMobility(uint32_t satelliteId,
                                                       Ptr<SatMobilityModel> mobility)
{
    NS_LOG_FUNCTION(this << satelliteId << mobility);

    m_mobilityModelMap[satelliteId] = mobility;
}

void
SatAntennaGainPatternContainer::SetEnabledBeams(BeamUserInfoMap_t& info)
{
    std::map<uint32_t, Ptr<SatAntennaGainPattern>>::iterator it = m_antennaPatternMap.begin();
    while (it != m_antennaPatternMap.end())
    {
        uint32_t beamIdIterator = it->first;
        BeamUserInfoMap_t::iterator infoIterator;
        bool found = false;
        for (infoIterator = info.begin(); infoIterator != info.end(); infoIterator++)
        {
            uint32_t beamId = infoIterator->first.second;
            if (beamId == beamIdIterator)
            {
                found = true;
            }
        }
        if (!found)
        {
            it = m_antennaPatternMap.erase(it);
        }
        else
        {
            it++;
        }
    }
}

} // namespace ns3
