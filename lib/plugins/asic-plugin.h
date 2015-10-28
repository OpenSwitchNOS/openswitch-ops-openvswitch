/*! \file asic-plugin.h
    \brief This file contains all of the functions owned of switchd_sample_plugin
 */

/* Copyright (C) 2016 Hewlett-Packard Development Company, L.P.
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

#include <stdarg.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include "cfm.h"
#include "classifier.h"
#include "guarded-list.h"
#include "heap.h"
#include "hindex.h"
#include "list.h"
#include "ofp-actions.h"
#include "ofp-errors.h"
#include "ofp-util.h"
#include "ofproto/ofproto.h"
#include "ovs-atomic.h"
#include "ovs-rcu.h"
#include "ovs-thread.h"
#include "shash.h"
#include "simap.h"
#include "timeval.h"

#ifndef __ASIC_PLUGIN_H__
#define __ASIC_PLUGIN_H__

/** @def asic_plugin_MAJOR
 *  @brief plugin major version definition
 */
#define asic_plugin_MAJOR    1

/** @def asic_plugin_MINOR
 *  @brief plugin_name version definition
 */
#define asic_plugin_MINOR    1

/* structures */

/** @struct asic_plugin_interface
 *  @brief Plugin interface structure
 *
 *  @param plugin_name key
 *  @param major
 *  @param minor
 */

struct asic_plugin_interface {
    char *plugin_name;   /* ID of Plugin */
    int major;           /* major Version */
    int minor;           /* Minor Version */
    int (*add_l3_host_entry)(const struct ofproto *ofproto, void *aux,
                             bool is_ipv6_addr, char *ip_addr,
                             char *next_hop_mac_addr, int *l3_egress_id);
    int (*delete_l3_host_entry)(const struct ofproto *ofproto, void *aux,
                                bool is_ipv6_addr, char *ip_addr,
                                int *l3_egress_id);
    int (*get_l3_host_hit_bit)(const struct ofproto *ofproto, void *aux,
                           bool is_ipv6_addr, char *ip_addr, bool *hit_bit);
    int (*l3_route_action)(const struct ofproto *ofproto,
                           enum ofproto_route_action action,
                           struct ofproto_route *route);
    int (*l3_ecmp_set)(const struct ofproto *ofproto, bool enable);
    int (*l3_ecmp_hash_set)(const struct ofproto *ofproto, unsigned int hash,
                            bool enable);
};

#endif /*__ASIC_PLUGIN_H__*/
