/* Copyright (C) 2015 Hewlett-Packard Development Company, L.P.
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

#include <arpa/inet.h>
#include <errno.h>
#include "vrf.h"
#include "hash.h"
#include "shash.h"
#include "ofproto/ofproto.h"
#include "openvswitch/vlog.h"
#include "openhalon-idl.h"

VLOG_DEFINE_THIS_MODULE(vrf);

extern struct ovsdb_idl *idl;
extern unsigned int idl_seqno;

/* == Managing routes == */
/* VRF maintains a per-vrf route hash of Routes->hash(Nexthop1, Nexthop2, ...) per-vrf.
 * VRF maintains a per-vrf nexthop hash with backpointer to the route entry.
 * The nexthop hash is only maintained for nexthops with IP address and not for
 * nexthops that point to interfaces. This hash is maintained so that when a
 * neighbor ARP gets resolved, we can quickly look up the route entry that has
 * a nexthop with the same IP as the neighbor that got resolved and update the
 * route entry in the system.
 *
 * When route is created, Route hash is updated with the new route and the list
 * of nexthops in the route. ofproto API is called to program this route and the
 * list of nexthops. Use the egress id and MAC resolved fields from the neighbor
 * hash for this nexthop. Also, nexthop hash entry is created with this route.
 *
 * When route is deleted, route hash and all its next hops are deleted. ofproto
 * API is called to delete this route from system. nexthops are also deleted from
 * the nexthop hash.
 *
 * When route is modified (means nexthops are added/deleted from the route),
 * route hash's nexthop list is updated and ofproto API is called to delete
 * and add the new nexthops being added.
 *
 * When neighbor entry is created (means a neighbor IP got MAC resolved), the
 * nexthop hash is searched for all nexthops that has the same IP as the neighbor
 * that got resolved and the routes associated with the nexthops are updated
 * in the system.
 *
 * When neighbor entry is deleted, all routes in the nexthop hash matching the
 * neighbor IP will be updated in ofproto with the route->nexthop marked as MAC
 * unresolved.
 *
 * Note: Nexthops are assumed to have either IP or port, but not both.
 */

/* determine if nexthop row is selected. Default is true */
static bool
vrf_is_nh_row_selected(const struct ovsrec_nexthop *nh_row)
{
    if (!nh_row->selected) { /* if not configured, default is true */
        return true;
    } else if (nh_row->selected[0]) { /* configured and value set as true */
        return true;
    }

    return false;
}

/* determine if route row is selected. Default is false */
static bool
vrf_is_route_row_selected(const struct ovsrec_route *route_row)
{
    if (route_row->selected && route_row->selected[0]) {
        /* configured and value set as true */
        return true;
    }
    return false;
}

static void
vrf_route_hash(char *from, char *prefix, char *hashstr, int hashlen)
{
    snprintf(hashstr, hashlen, "%s:%s", from, prefix);
}

static char *
vrf_nh_hash(char *ip_address, char *port_name)
{
    char *hashstr;
    if (ip_address) {
        hashstr = ip_address;
    } else {
        hashstr = port_name;
    }
    return hashstr;
}

/* Try and find the nexthop matching the db entry in the route->nexthops hash */
static struct nexthop *
vrf_route_nexthop_lookup(struct route *route, char *ip_address, char *port_name)
{
    char *hashstr;
    struct nexthop *nh;

    hashstr = vrf_nh_hash(ip_address, port_name);
    HMAP_FOR_EACH_WITH_HASH(nh, node, hash_string(hashstr, 0), &route->nexthops) {
        /* match either the ip address or the first port name */
        if ((nh->ip_addr && (strcmp(nh->ip_addr, ip_address) == 0)) ||
            ((nh->port_name && (strcmp(nh->port_name, port_name) == 0)))) {
            return nh;
        }
    }
    return NULL;
}

/* call ofproto API to add this route and nexthops */
static void
vrf_ofproto_route_add(struct vrf *vrf, struct ofproto_route *ofp_route,
                      struct route *route)
{

    int i;
    int rc = 0;
    struct nexthop *nh;

    ofp_route->family = route->is_ipv6 ? OFPROTO_ROUTE_IPV6 : OFPROTO_ROUTE_IPV4;
    ofp_route->prefix = route->prefix;

