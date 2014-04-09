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

#include "face-manager.hpp"
#include "face-flags.hpp"

#include "core/logger.hpp"
#include "core/face-uri.hpp"
#include "core/network-interface.hpp"
#include "fw/face-table.hpp"
#include "face/tcp-factory.hpp"
#include "face/udp-factory.hpp"

#include <ndn-cpp-dev/management/nfd-face-event-notification.hpp>

#ifdef HAVE_UNIX_SOCKETS
#include "face/unix-stream-factory.hpp"
#endif // HAVE_UNIX_SOCKETS

#ifdef HAVE_PCAP
#include "face/ethernet-factory.hpp"
#endif // HAVE_PCAP

namespace nfd {

NFD_LOG_INIT("FaceManager");

const Name FaceManager::COMMAND_PREFIX("/localhost/nfd/faces");

const size_t FaceManager::COMMAND_UNSIGNED_NCOMPS =
  FaceManager::COMMAND_PREFIX.size() +
  1 + // verb
  1;  // verb parameters

const size_t FaceManager::COMMAND_SIGNED_NCOMPS =
  FaceManager::COMMAND_UNSIGNED_NCOMPS +
  4; // (timestamp, nonce, signed info tlv, signature tlv)

const FaceManager::SignedVerbAndProcessor FaceManager::SIGNED_COMMAND_VERBS[] =
  {
    SignedVerbAndProcessor(
                           Name::Component("create"),
                           &FaceManager::createFace
                           ),

    SignedVerbAndProcessor(
                           Name::Component("destroy"),
                           &FaceManager::destroyFace
                           ),

    SignedVerbAndProcessor(
                           Name::Component("enable-local-control"),
                           &FaceManager::enableLocalControl
                           ),

    SignedVerbAndProcessor(
                           Name::Component("disable-local-control"),
                           &FaceManager::disableLocalControl
                           ),
  };

const FaceManager::UnsignedVerbAndProcessor FaceManager::UNSIGNED_COMMAND_VERBS[] =
  {
    UnsignedVerbAndProcessor(
                             Name::Component("list"),
                             &FaceManager::listFaces
                             ),

    UnsignedVerbAndProcessor(
                             Name::Component("events"),
                             &FaceManager::ignoreUnsignedVerb
                             ),
  };

const Name FaceManager::LIST_COMMAND_PREFIX("/localhost/nfd/faces/list");
const size_t FaceManager::LIST_COMMAND_NCOMPS = LIST_COMMAND_PREFIX.size();

const Name FaceManager::EVENTS_COMMAND_PREFIX("/localhost/nfd/faces/events");

FaceManager::FaceManager(FaceTable& faceTable,
                         shared_ptr<InternalFace> face)
  : ManagerBase(face, FACE_MANAGER_PRIVILEGE)
  , m_faceTable(faceTable)
  , m_statusPublisher(m_faceTable, m_face, LIST_COMMAND_PREFIX)
  , m_notificationStream(m_face, EVENTS_COMMAND_PREFIX)
  , m_signedVerbDispatch(SIGNED_COMMAND_VERBS,
                         SIGNED_COMMAND_VERBS +
                         (sizeof(SIGNED_COMMAND_VERBS) / sizeof(SignedVerbAndProcessor)))
  , m_unsignedVerbDispatch(UNSIGNED_COMMAND_VERBS,
                           UNSIGNED_COMMAND_VERBS +
                           (sizeof(UNSIGNED_COMMAND_VERBS) / sizeof(UnsignedVerbAndProcessor)))

{
  face->setInterestFilter("/localhost/nfd/faces",
                          bind(&FaceManager::onFaceRequest, this, _2));

  m_faceTable.onAdd    += bind(&FaceManager::onAddFace, this, _1);
  m_faceTable.onRemove += bind(&FaceManager::onRemoveFace, this, _1);
}

FaceManager::~FaceManager()
{

}

void
FaceManager::setConfigFile(ConfigFile& configFile)
{
  configFile.addSectionHandler("face_system",
                               bind(&FaceManager::onConfig, this, _1, _2, _3));
}


void
FaceManager::onConfig(const ConfigSection& configSection,
                      bool isDryRun,
                      const std::string& filename)
{
  bool hasSeenUnix = false;
  bool hasSeenTcp = false;
  bool hasSeenUdp = false;
  bool hasSeenEther = false;

  const std::list<shared_ptr<NetworkInterfaceInfo> > nicList(listNetworkInterfaces());

  for (ConfigSection::const_iterator item = configSection.begin();
       item != configSection.end();
       ++item)
    {
      if (item->first == "unix")
        {
          if (hasSeenUnix)
            throw Error("Duplicate \"unix\" section");
          hasSeenUnix = true;

          processSectionUnix(item->second, isDryRun);
        }
      else if (item->first == "tcp")
        {
          if (hasSeenTcp)
            throw Error("Duplicate \"tcp\" section");
          hasSeenTcp = true;

          processSectionTcp(item->second, isDryRun);
        }
      else if (item->first == "udp")
        {
          if (hasSeenUdp)
            throw Error("Duplicate \"udp\" section");
          hasSeenUdp = true;

          processSectionUdp(item->second, isDryRun, nicList);
        }
      else if (item->first == "ether")
        {
          if (hasSeenEther)
            throw Error("Duplicate \"ether\" section");
          hasSeenEther = true;

          processSectionEther(item->second, isDryRun, nicList);
        }
      else
        {
          throw Error("Unrecognized option \"" + item->first + "\"");
        }
    }
}

void
FaceManager::processSectionUnix(const ConfigSection& configSection, bool isDryRun)
{
  // ; the unix section contains settings of UNIX stream faces and channels
  // unix
  // {
  //   listen yes ; set to 'no' to disable UNIX stream listener, default 'yes'
  //   path /var/run/nfd.sock ; UNIX stream listener path
  // }

#if defined(HAVE_UNIX_SOCKETS)

  bool needToListen = true;
  std::string path = "/var/run/nfd.sock";

  for (ConfigSection::const_iterator i = configSection.begin();
       i != configSection.end();
       ++i)
    {
      if (i->first == "path")
        {
          path = i->second.get_value<std::string>();
        }
      else if (i->first == "listen")
        {
          needToListen = parseYesNo(i, i->first, "unix");
        }
      else
        {
          throw ConfigFile::Error("Unrecognized option \"" + i->first + "\" in \"unix\" section");
        }
    }

  if (!isDryRun)
    {
      shared_ptr<UnixStreamFactory> factory = make_shared<UnixStreamFactory>();
      shared_ptr<UnixStreamChannel> unixChannel = factory->createChannel(path);

      if (needToListen)
        {
          // Should acceptFailed callback be used somehow?
          unixChannel->listen(bind(&FaceTable::add, &m_faceTable, _1),
                              UnixStreamChannel::ConnectFailedCallback());
        }

      m_factories.insert(std::make_pair("unix", factory));
    }
#else
  throw ConfigFile::Error("NFD was compiled without UNIX sockets support, cannot process \"unix\" section");
#endif // HAVE_UNIX_SOCKETS

}

void
FaceManager::processSectionTcp(const ConfigSection& configSection, bool isDryRun)
{
  // ; the tcp section contains settings of TCP faces and channels
  // tcp
  // {
  //   listen yes ; set to 'no' to disable TCP listener, default 'yes'
  //   port 6363 ; TCP listener port number
  // }

  std::string port = "6363";
  bool needToListen = true;

  for (ConfigSection::const_iterator i = configSection.begin();
       i != configSection.end();
       ++i)
    {
      if (i->first == "port")
        {
          port = i->second.get_value<std::string>();
          try
            {
              uint16_t portNo = boost::lexical_cast<uint16_t>(port);
              NFD_LOG_TRACE("TCP port set to " << portNo);
            }
          catch (const std::bad_cast& error)
            {
              throw ConfigFile::Error("Invalid value for option " +
                                      i->first + "\" in \"udp\" section");
            }
        }
      else if (i->first == "listen")
        {
          needToListen = parseYesNo(i, i->first, "tcp");
        }
      else
        {
          throw ConfigFile::Error("Unrecognized option \"" + i->first + "\" in \"tcp\" section");
        }
    }

  if (!isDryRun)
    {
      shared_ptr<TcpFactory> factory = make_shared<TcpFactory>(boost::cref(port));

      using namespace boost::asio::ip;

      shared_ptr<TcpChannel> ipv4Channel = factory->createChannel("0.0.0.0", port);
      shared_ptr<TcpChannel> ipv6Channel = factory->createChannel("::", port);

      if (needToListen)
        {
          // Should acceptFailed callback be used somehow?

          ipv4Channel->listen(bind(&FaceTable::add, &m_faceTable, _1),
                              TcpChannel::ConnectFailedCallback());
          ipv6Channel->listen(bind(&FaceTable::add, &m_faceTable, _1),
                              TcpChannel::ConnectFailedCallback());
        }

      m_factories.insert(std::make_pair("tcp", factory));
      m_factories.insert(std::make_pair("tcp4", factory));
      m_factories.insert(std::make_pair("tcp6", factory));
    }
}

void
FaceManager::processSectionUdp(const ConfigSection& configSection,
                               bool isDryRun,
                               const std::list<shared_ptr<NetworkInterfaceInfo> >& nicList)
{
  // ; the udp section contains settings of UDP faces and channels
  // udp
  // {
  //   port 6363 ; UDP unicast port number
  //   idle_timeout 30 ; idle time (seconds) before closing a UDP unicast face
  //   keep_alive_interval 25; interval (seconds) between keep-alive refreshes

  //   ; NFD creates one UDP multicast face per NIC
  //   mcast yes ; set to 'no' to disable UDP multicast, default 'yes'
  //   mcast_port 56363 ; UDP multicast port number
  //   mcast_group 224.0.23.170 ; UDP multicast group (IPv4 only)
  // }

  std::string port = "6363";
  size_t timeout = 30;
  size_t keepAliveInterval = 25;
  bool useMcast = true;
  std::string mcastGroup = "224.0.23.170";
  std::string mcastPort = "56363";


  for (ConfigSection::const_iterator i = configSection.begin();
       i != configSection.end();
       ++i)
    {
      if (i->first == "port")
        {
          port = i->second.get_value<std::string>();
          try
            {
              uint16_t portNo = boost::lexical_cast<uint16_t>(port);
              NFD_LOG_TRACE("UDP port set to " << portNo);
            }
          catch (const std::bad_cast& error)
            {
              throw ConfigFile::Error("Invalid value for option " +
                                      i->first + "\" in \"udp\" section");
            }
        }
      else if (i->first == "idle_timeout")
        {
          try
            {
              timeout = i->second.get_value<size_t>();
            }
          catch (const std::exception& e)
            {
              throw ConfigFile::Error("Invalid value for option \"" +
                                      i->first + "\" in \"udp\" section");
            }
        }
      else if (i->first == "keep_alive_interval")
        {
          try
            {
              keepAliveInterval = i->second.get_value<size_t>();

              /// \todo Make use of keepAliveInterval
              (void)(keepAliveInterval);
            }
          catch (const std::exception& e)
            {
              throw ConfigFile::Error("Invalid value for option \"" +
                                      i->first + "\" in \"udp\" section");
            }
        }
      else if (i->first == "mcast")
        {
          useMcast = parseYesNo(i, i->first, "udp");
        }
      else if (i->first == "mcast_port")
        {
          mcastPort = i->second.get_value<std::string>();
          try
            {
              uint16_t portNo = boost::lexical_cast<uint16_t>(mcastPort);
              NFD_LOG_TRACE("UDP multicast port set to " << portNo);
            }
          catch (const std::bad_cast& error)
            {
              throw ConfigFile::Error("Invalid value for option " +
                                      i->first + "\" in \"udp\" section");
            }
        }
      else if (i->first == "mcast_group")
        {
          using namespace boost::asio::ip;
          mcastGroup = i->second.get_value<std::string>();
          try
            {
              address mcastGroupTest = address::from_string(mcastGroup);
              if (!mcastGroupTest.is_v4())
                {
                  throw ConfigFile::Error("Invalid value for option \"" +
                                          i->first + "\" in \"udp\" section");
                }
            }
          catch(const std::runtime_error& e)
            {
              throw ConfigFile::Error("Invalid value for option \"" +
                                      i->first + "\" in \"udp\" section");
            }
        }
      else
        {
          throw ConfigFile::Error("Unrecognized option \"" + i->first + "\" in \"udp\" section");
        }
    }

  /// \todo what is keep alive interval used for?

  if (!isDryRun)
    {
      shared_ptr<UdpFactory> factory = make_shared<UdpFactory>(boost::cref(port));

      shared_ptr<UdpChannel> v4Channel =
        factory->createChannel("0.0.0.0", port, time::seconds(timeout));

      shared_ptr<UdpChannel> v6Channel =
        factory->createChannel("::", port, time::seconds(timeout));

      v4Channel->listen(bind(&FaceTable::add, &m_faceTable, _1),
                        UdpChannel::ConnectFailedCallback());

      v6Channel->listen(bind(&FaceTable::add, &m_faceTable, _1),
                        UdpChannel::ConnectFailedCallback());

      m_factories.insert(std::make_pair("udp", factory));
      m_factories.insert(std::make_pair("udp4", factory));
      m_factories.insert(std::make_pair("udp6", factory));

      if (useMcast)
        {
          for (std::list<shared_ptr<NetworkInterfaceInfo> >::const_iterator i = nicList.begin();
               i != nicList.end();
               ++i)
            {
              const shared_ptr<NetworkInterfaceInfo>& nic = *i;
              if (nic->isUp() && nic->isMulticastCapable() && !nic->ipv4Addresses.empty())
                {
                  shared_ptr<MulticastUdpFace> newFace =
                    factory->createMulticastFace(nic->ipv4Addresses[0].to_string(),
                                                 mcastGroup, mcastPort);

                  addCreatedFaceToForwarder(newFace);
                }
            }
        }
    }
}

void
FaceManager::processSectionEther(const ConfigSection& configSection,
                                 bool isDryRun,
                                 const std::list<shared_ptr<NetworkInterfaceInfo> >& nicList)
{
  // ; the ether section contains settings of Ethernet faces and channels
  // ether
  // {
  //   ; NFD creates one Ethernet multicast face per NIC
  //   mcast yes ; set to 'no' to disable Ethernet multicast, default 'yes'
  //   mcast_group 01:00:5E:00:17:AA ; Ethernet multicast group
  // }

#if defined(HAVE_PCAP)

  using ethernet::Address;

  bool useMcast = true;
  Address mcastGroup(ethernet::getDefaultMulticastAddress());

  for (ConfigSection::const_iterator i = configSection.begin();
       i != configSection.end();
       ++i)
    {
      if (i->first == "mcast")
        {
          useMcast = parseYesNo(i, i->first, "ether");
        }

      else if (i->first == "mcast_group")
        {
          mcastGroup = Address::fromString(i->second.get_value<std::string>());
          if (mcastGroup.isNull())
            {
              throw ConfigFile::Error("Invalid value for option \"" +
                                      i->first + "\" in \"ether\" section");
            }
        }
      else
        {
          throw ConfigFile::Error("Unrecognized option \"" + i->first + "\" in \"ether\" section");
        }
    }

  if (!isDryRun)
    {
      shared_ptr<EthernetFactory> factory = make_shared<EthernetFactory>();
      m_factories.insert(std::make_pair("ether", factory));

      if (useMcast)
        {
          for (std::list<shared_ptr<NetworkInterfaceInfo> >::const_iterator i = nicList.begin();
               i != nicList.end();
               ++i)
            {
              const shared_ptr<NetworkInterfaceInfo>& nic = *i;
              if (nic->isUp() && nic->isMulticastCapable())
                {
                  try
                    {
                      shared_ptr<EthernetFace> newFace =
                        factory->createMulticastFace(nic, mcastGroup);

                      addCreatedFaceToForwarder(newFace);
                    }
                  catch (const EthernetFactory::Error& factoryError)
                    {
                      NFD_LOG_ERROR(factoryError.what() << ", continuing");
                    }
                  catch (const EthernetFace::Error& faceError)
                    {
                      NFD_LOG_ERROR(faceError.what() << ", continuing");
                    }
                }
            }
        }
    }
#else
  throw ConfigFile::Error("NFD was compiled without libpcap, cannot process \"ether\" section");
#endif // HAVE_PCAP
}


void
FaceManager::onFaceRequest(const Interest& request)
{
  const Name& command = request.getName();
  const size_t commandNComps = command.size();
  const Name::Component& verb = command.get(COMMAND_PREFIX.size());

  UnsignedVerbDispatchTable::const_iterator unsignedVerbProcessor = m_unsignedVerbDispatch.find(verb);
  if (unsignedVerbProcessor != m_unsignedVerbDispatch.end())
    {
      NFD_LOG_DEBUG("command result: processing verb: " << verb);
      (unsignedVerbProcessor->second)(this, boost::cref(request));
    }
  else if (COMMAND_UNSIGNED_NCOMPS <= commandNComps &&
           commandNComps < COMMAND_SIGNED_NCOMPS)
    {
      NFD_LOG_DEBUG("command result: unsigned verb: " << command);
      sendResponse(command, 401, "Signature required");
    }
  else if (commandNComps < COMMAND_SIGNED_NCOMPS ||
           !COMMAND_PREFIX.isPrefixOf(command))
    {
      NFD_LOG_DEBUG("command result: malformed");
      sendResponse(command, 400, "Malformed command");
    }
  else
    {
      validate(request,
               bind(&FaceManager::onValidatedFaceRequest, this, _1),
               bind(&ManagerBase::onCommandValidationFailed, this, _1, _2));
    }
}

void
FaceManager::onValidatedFaceRequest(const shared_ptr<const Interest>& request)
{
  const Name& command = request->getName();
  const Name::Component& verb = command[COMMAND_PREFIX.size()];
  const Name::Component& parameterComponent = command[COMMAND_PREFIX.size() + 1];

  SignedVerbDispatchTable::const_iterator signedVerbProcessor = m_signedVerbDispatch.find(verb);
  if (signedVerbProcessor != m_signedVerbDispatch.end())
    {
      ControlParameters parameters;
      if (!extractParameters(parameterComponent, parameters))
        {
          sendResponse(command, 400, "Malformed command");
          return;
        }

      NFD_LOG_DEBUG("command result: processing verb: " << verb);
      (signedVerbProcessor->second)(this, *request, parameters);
    }
  else
    {
      NFD_LOG_DEBUG("command result: unsupported verb: " << verb);
      sendResponse(command, 501, "Unsupported command");
    }

}

void
FaceManager::addCreatedFaceToForwarder(const shared_ptr<Face>& newFace)
{
  m_faceTable.add(newFace);

  //NFD_LOG_DEBUG("Created face " << newFace->getRemoteUri() << " ID " << newFace->getId());
}

void
FaceManager::onCreated(const Name& requestName,
                       ControlParameters& parameters,
                       const shared_ptr<Face>& newFace)
{
  addCreatedFaceToForwarder(newFace);
  parameters.setFaceId(newFace->getId());

  sendResponse(requestName, 200, "Success", parameters.wireEncode());
}

void
FaceManager::onConnectFailed(const Name& requestName, const std::string& reason)
{
  NFD_LOG_DEBUG("Failed to create face: " << reason);
  sendResponse(requestName, 408, reason);
}

void
FaceManager::createFace(const Interest& request,
                        ControlParameters& parameters)
{
  const Name& requestName = request.getName();
  ndn::nfd::FaceCreateCommand command;

  if (!validateParameters(command, parameters))
    {
      sendResponse(requestName, 400, "Malformed command");
      NFD_LOG_TRACE("invalid control parameters URI");
      return;
    }

  FaceUri uri;
  if (!uri.parse(parameters.getUri()))
    {
      sendResponse(requestName, 400, "Malformed command");
      NFD_LOG_TRACE("failed to parse URI");
      return;
    }

  FactoryMap::iterator factory = m_factories.find(uri.getScheme());
  if (factory == m_factories.end())
    {
      sendResponse(requestName, 501, "Unsupported protocol");
      return;
    }

  try
    {
      factory->second->createFace(uri,
                                  bind(&FaceManager::onCreated,
                                       this, requestName, parameters, _1),
                                  bind(&FaceManager::onConnectFailed,
                                       this, requestName, _1));
    }
  catch (const std::runtime_error& error)
    {
      std::string errorMessage = "NFD error: ";
      errorMessage += error.what();

      NFD_LOG_ERROR(errorMessage);
      sendResponse(requestName, 500, errorMessage);
    }
  catch (const std::logic_error& error)
    {
      std::string errorMessage = "NFD error: ";
      errorMessage += error.what();

      NFD_LOG_ERROR(errorMessage);
      sendResponse(requestName, 500, errorMessage);
    }
}


void
FaceManager::destroyFace(const Interest& request,
                         ControlParameters& parameters)
{
  const Name& requestName = request.getName();
  ndn::nfd::FaceDestroyCommand command;

  if (!validateParameters(command, parameters))
    {
      sendResponse(requestName, 400, "Malformed command");
      return;
    }

  shared_ptr<Face> target = m_faceTable.get(parameters.getFaceId());
  if (static_cast<bool>(target))
    {
      target->close();
    }

  sendResponse(requestName, 200, "Success", parameters.wireEncode());

}

void
FaceManager::onAddFace(shared_ptr<Face> face)
{
  ndn::nfd::FaceEventNotification notification;
  notification.setKind(ndn::nfd::FACE_EVENT_CREATED)
              .setFaceId(face->getId())
              .setRemoteUri(face->getRemoteUri().toString())
              .setLocalUri(face->getLocalUri().toString())
              .setFlags(getFaceFlags(*face));

  m_notificationStream.postNotification(notification);
}

void
FaceManager::onRemoveFace(shared_ptr<Face> face)
{
  ndn::nfd::FaceEventNotification notification;
  notification.setKind(ndn::nfd::FACE_EVENT_DESTROYED)
              .setFaceId(face->getId())
              .setRemoteUri(face->getRemoteUri().toString())
              .setLocalUri(face->getLocalUri().toString())
              .setFlags(getFaceFlags(*face));

  m_notificationStream.postNotification(notification);
}


bool
FaceManager::extractLocalControlParameters(const Interest& request,
                                           ControlParameters& parameters,
                                           ControlCommand& command,
                                           shared_ptr<LocalFace>& outFace,
                                           LocalControlFeature& outFeature)
{
  if (!validateParameters(command, parameters))
    {
      sendResponse(request.getName(), 400, "Malformed command");
      return false;
    }

  shared_ptr<Face> face = m_faceTable.get(request.getIncomingFaceId());

  if (!static_cast<bool>(face))
    {
      NFD_LOG_DEBUG("command result: faceid " << request.getIncomingFaceId() << " not found");
      sendResponse(request.getName(), 410, "Face not found");
      return false;
    }
  else if (!face->isLocal())
    {
      NFD_LOG_DEBUG("command result: cannot enable local control on non-local faceid " <<
                    face->getId());
      sendResponse(request.getName(), 412, "Face is non-local");
      return false;
    }

  outFace = dynamic_pointer_cast<LocalFace>(face);
  outFeature = static_cast<LocalControlFeature>(parameters.getLocalControlFeature());

  return true;
}

void
FaceManager::enableLocalControl(const Interest& request,
                                ControlParameters& parameters)
{
  ndn::nfd::FaceEnableLocalControlCommand command;


  shared_ptr<LocalFace> face;
  LocalControlFeature feature;

  if (extractLocalControlParameters(request, parameters, command, face, feature))
    {
      face->setLocalControlHeaderFeature(feature, true);
      sendResponse(request.getName(), 200, "Success", parameters.wireEncode());
    }
}

void
FaceManager::disableLocalControl(const Interest& request,
                                 ControlParameters& parameters)
{
  ndn::nfd::FaceDisableLocalControlCommand command;
  shared_ptr<LocalFace> face;
  LocalControlFeature feature;

  if (extractLocalControlParameters(request, parameters, command, face, feature))
    {
      face->setLocalControlHeaderFeature(feature, false);
      sendResponse(request.getName(), 200, "Success", parameters.wireEncode());
    }
}

void
FaceManager::listFaces(const Interest& request)
{
  const Name& command = request.getName();
  const size_t commandNComps = command.size();

  if (commandNComps < LIST_COMMAND_NCOMPS ||
      !LIST_COMMAND_PREFIX.isPrefixOf(command))
    {
      NFD_LOG_DEBUG("command result: malformed");
      sendResponse(command, 400, "Malformed command");
      return;
    }

  m_statusPublisher.publish();
}

} // namespace nfd