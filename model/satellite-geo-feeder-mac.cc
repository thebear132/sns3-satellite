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

#include <ns3/log.h>
#include <ns3/simulator.h>
#include <ns3/double.h>
#include <ns3/uinteger.h>
#include <ns3/pointer.h>
#include <ns3/enum.h>

#include "satellite-utils.h"
#include "satellite-geo-feeder-mac.h"
#include "satellite-mac.h"
#include "satellite-signal-parameters.h"


NS_LOG_COMPONENT_DEFINE ("SatGeoFeederMac");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SatGeoFeederMac);

TypeId
SatGeoFeederMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SatGeoFeederMac")
    .SetParent<SatMac> ()
    .AddConstructor<SatGeoFeederMac> ()
  ;
  return tid;
}

TypeId
SatGeoFeederMac::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);

  return GetTypeId ();
}

SatGeoFeederMac::SatGeoFeederMac (void)
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("SatGeoFeederMac default constructor is not allowed to use");
}

SatGeoFeederMac::SatGeoFeederMac (uint32_t beamId,
                                  SatEnums::RegenerationMode_t forwardLinkRegenerationMode,
                                  SatEnums::RegenerationMode_t returnLinkRegenerationMode)
 : SatMac (beamId)
{
  NS_LOG_FUNCTION (this);

  m_forwardLinkRegenerationMode = forwardLinkRegenerationMode;
  m_returnLinkRegenerationMode = returnLinkRegenerationMode;
}

SatGeoFeederMac::~SatGeoFeederMac ()
{
  NS_LOG_FUNCTION (this);
}

void
SatGeoFeederMac::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  Object::DoDispose ();
}

void
SatGeoFeederMac::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  Object::DoInitialize ();
}

void
SatGeoFeederMac::SendPackets (SatPhy::PacketContainer_t packets, Ptr<SatSignalParameters> txParams)
{
  NS_LOG_FUNCTION (this);

  // Update MAC address source and destination
  for (SatPhy::PacketContainer_t::const_iterator it = packets.begin ();
       it != packets.end (); ++it)
    {
      SatMacTag mTag;
      bool success = (*it)->RemovePacketTag (mTag);

      SatAddressE2ETag addressE2ETag;
      success &= (*it)->PeekPacketTag (addressE2ETag);

      // MAC tag and E2E address tag found
      if (success)
        {
          mTag.SetDestAddress (addressE2ETag.GetE2EDestAddress ());
          mTag.SetSourceAddress (m_nodeInfo->GetMacAddress ());
          (*it)->AddPacketTag (mTag);
        }
    }

  // TODO
  m_txFeederCallback (txParams);
}

void
SatGeoFeederMac::Receive (SatPhy::PacketContainer_t packets, Ptr<SatSignalParameters> rxParams)
{
  NS_LOG_FUNCTION (this);

  // TODO
  m_rxFeederCallback (packets, rxParams);
}

void
SatGeoFeederMac::SetTransmitFeederCallback (TransmitFeederCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
  m_txFeederCallback = cb;
}

void
SatGeoFeederMac::SetReceiveFeederCallback (ReceiveFeederCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
  m_rxFeederCallback = cb;
}

void
SatGeoFeederMac::RxTraces (SatPhy::PacketContainer_t packets)
{
  NS_LOG_FUNCTION (this);

  if (m_isStatisticsTagsEnabled)
    {
      // TODO

    } // end of `if (m_isStatisticsTagsEnabled)`
}

SatEnums::SatLinkDir_t
SatGeoFeederMac::GetSatLinkTxDir ()
{
  return SatEnums::LD_FORWARD;
}

SatEnums::SatLinkDir_t
SatGeoFeederMac::GetSatLinkRxDir ()
{
  return SatEnums::LD_RETURN;
}

} // namespace ns3