    if ((rc = vrf_l3_route_action(vrf, OFPROTO_ROUTE_ADD, ofp_route)) == 0) {
        VLOG_DBG("Route added for %s", route->prefix);
    } else {
        VLOG_ERR("Unable to add route for %s. rc %d", route->prefix, rc);
    }

    if (VLOG_IS_DBG_ENABLED()) {
        VLOG_DBG("--------------------------");
        VLOG_DBG("ofproto add route. family (%d), prefix (%s), nhs (%d)",
                  ofp_route->family, route->prefix, ofp_route->n_nexthops);
        for (i = 0; i < ofp_route->n_nexthops; i++) {
            VLOG_DBG("NH : state (%d), l3_egress_id (%d), rc (%d)",
                      ofp_route->nexthops[i].state,
                      ofp_route->nexthops[i].l3_egress_id,
                      ofp_route->nexthops[i].rc);
        }
        VLOG_DBG("--------------------------");
    }

    if (rc != 0) { /* return on error */
        return;
    }

    /* process the nexthop return code */
    for (i = 0; i < ofp_route->n_nexthops; i++) {
        if (ofp_route->nexthops[i].type == OFPROTO_NH_IPADDR) {
            nh = vrf_route_nexthop_lookup(route, ofp_route->nexthops[i].id, NULL);
        } else {
            nh = vrf_route_nexthop_lookup(route, NULL, ofp_route->nexthops[i].id);
        }
        if (nh && nh->idl_row) {
            struct smap nexthop_error;
            const char *error = smap_get(&nh->idl_row->status,
                                         OVSDB_NEXTHOP_STATUS_ERROR);

            if (ofp_route->nexthops[i].rc != 0) { /* ofproto error */
                smap_init(&nexthop_error);
                smap_add(&nexthop_error, OVSDB_NEXTHOP_STATUS_ERROR,
                         ofp_route->nexthops[i].err_str);
                VLOG_DBG("Update error status with '%s'",
                                            ofp_route->nexthops[i].err_str);
                ovsrec_nexthop_set_status(nh->idl_row, &nexthop_error);
                smap_destroy(&nexthop_error);
            } else { /* ofproto success */
                if (error) { /* some error was already set in db, clear it */
                    VLOG_DBG("Clear error status");
                    ovsrec_nexthop_set_status(nh->idl_row, NULL);
                }
            }
        }
        free(ofp_route->nexthops[i].id);
    }

}

/* call ofproto API to delete this route and nexthops */
static void
vrf_ofproto_route_delete(struct vrf *vrf, struct ofproto_route *ofp_route,
                         struct route *route, bool del_route)
{
    int i;
    int rc = 0;
    enum ofproto_route_action action;

    ofp_route->family = route->is_ipv6 ? OFPROTO_ROUTE_IPV6 : OFPROTO_ROUTE_IPV4;
    ofp_route->prefix = route->prefix;
    action = del_route ? OFPROTO_ROUTE_DELETE : OFPROTO_ROUTE_DELETE_NH;

    if ((rc = vrf_l3_route_action(vrf, action, ofp_route)) == 0) {
        VLOG_DBG("Route deleted for %s", route->prefix);
    } else {
        VLOG_ERR("Unable to delete route for %s. rc %d", route->prefix, rc);
    }
    for (i = 0; i < ofp_route->n_nexthops; i++) {
        free(ofp_route->nexthops[i].id);
    }

    if (VLOG_IS_DBG_ENABLED()) {
        VLOG_DBG("--------------------------");
        VLOG_DBG("ofproto delete route [%d] family (%d), prefix (%s), nhs (%d)",
                  del_route, ofp_route->family, route->prefix,
                  ofp_route->n_nexthops);
        for (i = 0; i < ofp_route->n_nexthops; i++) {
            VLOG_DBG("NH : state (%d), l3_egress_id (%d)",
                      ofp_route->nexthops[i].state,
                      ofp_route->nexthops[i].l3_egress_id);
        }
        VLOG_DBG("--------------------------");
    }
}

/* Update an ofproto route with the neighbor as [un]resolved. */
void
vrf_ofproto_update_route_with_neighbor(struct vrf *vrf,
                                       struct neighbor *neighbor, bool resolved)
{
    char *hashstr;
    struct nexthop *nh;
    struct ofproto_route ofp_route;

