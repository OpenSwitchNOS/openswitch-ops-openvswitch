/*
 * Copyright (C) 2015 Hewlett-Packard Development Company, L.P.
 * All Rights Reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License"); you may
 *    not use this file except in compliance with the License. You may obtain
 *    a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *    WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *    License for the specific language governing permissions and limitations
 *    under the License.
 *
 * File:    openhalon-dflt.h
 *
 * Purpose: This file contains default values for various columns in the OVSDB.
 *          The purpose is to avoid hard-coded values inside each module/daemon code.
 *
 */

#ifndef OPENHALON_DFLT_HEADER
#define OPENHALON_DFLT_HEADER 1

/************************* Open vSwitch Table  ***************************/

/* Interface Statistics update interval should
 * always be greater than or equal to 5 seconds. */
#define DFLT_OPEN_VSWITCH_OTHER_CONFIG_STATS_UPDATE_INTERVAL        5000

/* Default min_vlan ID for internal VLAN range */
#define DFLT_OPEN_VSWITCH_OTHER_CONFIG_MAP_MIN_INTERNAL_VLAN_ID     1024

/* Default max_vlan ID for internal VLAN range */
#define DFLT_OPEN_VSWITCH_OTHER_CONFIG_MAP_MAX_INTERNAL_VLAN_ID     4094

/* Defaults and min/max values LACP parameters */
#define DFLT_OPEN_VSWITCH_LACP_CONFIG_SYSTEM_PRIORITY   65534
#define MIN_OPEN_VSWITCH_LACP_CONFIG_SYSTEM_PRIORITY    0
#define MAX_OPEN_VSWITCH_LACP_CONFIG_SYSTEM_PRIORITY    65535

#define MIN_INTERFACE_OTHER_CONFIG_LACP_PORT_ID                 1
#define MAX_INTERFACE_OTHER_CONFIG_LACP_PORT_ID                 65535
#define MIN_INTERFACE_OTHER_CONFIG_LACP_PORT_PRIORITY           1
#define MAX_INTERFACE_OTHER_CONFIG_LACP_PORT_PRIORITY           65535
#define MIN_INTERFACE_OTHER_CONFIG_LACP_AGGREGATION_KEY         1
#define MAX_INTERFACE_OTHER_CONFIG_LACP_AGGREGATION_KEY         65535

#define MAX_NEXTHOPS_PER_ROUTE                                      32
#endif /* OPENHALON_DFLT_HEADER */