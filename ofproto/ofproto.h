/*
 * Copyright (c) 2009, 2010, 2011, 2012, 2013, 2014 Nicira, Inc.
 * Copyright (C) 2015-2016 Hewlett-Packard Development Company, L.P.
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

#ifndef OFPROTO_H
#define OFPROTO_H 1

#include <sys/types.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "cfm.h"
#include "classifier.h"
#include "flow.h"
#include "meta-flow.h"
#include "netflow.h"
#include "rstp.h"
#include "smap.h"
#include "sset.h"
#include "stp.h"
#include "lacp.h"

#ifdef  __cplusplus
extern "C" {
#endif

struct bfd_cfg;
struct cfm_settings;
struct cls_rule;
struct netdev;
struct netdev_stats;
struct ofport;
struct ofproto;
struct shash;
struct simap;

/* Needed for the lock annotations. */
extern struct ovs_mutex ofproto_mutex;

struct ofproto_controller_info {
    bool is_connected;
    enum ofp12_controller_role role;
    struct smap pairs;
};

struct ofproto_sflow_options {
    struct sset targets;
    uint32_t sampling_rate;
    uint32_t polling_interval;
    uint32_t header_len;
    uint32_t sub_id;
    char *agent_device;
    char *control_ip;
};

struct ofproto_ipfix_bridge_exporter_options {
    struct sset targets;
    uint32_t sampling_rate;
    uint32_t obs_domain_id;  /* Bridge-wide Observation Domain ID. */
    uint32_t obs_point_id;  /* Bridge-wide Observation Point ID. */
    uint32_t cache_active_timeout;
    uint32_t cache_max_flows;
    bool enable_tunnel_sampling;
    bool enable_input_sampling;
    bool enable_output_sampling;
};

struct ofproto_ipfix_flow_exporter_options {
    uint32_t collector_set_id;
    struct sset targets;
    uint32_t cache_active_timeout;
    uint32_t cache_max_flows;
};

struct ofproto_rstp_status {
    bool enabled;               /* If false, ignore other members. */
    rstp_identifier root_id;
    rstp_identifier bridge_id;
    rstp_identifier designated_id;
    uint32_t root_path_cost;
    uint16_t designated_port_id;
    uint16_t bridge_port_id;
};

struct ofproto_rstp_settings {
    rstp_identifier address;
    uint16_t priority;
    uint32_t ageing_time;
    enum rstp_force_protocol_version force_protocol_version;
    uint16_t bridge_forward_delay;
    uint16_t bridge_max_age;
    uint16_t transmit_hold_count;
};

struct ofproto_port_rstp_status {
    bool enabled;               /* If false, ignore other members. */
    uint16_t port_id;
    enum rstp_port_role role;
    enum rstp_state state;
    rstp_identifier designated_bridge_id;
    uint16_t designated_port_id;
    uint32_t designated_path_cost;
    int tx_count;               /* Number of BPDUs transmitted. */
    int rx_count;               /* Number of valid BPDUs received. */
    int error_count;            /* Number of bad BPDUs received. */
    int uptime;
};

struct ofproto_port_rstp_settings {
    bool enable;
    uint16_t port_num;           /* In the range 1-4095, inclusive. */
    uint8_t priority;
    uint32_t path_cost;
    bool admin_edge_port;
    bool auto_edge;
    bool mcheck;
    uint8_t admin_p2p_mac_state;
    bool admin_port_state;
};

struct ofproto_stp_settings {
    stp_identifier system_id;
    uint16_t priority;
    uint16_t hello_time;
    uint16_t max_age;
    uint16_t fwd_delay;
};

struct ofproto_stp_status {
    bool enabled;               /* If false, ignore other members. */
    stp_identifier bridge_id;
    stp_identifier designated_root;
    int root_path_cost;
};

struct ofproto_port_stp_settings {
    bool enable;
    uint8_t port_num;           /* In the range 1-255, inclusive. */
    uint8_t priority;
    uint16_t path_cost;
};

