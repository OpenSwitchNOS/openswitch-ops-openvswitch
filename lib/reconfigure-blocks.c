/* Copyright (c) 2008, 2009, 2010, 2011, 2012, 2013, 2014 Nicira, Inc.
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

#include <stdlib.h>
#include <errno.h>
#include "reconfigure-blocks.h"
#include "openvswitch/vlog.h"
#include "vswitchd/bridge.h"
#include "vswitchd/vrf.h"
#include "vswitch-idl.h"

VLOG_DEFINE_THIS_MODULE(blocks);

static bool blocks_init = false;
static struct ovs_list** blk_list = NULL;

static int init_reconfigure_blocks(void);
static void insert_node_on_blk(struct blk_list_node *new_node,
                               struct ovs_list *func_list);


/* This function registers the passed 'callback_handler' function to the
 * specified block depending on the priority given by the user. A priority
 * value of 'NO_PRIORITY' means that callback_handler should be registered
 * at the end of the block.
 */
int
register_callback_function(void (*callback_handler)(struct blk_params*),
                           Block_ID blk_id, unsigned int priority)
{
    struct blk_list_node *new_node = NULL;

    /* Initialize reconfigure blocks if are not initialized yet */
    if (!blocks_init) {
        if(init_reconfigure_blocks()) {
            VLOG_ERR("Cannot initialize blocks");
            goto error;
        }
    }

    if (callback_handler == NULL) {
        VLOG_ERR("NULL callback function");
        goto error;
    }

    if ((blk_id < 0) || (blk_id >= MAX_BLOCKS_NUM)) {
        VLOG_ERR("Invalid blk_id passed as parameter");
        goto error;
    }

    new_node = (struct blk_list_node *) xmalloc (sizeof(struct blk_list_node));
    new_node->callback_handler = callback_handler;
    new_node->priority = priority;
    insert_node_on_blk(new_node, blk_list[blk_id]);
    return 0;

error:
    return EINVAL;
}

/* Insert a new element into the list passed 'func_list' acording to its priority
 * this function was written asumming that priority is incremental
 */
static void
insert_node_on_blk(struct blk_list_node *new_node, struct ovs_list *func_list)
{
    struct blk_list_node *blk_node = NULL;
    struct ovs_list *last_node;

    /* check empty list */
    if (list_is_empty(func_list)) {
        list_push_back(func_list, &new_node->node);
        return;
    }

    /* check last elemnt of list */
    last_node = list_back(func_list);
    blk_node = CONTAINER_OF(last_node, struct blk_list_node, node);
    if ((new_node->priority) > (blk_node->priority)) {
        list_push_back(func_list, &new_node->node);
        return;
    }

    /* add elemnt in between nodes */
    LIST_FOR_EACH(blk_node, node, func_list) {
        if ((blk_node->priority) > (new_node->priority)) {
            list_insert(&blk_node->node, &new_node->node);
            break;
        }
    }
}

/* Initialize list of blocks, this is to easily call the insert_node_on_blk() from
 * register_callback_on_blk() and the execute functions using the enum as index.
 */
static int
init_reconfigure_blocks(void)
{
    int blk_counter;
    blk_list = (struct ovs_list**) xcalloc (MAX_BLOCKS_NUM,
                                            sizeof(struct ovs_list*));

    /* Initialize each of the Blocks */
    for (blk_counter = 0; blk_counter < MAX_BLOCKS_NUM; blk_counter++) {
        blk_list[blk_counter] = (struct ovs_list *) xmalloc (sizeof(struct ovs_list));
        list_init(blk_list[blk_counter]);
    }

    blocks_init = true;
    return 0;
}

/* This function executes all the callback_handler functions previously registered for
 * the specified block. The execution is sequential in the order given by the
 * registered priorities for each callback_handler function.
*/
int
execute_reconfigure_block(struct blk_params *params, Block_ID blk_id)
{
    struct blk_list_node *actual_node;

    /* Initialize reconfigure blocks if are not initialized yet */
    if (!blocks_init) {
        if(init_reconfigure_blocks()) {
            VLOG_ERR("Cannot initialize blocks");
            goto error;
        }
    }

    if (!params) {
        VLOG_ERR("Invalid NULL params structure");
        goto error;
    }

    if ((blk_id < 0) || (blk_id >= MAX_BLOCKS_NUM)) {
        VLOG_ERR("Invalid blk_id passed as parameter");
        goto error;
    }

    VLOG_INFO("Executing block %d of bridge reconfigure", blk_id);

    LIST_FOR_EACH(actual_node, node, blk_list[blk_id]) {
        if (!actual_node->callback_handler) {
            VLOG_ERR("Invalid function callback_handler found");
            goto error;
        }
        actual_node->callback_handler(params);
    }

    return 0;

 error:
    return EINVAL;
}

/* This functions releases the memory previously allocated for the list of
   blocks. Useful on external calls to release the whole list memory.
 */
void
free_blocks_list(void)
{
    struct blk_list_node *blk_node;
    int blk_counter;

    for (blk_counter = 0; blk_counter < MAX_BLOCKS_NUM; blk_counter++) {
        if (blk_list[blk_counter]) {
            LIST_FOR_EACH(blk_node, node, blk_list[blk_counter]) {
                if (blk_node) {
                    free(blk_node);
                }
            }
            free(blk_list[blk_counter]);
        }
    }
    free(blk_list);
}
