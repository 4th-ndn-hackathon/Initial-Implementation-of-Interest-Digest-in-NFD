/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TEST_FACE_DUMMY_FACE_HPP
#define NFD_TEST_FACE_DUMMY_FACE_HPP

#include "face/face.hpp"

namespace nfd {

/** \class DummyFace
 *  \brief provides a Face that cannot communicate
 *  for unit testing only
 */
class DummyFace : public Face
{
public:
  explicit
  DummyFace(bool isLocal = false)
    : m_isLocal(isLocal)
  {
  }
  
  virtual void
  sendInterest(const Interest& interest)
  {
  }
  
  virtual void
  sendData(const Data& data)
  {
  }

  virtual void
  close()
  {
  }

  virtual bool
  isLocal() const
  {
    return m_isLocal;
  }

private:
  bool m_isLocal;
};

} // namespace nfd

#endif // TEST_FACE_DUMMY_FACE_HPP