    VLOG_DBG("%s : neighbor %s, resolved : %d", __func__, neighbor->ip_address,
                                                resolved);
    hashstr = vrf_nh_hash(neighbor->ip_address, NULL);
    HMAP_FOR_EACH_WITH_HASH(nh, vrf_node, hash_string(hashstr, 0),
                            &vrf->all_nexthops) {
        /* match the neighbor's IP address */
        if (nh->ip_addr && (strcmp(nh->ip_addr, neighbor->ip_address) == 0)) {
            ofp_route.nexthops[0].state =
                        resolved ? OFPROTO_NH_RESOLVED : OFPROTO_NH_UNRESOLVED;
            if (resolved) {
                ofp_route.nexthops[0].l3_egress_id = neighbor->l3_egress_id;
            }
            ofp_route.nexthops[0].rc = 0;
            ofp_route.nexthops[0].type = OFPROTO_NH_IPADDR;
            ofp_route.nexthops[0].id = xstrdup(nh->ip_addr);
            ofp_route.n_nexthops = 1;
            vrf_ofproto_route_add(vrf, &ofp_route, nh->route);
        }
    }
}

/* populate the ofproto nexthop entry with information from the nh */
static void
vrf_ofproto_set_nh(struct vrf *vrf, struct ofproto_route_nexthop *ofp_nh,
                   struct nexthop *nh)
{
    struct neighbor *neighbor;

    ofp_nh->rc = 0;
    if (nh->port_name) { /* nexthop is a port */
        ofp_nh->state = OFPROTO_NH_UNRESOLVED;
        ofp_nh->type  = OFPROTO_NH_PORT;
        ofp_nh->id = xstrdup(nh->port_name);
        VLOG_DBG("%s : nexthop port : (%s)", __func__, nh->port_name);
    } else { /* nexthop has IP */
        ofp_nh->type  = OFPROTO_NH_IPADDR;
        neighbor = neighbor_hash_lookup(vrf, nh->ip_addr);
        if (neighbor) {
            ofp_nh->state = OFPROTO_NH_RESOLVED;
            ofp_nh->l3_egress_id = neighbor->l3_egress_id;
        } else {
            ofp_nh->state = OFPROTO_NH_UNRESOLVED;
        }
        ofp_nh->id = xstrdup(nh->ip_addr);
        VLOG_DBG("%s : nexthop IP : (%s), neighbor %s found", __func__,
                nh->ip_addr, neighbor ? "" : "not");
    }
}


/* Delete the nexthop from the route entry in the local cache */
static int
vrf_nexthop_delete(struct vrf *vrf, struct route *route, struct nexthop *nh)
{
    if (!route || !nh) {
        return -1;
    }

    VLOG_DBG("Cache delete NH %s/%s in route %s/%s",
              nh->ip_addr ? nh->ip_addr : "", nh->port_name ? nh->port_name : "",
              route->from, route->prefix);
    hmap_remove(&route->nexthops, &nh->node);
    if (nh->ip_addr) {
        hmap_remove(&vrf->all_nexthops, &nh->vrf_node);
        free(nh->ip_addr);
    }
    if (nh->port_name) {
        free(nh->port_name);
    }
    free(nh);

    return 0;
}

/* Add the nexthop into the route entry in the local cache */
static struct nexthop *
vrf_nexthop_add(struct vrf *vrf, struct route *route,
                const struct ovsrec_nexthop *nh_row)
{
    char *hashstr;
    struct nexthop *nh;

    if (!route || !nh_row) {
        return NULL;
    }

    nh = xzalloc(sizeof(*nh));
    /* NOTE: Either IP or Port, not both */
    if (nh_row->ip_address) {
        nh->ip_addr = xstrdup(nh_row->ip_address);
    } else if ((nh_row->n_ports > 0) && nh_row->ports[0]) {
        /* consider only one port for now */
        nh->port_name = xstrdup(nh_row->ports[0]->name);
    } else {
        VLOG_ERR("No IP address or port[0] in the nexthop entry");
        free(nh);
        return NULL;
    }
    nh->route = route;
    nh->idl_row = (struct ovsrec_nexthop *)nh_row;

    hashstr = nh_row->ip_address ? nh_row->ip_address : nh_row->ports[0]->name;
    hmap_insert(&route->nexthops, &nh->node, hash_string(hashstr, 0));
    if (nh_row->ip_address) { /* only add nexthops with IP address */
        hmap_insert(&vrf->all_nexthops, &nh->vrf_node, hash_string(hashstr, 0));
    }

    VLOG_DBG("Cache add NH %s/%s from route %s/%s",
              nh->ip_addr ? nh->ip_addr : "", nh->port_name ? nh->port_name : "",
              route->from, route->prefix);
    return nh;
}

