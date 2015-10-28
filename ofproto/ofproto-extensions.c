/** @file ofproto_extensions.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#include "hash.h"
#include "shash.h"
#include "openvswitch/vlog.h"
#include "ofproto-extensions.h"

VLOG_DEFINE_THIS_MODULE(ofproto_extensions);

/**
 @struct plugin_extension_interface
 @brief  Plugin interface structure, every plugin should register
 its own interface with pointers to internal functions.
*/
struct plugin_extension_interface {
  const char* plugin_name; /**< Key for the hash interface */
  int major; /**< Major number to check plugins version */
  int minor; /**< Minor number to check plugins version */
  void *ptr; /**< Pointer to plugin functions */
};

static int init_done = 0;         /**< Holds the status boolean for hash_init */
static const char *asic_plugin_name;   /**< Holds the active asic-plugin name */
static struct shash sh_extensions; /**< Main hash with the interfaces of plugins */

int ofproto_extensions_init(void)
{
    if (init_done) {
        return 0;
    }
    init_done = 1;
    shash_init(&sh_extensions);
    if (sh_extensions.map.one != 0) {
        VLOG_ERR("Hash initialization failed");
        goto error;
    }
    return 0;

error:
    return EPERM;
}

int register_ofproto_extension(const char *plugin_name, void *ptr)
{
  struct plugin_extension_interface *ext = NULL;
  struct ofproto_extension_header *header = (struct ofproto_extension_header *)ptr;

  if (ofproto_extensions_init() != 0) {
    VLOG_ERR("Hash initialization error");
    goto error;
  }

  if (!plugin_name) {
    VLOG_ERR("Cannot add extention with null plugin_name");
    goto error;
  }

  if (ptr == NULL) {
    VLOG_ERR("Cannot add extention with null ptr");
    goto error;
  }

  if (strcmp(plugin_name,header->plugin_name) != 0) {
    VLOG_ERR("Plugin_name and structure plugin_name do not match \
              [%s]!=[%s]", plugin_name, header->plugin_name);
    goto error;
  }

  VLOG_INFO("Register plugin_name %s ptr %p",
             plugin_name, ptr);
  ext = shash_find_data(&sh_extensions, plugin_name);

  if (!ext) {
      ext = (struct plugin_extension_interface*)xmalloc(
                                     sizeof(struct plugin_extension_interface));
      if (!ext){
		  VLOG_ERR("Not enough memory for malloc.");
          goto error;
      }
      ext->plugin_name = plugin_name;
      ext->major = header->major;
      ext->minor = header->minor;
      ext->ptr = ptr;
      if(!shash_add_once(&sh_extensions, plugin_name, ext)){
	     free(ext);
	     VLOG_ERR("Failed to add plugin into hash table");
	     goto error;
      }
      return 0;
  } else {
	  VLOG_ERR("There is already an extension with the plugin_name [%s]\n",
            plugin_name);
      goto error;
  }

error:
    return EINVAL;
}

int unregister_ofproto_extension(const char *plugin_name)
{
    struct plugin_extension_interface *ext = NULL;
    ext = shash_find_data(&sh_extensions, plugin_name);

    if (ext) {
      if (!shash_find_and_delete(&sh_extensions, plugin_name)){
        VLOG_ERR("Unable to delete extension with plugin_name [%s]\n", plugin_name);
        goto error;
      }
      /* release memory used by struct */
      free(ext);
      return 0;
    }

error:
    return EINVAL;
}

int find_ofproto_extension(
                const char *plugin_name, int major, int minor, void **interface)
{
    struct plugin_extension_interface *ext = NULL;
    ext = shash_find_data(&sh_extensions, plugin_name);

    if (ext) {
      VLOG_INFO("Found ofproto extension with plugin_name \
                 [%s] major [%d] minor [%d]\n",
                 ext->plugin_name, ext->major, ext->minor);
      /* found a registered extension, now do some sanity checks */

      if (major != ext->major) {
        VLOG_ERR("Found ofproto extension major \
                  check fails. Extension has major [%d] requested major [%d]\n",
                  ext->major, major);
        goto error;
      }

      if (minor > ext->minor) {
        VLOG_ERR("Found ofproto extension minor \
                  check fails. Extension has minor [%d] requested minor [%d]\n",
                  ext->minor, minor);
        goto error;
      }

      *interface = ext->ptr;
      return 0;
    } else {
      VLOG_ERR("Unable to find requested ofproto \
                extension with plugin_name [%s]\n", plugin_name);
      goto error;
    }

error:
    return EINVAL;
}

int register_asic_plugin(const char *plugin_name)
{
    /* check that we don't already have registered an asic plugin */
    if (asic_plugin_name) {
        VLOG_ERR("Cannot register null plugin_name");
        goto error;
    }

    asic_plugin_name = plugin_name;
    VLOG_INFO("Registering plugin_name [%s]", plugin_name);
    return 0;

error:
    return EINVAL;
}

int find_asic_extension(int major, int minor, void **interface)
{
    if (!asic_plugin_name) {
        VLOG_ERR("No asic-plugin registered");
        goto error;
    }

    return find_ofproto_extension(asic_plugin_name, major, minor, interface);

error:
    return EINVAL;
}
