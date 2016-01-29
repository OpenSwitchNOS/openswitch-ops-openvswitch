/* Copyright (c) 2008, 2009, 2010, 2011, 2012, 2014 Nicira, Inc.
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

#ifndef RECONFIGURE_BLOCKS_H
#define RECONFIGURE_BLOCKS_H

#include <limits.h>
#include "list.h"

/* Code Functionality

   The reconfigure blocks is a concept apart from the plugins,
   (it's like a tool for plugin developers) and was designed to facilitate
   calling specific code of  feature plugins that should be otherwise,
   added manually to bridge.c file. The main idea is to keep bridge.c
   the cleanest possible and avoid dependencies between feature code.
   Each of this blocks have common functionality and dependencies,
   so, the developer can execute a plugin function within any block,
   just looking where does it fits the best.

   Blocks API

   - register_callback_function: registers a plugin function into a specified
     block. This function receives a priority level so the block is able to
     do an ordered execution of callbacks (NO_PRIORITY can be used when
     ordering is not important or needed).
     Also, developer is allowed to choose the block that better fits
     its sequence or to create a new block to be executed inside
     bridge.c file if the current blocks does not meet requirements.

   - execute_reconfigure_block: starts sequential execution of all the
     callbacks registered in the specified block. Parameter structure is the
     same for all blocks containing important enviroment variables as the
     ovsdb_dl, the ovsrec_open_vswitch and bridge information. Developers
     can add more variables in the structure if needed.

   - free_blocks: Frees the previously allocated memory for the block
     callbacks, lists and structures.
*/



#define NO_PRIORITY  UINT_MAX

typedef enum {
    BLK_INIT_UPDATE_DATA = 0,
    BLK_BR_UPDATE_PORTS,
    BLK_VRF_UPDATE_PORTS,
    BLK_BR_UPDATE_PORTS_STEP2,
    BLK_VRF_UPDATE_PORTS_STEP2,
    BLK_BR_UPDATE_PORTS_STEP3,
    BLK_VRF_UPDATE_PORTS_STEP3,
    BLK_BR_CFG,
    BLK_CFG_NEIGHBORS,
    BLK_CFG_NEIGHBORS_STEP2,
    /* Add more blocks here*/

    /* This enums allows to control the total number of blocks in the system.
     * If you want to add a new block, be sure to add it above this enum since
     * 'MAX_BLOCKS_NUM' needs to be always at the bottom of this list.
    */
    MAX_BLOCKS_NUM,

} Block_ID;


struct blk_params{
    const struct ovsrec_open_vswitch *ovs_cfg;
    struct ovsdb_idl *idl;
    struct bridge    *br;
    struct vrf       *vrf;
};

/* This structure represents a node of the function list for any block */
struct blk_list_node{
    void (*callback_handler)(struct blk_params*);
    unsigned int priority;
    struct ovs_list node;
};


int register_callback_function(void (*callback_handler)(struct blk_params*), Block_ID blkId,
                               unsigned int priority);
int execute_reconfigure_block(struct blk_params *params, Block_ID blk_id);
void free_blocks_list(void);

#endif /* reconfigure-blocks.h */
