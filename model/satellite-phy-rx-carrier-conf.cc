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

#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "satellite-utils.h"
#include "satellite-phy-rx-carrier-conf.h"

NS_LOG_COMPONENT_DEFINE ("SatPhyRxCarrierConf");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SatPhyRxCarrierConf);

SatPhyRxCarrierConf::SatPhyRxCarrierConf ()
: m_ifModel (),
  m_errorModel (),
  m_linkResults (),
  m_rxTemperatureK (),
  m_rxExtNoiseDensityDbwhz (),
  m_rxAciIfWrtNoise (),
  m_rxMode (),
  m_enableIntfOutputTrace (false),
  m_carrierCount (),
  m_carrierBandwidthConverter (),
  m_channelType (),
  m_sinrCalculate (),
  m_constantErrorRate ()
{
  NS_FATAL_ERROR ("SatPhyRxCarrierConf::SatPhyRxCarrierConf - Constructor not in use");
}

SatPhyRxCarrierConf::SatPhyRxCarrierConf ( double rxTemperatureDbk, ErrorModel errorModel, InterferenceModel ifModel,
                                           RxMode rxMode, SatEnums::ChannelType_t chType,
                                           CarrierBandwidthConverter converter, uint32_t carrierCount)
 : m_ifModel (ifModel),
   m_errorModel (errorModel),
   m_linkResults (),
   m_rxTemperatureK (SatUtils::DbToLinear (rxTemperatureDbk)),
   m_rxExtNoiseDensityDbwhz (0),
   m_rxAciIfWrtNoise (0),
   m_rxMode (rxMode),
   m_enableIntfOutputTrace (false),
   m_carrierCount (carrierCount),
   m_carrierBandwidthConverter (converter),
   m_channelType (chType),
   m_sinrCalculate (),
   m_constantErrorRate (0.0)
{

}

TypeId
SatPhyRxCarrierConf::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SatPhyRxCarrierConf")
    .SetParent<Object> ()
    .AddAttribute( "ExtNoiseDensityDbwhz",
                   "External noise power density.",
                    DoubleValue (SatUtils::MinDb<double> ()),
                    MakeDoubleAccessor (&SatPhyRxCarrierConf::m_rxExtNoiseDensityDbwhz),
                    MakeDoubleChecker<double> ())
	  .AddAttribute( "RxAciIfWrtNoise",
                   "Adjacent channel interference wrt noise in percents.",
                    DoubleValue (0.0),
                    MakeDoubleAccessor (&SatPhyRxCarrierConf::m_rxAciIfWrtNoise),
                    MakeDoubleChecker<double> ())                    
    .AddAttribute( "EnableIntfOutputTrace",
                   "Enable interference output trace.",
                    BooleanValue (false),
                    MakeBooleanAccessor (&SatPhyRxCarrierConf::m_enableIntfOutputTrace),
                    MakeBooleanChecker ())
    .AddAttribute( "ConstantErrorRatio",
                   "Constant error ratio",
                   DoubleValue (0.01),
                   MakeDoubleAccessor (&SatPhyRxCarrierConf::m_constantErrorRate),
                   MakeDoubleChecker<double> ())
    .AddConstructor<SatPhyRxCarrierConf> ()
  ;
  return tid;
}

void
SatPhyRxCarrierConf::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_linkResults = NULL;
  m_carrierBandwidthConverter.Nullify();
  m_sinrCalculate.Nullify();

  Object::DoDispose ();
}

void
SatPhyRxCarrierConf::SetLinkResults (Ptr<SatLinkResults> linkResults)
{
  m_linkResults = linkResults;
}

uint32_t
SatPhyRxCarrierConf::GetCarrierCount () const
{
  return m_carrierCount;
}


SatPhyRxCarrierConf::ErrorModel
SatPhyRxCarrierConf::GetErrorModel () const
{
  return m_errorModel;
}

SatPhyRxCarrierConf::InterferenceModel
SatPhyRxCarrierConf::GetInterferenceModel () const
{
  return m_ifModel;
}

Ptr<SatLinkResults>
SatPhyRxCarrierConf::GetLinkResults () const
{
  return m_linkResults;
}

double
SatPhyRxCarrierConf::GetCarrierBandwidthHz ( uint32_t carrierId, SatEnums::CarrierBandwidthType_t bandwidhtType ) const
{
  return m_carrierBandwidthConverter( m_channelType, carrierId, bandwidhtType );
}

double
SatPhyRxCarrierConf::GetRxTemperatureK () const
{
  return m_rxTemperatureK;
}

double
SatPhyRxCarrierConf::GetExtPowerDensityDbwhz () const
{
  return m_rxExtNoiseDensityDbwhz;
}

double
SatPhyRxCarrierConf::GetRxAciInterferenceWrtNoise () const
{
  return m_rxAciIfWrtNoise;
}

SatPhyRxCarrierConf::RxMode
SatPhyRxCarrierConf::GetRxMode () const
{
  return m_rxMode;
}

SatEnums::ChannelType_t
SatPhyRxCarrierConf::GetChannelType () const
{
  return m_channelType;
}

bool
SatPhyRxCarrierConf::IsIntfOutputTraceEnabled () const
{
  return m_enableIntfOutputTrace;
}

double
SatPhyRxCarrierConf::GetConstantErrorRate () const
{
  return m_constantErrorRate;
}

} // namespace ns3
