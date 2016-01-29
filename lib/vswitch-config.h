
/* Copyright (c) 2008, 2009, 2010, 2011, 2012, 2013, 2014 Nicira, Inc.
 * Copyright (C) 2015 Hewlett-Packard Development Company, L.P.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef VSWITCH_CONFIG_H
#define VSWITCH_CONFIG_H

#include "ofproto/ofproto-provider.h"
#include "vswitch-idl.h"
#include "shash.h"
#include "hmap.h"

struct bridge {
    struct hmap_node node;      /* In 'all_bridges'. */
    char *name;                 /* User-specified arbitrary name. */
    char *type;                 /* Datapath type. */
    uint8_t ea[ETH_ADDR_LEN];   /* Bridge Ethernet Address. */
    uint8_t default_ea[ETH_ADDR_LEN]; /* Default MAC. */
    const struct ovsrec_bridge *cfg;

    /* OpenFlow switch processing. */
    struct ofproto *ofproto;    /* OpenFlow switch. */

    /* Bridge ports. */
    struct hmap ports;          /* "struct port"s indexed by name. */
    struct hmap ifaces;         /* "struct iface"s indexed by ofp_port. */
    struct hmap iface_by_name;  /* "struct iface"s indexed by name. */

#ifdef OPS
    /* Bridge VLANs. */
    struct hmap vlans;          /* "struct vlan"s indexed by VID. */
#endif

#ifndef OPS_TEMP
    /* Port mirroring. */
    struct hmap mirrors;        /* "struct mirror" indexed by UUID. */
#endif
    /* Used during reconfiguration. */
    struct shash wanted_ports;

    /* Synthetic local port if necessary. */
    struct ovsrec_port synth_local_port;
    struct ovsrec_interface synth_local_iface;
    struct ovsrec_interface *synth_local_ifacep;
};

struct vrf {
    struct bridge *up;
    struct hmap_node node;              /* In 'all_vrfs'. */
    const struct ovsrec_vrf *cfg;
    struct hmap all_neighbors;
    struct hmap all_routes;
    struct hmap all_nexthops;
};


#endif /* VSWITCH_CONFIG_H */