struct ofproto_port_stp_status {
    bool enabled;               /* If false, ignore other members. */
    int port_id;
    enum stp_state state;
    unsigned int sec_in_state;
    enum stp_role role;
};

struct ofproto_port_stp_stats {
    bool enabled;               /* If false, ignore other members. */
    int tx_count;               /* Number of BPDUs transmitted. */
    int rx_count;               /* Number of valid BPDUs received. */
    int error_count;            /* Number of bad BPDUs received. */
};

struct ofproto_port_queue {
    uint32_t queue;             /* Queue ID. */
    uint8_t dscp;               /* DSCP bits (e.g. [0, 63]). */
};

struct ofproto_mcast_snooping_settings {
    bool flood_unreg;           /* If true, flood unregistered packets to all
                                   all ports. If false, send only to ports
                                   connected to multicast routers. */
    unsigned int idle_time;     /* Entry is removed after the idle time
                                 * in seconds. */
    unsigned int max_entries;   /* Size of the multicast snooping table. */
};

/* How the switch should act if the controller cannot be contacted. */
enum ofproto_fail_mode {
    OFPROTO_FAIL_SECURE,        /* Preserve flow table. */
    OFPROTO_FAIL_STANDALONE     /* Act as a standalone switch. */
};

enum ofproto_band {
    OFPROTO_IN_BAND,            /* In-band connection to controller. */
    OFPROTO_OUT_OF_BAND         /* Out-of-band connection to controller. */
};

struct ofproto_controller {
    char *target;               /* e.g. "tcp:127.0.0.1" */
    int max_backoff;            /* Maximum reconnection backoff, in seconds. */
    int probe_interval;         /* Max idle time before probing, in seconds. */
    enum ofproto_band band;     /* In-band or out-of-band? */
    bool enable_async_msgs;     /* Initially enable asynchronous messages? */

    /* OpenFlow packet-in rate-limiting. */
    int rate_limit;             /* Max packet-in rate in packets per second. */
    int burst_limit;            /* Limit on accumulating packet credits. */

    uint8_t dscp;               /* DSCP value for controller connection. */
};

#ifdef OPS

/* FIXME: Use MAX_NEXTHOPS_PER_ROUTE from common header */
#define OFPROTO_MAX_NH_PER_ROUTE    32 /* maximum number of nexthops per route.
                                          only consider non-weighted ECMP now */
enum ofproto_route_family {
    OFPROTO_ROUTE_IPV4,
    OFPROTO_ROUTE_IPV6
};

enum ofproto_route_action {
    OFPROTO_ROUTE_ADD,
    OFPROTO_ROUTE_DELETE,
    OFPROTO_ROUTE_DELETE_NH
};

enum ofproto_nexthop_state {
    OFPROTO_NH_UNRESOLVED,
    OFPROTO_NH_RESOLVED
};

enum ofproto_nexthop_type {
    OFPROTO_NH_IPADDR,
    OFPROTO_NH_PORT
};

struct ofproto_route_nexthop {
    char *id;                         /* IP address or Port name */
    enum ofproto_nexthop_type type;
    enum ofproto_nexthop_state state; /* is arp resolved for this next hop */
    int  rc;                          /* rc = 0 means success */
    const char *err_str;              /* set if rc != 0 */
    int  l3_egress_id;
};

struct ofproto_route {
    enum ofproto_route_family family;
    char *prefix;

    uint8_t  n_nexthops;              /* number of nexthops */
    struct ofproto_route_nexthop nexthops[OFPROTO_MAX_NH_PER_ROUTE]; /* nexthops */
};

/* ECMP hash bit-fields */
#define OFPROTO_ECMP_HASH_SRCPORT        0x1     /* destination L4 port */
#define OFPROTO_ECMP_HASH_DSTPORT        0x2     /* source L4 port */
#define OFPROTO_ECMP_HASH_SRCIP          0x4     /* source IP v4/v6 */
#define OFPROTO_ECMP_HASH_DSTIP          0x8     /* source IP v4/v6 */

