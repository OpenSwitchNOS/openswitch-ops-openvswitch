/*! \file ofproto_extensions.c
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
  int magic; /**< Key for the hash interface */
  int major; /**< Major number to check plugins version */
  int minor; /**< Minor number to check plugins version */
  void *ptr; /**< Pointer to plugin functions */
};

/**< Main hash with the interfaces of plugins */
static struct shash sh_extensions;

int ofproto_extensions_init(void)
{
  shash_init(&sh_extensions);
  if (sh_extensions.map.one != 0) {
    goto error;
  }
  return 0;

 error:
  return EPERM;
}

int register_ofproto_extension(int magic, void *ptr)
{
  struct plugin_extension_interface *ext;
  struct ofproto_extension_header *header = (struct ofproto_extension_header *)ptr;
  if (magic==0) {
    VLOG_ERR("[register_ofproto_extension] Error cannot add extention with null \
              magic\n");
    goto error;
  }

  if (ptr==NULL) {
    VLOG_ERR("[register_ofproto_extension] Error cannot add extention with null \
              ptr\n");
    goto error;
  }

  if (magic != header->magic) {
    VLOG_ERR("[register_ofproto_extension] Error magic and structure magic do \
              not match [0x%08x]!=[0x%08x]\n", magic, header->magic);
    goto error;
  }

  sprintf(s_magic, "%d", magic);
  ext = shash_find_data(&sh_extensions, s_magic);

  if (!ext) {
      ext = (struct plugin_extension_interface*)xmalloc(
                                     sizeof(struct plugin_extension_interface));
      if (!ext){
          goto error;
      }
      ext->magic = magic;
      ext->major = header->major;
      ext->minor = header->minor;
      ext->ptr = ptr;
      if(!shash_add_once(&sh_extensions, s_magic, ext)){
	free(ext);
	goto error;
      }
      return 0;
  }

  VLOG_ERR("[register_ofproto_extension] Error: there is already an extension \
            with the magic [0x%08x]\n", magic);
  goto error;

 error:
  return EINVAL;

}

int unregister_ofproto_extension(int magic)
{
    struct plugin_extension_interface *ext = NULL;
    sprintf(s_magic, "%d", magic);
    ext = shash_find_data(&sh_extensions, s_magic);

    if (ext) {
      shash_find_and_delete(&sh_extensions, s_magic);
      /* release memory used by struct */
      free(ext);
      return 0;
    }
    VLOG_ERR("[unregister_ofproto_extension] unable to find extension with \
              magic [0x%08x]\n", magic);
    return EINVAL;
}

int find_ofproto_extension(int magic, int major, int minor, void **interface)
{
    struct plugin_extension_interface *ext = NULL;
    sprintf(s_magic, "%d", magic);
    ext = shash_find_data(&sh_extensions, s_magic);

    if (ext) {
      VLOG_INFO("Found ofproto extension with magic \
                 [0x%08x] major [%d] minor [%d]\n",
                 ext->magic, ext->major, ext->minor);
      /* found a registered extension, now do some sanity checks */

      if (major != ext->major) {
        VLOG_ERR("[find_ofproto_extension] Error Found ofproto extension major \
                  check fails. Extension has major [%d] requested major [%d]\n",
                  ext->major, major);
        goto error;
      }

      if (minor > ext->minor) {
        VLOG_ERR("[find_ofproto_extension] Error Found ofproto extension minor \
                  check fails. Extension has minor [%d] requested minor [%d]\n",
                  ext->minor, minor);
        goto error;
      }

      *interface = ext->ptr;
      return 0;
    } else {
      VLOG_ERR("[find_ofproto_extension] unable to find requested ofproto \
                extension with magic [0x%08x]\n", magic);
      goto error;
    }

 error:
    return EINVAL;
}
