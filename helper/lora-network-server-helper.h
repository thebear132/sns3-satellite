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

#ifndef LORA_NETWORK_SERVER_HELPER_H
#define LORA_NETWORK_SERVER_HELPER_H

#include "ns3/address.h"
#include "ns3/application-container.h"
#include "ns3/attribute.h"
#include "ns3/lora-network-server.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/point-to-point-helper.h"

#include <stdint.h>
#include <string>

namespace ns3
{

/**
 * This class can install Network Server applications on multiple nodes at once.
 */
class LoraNetworkServerHelper : public Object
{
  public:
    /**
     * \brief Get the type ID
     * \return the object TypeId
     */
    static TypeId GetTypeId(void);

    LoraNetworkServerHelper();

    ~LoraNetworkServerHelper();

    void SetAttribute(std::string name, const AttributeValue& value);

    ApplicationContainer Install(NodeContainer c);

    ApplicationContainer Install(Ptr<Node> node);

    /**
     * Enable (true) or disable (false) the ADR component in the Network
     * Server created by this helper.
     */
    void EnableAdr(bool enableAdr);

    /**
     * Set the ADR implementation to use in the Network Server created
     * by this helper.
     */
    void SetAdr(std::string type);

  private:
    void InstallComponents(Ptr<LoraNetworkServer> netServer);
    Ptr<Application> InstallPriv(Ptr<Node> node);

    ObjectFactory m_factory;

    PointToPointHelper p2pHelper; //!< Helper to create PointToPoint links

    bool m_adrEnabled;

    ObjectFactory m_adrSupportFactory;
};

} // namespace ns3

#endif /* LORA_NETWORK_SERVER_HELPER_H */
