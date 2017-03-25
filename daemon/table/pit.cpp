/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
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
 */

#include "pit.hpp"
#include <ndn-cxx/lp/tags.hpp>
#include "core/logger.hpp"

namespace nfd {
namespace pit {

NFD_LOG_INIT("Pit");

Pit::Pit()
  : m_nItems(0)
{
}

shared_ptr<Entry>
Pit::find(const Interest& interest) const
{
  InterestDigest d = interest.computeDigest();
  auto it = m_table.find(d);
  return it == m_table.end() ? nullptr : it->second;
}

std::pair<shared_ptr<Entry>, bool>
Pit::insert(const Interest& interest)
{
  InterestDigest d = interest.computeDigest();
  std::map<InterestDigest, shared_ptr<Entry>>::iterator it, upper;
  std::tie(it, upper) = m_table.equal_range(d);

  if (it == upper) {
    it = m_table.emplace_hint(it, d, make_shared<Entry>(interest, d));
    BOOST_ASSERT(it->second->canMatch(interest));
    ++m_nItems;
    return {it->second, true};
  }
  else {
    return {it->second, false};
  }
}

DataMatchResult
Pit::findAllDataMatches(const Data& data) const
{
  DataMatchResult matches;

  auto tag = data.getTag<lp::InterestDigestTag>();
  if (tag == nullptr) {
    NFD_LOG_WARN("Data " << data.getName() << " has no InterestDigestTag");
    return matches;
  }

  InterestDigest d = *tag;
  auto it = m_table.find(d);
  if (it != m_table.end() && it->second->getInterest().matchesData(data)) {
    matches.emplace_back(it->second);
  }

  return matches;
}

void
Pit::erase(Entry* entry)
{
  BOOST_ASSERT(entry != nullptr);
  auto it = m_table.find(entry->m_digest);
  BOOST_ASSERT(it != m_table.end());
  m_table.erase(it);
  --m_nItems;
}

void
Pit::deleteInOutRecords(Entry* entry, const Face& face)
{
  BOOST_ASSERT(entry != nullptr);

  entry->deleteInRecord(face);
  entry->deleteOutRecord(face);

  /// \todo decide whether to delete PIT entry if there's no more in/out-record left
}

Pit::ForwardRange
Pit::getForwardRange() const
{
  return m_table | boost::adaptors::map_values | boost::adaptors::indirected;
}

Pit::const_iterator
Pit::begin() const
{
  return this->getForwardRange().begin();
}

Pit::const_iterator
Pit::end() const
{
  return this->getForwardRange().end();
}

} // namespace pit
} // namespace nfd
