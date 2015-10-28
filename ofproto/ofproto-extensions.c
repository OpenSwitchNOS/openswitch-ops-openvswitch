/*! \file ofproto_extensions.c
*/

/**
 * Copyright (C) 2015 Hewlett-Packard Development Company, L.P.
 * Hewlett Packard Enterprise Development LP
 * All Rights Reserved.
 *
 * The contents of this software are proprietary and confidential
 * to the Hewlett Packard Enterprise Development LP. No part of
 * this program  may be photocopied, reproduced, or translated
 * into another programming language without prior written consent
 * of the Hewlett Packard Enterprise Development LP.
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

char s_magic[32];		/**< Holds a string of the magic number key */

/**
    @struct plugin_extension_interface
    @brief  Plugin interface structure, every plugin should register
    its own interface with pointers to internal functions.
*/
struct plugin_extension_interface {
  int magic;          /**< Key for the hash interface */
  int major;	      /**< Major number to check plugins version */
  int minor;	      /**< Minor number to check plugins version */
  void *ptr;	      /**< Pointer to plugin functions */
};

static struct shash sh_extensions; /**< Main hash with the interfaces of plugins */

int ofproto_extensions_init(void){
  shash_init(&sh_extensions);
  if (sh_extensions.map.one != 0)
    goto err_null_hash;
  return 0;

 err_null_hash:
  return EPERM;
}

int register_ofproto_extension(int magic, void *ptr) {
  struct plugin_extension_interface *ext;
  struct ofproto_extension_header *header = (struct ofproto_extension_header *)ptr;
  VLOG_INFO("[register_ofproto_extension] magic 0x%08x ptr %p\n", magic, ptr);
  if (magic==0) {
    VLOG_ERR("[register_ofproto_extension] Error cannot add extention with null magic\n");
    goto err_inval_param;
  }

  if (ptr==NULL) {
    VLOG_ERR("[register_ofproto_extension] Error cannot add extention with null ptr\n");
    goto err_inval_param;
  }

  if (magic != header->magic) {
    VLOG_ERR("[register_ofproto_extension] Error magic and structure magic do not match [0x%08x]!=[0x%08x]\n", magic, header->magic);
    goto err_inval_param;
  }

  VLOG_INFO("[register_ofproto_extension] with magic [0x%08x] major [%d] minor [%d] at [%p] \n", magic, header->major, header->minor, ptr);
  sprintf(s_magic, "%d", magic);
  ext = shash_find_data(&sh_extensions, s_magic);

  if (!ext) {
      ext = (struct plugin_extension_interface*)malloc(sizeof(struct plugin_extension_interface));
      memset(ext, 0, sizeof(struct plugin_extension_interface));
      ext->magic = magic;
      ext->major = header->major;
      ext->minor = header->minor;
      ext->ptr = ptr;
      //HASH_ADD_INT( extension_hashtable, magic, ext );
      shash_add_once(&sh_extensions, s_magic, ext);
      VLOG_INFO("[register_extension] registered extension with magic [0x%08x] major [%d] minor [%d] at [%p] \n", magic, header->major, header->minor, ptr);
      return 0;
  }

  VLOG_ERR("[register_ofproto_extension] Error: there is already an extension with the magic [0x%08x]\n", magic);
  goto err_inval_param;

 err_inval_param:
  return EINVAL;

}

int unregister_ofproto_extension(int magic) {
    struct plugin_extension_interface *ext = NULL;
    VLOG_INFO("[unregister_ofproto_extension] with magic [0x%08x]\n", magic);
    //HASH_FIND_INT( extension_hashtable, &magic, ext );
    sprintf(s_magic, "%d", magic);
    ext = shash_find_data(&sh_extensions, s_magic);

    if (ext) {
      //HASH_DEL( extension_hashtable, ext);  /* s: pointer to deletee */
      shash_find_and_delete(&sh_extensions, s_magic);
      // release memory used by struct
      free(ext);
      return 0;
    }
    VLOG_ERR("[unregister_ofproto_extension] unable to find extension with magic [0x%08x]\n", magic);
    return EINVAL;
}

int find_ofproto_extension(int magic, int major, int minor, void **interface) {
    struct plugin_extension_interface *ext = NULL;
    VLOG_INFO("[find_ofproto_extension] with magic [0x%08x] major [%d] minor [%d]\n", magic, major, minor);
    sprintf(s_magic, "%d", magic);
    ext = shash_find_data(&sh_extensions, s_magic);

    if (ext) {
      VLOG_INFO("[find_ofproto_extension] Found ofproto extension with magic [0x%08x] major [%d] minor [%d]\n", ext->magic, ext->major, ext->minor);
      // fount a registered extension, now do some sanity checks

      if (major != ext->major) {
        VLOG_ERR("[find_ofproto_extension] Error Found ofproto extension major check fails. Extension has major [%d] requested major [%d]\n", ext->major, major);
        goto err_inval_param;
      }

      if (minor > ext->minor) {
        VLOG_ERR("[find_ofproto_extension] Error Found ofproto extension minor check fails. Extension has minor [%d] requested minor [%d]\n", ext->minor, minor);
        goto err_inval_param;
      }

      // all ok
      *interface = ext->ptr;
      return 0;
    } else {
      VLOG_ERR("[find_ofproto_extension] unable to find requested ofproto extension with magic [0x%08x]\n", magic);
      goto err_inval_param;
    }

 err_inval_param:
    return EINVAL;
}

int dump_registered_ofproto_extensions(void) {
    struct plugin_extension_interface *ext;
    struct shash_node *node;

    SHASH_FOR_EACH (node, &sh_extensions) {
      ext = (struct plugin_extension_interface*) node->data;
      if (ext)
	VLOG_INFO("[dump_registered_ofproto_extensions] extension magic[0x%08x] major [%d] [%d] at ptr[%p]\n", ext->magic, ext->major, ext->minor, ext->ptr);
      else goto err_inv_ext;
    }

 err_inv_ext:
    VLOG_ERR("[dump_registered_ofproto_extensions] invalid extension");
    return EINVAL;
}
