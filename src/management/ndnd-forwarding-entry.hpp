/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2013 Regents of the University of California.
 * See COPYING for copyright and distribution information.
 */

#ifndef NDN_MANAGEMENT_NDND_FORWARDING_ENTRY_HPP
#define NDN_MANAGEMENT_NDND_FORWARDING_ENTRY_HPP

#include "../encoding/tlv-ndnd.hpp"
#include "../name.hpp"
#include "../encoding/block.hpp"

#include "ndnd-forwarding-flags.hpp"

namespace ndn {
namespace ndnd {

/**
 * An ForwardingEntry holds an action and  Name prefix and other fields for an forwarding entry.
 */
class ForwardingEntry {
public:
  ForwardingEntry(const std::string& action,
                  const Name& prefix,
                  int faceId = -1,
                  const ForwardingFlags& forwardingFlags = ForwardingFlags(),
                  time::milliseconds freshnessPeriod = time::milliseconds::min())
    : action_(action)
    , prefix_(prefix)
    , faceId_(faceId)
    , forwardingFlags_(forwardingFlags)
    , freshnessPeriod_(freshnessPeriod)
  {
  }

  ForwardingEntry()
  : faceId_(-1)
  , freshnessPeriod_(time::milliseconds::min())
  {
  }

  const std::string&
  getAction() const { return action_; }

  void
  setAction(const std::string& action) { action_ = action; wire_.reset(); }

  const Name&
  getPrefix() const { return prefix_; }

  void
  setPrefix(const Name &prefix) { prefix_ = prefix; wire_.reset(); }

  int
  getFaceId() const { return faceId_; }

  void
  setFaceId(int faceId) { faceId_ = faceId; wire_.reset(); }

  const ForwardingFlags&
  getForwardingFlags() const { return forwardingFlags_; }

  void
  setForwardingFlags(const ForwardingFlags& forwardingFlags) { forwardingFlags_ = forwardingFlags; wire_.reset(); }

  const time::milliseconds&
  getFreshnessPeriod() const { return freshnessPeriod_; }

  void
  setFreshnessPeriod(const time::milliseconds& freshnessPeriod) { freshnessPeriod_ = freshnessPeriod; wire_.reset(); }

  inline const Block&
  wireEncode() const;

  inline void
  wireDecode(const Block &wire);

private:
  std::string action_;   /**< empty for none. */
  Name prefix_;
  int faceId_;           /**< -1 for none. */
  ForwardingFlags forwardingFlags_;
  time::milliseconds freshnessPeriod_; /**< time::milliseconds::min() for none. */

  mutable Block wire_;
};

inline const Block&
ForwardingEntry::wireEncode() const
{
  if (wire_.hasWire())
    return wire_;

  // ForwardingEntry ::= FORWARDING-ENTRY TLV-LENGTH
  //                       Action?
  //                       Name?
  //                       FaceID?
  //                       ForwardingFlags?
  //                       FreshnessPeriod?

  wire_ = Block(tlv::ndnd::ForwardingEntry);

  // Action
  if (!action_.empty())
    {
      wire_.push_back
        (dataBlock(tlv::ndnd::Action, action_.c_str(), action_.size()));
    }

  // Name
  wire_.push_back
    (prefix_.wireEncode());

  // FaceID
  if (faceId_ >= 0)
    {
      wire_.push_back
        (nonNegativeIntegerBlock(tlv::ndnd::FaceID, faceId_));
    }

  // ForwardingFlags
  wire_.push_back
    (forwardingFlags_.wireEncode());

  // FreshnessPeriod
  if (freshnessPeriod_ >= time::milliseconds::zero())
    {
      wire_.push_back
        (nonNegativeIntegerBlock(Tlv::FreshnessPeriod, freshnessPeriod_.count()));
    }

  wire_.encode();
  return wire_;
}

inline void
ForwardingEntry::wireDecode(const Block &wire)
{
  action_.clear();
  prefix_.clear();
  faceId_ = -1;
  forwardingFlags_ = ForwardingFlags();
  freshnessPeriod_ = time::milliseconds::min();

  wire_ = wire;
  wire_.parse();

  // ForwardingEntry ::= FORWARDING-ENTRY TLV-LENGTH
  //                       Action?
  //                       Name?
  //                       FaceID?
  //                       ForwardingFlags?
  //                       FreshnessPeriod?

  // Action
  Block::element_const_iterator val = wire_.find(tlv::ndnd::Action);
  if (val != wire_.elements_end())
    {
      action_ = std::string(reinterpret_cast<const char*>(val->value()), val->value_size());
    }

  // Name
  val = wire_.find(Tlv::Name);
  if (val != wire_.elements_end())
    {
      prefix_.wireDecode(*val);
    }

  // FaceID
  val = wire_.find(tlv::ndnd::FaceID);
  if (val != wire_.elements_end())
    {
      faceId_ = readNonNegativeInteger(*val);
    }

  // ForwardingFlags
  val = wire_.find(tlv::ndnd::ForwardingFlags);
  if (val != wire_.elements_end())
    {
      forwardingFlags_.wireDecode(*val);
    }

  // FreshnessPeriod
  val = wire_.find(Tlv::FreshnessPeriod);
  if (val != wire_.elements_end())
    {
      freshnessPeriod_ = time::milliseconds(readNonNegativeInteger(*val));
    }
}

inline std::ostream&
operator << (std::ostream &os, const ForwardingEntry &entry)
{
  os << "ForwardingEntry(";

  // Action
  if (!entry.getAction().empty())
    {
      os << "Action:" << entry.getAction() << ", ";
    }

  // Name
  os << "Prefix:" << entry.getPrefix() << ", ";

  // FaceID
  if (entry.getFaceId() >= 0)
    {
      os << "FaceID:" << entry.getFaceId() << ", ";
    }

  // ForwardingFlags
  os << "ForwardingFlags:" << entry.getForwardingFlags() << ", ";

  // FreshnessPeriod
  if (entry.getFreshnessPeriod() >= time::milliseconds::zero())
    {
      os << "FreshnessPeriod:" << entry.getFreshnessPeriod() << ", ";
    }

  os << ")";
  return os;
}

} // namespace ndnd
} // namespace ndn

#endif // NDN_MANAGEMENT_NDND_FORWARDING_ENTRY_HPP