enum ofproto_host_action {
    OFPROTO_HOST_ADD,
    OFPROTO_HOST_DELETE,
    OFPROTO_NEIGHBOR_ADD,
    OFPROTO_NEIGHBOR_MODIFY,
    OFPROTO_NEIGHBOR_DELETE
};

struct ofproto_l3_host {
    bool family;                      /* Type of host */
    char *ip_address;                 /* V4/6 IP address (prefix/len)*/
    int  rc;                          /* rc = 0 means success */
    const char *err_str;              /* set if rc != 0 */
    char *mac;                        /* These are for neighbor, mac */
    int  l3_egress_id;                /* Egress ID in case if we need */
};

/* QOS */
/* In System or Port table, possible values in qos_enum_config column. */
enum qos_trust {
    QOS_TRUST_NONE = 0,
    QOS_TRUST_COS,
    QOS_TRUST_DSCP,
    QOS_TRUST_MAX /* Used for validation only! */
};

/* collection of parameters to set_port_qos_cfg API */
struct qos_port_settings {
    enum qos_trust qos_trust;
    const struct smap *other_config;
};

/* in QoS_DSCP_Map or QoS_COS_Map, possibible values for color column */
enum cos_color {
    COS_COLOR_GREEN = 0,
    COS_COLOR_YELLOW,
    COS_COLOR_RED,
    COS_COLOR_MAX
};

/* single row from QoS_DSCP_Map table */
struct dscp_map_entry {
    enum cos_color  color;
    int codepoint;
    int local_priority;
    int cos;
    struct smap *other_config;
};

/* 1 or more rows in QoS_DSCP_Map passed to set_dscp_map API */
struct dscp_map_settings {
    int n_entries;
    struct dscp_map_entry *entries;   /* array of 'struct dscp_map_entry' */
};

/* single row from QoS_COS_Map table */
struct cos_map_entry {
    enum cos_color color;
    int codepoint;
    int local_priority;
    struct smap *other_config;
};

/* 1 or more rows in QoS_COS_Map passed to set_cos_map API */
struct cos_map_settings {
    int n_entries;
    struct cos_map_entry *entries;   /* array of 'struct cos_map_entry' */
};

#endif

void ofproto_enumerate_types(struct sset *types);
const char *ofproto_normalize_type(const char *);

int ofproto_enumerate_names(const char *type, struct sset *names);
void ofproto_parse_name(const char *name, char **dp_name, char **dp_type);

/* An interface hint element, which is used by ofproto_init() to
 * describe the caller's understanding of the startup state. */
struct iface_hint {
    char *br_name;              /* Name of owning bridge. */
    char *br_type;              /* Type of owning bridge. */
    ofp_port_t ofp_port;        /* OpenFlow port number. */
};

void ofproto_init(const struct shash *iface_hints);

int ofproto_type_run(const char *datapath_type);
void ofproto_type_wait(const char *datapath_type);

int ofproto_create(const char *datapath, const char *datapath_type,
                   struct ofproto **ofprotop);
void ofproto_destroy(struct ofproto *);
int ofproto_delete(const char *name, const char *type);

int ofproto_run(struct ofproto *);
void ofproto_wait(struct ofproto *);
bool ofproto_is_alive(const struct ofproto *);

void ofproto_get_memory_usage(const struct ofproto *, struct simap *);
void ofproto_type_get_memory_usage(const char *datapath_type, struct simap *);

/* A port within an OpenFlow switch.
 *
 * 'name' and 'type' are suitable for passing to netdev_open(). */
struct ofproto_port {
    char *name;                 /* Network device name, e.g. "eth0". */
    char *type;                 /* Network device type, e.g. "system". */
    ofp_port_t ofp_port;        /* OpenFlow port number. */
};
void ofproto_port_clone(struct ofproto_port *, const struct ofproto_port *);
void ofproto_port_destroy(struct ofproto_port *);

