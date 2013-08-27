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

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/names.h"
#include "ns3/uinteger.h"
#include "ns3/enum.h"
#include "ns3/double.h"
#include "../model/satellite-geo-net-device.h"
#include "../model/satellite-phy.h"
#include "../model/satellite-phy-tx.h"
#include "../model/satellite-phy-rx.h"
#include "../model/satellite-phy-rx-carrier-conf.h"

#include "satellite-geo-helper.h"

NS_LOG_COMPONENT_DEFINE ("SatGeoHelper");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SatGeoHelper);

TypeId
SatGeoHelper::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::SatGeoHelper")
      .SetParent<Object> ()
      .AddConstructor<SatGeoHelper> ()
      .AddAttribute ("FwdLinkInterferenceModel",
                     "Forward link interference model",
                     EnumValue (SatPhyRxCarrierConf::IF_CONSTANT),
                     MakeEnumAccessor (&SatGeoHelper::m_fwdLinkInterferenceModel),
                     MakeEnumChecker (SatPhyRxCarrierConf::IF_CONSTANT, "Constant",
                                      SatPhyRxCarrierConf::IF_PER_PACKET, "PerPacket"))
      .AddAttribute ("RtnLinkInterferenceModel",
                     "Return link interference model",
                     EnumValue (SatPhyRxCarrierConf::IF_PER_PACKET),
                     MakeEnumAccessor (&SatGeoHelper::m_rtnLinkInterferenceModel),
                     MakeEnumChecker (SatPhyRxCarrierConf::IF_CONSTANT, "Constant",
                                      SatPhyRxCarrierConf::IF_PER_PACKET, "PerPacket"))
      .AddAttribute( "FwdLinkRxTemperature",
                     "The forward link RX noise temperature in Geo satellite.",
                     DoubleValue(482.87),
                     MakeDoubleAccessor(&SatGeoHelper::m_fwdLinkRxTemperature_K),
                     MakeDoubleChecker<double>())
      .AddAttribute( "RtnLinkRxTemperature",
                     "The return link RX noise temperature in Geo satellite.",
                     DoubleValue(490.94),
                     MakeDoubleAccessor(&SatGeoHelper::m_rtnLinkRxTemperature_K),
                     MakeDoubleChecker<double>())
      .AddTraceSource ("Creation", "Creation traces",
                       MakeTraceSourceAccessor (&SatGeoHelper::m_creation))
    ;
    return tid;
}

TypeId
SatGeoHelper::GetInstanceTypeId (void) const
{
  return GetTypeId();
}

SatGeoHelper::SatGeoHelper ()
  :m_deviceCount(0)
{
  m_deviceFactory.SetTypeId ("ns3::SatGeoNetDevice");
}

void 
SatGeoHelper::SetDeviceAttribute (std::string n1, const AttributeValue &v1)
{
  m_deviceFactory.Set (n1, v1);
}

NetDeviceContainer 
SatGeoHelper::Install (NodeContainer c)
{
  // currently only one node supported by helper
  NS_ASSERT (c.GetN () == 1);

  NetDeviceContainer devs;

  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); i++)
  {
    devs.Add(Install(*i));
  }

  return devs;
}

Ptr<NetDevice>
SatGeoHelper::Install (Ptr<Node> n)
{
  NS_ASSERT (m_deviceCount == 0);

  // Create SatGeoNetDevice
  Ptr<SatGeoNetDevice> satDev = m_deviceFactory.Create<SatGeoNetDevice> ();

  satDev->SetAddress (Mac48Address::Allocate ());
  n->AddDevice(satDev);
  m_deviceCount++;

  return satDev;
}

Ptr<NetDevice>
SatGeoHelper::Install (std::string nName)
{
  Ptr<Node> n = Names::Find<Node> (nName);
  return Install (n);
}

