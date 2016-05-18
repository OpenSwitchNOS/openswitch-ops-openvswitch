/*
 * (c) Copyright 2016 Hewlett Packard Enterprise Development LP
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

/* calculate hash string for Logical Switch table */

#include <config.h>
#include "logical-switch.h"
#include <inttypes.h>
#include <stdio.h>

void
logical_switch_hash(char* dest, unsigned int hash_len, const char *br_name,
                    const unsigned int tunnel_key)
{
    if((NULL != dest) && (NULL != br_name)) {
        /* Note for future implementation:
         * The hash should really be bridge.name+tunnel_type+tunnel_key */
        snprintf(dest, hash_len, "%s.%d", br_name, tunnel_key);
    }
}
