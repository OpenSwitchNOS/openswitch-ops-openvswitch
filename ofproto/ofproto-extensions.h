/** @file ofproto_extensions.h
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

/**
 * Ofproto Extensions API
 *
 * ofproto-extensions.h would expose the following API:
 *
 * int register_ofproto_extension (
 *       const char plugin_name, struct ofproto_extension_interface *plugin_functions);
 * int find_ofproto_extension (
 *       const char plugin_name, unsigned int mayor, unsigned int minor,
 *       void **plugin_interface);
 * int register_asic_plugin (const char plugin_name);
 * int find_asic_extension (
 *             unsigned int mayor, unsigned int minor, void **plugin_interface);
 *
 *    - register_ofproto_extension: registers an extension interface with a given
 *      plugin_name. If the extension is registered successfully 0 is returned
 *      otherwise EINVAL is returned. A registration can fail, among other reasons,
 *      if there is already another extension registered with the same plugin_name.
 *
 *    - find_ofproto_extension: searches for a plugin extension with the given
 *      plugin_name, mayor and minor numbers, if one is found 0 is returned and
 *      the interface parameter gets the interface pointer, EINVAL is returned
 *      otherwise.
 *
 *    - register_asic_plugin: registers an asic-plugin plugin name.
 *      Returns 0 if successfully registered, EINVAL otherwise. This function
 *      checks if there is already an asci-plugin registered, in that case it
 *      avoids registration of a new asic-plugin in the system.
 *
 *    - find_asic_extension: gets the active asic-plugin interface through the
 *      interface parameter. If successful pointer is obtained, 0 is returned,
 *      EINVAL is returned otherwise.
 *
 *  Versioning
 *
 *  The guard against ABI breakage the following guidelines must be followed:
 *     - A Plugin will export via their public header their plugin name, major
 *       and minor numbers.
 *     - When another plugin is compiled to use its interface it will be compiled
 *       against the exported plugin name, mayor and minor numbers.
 *     - The interface minor number is increased if more items are added to the
 *       end of the interface structure.
 *     - No items can be added in the middle of the interface structure (or at
 *       least should be really discouraged).
 *     - The interface mayor number is increased if any parameters of existing
 *       functions are modified.
 *     - The find_ofproto_extension will enforce proper versioning, if no
 *       compatible match is found for the given parameters then a null result
 *       is returned.
 *
 *  Examples:
 *
 *     - Plugin that got compiled with version 1.2 can use an interface from a
 *       plugin running 1.3, it will just not have any visibility for the new items.
 *     - Plugin that got compiled with version 1.2 can not use an interface from
 *       a plugin running 1.1, as it doesn't have all the items it needs.
 *     - Plugin that got compiled with version 2.X can only use other plugins
 *       compiled with 2.X (and the same minor checking is also applied after that).
 *
 **/

#ifndef __OFPROTO_EXTENSIONS_H__
#define __OFPROTO_EXTENSIONS_H__

#include <stdint.h>
#include <stdbool.h>

/**
    @struct ofproto_extension_header
    @brief  Interface header structure, contains plugin version
    information as plugin_name, major and minor.
 */
struct ofproto_extension_header {
    const char *plugin_name; /**< ID of Plugin */
    int major;               /**< Major version */
    int minor;               /**< Minor version */
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
    @param[in] plugin_name The key for the hash table to find the respective
                           plugin interface
    @param[in] plugin_functions The pointer to the plugin's interface structure
 */
int register_ofproto_extension (const char *plugin_name, void *plugin_functions);

/**
    @fn unregister_ofproto_extension
    @brief  Unregistration of existing plugins, could be called from the plugin
    itself to delete its interface from the hash table
    @param[in] plugin_name The key for the hash table to find the respective
                           plugin interface
    @return 0 if success, errno value otherwise.
 */
int unregister_ofproto_extension (const char *plugin_name);

/**
    @fn find_ofproto_extension
    @brief  Lookup for registered interfaces, could be called either from a
    plugin or any other place. Allows to search for specific plugin functions.
    @param[in] plugin_name The key for the hash table to find the respective
                           plugin interface.
    @param[in] major The major value for the plugin expected version.
    @param[in] minr  The minor value for the plugin expected version.
    @param[out] plugin_interface Returns the found interface, NULL if not found.
                          You can pass the direction of your struct pointer:
       struct my_interface *intf;
       res = find_ofproto_extension(plugin_name, major, minor, (void *)intf );
       if (res == 0 && inft) ...
    @return 0 if success, errno value otherwise.
 */
int find_ofproto_extension (
        const char *plugin_name, int major, int minor, void **plugin_interface);

/**
    @fn register_asic_plugin_name
    @brief Called from all asics-plugins init function to register their
           plugin_name.
    @param[in] plugin_name The plugin_name of the caller asic plugin.
    @return 0 if success, errno value otherwise.
 */
int register_asic_plugin(const char *plugin_name);

/**
    @fn find_asic_extension
    @brief  Lookup for the current registered asic plugin. This call should be
    used after plugins registration and from any feature-plugin.
    @param[in] major The major value for the plugin expected version.
    @param[in] minr  The minor value for the plugin expected version.
    @param[out] plugin_interface Returns the found interface, NULL if not found.
                You can pass the direction of your struct pointer:
       struct my_asic_interface *asic_int;
       res = find_asic_extension(major, minor, (void *)asic_int );
       if (res == 0 && asic_int) ...
    @return 0 if success, errno value otherwise.
 */
int find_asic_extension(int major, int minor, void **plugin_interface);

#endif /*__OFPROTO_EXTENSIONS_H__*/
