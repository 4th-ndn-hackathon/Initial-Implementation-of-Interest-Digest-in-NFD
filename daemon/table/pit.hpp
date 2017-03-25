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

#ifndef NFD_DAEMON_TABLE_PIT_HPP
#define NFD_DAEMON_TABLE_PIT_HPP

#include "pit-entry.hpp"
#include <boost/range/adaptor/indirected.hpp>
#include <boost/range/adaptor/map.hpp>

namespace nfd {
namespace pit {

/** \brief represents the Interest Table
 */
class Pit : noncopyable
{
public:
  Pit();

  /** \return number of entries
   */
  size_t
  size() const
  {
    return m_nItems;
  }

  /** \brief finds a PIT entry for Interest
   *  \param interest the Interest
   *  \return an existing entry with same Name and Selectors; otherwise nullptr
   */
  shared_ptr<Entry>
  find(const Interest& interest) const;

  /** \brief inserts a PIT entry for Interest
   *  \param interest the Interest; must be created with make_shared
   *  \return a new or existing entry with same Name and Selectors,
   *          and true for new entry, false for existing entry
   */
  std::pair<shared_ptr<Entry>, bool>
  insert(const Interest& interest);

  /** \brief performs a Data match
   *  \return an iterable of all PIT entries matching data
   */
  shared_ptr<Entry>
  findDataMatch(const Data& data) const;

  /** \brief deletes an entry
   */
  void
  erase(Entry* entry);

  /** \brief deletes in-record and out-record for face
   */
  void
  deleteInOutRecords(Entry* entry, const Face& face);

public: // enumeration
  using Table = std::map<InterestDigest, shared_ptr<Entry>>;
  using ForwardRange = boost::indirected_range<const boost::select_second_const_range<Table>>;
  using const_iterator = boost::range_iterator<ForwardRange>::type;

  /** \return an iterator to the beginning
   *  \note Iteration order is implementation-defined.
   *  \warning Undefined behavior may occur if a FIB/PIT/Measurements/StrategyChoice entry
   *           is inserted or erased during enumeration.
   */
  const_iterator
  begin() const;

  /** \return an iterator to the end
   *  \sa begin()
   */
  const_iterator
  end() const;

private:
  ForwardRange
  getForwardRange() const;

private:
  Table m_table;
  size_t m_nItems;
};

} // namespace pit

using pit::Pit;

} // namespace nfd

#endif // NFD_DAEMON_TABLE_PIT_HPP