struct ofproto_port_dump {
    const struct ofproto *ofproto;
    int error;
    void *state;
};
void ofproto_port_dump_start(struct ofproto_port_dump *,
                             const struct ofproto *);
bool ofproto_port_dump_next(struct ofproto_port_dump *, struct ofproto_port *);
int ofproto_port_dump_done(struct ofproto_port_dump *);

/* Iterates through each OFPROTO_PORT in OFPROTO, using DUMP as state.
 *
 * Arguments all have pointer type.
 *
 * If you break out of the loop, then you need to free the dump structure by
 * hand using ofproto_port_dump_done(). */
#define OFPROTO_PORT_FOR_EACH(OFPROTO_PORT, DUMP, OFPROTO)  \
    for (ofproto_port_dump_start(DUMP, OFPROTO);            \
         (ofproto_port_dump_next(DUMP, OFPROTO_PORT)        \
          ? true                                            \
          : (ofproto_port_dump_done(DUMP), false));         \
        )

#define OFPROTO_FLOW_LIMIT_DEFAULT 200000
#define OFPROTO_MAX_IDLE_DEFAULT 10000 /* ms */

const char *ofproto_port_open_type(const char *datapath_type,
                                   const char *port_type);
int ofproto_port_add(struct ofproto *, struct netdev *, ofp_port_t *ofp_portp);
int ofproto_port_del(struct ofproto *, ofp_port_t ofp_port);
int ofproto_port_get_stats(const struct ofport *, struct netdev_stats *stats);

int ofproto_port_query_by_name(const struct ofproto *, const char *devname,
                               struct ofproto_port *);

/* Top-level configuration. */
uint64_t ofproto_get_datapath_id(const struct ofproto *);
void ofproto_set_datapath_id(struct ofproto *, uint64_t datapath_id);
void ofproto_set_controllers(struct ofproto *,
                             const struct ofproto_controller *, size_t n,
                             uint32_t allowed_versions);
void ofproto_set_fail_mode(struct ofproto *, enum ofproto_fail_mode fail_mode);
void ofproto_reconnect_controllers(struct ofproto *);
void ofproto_set_extra_in_band_remotes(struct ofproto *,
                                       const struct sockaddr_in *, size_t n);
void ofproto_set_in_band_queue(struct ofproto *, int queue_id);
void ofproto_set_flow_limit(unsigned limit);
void ofproto_set_max_idle(unsigned max_idle);
void ofproto_set_forward_bpdu(struct ofproto *, bool forward_bpdu);
void ofproto_set_mac_table_config(struct ofproto *, unsigned idle_time,
                                  size_t max_entries);
int ofproto_set_mcast_snooping(struct ofproto *ofproto,
                              const struct ofproto_mcast_snooping_settings *s);
int ofproto_port_set_mcast_snooping(struct ofproto *ofproto, void *aux,
                                    bool flood);
void ofproto_set_threads(int n_handlers, int n_revalidators);
void ofproto_set_n_dpdk_rxqs(int n_rxqs);
void ofproto_set_cpu_mask(const char *cmask);
void ofproto_set_dp_desc(struct ofproto *, const char *dp_desc);
int ofproto_set_snoops(struct ofproto *, const struct sset *snoops);
int ofproto_set_netflow(struct ofproto *,
                        const struct netflow_options *nf_options);
int ofproto_set_sflow(struct ofproto *, const struct ofproto_sflow_options *);
int ofproto_set_ipfix(struct ofproto *,
                      const struct ofproto_ipfix_bridge_exporter_options *,
                      const struct ofproto_ipfix_flow_exporter_options *,
                      size_t);
void ofproto_set_flow_restore_wait(bool flow_restore_wait_db);
bool ofproto_get_flow_restore_wait(void);
int ofproto_set_stp(struct ofproto *, const struct ofproto_stp_settings *);
int ofproto_get_stp_status(struct ofproto *, struct ofproto_stp_status *);

int ofproto_set_rstp(struct ofproto *, const struct ofproto_rstp_settings *);
int ofproto_get_rstp_status(struct ofproto *, struct ofproto_rstp_status *);

