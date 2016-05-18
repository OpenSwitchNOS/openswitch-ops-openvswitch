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
#ifndef LOGICAL_SWITCH_H
#define LOGICAL_SWITCH_H 1

#ifdef  __cplusplus
extern "C" {
#endif

#define LSWITCH_HASH_STR_SIZE                  MAX_INPUT

/*
 * logical_switch_hash
 *
 * calculate hash string for Logical Switch table
 *
 * @param dset       pointer to string to return hash string for Logical Switch table.
 * @param hash_len   hash string length
 * @param br_name    bridge name
 * @param tunnel_key tunnel key.
 *
 * @return none
 */
void
logical_switch_hash(char* dest, const unsigned int hash_len, const char *br_name,
                    const unsigned int tunnel_key);

#ifdef  __cplusplus
}
#endif

#endif /* LOGICAL_SWITCH_H */
