/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "measurements.hpp"
#include "name-tree.hpp"
#include "pit-entry.hpp"
#include "fib-entry.hpp"

namespace nfd {

const time::nanoseconds Measurements::s_defaultLifetime = time::seconds(4);

Measurements::Measurements(NameTree& nameTree)
  : m_nameTree(nameTree)
  , m_nItems(0)
{
}

Measurements::~Measurements()
{
}

static inline bool
predicate_NameTreeEntry_hasMeasurementsEntry(const name_tree::Entry& entry)
{
  return static_cast<bool>(entry.getMeasurementsEntry());
}

shared_ptr<measurements::Entry>
Measurements::get(shared_ptr<name_tree::Entry> nameTreeEntry)
{
  shared_ptr<measurements::Entry> entry = nameTreeEntry->getMeasurementsEntry();
  if (static_cast<bool>(entry))
    return entry;
  entry = make_shared<measurements::Entry>(nameTreeEntry->getPrefix());
  nameTreeEntry->setMeasurementsEntry(entry);
  m_nItems++;
  return entry;
}

shared_ptr<measurements::Entry>
Measurements::get(const Name& name)
{
  shared_ptr<name_tree::Entry> nameTreeEntry = m_nameTree.lookup(name);
  return get(nameTreeEntry);
}

shared_ptr<measurements::Entry>
Measurements::get(const fib::Entry& fibEntry)
{
  shared_ptr<name_tree::Entry> nameTreeEntry = m_nameTree.get(fibEntry);

  BOOST_ASSERT(static_cast<bool>(nameTreeEntry));

  return get(nameTreeEntry);
}

shared_ptr<measurements::Entry>
Measurements::get(const pit::Entry& pitEntry)
{
  shared_ptr<name_tree::Entry> nameTreeEntry = m_nameTree.get(pitEntry);

  BOOST_ASSERT(static_cast<bool>(nameTreeEntry));

  return get(nameTreeEntry);
}

shared_ptr<measurements::Entry>
Measurements::getParent(shared_ptr<measurements::Entry> child)
{
  BOOST_ASSERT(child);

  if (child->getName().size() == 0) {
    return shared_ptr<measurements::Entry>();
  }

  return this->get(child->getName().getPrefix(-1));
}

shared_ptr<measurements::Entry>
Measurements::findLongestPrefixMatch(const Name& name) const
{
  shared_ptr<name_tree::Entry> nameTreeEntry =
    m_nameTree.findLongestPrefixMatch(name, &predicate_NameTreeEntry_hasMeasurementsEntry);
  if (static_cast<bool>(nameTreeEntry)) {
    return nameTreeEntry->getMeasurementsEntry();
  }
  return shared_ptr<measurements::Entry>();
}

shared_ptr<measurements::Entry>
Measurements::findExactMatch(const Name& name) const
{
  shared_ptr<name_tree::Entry> nameTreeEntry = m_nameTree.lookup(name);
  if (static_cast<bool>(nameTreeEntry))
    return nameTreeEntry->getMeasurementsEntry();
  return shared_ptr<measurements::Entry>();
}

void
Measurements::extendLifetime(measurements::Entry& entry, const time::nanoseconds& lifetime)
{
  shared_ptr<measurements::Entry> ret = this->findExactMatch(entry.getName());
  if (static_cast<bool>(ret))
  {
    time::steady_clock::TimePoint expiry = time::steady_clock::now() + lifetime;
    if (ret->m_expiry >= expiry) // has longer lifetime, not extending
      return;
    scheduler::cancel(entry.m_cleanup);
    entry.m_expiry = expiry;
    entry.m_cleanup = scheduler::schedule(lifetime,
                         bind(&Measurements::cleanup, this, ret));
  }
}

void
Measurements::cleanup(shared_ptr<measurements::Entry> entry)
{
  BOOST_ASSERT(entry);

  shared_ptr<name_tree::Entry> nameTreeEntry = m_nameTree.findExactMatch(entry->getName());
  if (static_cast<bool>(nameTreeEntry))
  {
    nameTreeEntry->setMeasurementsEntry(shared_ptr<measurements::Entry>());
    m_nItems--;
  }

}

} // namespace nfd