/* find a route entry in local cache matching the prefix,from in IDL route row */
static struct route *
vrf_route_hash_lookup(struct vrf *vrf, const struct ovsrec_route *route_row)
{
    struct route *route;
    char hashstr[VRF_ROUTE_HASH_MAXSIZE];

    vrf_route_hash(route_row->from, route_row->prefix, hashstr, sizeof(hashstr));
    HMAP_FOR_EACH_WITH_HASH(route, node, hash_string(hashstr, 0), &vrf->all_routes) {
        if ((strcmp(route->prefix, route_row->prefix) == 0) &&
            (strcmp(route->from, route_row->from) == 0)) {
            return route;
        }
    }
    return NULL;
}

/* delete route entry from cache */
static void
vrf_route_delete(struct vrf *vrf, struct route *route)
{
    struct nexthop *nh, *next;
    struct ofproto_route ofp_route;

    if (!route) {
        return;
    }

    VLOG_DBG("Cache delete route %s/%s",
            route->from ? route->from : "", route->prefix ? route->prefix : "");
    hmap_remove(&vrf->all_routes, &route->node);

    ofp_route.n_nexthops = 0;
    HMAP_FOR_EACH_SAFE(nh, next, node, &route->nexthops) {
        vrf_ofproto_set_nh(vrf, &ofp_route.nexthops[ofp_route.n_nexthops], nh);
        if (vrf_nexthop_delete(vrf, route, nh) == 0) {
            ofp_route.n_nexthops++;
        }
    }
    if (ofp_route.n_nexthops > 0) {
        vrf_ofproto_route_delete(vrf, &ofp_route, route, true);
    }
    if (route->prefix) {
        free(route->prefix);
    }
    if (route->from) {
        free(route->from);
    }

    free(route);
}

/* Add the new route and its NHs into the local cache */
static void
vrf_route_add(struct vrf *vrf, const struct ovsrec_route *route_row)
{
    int i;
    struct route *route;
    struct nexthop *nh;
    const struct ovsrec_nexthop *nh_row;
    char hashstr[VRF_ROUTE_HASH_MAXSIZE];
    struct ofproto_route ofp_route;

    if (!route_row) {
        return;
    }

    route = xzalloc(sizeof(*route));
    route->prefix = xstrdup(route_row->prefix);
    route->from = xstrdup(route_row->from);
    if (route_row->address_family &&
        (strcmp(route_row->address_family, OVSREC_NEIGHBOR_ADDRESS_FAMILY_IPV6)
                                                                        == 0)) {
        route->is_ipv6 = true;
    }

    hmap_init(&route->nexthops);
    ofp_route.n_nexthops = 0;
    for (i = 0; i < route_row->n_nexthops; i++) {
        nh_row = route_row->nexthops[i];
        /* valid IP or valid port. consider only one port for now */
        if (vrf_is_nh_row_selected(nh_row) && (nh_row->ip_address ||
           ((nh_row->n_ports > 0) && nh_row->ports[0]))) {
            if ((nh = vrf_nexthop_add(vrf, route, nh_row))) {
                vrf_ofproto_set_nh(vrf, &ofp_route.nexthops[ofp_route.n_nexthops],
                                   nh);
                ofp_route.n_nexthops++;
            }
        }
    }
    if (ofp_route.n_nexthops > 0) {
        vrf_ofproto_route_add(vrf, &ofp_route, route);
    }

    route->vrf = vrf;
    route->idl_row = route_row;

    vrf_route_hash(route_row->from, route_row->prefix, hashstr, sizeof(hashstr));
    hmap_insert(&vrf->all_routes, &route->node, hash_string(hashstr, 0));

    VLOG_DBG("Cache add route %s/%s",
            route->from ? route->from : "", route->prefix ? route->prefix : "");
}

