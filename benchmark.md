# ONL Benchmark

This document describes how to benchmark NFD-InterestDigest on [Open Network Laboratory](https://onl.wustl.edu).

## Topology Setup

We use [WU-ARL's 16x1x16 topology](https://github.com/WU-ARL/NFD_Performance_Testing_on_ONL/tree/2fa413c219e0670f5e57b55379fe2c0824c7110b/Variable_Length_Names/ONL_RLI_Files/nfd_16_clients_16_servers_1_rtr), which contains 16 consumers, 1 router, and 16 producers.
The corresponding [hosts file](https://github.com/WU-ARL/NFD_Performance_Testing_on_ONL/blob/2fa413c219e0670f5e57b55379fe2c0824c7110b/Variable_Length_Names/hosts) is also needed.

## NFD Installation

ndn-cxx, NFD, and ndn-tools should be compiled in release mode on a Vagrant `ubuntu/trusty64` virtual machine.
Within each repository, execute `./waf install --destdir=$HOME/local`.
Copy `$HOME/local/usr/local/*` on virtual machines to `$HOME/local/*` on ONL.
Compile [infoedit](https://github.com/NDN-Routing/infoedit) and install the binary to `$HOME/local/bin/` on ONL.

Prepare configuration with the following commands on a pc2core node:

    cp local/etc/ndn/nfd.conf.sample local/etc/ndn/nfd.conf
    local/bin/infoedit -f local/etc/ndn/nfd.conf -s log.default_level -v WARN
    local/bin/infoedit -f local/etc/ndn/nfd.conf -s tables.cs_max_packets -v 0
    local/bin/infoedit -f local/etc/ndn/nfd.conf -s face_system.unix.path -v /tmp/nfd.sock
    local/bin/infoedit -f local/etc/ndn/nfd.conf -d face_system.tcp
    local/bin/infoedit -f local/etc/ndn/nfd.conf -s face_system.udp.mcast -v no
    local/bin/infoedit -f local/etc/ndn/nfd.conf -d face_system.ether
    local/bin/infoedit -f local/etc/ndn/nfd.conf -d face_system.websocket
    local/bin/infoedit -f local/etc/ndn/nfd.conf -d rib.auto_prefix_propagate

    rm -rf .ndn
    mkdir .ndn
    echo 'transport=unix:///tmp/nfd.sock' > .ndn/client.conf
    local/bin/ndnsec-keygen /operator | local/bin/ndnsec-install-cert -

## Execute Benchmark

SSH into `onlusr` after the topology has been committed.
Execute the following:

    # start NFD
    for H in $RTR_HOST $CLIENT_HOSTS $SERVER_HOSTS; do
      echo 'local/bin/nfd --config local/etc/ndn/nfd.conf &> /tmp/nfd.log &' | ssh ${!H} bash
    done

    # start producers
    I=0
    for H in $SERVER_HOSTS; do
      I=$((I+1))
      echo "local/bin/ndnpingserver /A/B/C/D/E/F/G/H/P$I &> /tmp/ndnpingserver.log &" | ssh ${!H} bash
      echo "local/bin/nfdc face create udp://$H; local/bin/nfdc route add /A/B/C/D/E/F/G/H/P$I udp://$H" | ssh ${!RTR_HOST} bash
    done

    # start consumers
    I=0
    for H in $CLIENT_HOSTS; do
      I=$((I+1))
      echo "local/bin/nfdc face create udp://${RTR_HOST}; local/bin/nfdc route add /A/B/C/D/E/F/G/H udp://${RTR_HOST}; sleep 1; local/bin/ndnping -i 1 /A/B/C/D/E/F/G/H/P$I &> /tmp/ndnping.log &" | ssh ${!H} bash
    done

    # wait for a few minutes, look at "NFD Pkt Count" window, and run `top` on router to see CPU usage

    # stop consumers and NFD
    for H in $CLIENT_HOSTS; do
      echo 'killall -s SIGINT ndnping' | ssh ${!H} bash
    done
    for H in $RTR_HOST $CLIENT_HOSTS $SERVER_HOSTS; do
      echo 'killall nfd' | ssh ${!H} bash
    done

    # collect ping stats
    for H in $CLIENT_HOSTS; do
      ssh ${!H} tail -3 /tmp/ndnping.log
    done