/* Configuration of ports. */
void ofproto_port_unregister(struct ofproto *, ofp_port_t ofp_port);

void ofproto_port_clear_cfm(struct ofproto *, ofp_port_t ofp_port);
void ofproto_port_set_cfm(struct ofproto *, ofp_port_t ofp_port,
                          const struct cfm_settings *);
void ofproto_port_set_bfd(struct ofproto *, ofp_port_t ofp_port,
                          const struct smap *cfg);
bool ofproto_port_bfd_status_changed(struct ofproto *, ofp_port_t ofp_port);
int ofproto_port_get_bfd_status(struct ofproto *, ofp_port_t ofp_port,
                                struct smap *);
int ofproto_port_is_lacp_current(struct ofproto *, ofp_port_t ofp_port);
int ofproto_port_get_lacp_stats(const struct ofport *, struct lacp_slave_stats *);
int ofproto_port_set_stp(struct ofproto *, ofp_port_t ofp_port,
                         const struct ofproto_port_stp_settings *);
int ofproto_port_get_stp_status(struct ofproto *, ofp_port_t ofp_port,
                                struct ofproto_port_stp_status *);
int ofproto_port_get_stp_stats(struct ofproto *, ofp_port_t ofp_port,
                               struct ofproto_port_stp_stats *);
int ofproto_port_set_queues(struct ofproto *, ofp_port_t ofp_port,
                            const struct ofproto_port_queue *,
                            size_t n_queues);
int ofproto_port_get_rstp_status(struct ofproto *, ofp_port_t ofp_port,
                                struct ofproto_port_rstp_status *);

int ofproto_port_set_rstp(struct ofproto *, ofp_port_t ofp_port,
        const struct ofproto_port_rstp_settings *);

/* The behaviour of the port regarding VLAN handling */
enum port_vlan_mode {
    /* This port is an access port.  'vlan' is the VLAN ID.  'trunks' is
     * ignored. */
    PORT_VLAN_ACCESS,

    /* This port is a trunk.  'trunks' is the set of trunks. 'vlan' is
     * ignored. */
    PORT_VLAN_TRUNK,

    /* Untagged incoming packets are part of 'vlan', as are incoming packets
     * tagged with 'vlan'.  Outgoing packets tagged with 'vlan' stay tagged.
     * Other VLANs in 'trunks' are trunked. */
    PORT_VLAN_NATIVE_TAGGED,

    /* Untagged incoming packets are part of 'vlan', as are incoming packets
     * tagged with 'vlan'.  Outgoing packets tagged with 'vlan' are untagged.
     * Other VLANs in 'trunks' are trunked. */
    PORT_VLAN_NATIVE_UNTAGGED
};

#ifdef OPS
/* Port option configuration list */
enum port_option_args {
    /* Port vlan configuration option change */
    PORT_OPT_VLAN,

    /* Port Bond (LAG) configuration option change */
    PORT_OPT_BOND,

    /* hw_config driven mostly by l3portd */
    PORT_HW_CONFIG,

    /* Array size */
    PORT_OPT_MAX
};

/* Indicate whether port primary/secondary v4/v6 ip is changed */
#define PORT_PRIMARY_IPv4_CHANGED     0x1
#define PORT_PRIMARY_IPv6_CHANGED     0x2
#define PORT_SECONDARY_IPv4_CHANGED   0x4
#define PORT_SECONDARY_IPv6_CHANGED   0x8
#endif

/* Configuration of bundles. */
struct ofproto_bundle_settings {
    char *name;                 /* For use in log messages. */

    ofp_port_t *slaves;         /* OpenFlow port numbers for slaves. */
    size_t n_slaves;

    enum port_vlan_mode vlan_mode; /* Selects mode for vlan and trunks */
    int vlan;                   /* VLAN VID, except for PORT_VLAN_TRUNK. */
    unsigned long *trunks;      /* vlan_bitmap, except for PORT_VLAN_ACCESS. */
    bool use_priority_tags;     /* Use 802.1p tag for frames in VLAN 0? */

