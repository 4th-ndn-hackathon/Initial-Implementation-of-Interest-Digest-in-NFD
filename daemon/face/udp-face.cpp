/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "udp-face.hpp"

namespace nfd {

NFD_LOG_INCLASS_TEMPLATE_SPECIALIZATION_DEFINE(DatagramFace, UdpFace::protocol, "UdpFace");

UdpFace::UdpFace(const shared_ptr<UdpFace::protocol::socket>& socket, bool isOnDemand)
  : DatagramFace<protocol>(FaceUri(socket->remote_endpoint()),
                           FaceUri(socket->local_endpoint()),
                           socket, isOnDemand)
{
}

void
UdpFace::handleFirstReceive(const uint8_t* buffer,
                            std::size_t nBytesReceived,
                            const boost::system::error_code& error)
{
  NFD_LOG_TRACE("handleFirstReceive");

  // Checking if the received message size is too big.
  // This check is redundant, since in the actual implementation
  // a packet cannot be larger than MAX_NDN_PACKET_SIZE.
  if (!error && (nBytesReceived > MAX_NDN_PACKET_SIZE))
    {
      NFD_LOG_WARN("[id:" << this->getId()
                   << ",endpoint:" << m_socket->local_endpoint()
                   << "] Received message too big. Maximum size is "
                   << MAX_NDN_PACKET_SIZE );
      return;
    }

  receiveDatagram(buffer, nBytesReceived, error);
}

} // namespace nfd