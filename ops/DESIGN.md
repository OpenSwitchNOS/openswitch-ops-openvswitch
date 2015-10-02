# High-level design of ops-switchd
The ops-switchd daemon is responsible for driving the configuration from the database into the ASIC and reads the statuses and statistics from ASIC into the database. ops-switchd has three primary layers which are the SDK independent code, SDK plugin and the ASIC SDK. SDK independent code reads configuration from database and pushes this configuration through the SDK plugin layer which translates the configuration into ASIC SDK APIs. The SDK plugin layer is an extension of the "ofproto provider" and the "netdev provider" interfaces and the SDK plugin is compiled as a dynamically linked library which is loaded at run time by ops-switchd.

## Reponsibilities
* Manages VRF configuration and L3 ports created as part of the VRF.
* Update ASIC with the route and nexthop configuration from the database.
* Update ASIC with the neighbor information from the database.
* Manage VLAN creation/deletion in the ASIC.
* Manage interface configuration in the ASIC.
* Gather interface stats from the ASIC and update the database.

##  Design choices
ops-switchd is a modified version of ovs-vswitchd from Open vSwitch. ovs-vswitchd was extended to support full configurability of the interfaces, introduce the concept of VRFs, route and nexthop management.

## Relationships to external OpenSwitch entities

```ditaa
+-----------------------+
|                       |
|         OVSDB         |
|                       |
+-----------^-----------+
            |
            |
            |
+-----------v-----------+
| SDK independent layer |
|                       |
+-----------------------+
|   SDK plugin layer    |
|                       | ops-switchd
+-----------------------+
|       ASIC SDK        |
|                       |
+-----------^-----------+
            |
            |
+-----------v-----------+
|         ASIC          |
|                       |
+-----------------------+
```
ops-switchd reads configuration from database and updates ASIC and reads statuses and statistics from ASIC and updates database. In OpenSwitch, ops-switchd is the only daemon that can talk to the ASIC.

## OVSDB-Schema
The following columns are read by ops-switchd:
```
  System:cur_cfg
  System:bridges
  System:vrfs
  System:system_mac
  System:ecmp_config
  System:other_config
  Subsystem:interfaces
  Bridge:name
  Bridge:ports
  Port:name
  Port:interfaces
  Port:tag
  Port:vlan_option
  Port:bond_option
  Port:hw_config
  Interface:name
  Interface:hw_intf_info
  VLAN:name
  VLAN:vid
  VLAN:enable
  VRF:name
  VRF:ports
  Neighbor:ip_address
  Neighbor:mac
  Neighbor:port
  Neighbor:vrf
  Nexthop:selected
  Nexthop:ip_address
  Nexthop:ports
  Route:selected
  Route:from
  Route:prefix
  Route:address_family
  Route:nexthops
```

The following columns are written by ops-switchd:
```
  Port:status
  Port:statistics
  Interface:statistics
  Interface:admin_state
  Interface:link_speed
  Interface:duplex
  Interface:mtu
  Interface:mac_in_use
  Interface:pause
  Neighbor:status
  Nexthop:status
```

## Code Design
* Initialization : Load the SDK ASIC plugins that are found in the plugins directory. Update database with values that are written to by ops-switchd.
* Main loop: The run functions of the various sub-modules are called from here. This includes the bridge, subsystem, bufmon, plugins, netdev. VRF and bridge handling are integrated and processed inside bridge code. Bridge looks at the VLAN table in the database to update the ASIC plugin with the updated VLAN information through the ofproto layer. Interface configuration and interface statistics collection in handled inside subystem run through the netdev layer. VRF is handled by creating a new ofproto class of type "vrf". This new ofproto class has APIs defined for L3 management including L3 interface creation/deletion, neighbor management, route and nexthop management. VRF code reads the route and nexthop (with support for ECMP too) information from the database and updates the ASIC with this configuration. VRF reads the neighbor table from database which in-turn is driven by the Linux ARP table and updates ASIC with this information.
