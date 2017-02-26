/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2013-2017 Regents of the University of California.
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 */

#include "face-event-notification.hpp"
#include "encoding/block-helpers.hpp"
#include "encoding/encoding-buffer.hpp"
#include "encoding/tlv-nfd.hpp"
#include "util/concepts.hpp"

#include <iomanip>

namespace ndn {
namespace nfd {

BOOST_CONCEPT_ASSERT((NotificationStreamItem<FaceEventNotification>));

FaceEventNotification::FaceEventNotification()
  : m_kind(FACE_EVENT_NONE)
{
}

FaceEventNotification::FaceEventNotification(const Block& block)
{
  this->wireDecode(block);
}

template<encoding::Tag TAG>
size_t
FaceEventNotification::wireEncode(EncodingImpl<TAG>& encoder) const
{
  size_t totalLength = 0;

  totalLength += prependNonNegativeIntegerBlock(encoder,
                 tlv::nfd::Flags, m_flags);
  totalLength += prependNonNegativeIntegerBlock(encoder,
                 tlv::nfd::LinkType, m_linkType);
  totalLength += prependNonNegativeIntegerBlock(encoder,
                 tlv::nfd::FacePersistency, m_facePersistency);
  totalLength += prependNonNegativeIntegerBlock(encoder,
                 tlv::nfd::FaceScope, m_faceScope);
  totalLength += encoder.prependByteArrayBlock(tlv::nfd::LocalUri,
                 reinterpret_cast<const uint8_t*>(m_localUri.data()), m_localUri.size());
  totalLength += encoder.prependByteArrayBlock(tlv::nfd::Uri,
                 reinterpret_cast<const uint8_t*>(m_remoteUri.data()), m_remoteUri.size());
  totalLength += prependNonNegativeIntegerBlock(encoder,
                 tlv::nfd::FaceId, m_faceId);
  totalLength += prependNonNegativeIntegerBlock(encoder,
                 tlv::nfd::FaceEventKind, m_kind);

  totalLength += encoder.prependVarNumber(totalLength);
  totalLength += encoder.prependVarNumber(tlv::nfd::FaceEventNotification);
  return totalLength;
}

template size_t
FaceEventNotification::wireEncode<encoding::EncoderTag>(EncodingImpl<encoding::EncoderTag>& block) const;

template size_t
FaceEventNotification::wireEncode<encoding::EstimatorTag>(EncodingImpl<encoding::EstimatorTag>& block) const;

const Block&
FaceEventNotification::wireEncode() const
{
  if (m_wire.hasWire())
    return m_wire;

  EncodingEstimator estimator;
  size_t estimatedSize = wireEncode(estimator);

  EncodingBuffer buffer(estimatedSize, 0);
  wireEncode(buffer);

  m_wire = buffer.block();
  return m_wire;
}

void
FaceEventNotification::wireDecode(const Block& block)
{
  if (block.type() != tlv::nfd::FaceEventNotification) {
    BOOST_THROW_EXCEPTION(Error("expecting FaceEventNotification block"));
  }
  m_wire = block;
  m_wire.parse();
  Block::element_const_iterator val = m_wire.elements_begin();

  if (val != m_wire.elements_end() && val->type() == tlv::nfd::FaceEventKind) {
    m_kind = static_cast<FaceEventKind>(readNonNegativeInteger(*val));
    ++val;
  }
  else {
    BOOST_THROW_EXCEPTION(Error("missing required FaceEventKind field"));
  }

  if (val != m_wire.elements_end() && val->type() == tlv::nfd::FaceId) {
    m_faceId = readNonNegativeInteger(*val);
    ++val;
  }
  else {
    BOOST_THROW_EXCEPTION(Error("missing required FaceId field"));
  }

  if (val != m_wire.elements_end() && val->type() == tlv::nfd::Uri) {
    m_remoteUri.assign(reinterpret_cast<const char*>(val->value()), val->value_size());
    ++val;
  }
  else {
    BOOST_THROW_EXCEPTION(Error("missing required Uri field"));
  }

  if (val != m_wire.elements_end() && val->type() == tlv::nfd::LocalUri) {
    m_localUri.assign(reinterpret_cast<const char*>(val->value()), val->value_size());
    ++val;
  }
  else {
    BOOST_THROW_EXCEPTION(Error("missing required LocalUri field"));
  }

  if (val != m_wire.elements_end() && val->type() == tlv::nfd::FaceScope) {
    m_faceScope = static_cast<FaceScope>(readNonNegativeInteger(*val));
    ++val;
  }
  else {
    BOOST_THROW_EXCEPTION(Error("missing required FaceScope field"));
  }

  if (val != m_wire.elements_end() && val->type() == tlv::nfd::FacePersistency) {
    m_facePersistency = static_cast<FacePersistency>(readNonNegativeInteger(*val));
    ++val;
  }
  else {
    BOOST_THROW_EXCEPTION(Error("missing required FacePersistency field"));
  }

  if (val != m_wire.elements_end() && val->type() == tlv::nfd::LinkType) {
    m_linkType = static_cast<LinkType>(readNonNegativeInteger(*val));
    ++val;
  }
  else {
    BOOST_THROW_EXCEPTION(Error("missing required LinkType field"));
  }

  if (val != m_wire.elements_end() && val->type() == tlv::nfd::Flags) {
    m_flags = readNonNegativeInteger(*val);
    ++val;
  }
  else {
    BOOST_THROW_EXCEPTION(Error("missing required Flags field"));
  }
}

FaceEventNotification&
FaceEventNotification::setKind(FaceEventKind kind)
{
  m_wire.reset();
  m_kind = kind;
  return *this;
}

bool
operator==(const FaceEventNotification& a, const FaceEventNotification& b)
{
  return a.getFaceId() == b.getFaceId() &&
      a.getRemoteUri() == b.getRemoteUri() &&
      a.getLocalUri() == b.getLocalUri() &&
      a.getFaceScope() == b.getFaceScope() &&
      a.getFacePersistency() == b.getFacePersistency() &&
      a.getLinkType() == b.getLinkType() &&
      a.getFlags() == b.getFlags() &&
      a.getKind() == b.getKind();
}

std::ostream&
operator<<(std::ostream& os, const FaceEventNotification& notification)
{
  os << "FaceEvent(Kind: " << notification.getKind() << ",\n"
     << "          FaceId: " << notification.getFaceId() << ",\n"
     << "          RemoteUri: " << notification.getRemoteUri() << ",\n"
     << "          LocalUri: " << notification.getLocalUri() << ",\n"
     << "          FaceScope: " << notification.getFaceScope() << ",\n"
     << "          FacePersistency: " << notification.getFacePersistency() << ",\n"
     << "          LinkType: " << notification.getLinkType() << ",\n";

  auto osFlags = os.flags();
  // std::showbase doesn't work with number 0
  os << "          Flags: 0x" << std::noshowbase << std::noshowpos << std::nouppercase
     << std::hex << notification.getFlags() << "\n";
  os.flags(osFlags);

  return os << "          )";
}

} // namespace nfd
} // namespace ndn