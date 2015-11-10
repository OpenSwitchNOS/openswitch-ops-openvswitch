/*! \file ofproto_extensions.h
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


#ifndef __OFPROTO_EXTENSIONS_H__
#define __OFPROTO_EXTENSIONS_H__

#include <stdint.h>
#include <stdbool.h>

/**
    @struct ofproto_extension_header
    @brief  Interface header structure, contains plugin version
    information as magic, major and minor.
 */
struct ofproto_extension_header {
    int magic; /**< ID of Plugin */
    int major; /**< Major version */
    int minor; /**< Minor version */
};

/**
    @fn ofproto_extensions_init
    @brief  Initialization of the extensions hash table, needs to be run
    once before start the plugins registration.
    @return 0 if success, errno value otherwise.
 */
int ofproto_extensions_init(void);

/**
    @fn register_ofproto_extension
    @brief  Registration of new plugins, should be called from the plugin
    itself inside <plugin>_init function to register the plugin's interface
    @param[in] magic The key for the hash table to find the respective interface
    @param[in] ptr The pointer to the plugin's interface structure
 */
int register_ofproto_extension (int magic, void *ptr);

/**
    @fn unregister_ofproto_extension
    @brief  Unregistration of existing plugins, could be called from the plugin
    itself to delete its interface from the hash table
    @param[in] magic The key for the hash table to find the respective interface
    @return 0 if success, errno value otherwise.
 */
int unregister_ofproto_extension (int magic);

/**
    @fn find_ofproto_extension
    @brief  Lookup for registered interfaces, could be called either from a
    plugin or any other place. Allows to search for specific plugin functions.
    @param[in] magic The key for the hash table to find the respective interface.
    @param[in] major The major value for the plugin expected version.
    @param[in] minr  The minor value for the plugin expected version.
    @param[out] interface Returns the found interface, NULL if not found. You can
    pass the direction of your struct pointer:
       struct my_interface *intf;
       res = find_ofproto_extension(magic, major, minor, (void *)intf );
       if (res == 0 && inft) ...
    @return 0 if success, errno value otherwise.
 */
int find_ofproto_extension (int magic, int major, int minor, void **interface);

/**
    @fn register_asic_plugin_magic
    @brief Called from all asics-plugins init function to register their magic.
    @param[in] magic The magic number of the caller asic plugin.
    @return 0 if success, errno value otherwise.
 */
int register_asic_plugin_magic(int magic);

/**
    @fn find_asic_plugin_magic
    @brief Used by feature plugins to get the asic plugin magic number.
    @param[out] magic The magic number of the registered asic plugin.
    @return 0 if success, errno value otherwise.
 */
int find_asic_plugin_magic(int *magic);

/**
    @fn find_asic_extension
    @brief  Lookup for the current registered asic plugin. This call should be
    used after plugins registration and from any feature-plugin.
    @param[in] major The major value for the plugin expected version.
    @param[in] minr  The minor value for the plugin expected version.
    @param[out] interface Returns the found interface, NULL if not found. You can
    pass the direction of your struct pointer:
       struct my_asic_interface *asic_int;
       res = find_asic_extension(major, minor, (void *)asic_int );
       if (res == 0 && asic_int) ...
    @return 0 if success, errno value otherwise.
 */
int find_asic_extension(int major, int minor, void **interface);

#endif /*__OFPROTO_EXTENSIONS_H__*/
