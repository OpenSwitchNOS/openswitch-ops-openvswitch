
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

#ifndef __ASIC_PLUGIN_H__
#define __ASIC_PLUGIN_H__ 1

/**
 * Asic Plugin
 *
 *  SwitchD Plugin Infrastructure includes two types of plugins:
 *
 * - Asic plugins: are plugins that implement the platform dependent asic code.
 *   There can be only one asic plugin loaded in the system. The asic plugin
 *   must implement all the functionality defined in asic_plugin.h. The plugin
 *   infrastructure will enforce that the asic plugin meets the major and minor
 *   versioning numbers specified in the asic_plugin.h file to guard against ABI
 *   breakage.
 *
 * - Feature plugins: are plugins that implement feature code that is
 *   indenpendent of the asic. A feature plugin can export public functions for
 *   other plugins or the main switchd code to use. A feature plugin will define
 *   its plublic interface with a major and minor number for versioning. The
 *   plugin infrastructure will provide methods to find and access the feature
 *   plugin interfaces; it will also validate that the requested major and minor
 *   numbers against the feature plugin public interface.
 *
 *   For asic plugins versioning is enforced by the plugin infrastructure based
 *   on the required version in asic_plugin.h file. For feature plugins
 *   versioning is enforced by the plugin infrastructure based on the exported
 *   plugin interface.
 *
 * asic-plugin.h expose the following functions:
 *
 * int register_asic_plugin(struct asic_plugin_interface *plugin_interface)
 * int find_asic_plugin(struct asic_plugin_interface **plugin_interface)
 *
 *    - register_asic_plugin: registers an asic-plugin interface. Its major and
 *      minor version are validated against the required versions specified in
 *      this header file.
 *      Returns 0 if successfully registered, EINVAL otherwise. This function
 *      checks if there is already an asic-plugin registered, in that case it
 *      avoids registration of a new asic-plugin and returns an EINVAL.
 *
 *    - find_asic_plugin: gets the registered asic-plugin interface. If
 *      successful pointer is obtained, 0 is returned, EINVAL is returned
 *      otherwise.
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

/** @def ASIC_PLUGIN_MAJOR
 *  @brief plugin major version definition
 */
#define ASIC_PLUGIN_INTERFACE_MAJOR    1

/** @def ASIC_PLUGIN_MINOR
 *  @brief plugin minor version definition
 */
#define ASIC_PLUGIN_INTERFACE_MINOR    1

/* structures */

/** @struct asic_plugin_interface
 *  @brief asic_plugin_interface enforces the interface that an ASIC plugin must
 *  provide to be compatible with SwitchD Asic plugin infrastructure.
 *  When an external plugin attempts to register itself as an ASIC plugin, the
 *  code will validate that the interface provided meets the requirements for
 *  MAJOR and MINOR versions.
 *
 *  - The ASIC_PLUGIN_INTERFACE_MAJOR identifies any large change in the fields
 *  of struct asic_plugin_interface that would break the ABI, so any extra
 *  fields added in the middle of previous fields, removal of previous fields
 *  would trigger a change in the MAJOR number.
 *
 *  - The ASIC_PLUGIN_INTERFACE_MINOR indentifies any incremental changes to the
 *  fields of struct asic_plugin_interface that would not break the ABI but
 *  would just make the new fields unavailable to the older component.
 *
 *  For example if ASIC_PLUGIN_INTERFACE_MAJOR is 1 and
 *  ASIC_PLUGIN_INTERFACE_MINOR is 2, then a plugin can register itself as an
 *  asic plugin if the provided interface has a MAJOR=1 and MINOR>=2. This means
 *  that even if the plugin provides more functionality in the interface fields
 *  those would not be used by SwitchD. But if the plugin has a MAJOR=1 and
 *  MINOR=1 then it cannot be used as an asic plugin as SwitchD will see fields
 *  in the interface struct that are not provided by the plugin.
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

/**
    @fn register_asic_plugin_name
    @brief Called from all asics-plugins init function to register their
           plugin_name.
    @param[in] plugin_interface.
    @return 0 if success, errno value otherwise.
 */
int register_asic_plugin(struct asic_plugin_interface *plugin_interface);

/**
    @fn find_asic_plugin
    @brief  Lookup for the current registered asic plugin. This call should be
    used after plugins registration and from any feature-plugin.
    @param[in] plugin_interface
    @return 0 if success, errno value otherwise.
 */
int find_asic_plugin(struct asic_plugin_interface **plugin_interface);

#endif /*__ASIC_PLUGIN_H__*/
