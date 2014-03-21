/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 * BSD license, See the LICENSE file for more information
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef NDNSEC_SIGN_REQ_HPP
#define NDNSEC_SIGN_REQ_HPP

#include "ndnsec-util.hpp"

int 
ndnsec_sign_req(int argc, char** argv)	
{
  using namespace ndn;
  namespace po = boost::program_options;

  std::string name;
  bool isKeyName = false;

  po::options_description desc("General Usage\n  ndnsec sign-req [-h] [-k] name\nGeneral options");
  desc.add_options()
    ("help,h", "produce help message")
    ("key,k", "optional, if specified, name is keyName (e.g. /ndn/edu/ucla/alice/ksk-123456789), otherwise identity name")
    ("name,n", po::value<std::string>(&name), "name, for example, /ndn/edu/ucla/alice")
    ;

  po::positional_options_description p;
  p.add("name", 1);

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
    po::notify(vm);
  }
  catch(const std::exception &e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    std::cerr << desc << std::endl;
    return 1;
  }

  if (vm.count("help")) 
    {
      std::cerr << desc << std::endl;
      return 0;
    }

  if (0 == vm.count("name"))
    {
      std::cerr << "identity_name must be specified" << std::endl;
      std::cerr << desc << std::endl;
      return 1;
    }
  
  if (vm.count("key"))
    isKeyName = true;

  shared_ptr<IdentityCertificate> selfSignCert;

  try
    {
      KeyChain keyChain;
      
      if(isKeyName)
        {
          selfSignCert = keyChain.selfSign(name);
        }
      else
        {
          Name keyName = keyChain.getDefaultKeyNameForIdentity(name);
          selfSignCert = keyChain.selfSign(keyName);
        }
    }
  catch(SecPublicInfo::Error& e)
    {
      std::cerr << "ERROR: " << e.what() << std::endl;
      return 1;
    }
  catch(SecTpm::Error& e)
    {
      std::cerr << "ERROR: " << e.what() << std::endl;
      return 1;
    }

  try
    {
      using namespace CryptoPP;
      StringSource ss(selfSignCert->wireEncode().wire(), selfSignCert->wireEncode().size(), true,
                      new Base64Encoder(new FileSink(std::cout), true, 64));
    }
  catch(CryptoPP::Exception& e)
    {
      std::cerr << "ERROR: " << e.what() << std::endl;
      return 1;
    }
  
  return 0;
}

#endif //NDNSEC_SIGN_REQ_HPP