    struct bond_settings *bond; /* Must be nonnull iff if n_slaves > 1. */

    struct lacp_settings *lacp;              /* Nonnull to enable LACP. */
    struct lacp_slave_settings *lacp_slaves; /* Array of n_slaves elements. */

    /* Linux VLAN device support (e.g. "eth0.10" for VLAN 10.)
     *
     * This is deprecated.  It is only for compatibility with broken device
     * drivers in old versions of Linux that do not properly support VLANs when
     * VLAN devices are not used.  When broken device drivers are no longer in
     * widespread use, we will delete these interfaces. */
    ofp_port_t realdev_ofp_port;/* OpenFlow port number of real device. */

#ifdef OPS
    const struct smap *port_options[PORT_OPT_MAX];  /* Port options list */
    bool hw_bond_should_exist;    /* Indicates if a bond should exist in h/w
                                     for this bundle.  If hw_bond_handle exists
                                     but this variable is false, it indicates
                                     the h/w bond should be deleted. */
    bool bond_handle_alloc_only;  /* Allocate a bond hanlde and return.
                                     This flag is set to true when a bond
                                     entry is initially created without
                                     active slave members. */
    ofp_port_t *slaves_tx_enable; /* OpenFlow port numbers for slaves in
                                     tx_enable state. */
    size_t n_slaves_tx_enable;    /* Number of slaves in tx_enable state. */
    size_t slaves_entered;         /* Number of slaves entered while adding a bond*/
    int  ip_change;
    char *ip4_address;
    char *ip6_address;
    size_t n_ip4_address_secondary;
    char **ip4_address_secondary; /* List of secondary IP address */
    size_t n_ip6_address_secondary;
    char **ip6_address_secondary; /* List of secondary IPv6 address */
    bool enable;                  /* Port enable */
#endif
};

int ofproto_bundle_register(struct ofproto *, void *aux,
                            const struct ofproto_bundle_settings *);
int ofproto_bundle_unregister(struct ofproto *, void *aux);

#ifdef OPS
int ofproto_bundle_get(struct ofproto *, void *aux, int *bundle_handle);
/* Configuration of VLANs. */
int ofproto_set_vlan(struct ofproto *, int vid, bool add);

int ofproto_add_l3_host_entry(struct ofproto *ofproto, void *aux,
                              bool is_ipv6_addr, char *ip_addr,
                              char *next_hop_mac_addr, int *l3_egress_id);

int ofproto_delete_l3_host_entry(struct ofproto *ofproto, void *aux,
                             bool is_ipv6_addr, char *ip_addr,
                             int *l3_egress_id);

int ofproto_get_l3_host_hit(struct ofproto *ofproto, void *aux,
                            bool addr_type, char *ip_addr, bool *hit_bit);
int ofproto_l3_route_action(struct ofproto *ofproto,
                            enum ofproto_route_action action,
                            struct ofproto_route *route);

int ofproto_l3_ecmp_set(struct ofproto *ofproto, bool enable);
int ofproto_l3_ecmp_hash_set(struct ofproto *ofproto, unsigned int hash,
                             bool enable);
#endif

/* Configuration of mirrors. */
struct ofproto_mirror_settings {
    /* Name for log messages. */
    char *name;

    /* Bundles that select packets for mirroring upon ingress.  */
    void **srcs;                /* A set of registered ofbundle handles. */
    size_t n_srcs;

    /* Bundles that select packets for mirroring upon egress.  */
    void **dsts;                /* A set of registered ofbundle handles. */
    size_t n_dsts;

    /* VLANs of packets to select for mirroring. */
    unsigned long *src_vlans;   /* vlan_bitmap, NULL selects all VLANs. */

    /* Output (mutually exclusive). */
    void *out_bundle;           /* A registered ofbundle handle or NULL. */
    uint16_t out_vlan;          /* Output VLAN, only if out_bundle is NULL. */
};