static void
vrf_route_modify(struct vrf *vrf, struct route *route,
                 const struct ovsrec_route *route_row)
{
    int i;
    char *nh_hash_str;
    struct nexthop *nh, *next;
    struct shash_node *shash_idl_nh;
    struct shash current_idl_nhs;   /* NHs in IDL for this route */
    const struct ovsrec_nexthop *nh_row;
    struct ofproto_route ofp_route;

    /* Look for added/deleted NHs in the route. Don't consider
     * modified NHs because the fields in NH we are interested in
     * (ip address, port) are not mutable in db.
     */

    /* collect current selected NHs in idl */
    shash_init(&current_idl_nhs);
    for (i = 0; i < route_row->n_nexthops; i++) {
        nh_row = route_row->nexthops[i];
        /* valid IP or valid port. consider only one port for now */
        if (vrf_is_nh_row_selected(nh_row) && (nh_row->ip_address ||
           ((nh_row->n_ports > 0) && nh_row->ports[0]))) {
            nh_hash_str = nh_row->ip_address ? nh_row->ip_address :
                          nh_row->ports[0]->name;
            if (!shash_add_once(&current_idl_nhs, nh_hash_str, nh_row)) {
                VLOG_DBG("nh %s specified twice", nh_hash_str);
            }
        }
    }
    SHASH_FOR_EACH(shash_idl_nh, &current_idl_nhs) {
        nh_row = shash_idl_nh->data;
        VLOG_DBG("DB Route %s/%s, nh_row %s", route->from, route->prefix,
                        nh_row->ip_address);
    }
    HMAP_FOR_EACH_SAFE(nh, next, node, &route->nexthops) {
        VLOG_DBG("Cached Route %s/%s, nh %s", route->from, route->prefix,
                    nh->ip_addr);
    }

    ofp_route.n_nexthops = 0;
    /* delete nexthops that got deleted from db */
    HMAP_FOR_EACH_SAFE(nh, next, node, &route->nexthops) {
        nh_hash_str = nh->ip_addr ? nh->ip_addr : nh->port_name;
        nh->idl_row = shash_find_data(&current_idl_nhs, nh_hash_str);
        if (!nh->idl_row) {
            vrf_ofproto_set_nh(vrf, &ofp_route.nexthops[ofp_route.n_nexthops],
                               nh);
            if (vrf_nexthop_delete(vrf, route, nh) == 0) {
                ofp_route.n_nexthops++;
            }
        }
    }
    if (ofp_route.n_nexthops > 0) {
        vrf_ofproto_route_delete(vrf, &ofp_route, route, false);
    }

    ofp_route.n_nexthops = 0;
    /* add new nexthops that got added in db */
    SHASH_FOR_EACH(shash_idl_nh, &current_idl_nhs) {
        nh_row = shash_idl_nh->data;
        nh = vrf_route_nexthop_lookup(route, nh_row->ip_address,
                nh_row->n_ports > 0 ? nh_row->ports[0]->name : NULL);
        if (!nh) {
            if ((nh = vrf_nexthop_add(vrf, route, nh_row))) {
                vrf_ofproto_set_nh(vrf, &ofp_route.nexthops[ofp_route.n_nexthops],
                                   nh);
                ofp_route.n_nexthops++;
            }
        }
    }
    if (ofp_route.n_nexthops > 0) {
        vrf_ofproto_route_add(vrf, &ofp_route, route);
    }

    shash_destroy(&current_idl_nhs);
}

