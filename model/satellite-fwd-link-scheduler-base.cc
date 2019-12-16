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
 * Author: Bastien Tauran <bastien.tauran@viveris.fr>
 */

#include "satellite-fwd-link-scheduler-base.h"


NS_LOG_COMPONENT_DEFINE ("SatFwdLinkSchedulerBase");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SatFwdLinkSchedulerBase);

TypeId
SatFwdLinkSchedulerBase::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SatFwdLinkSchedulerBase")
    .SetParent<SatFwdLinkScheduler> ()
    .AddConstructor<SatFwdLinkSchedulerBase> ()
    .AddAttribute ( "BBFrameContainer",
                    "BB frame container of this scheduler.",
                    PointerValue (),
                    MakePointerAccessor (&SatFwdLinkSchedulerBase::m_bbFrameContainer),
                    MakePointerChecker<SatBbFrameContainer> ())
  ;
  return tid;
}

SatFwdLinkSchedulerBase::SatFwdLinkSchedulerBase () : SatFwdLinkScheduler ()
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("Default constructor for SatFwdLinkScheduler not supported");
}

SatFwdLinkSchedulerBase::SatFwdLinkSchedulerBase (Ptr<SatBbFrameConf> conf, Mac48Address address, double carrierBandwidthInHz) :
    SatFwdLinkScheduler (conf, address, carrierBandwidthInHz)
{
  NS_LOG_FUNCTION (this);

  std::vector<SatEnums::SatModcod_t> modCods;
  SatEnums::GetAvailableModcodsFwdLink (modCods);
  m_bbFrameContainer = CreateObject<SatBbFrameContainer> (modCods, m_bbFrameConf);

  Simulator::Schedule (m_periodicInterval, &SatFwdLinkSchedulerBase::PeriodicTimerExpired, this);
}

SatFwdLinkSchedulerBase::~SatFwdLinkSchedulerBase ()
{
  NS_LOG_FUNCTION (this);
}

void
SatFwdLinkSchedulerBase::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  SatFwdLinkScheduler::DoDispose ();
  m_bbFrameContainer = NULL;
}


Ptr<SatBbFrame>
SatFwdLinkSchedulerBase::GetNextFrame ()
{
  NS_LOG_FUNCTION (this);

  if ( m_bbFrameContainer->GetTotalDuration () < m_schedulingStartThresholdTime )
    {
      ScheduleBbFrames ();
    }

  Ptr<SatBbFrame> frame = m_bbFrameContainer->GetNextFrame ();

  // create dummy frame
  if ( frame == NULL )
    {
      frame = Create<SatBbFrame> (m_bbFrameConf->GetDefaultModCod (), SatEnums::DUMMY_FRAME, m_bbFrameConf);

      // create dummy packet
      Ptr<Packet> dummyPacket = Create<Packet> (1);

      // Add MAC tag
      SatMacTag tag;
      tag.SetDestAddress (Mac48Address::GetBroadcast ());
      tag.SetSourceAddress (m_macAddress);
      dummyPacket->AddPacketTag (tag);

      // Add dummy packet to dummy frame
      frame->AddPayload (dummyPacket);
    }

  return frame;
}

void
SatFwdLinkSchedulerBase::PeriodicTimerExpired ()
{
  NS_LOG_FUNCTION (this);

  ScheduleBbFrames ();

  Simulator::Schedule (m_periodicInterval, &SatFwdLinkSchedulerBase::PeriodicTimerExpired, this);
}

void
SatFwdLinkSchedulerBase::ScheduleBbFrames ()
{
  NS_LOG_FUNCTION (this);

  // Get scheduling objects from LLC
  std::vector< Ptr<SatSchedulingObject> > so;
  GetSchedulingObjects (so);

  for ( std::vector< Ptr<SatSchedulingObject> >::const_iterator it = so.begin ();
        ( it != so.end () ) && ( m_bbFrameContainer->GetTotalDuration () < m_schedulingStopThresholdTime ); it++ )
    {
      uint32_t currentObBytes = (*it)->GetBufferedBytes ();
      uint32_t currentObMinReqBytes = (*it)->GetMinTxOpportunityInBytes ();
      uint8_t flowId = (*it)->GetFlowId ();
      SatEnums::SatModcod_t modcod = m_bbFrameContainer->GetModcod ( flowId, GetSchedulingObjectCno (*it));

      uint32_t frameBytes = m_bbFrameContainer->GetBytesLeftInTailFrame (flowId, modcod);

      while ( ( (m_bbFrameContainer->GetTotalDuration () < m_schedulingStopThresholdTime ))
              && (currentObBytes > 0) )
        {
          if ( frameBytes < currentObMinReqBytes)
            {
              frameBytes = m_bbFrameContainer->GetMaxFramePayloadInBytes (flowId, modcod);

              // if frame bytes still too small, we must have too long control message, so let's crash
              if ( frameBytes < currentObMinReqBytes )
                {
                  NS_FATAL_ERROR ("Control package too probably too long!!!");
                }
            }

          Ptr<Packet> p = m_txOpportunityCallback (frameBytes, (*it)->GetMacAddress (), flowId, currentObBytes, currentObMinReqBytes);

          if ( p )
            {
              m_bbFrameContainer->AddData (flowId, modcod, p);
              frameBytes = m_bbFrameContainer->GetBytesLeftInTailFrame (flowId, modcod);
            }
          else if ( m_bbFrameContainer->GetMaxFramePayloadInBytes (flowId, modcod ) != m_bbFrameContainer->GetBytesLeftInTailFrame (flowId, modcod))
            {
              frameBytes = m_bbFrameContainer->GetMaxFramePayloadInBytes (flowId, modcod);
            }
          else
            {
              NS_FATAL_ERROR ("Packet does not fit in empty BB Frame. Control package too long or fragmentation problem in user package!!!");
            }
        }

      m_bbFrameContainer->MergeBbFrames (m_carrierBandwidthInHz);
    }
}

void
SatFwdLinkSchedulerBase::GetSchedulingObjects (std::vector< Ptr<SatSchedulingObject> > & output)
{
  NS_LOG_FUNCTION (this);

  if ( m_bbFrameContainer->GetTotalDuration () < m_schedulingStopThresholdTime )
    {
      // Get scheduling objects from LLC
      m_schedContextCallback (output);

      SortSchedulingObjects (output);
    }
}

} // namespace ns3
