# sFlow Design

- [Overview](#overview)
- [High level design](#high-level-design)
    - [Opennsl plugin](#opennsl-plugin)
	- [Docker container plugin](#docker-container-plugin)
- [DB schema](#db-schema)
- [Code design](#code-design)
- [References](#references)

## Overview

sFlow is a real-time packet sampling technology that can forward sampled packet data from all the ports in a network device to an sFlow data collector. OpenSwitch implements the sFlow version 5 (sFlow v5) agent software that forwards the data to the sFlow collector. Along with the sampled packets, the sFlow agent software also periodically forwards interface statistics to the collector.


## High level design

The sFlow agent in OpenSwitch is designed as two separate entities with one entity responsible for sending the sampled data packets while the other responsible for sending the interface statistics. The ASIC SDK plugins configure sFlow sampling in the hardware and converts the sampled packets coming from the hardware into sFlow datagrams to be sent to the collector. `ops-vswitchd` would gather the interface statistics from the hardware and write them into the database while an sFlow daemon would periodically send these statistics to the collector as sFlow datagrams.

```ditaa


                                          +----------------------+
                                          |       database       |
                                     +---->                      |
         ops-switchd                 |    +----------------------+
                                     |                |
         +-----------------------+   |                |
         |                       |   |      +---------v---------+
         |                       |   |      |                   |
         |     SDK independent   +---+      |    sFlow daemon   |
         |     layer             |          |                   |
         |                       |          +-------------------+
         |                       |                    |
         +-----------------------+                    |
         |                       |                    |
         |                       |                    +---------------->
         |     SDK plugin        +---------+          sFlow interface
         |     layer             |         |          stats to collector
         |                       |         |
         |                       |         +--------------------------->
         +-----------^-----------+            sFlow packet samples to
                     |                        collector
                     |
                     |  sFlow statistics
                     |  and packet samples
                     |
         +-----------------------+
         |                       |
         |      hardware         |
         |                       |
         +-----------------------+
```

### Opennsl plugin

`ops-switchd` configures the opennsl compliant hardware with the sFlow configuration through the opennsl plugin layer. Opennsl APIs are used to configure the same sampling rate for both ingress and egress packets on a per-interface basis. Interface counter statistics can be polled with the Opennsl stats APIs.

### Docker container plugin
`ops-switchd` configures sFlow on the container software using the container plugin layer. The container plugin configures sFlow on the L2 interfaces by translating the sFlow configuration from the OpenSwitch database into sFlow configuration on the native `openvswitch-sim` database. This configuration is then applied on all the OVS bridges which in-turn applies it on all the interfaces(L2) under each bridge. sFlow configuration is applied on all the L3 interfaces in docker by using iptables and the NFLOG packet logging facility in the Linux kernel. Linux interface statistics can be collected using the ethtool facility.

## DB schema
The following columns are read by sFlow
```
Subystem: sFlow reference
sFlow: All the columns in the table.
```
The following columns are written by sFlow
```
Interface: statistics
```

## Code design

`bridge.c` in `ops-switchd` sends the global sFlow configuration from the Subsystem table to the hardware plugins through the `ofproto->set_sflow()` API. This API will be called for each bridge and VRF in the system. The plugins in general would run a thread that waits to receive sampled packets from the hardware. These sampled packets would be send to the collector by using the `sflow` library in `ops-switchd`. This library would have to be configured with the collector details and the library then would take care of packaging and sending the sampled packets to the collector as sFlow datagrams.
`ops-switchd` periodic timer would poll the hardware stats using OVS `netdev` APIs and these stats are then published into the database. A sFlow daemon would then pick these stats from the database and send to the collector once every polling interval.

## References

- [sFlow v5](http://www.sflow.org/sflow_version_5.txt)
- [Open vSwitch](http://openvswitch.org/)
- [iptables nflog](http://ipset.netfilter.org/iptables-extensions.man.html)
- [ethtool](http://linux.die.net/man/8/ethtool)