void
vrf_reconfigure_routes(struct vrf *vrf)
{
    struct route *route, *next;
    struct shash current_idl_routes;
    struct shash_node *shash_route_row;
    char route_hash_str[VRF_ROUTE_HASH_MAXSIZE];
    const struct ovsrec_route *route_row = NULL, *route_row_local = NULL;

    if (!vrf_has_l3_route_action(vrf)) {
        VLOG_DBG("No ofproto support for route management.");
        return;
    }

    route_row = ovsrec_route_first(idl);
    if (!route_row) {
        /* May be all routes got deleted, cleanup if any in this vrf hash */
        HMAP_FOR_EACH_SAFE (route, next, node, &vrf->all_routes) {
            vrf_route_delete(vrf, route);
        }
        return;
    }

    if ((!OVSREC_IDL_ANY_TABLE_ROWS_MODIFIED(route_row, idl_seqno)) &&
        (!OVSREC_IDL_ANY_TABLE_ROWS_DELETED(route_row, idl_seqno))  &&
        (!OVSREC_IDL_ANY_TABLE_ROWS_INSERTED(route_row, idl_seqno)) ) {
        return;
    }

    /* Collect all selected routes of this vrf */
    shash_init(&current_idl_routes);
    OVSREC_ROUTE_FOR_EACH(route_row, idl) {
        if (vrf_is_route_row_selected(route_row) &&
            strcmp(vrf->cfg->name, route_row->vrf->name) == 0) {
            vrf_route_hash(route_row->from, route_row->prefix,
                           route_hash_str, sizeof(route_hash_str));
            if (!shash_add_once(&current_idl_routes, route_hash_str,
                                route_row)) {
                VLOG_DBG("route %s specified twice", route_hash_str);
            }
        }
    }

    /* dump db and local cache */
    SHASH_FOR_EACH(shash_route_row, &current_idl_routes) {
        route_row_local = shash_route_row->data;
        VLOG_DBG("route in db '%s/%s'", route_row_local->from,
                                        route_row_local->prefix);
    }
    HMAP_FOR_EACH_SAFE(route, next, node, &vrf->all_routes) {
        VLOG_DBG("route in cache '%s/%s'", route->from, route->prefix);
    }

    route_row = ovsrec_route_first(idl);
    if (OVSREC_IDL_ANY_TABLE_ROWS_DELETED(route_row, idl_seqno)) {
        /* Delete the routes that are deleted from the db */
        HMAP_FOR_EACH_SAFE(route, next, node, &vrf->all_routes) {
            vrf_route_hash(route->from, route->prefix,
                           route_hash_str, sizeof(route_hash_str));
            route->idl_row = shash_find_data(&current_idl_routes, route_hash_str);
            if (!route->idl_row) {
                vrf_route_delete(vrf, route);
            }
        }
    }

    if (OVSREC_IDL_ANY_TABLE_ROWS_INSERTED(route_row, idl_seqno)) {
        /* Add new routes. We have the routes of interest in current_idl_routes */
        SHASH_FOR_EACH(shash_route_row, &current_idl_routes) {
            route_row_local = shash_route_row->data;
            route = vrf_route_hash_lookup(vrf, route_row_local);
            if (!route) {
                vrf_route_add(vrf, route_row_local);
            }
        }
    }

    /* Look for any modification of this route */
    if (OVSREC_IDL_ANY_TABLE_ROWS_MODIFIED(route_row, idl_seqno)) {
        OVSREC_ROUTE_FOR_EACH(route_row, idl) {
            if ((strcmp(vrf->cfg->name, route_row->vrf->name) == 0) &&
                (OVSREC_IDL_IS_ROW_MODIFIED(route_row, idl_seqno)) &&
                !(OVSREC_IDL_IS_ROW_INSERTED(route_row, idl_seqno))) {

               route = vrf_route_hash_lookup(vrf, route_row);
               if (vrf_is_route_row_selected(route_row)) {
                    if (route) {
                        vrf_route_modify(vrf, route, route_row);
                    } else {
                        /* maybe the route was unselected earlier and got
                         * selected now. it wouldn't be in our cache */
                        vrf_route_add(vrf, route_row);
                    }
                } else {
                    if (route) { /* route got unselected, delete from cache */
                        vrf_route_delete(vrf, route);
                    }
                }

            }
        }
    }
    shash_destroy(&current_idl_routes);

    /* dump our cache */
    if (VLOG_IS_DBG_ENABLED()) {
        struct nexthop *nh = NULL, *next_nh = NULL;
        HMAP_FOR_EACH_SAFE(route, next, node, &vrf->all_routes) {
            VLOG_DBG("Route : %s/%s", route->from, route->prefix);
            HMAP_FOR_EACH_SAFE(nh, next_nh, node, &route->nexthops) {
                VLOG_DBG("  NH : '%s/%s' ",
                         nh->ip_addr ? nh->ip_addr : "",
                         nh->port_name ? nh->port_name : "");
            }
        }
        HMAP_FOR_EACH_SAFE(nh, next_nh, vrf_node, &vrf->all_nexthops) {
            VLOG_DBG("VRF NH : '%s' -> Route '%s/%s'",
                    nh->ip_addr ? nh->ip_addr : "",
                    nh->route->from, nh->route->prefix);
        }
    }
    /* OPS_TODO : for port deletion, delete all routes in ofproto that has
     * NH as the deleted port. */
    /* OPS_TODO : for VRF deletion, delete all routes in ofproto that has
     * NH as any of the ports in the deleted VRF */
}

/* OPS_TODO : move vrf functions from bridge.c to this file */
/* OPS_TODO : move neighbor functions from bridge.c to this file */