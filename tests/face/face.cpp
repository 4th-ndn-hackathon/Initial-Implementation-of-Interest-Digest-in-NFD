/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "face/face.hpp"
#include "dummy-face.hpp"

#include <boost/test/unit_test.hpp>

namespace nfd {

BOOST_AUTO_TEST_SUITE(FaceFace)

BOOST_AUTO_TEST_CASE(Description)
{
  DummyFace face;
  face.setDescription("3pFsKrvWr");
  BOOST_CHECK_EQUAL(face.getDescription(), "3pFsKrvWr");
}

BOOST_AUTO_TEST_CASE(LocalControlHeaderEnabled)
{
  DummyFace face;
  
  BOOST_CHECK_EQUAL(face.isLocalControlHeaderEnabled(), false);
  
  face.setLocalControlHeaderFeature(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID, true);
  BOOST_CHECK_EQUAL(face.isLocalControlHeaderEnabled(), true);
  BOOST_CHECK_EQUAL(face.isLocalControlHeaderEnabled(
                         LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID), true);
  BOOST_CHECK_EQUAL(face.isLocalControlHeaderEnabled(
                         LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID), false);
  
  face.setLocalControlHeaderFeature(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID, false);
  BOOST_CHECK_EQUAL(face.isLocalControlHeaderEnabled(), false);
  BOOST_CHECK_EQUAL(face.isLocalControlHeaderEnabled(
                         LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID), false);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace nfd