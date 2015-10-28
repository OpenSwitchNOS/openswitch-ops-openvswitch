
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

#include "openvswitch/vlog.h"
#include "plugin-extensions.h"
#include "asic-plugin.h"

VLOG_DEFINE_THIS_MODULE(asic_plugin_module);

static struct asic_plugin_interface *asic_plugin = NULL;

int register_asic_plugin(struct asic_plugin_interface *plugin_interface)
{
    /* check that we don't already have registered an asic plugin */
    if (asic_plugin) {
        VLOG_ERR("There is already an asic-plugin registered.");
        goto error;
    }

    if (plugin_interface->major != ASIC_PLUGIN_INTERFACE_MAJOR){
        VLOG_ERR("Asic plugin MAJOR check failed. Plugin provided Major [%d] \
                  and required is [%d]",
                  plugin_interface->major, ASIC_PLUGIN_INTERFACE_MAJOR);
        goto error;
    }

    if (plugin_interface->minor < ASIC_PLUGIN_INTERFACE_MINOR) {
            VLOG_ERR("Asic Plugin MINOR check failed. Plugin provided Minor \
                      [%d] and required equal or greater than [%d]",
                      plugin_interface->minor, ASIC_PLUGIN_INTERFACE_MINOR);
            goto error;
    }

    asic_plugin = plugin_interface;
    VLOG_INFO("Registered ASIC Plugin [%s].", plugin_interface->plugin_name);
    return 0;

error:
    return EINVAL;
}

int find_asic_plugin(struct asic_plugin_interface **plugin_interface)
{
    if (!asic_plugin) {
        VLOG_ERR("No asic-plugin registered.");
        *plugin_interface = NULL;
        goto error;
    }

    *plugin_interface = asic_plugin;

    return 0;

error:
    return EINVAL;
}