void
SatGeoHelper::AttachChannels (Ptr<NetDevice> d, Ptr<SatChannel> ff, Ptr<SatChannel> fr, Ptr<SatChannel> uf, Ptr<SatChannel> ur, uint32_t beamId )
{
  NS_LOG_FUNCTION (this << ff << fr << uf << ur);

  Ptr<SatGeoNetDevice> dev = DynamicCast<SatGeoNetDevice> (d);
  Ptr<MobilityModel> mobility = dev->GetNode()->GetObject<MobilityModel>();

  // Create the first needed SatPhyTx and SatPhyRx modules
  Ptr<SatPhyTx> uPhyTx = CreateObject<SatPhyTx> ();
  Ptr<SatPhyRx> uPhyRx = CreateObject<SatPhyRx> ();
  Ptr<SatPhyTx> fPhyTx = CreateObject<SatPhyTx> ();
  Ptr<SatPhyRx> fPhyRx = CreateObject<SatPhyRx> ();

  // Set SatChannels to SatPhyTx/SatPhyRx
  uPhyTx->SetChannel (uf);
  uPhyRx->SetChannel (ur);
  uPhyRx->SetDevice (dev);
  uPhyTx->SetMobility(mobility);
  uPhyRx->SetMobility(mobility);


  // Configure the SatPhyRxCarrier instances
  // \todo We should pass the whole carrier configuration to the SatPhyRxCarrier,
  // instead of just the number of carriers, since it should hold information about
  // the number of carriers, carrier center frequencies and carrier bandwidths, etc.
  // Note, that in GEO satellite, there is no need for error modeling.
  uint32_t rtnLinkNumCarriers = 1;
  double rtnLinkRxBandwidth = 5e-6;

  Ptr<SatPhyRxCarrierConf> rtnCarrierConf =
        CreateObject<SatPhyRxCarrierConf> (rtnLinkNumCarriers,
                                           m_rtnLinkRxTemperature_K,
                                           rtnLinkRxBandwidth,
                                           SatPhyRxCarrierConf::EM_NONE,
                                           m_rtnLinkInterferenceModel);

  uPhyRx->ConfigurePhyRxCarriers (rtnCarrierConf);

  fPhyTx->SetChannel (fr);
  fPhyRx->SetChannel (ff);
  fPhyRx->SetDevice (dev);
  fPhyTx->SetMobility(mobility);
  fPhyRx->SetMobility(mobility);


  // Configure the SatPhyRxCarrier instances
  // \todo We should pass the whole carrier configuration to the SatPhyRxCarrier,
  // instead of just the number of carriers, since it should hold information about
  // the number of carriers, carrier center frequencies and carrier bandwidths, etc.
  // Note, that in GEO satellite, there is no need for error modeling.
  uint32_t fwdLinkNumCarriers = 1;
  double fwdLinkRxBandwidth = 5e-6;

  Ptr<SatPhyRxCarrierConf> fwdCarrierConf =
        CreateObject<SatPhyRxCarrierConf> (fwdLinkNumCarriers,
                                           m_fwdLinkRxTemperature_K,
                                           fwdLinkRxBandwidth,
                                           SatPhyRxCarrierConf::EM_NONE,
                                           m_fwdLinkInterferenceModel);

  fPhyRx->ConfigurePhyRxCarriers (fwdCarrierConf);

  SatPhy::ReceiveCallback uCb = MakeCallback (&SatGeoNetDevice::ReceiveUser, dev);
  SatPhy::ReceiveCallback fCb = MakeCallback (&SatGeoNetDevice::ReceiveFeeder, dev);

  // Create SatPhy modules
  Ptr<SatPhy> uPhy = CreateObject<SatPhy> (uPhyTx, uPhyRx, beamId, uCb);
  Ptr<SatPhy> fPhy = CreateObject<SatPhy> (fPhyTx, fPhyRx, beamId, fCb);

  dev->AddUserPhy(uPhy, beamId);
  dev->AddFeederPhy(fPhy, beamId);
}

void
SatGeoHelper::EnableCreationTraces(Ptr<OutputStreamWrapper> stream, CallbackBase &cb)
{
  TraceConnect("Creation", "SatGeoHelper",  cb);
}

} // namespace ns3
