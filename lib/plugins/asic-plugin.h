/*! \file asic-plugin.h
    \brief This file contains all of the functions owned of switchd_sample_plugin
 */

/**
 * Copyright (C) 2015 The Team <theteam@hpe.com>
 * Hewlett Packard Enterprise Development LP
 * All Rights Reserved.
 *
 * The contents of this software are proprietary and confidential
 * to the Hewlett Packard Enterprise Development LP. No part of
 * this program  may be photocopied, reproduced, or translated
 * into another programming language without prior written consent
 * of the Hewlett Packard Enterprise Development LP.
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

#ifndef __asic_plugin_H__
#define __asic_plugin_H__

/*! \def asic_plugin_MAJOR
 *  \brief plugin major version definition
 */
#define asic_plugin_MAJOR    2

/*! \def asic_plugin_MINOR
 *  \brief plugin magic version definition
 */
#define asic_plugin_MINOR    3

//structures

/*! \struct asic_plugin_interface
 *  \brief Plugin interface structure
 *
 *  \param magic key
 *  \param major
 *  \param minor
 */
/* struct asic_plugin_interface { */
/*    int magic; //!< plugin magic version */
/*    int major; //!< plugin major version */
/*    int minor; //!< plugin minor version */
/* }; */

struct asic_plugin_interface {
    int magic;   // ID of Plugin
    int major;   // major Version
    int minor;   // Minor Version
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

#endif //__asic_plugin_H__