int ofproto_mirror_register(struct ofproto *, void *aux,
                            const struct ofproto_mirror_settings *);
int ofproto_mirror_unregister(struct ofproto *, void *aux);
int ofproto_mirror_get_stats(struct ofproto *, void *aux,
                             uint64_t *packets, uint64_t *bytes);

int ofproto_set_flood_vlans(struct ofproto *, unsigned long *flood_vlans);
bool ofproto_is_mirror_output_bundle(const struct ofproto *, void *aux);

/* Configuration of QOS tables. */
enum qos_trust get_qos_trust_value(const struct smap *cfg);
int ofproto_set_port_qos_cfg(struct ofproto *ofproto, void *aux,
                             const enum qos_trust qos_trust,
                             const struct smap *cfg);
int ofproto_set_cos_map(struct ofproto *ofproto, void *aux,
                        const struct cos_map_settings *settings);
int ofproto_set_dscp_map(struct ofproto *ofproto, void *aux,
                         const struct dscp_map_settings *settings);

/* Configuration of OpenFlow tables. */
struct ofproto_table_settings {
    char *name;                 /* Name exported via OpenFlow or NULL. */
    unsigned int max_flows;     /* Maximum number of flows or UINT_MAX. */

    /* These members determine the handling of an attempt to add a flow that
     * would cause the table to have more than 'max_flows' flows.
     *
     * If 'groups' is NULL, overflows will be rejected with an error.
     *
     * If 'groups' is nonnull, an overflow will cause a flow to be removed.
     * The flow to be removed is chosen to give fairness among groups
     * distinguished by different values for the subfields within 'groups'. */
    struct mf_subfield *groups;
    size_t n_groups;

    /*
     * Fields for which prefix trie lookup is maintained.
     */
    unsigned int n_prefix_fields;
    enum mf_field_id prefix_fields[CLS_MAX_TRIES];
};

extern const enum mf_field_id default_prefix_fields[2];
BUILD_ASSERT_DECL(ARRAY_SIZE(default_prefix_fields) <= CLS_MAX_TRIES);

int ofproto_get_n_tables(const struct ofproto *);
uint8_t ofproto_get_n_visible_tables(const struct ofproto *);
void ofproto_configure_table(struct ofproto *, int table_id,
                             const struct ofproto_table_settings *);

/* Configuration querying. */
bool ofproto_has_snoops(const struct ofproto *);
void ofproto_get_snoops(const struct ofproto *, struct sset *);
void ofproto_get_all_flows(struct ofproto *p, struct ds *);
void ofproto_get_netflow_ids(const struct ofproto *,
                             uint8_t *engine_type, uint8_t *engine_id);

void ofproto_get_ofproto_controller_info(const struct ofproto *, struct shash *);
void ofproto_free_ofproto_controller_info(struct shash *);

bool ofproto_port_cfm_status_changed(struct ofproto *, ofp_port_t ofp_port);

int ofproto_port_get_cfm_status(const struct ofproto *,
                                ofp_port_t ofp_port,
                                struct cfm_status *);

/* Linux VLAN device support (e.g. "eth0.10" for VLAN 10.)
 *
 * This is deprecated.  It is only for compatibility with broken device drivers
 * in old versions of Linux that do not properly support VLANs when VLAN
 * devices are not used.  When broken device drivers are no longer in
 * widespread use, we will delete these interfaces. */

void ofproto_get_vlan_usage(struct ofproto *, unsigned long int *vlan_bitmap);
bool ofproto_has_vlan_usage_changed(const struct ofproto *);
int ofproto_port_set_realdev(struct ofproto *, ofp_port_t vlandev_ofp_port,
                             ofp_port_t realdev_ofp_port, int vid);

/* Table configuration */

enum ofputil_table_miss ofproto_table_get_miss_config(const struct ofproto *,
                                                      uint8_t table_id);

#ifdef  __cplusplus
}
#endif

#endif /* ofproto.h */
