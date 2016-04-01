#ifndef ACLLOG_H
#define ACLLOG_H

#include <stdint.h>

#include "uuid.h"

#define ACLLOG_INGRESS_PORT   0x00000001
#define ACLLOG_EGRESS_PORT    0x00000002
#define ACLLOG_INGRESS_VLAN   0x00000004
#define ACLLOG_EGRESS_VLAN    0x00000008
#define ACLLOG_NODE           0x00000010
#define ACLLOG_IN_COS         0x00000020
#define ACLLOG_ENTRY_NUM      0x00000040
#define ACLLOG_LIST_TYPE      0x00000080
#define ACLLOG_LIST_NAME      0x00000100
#define ACLLOG_LIST_ID        0x00000200

typedef struct {
   /* data needed from the ASIC */
   uint32_t    valid_fields;
   uint32_t    ingress_port;
   uint32_t    egress_port;
   uint16_t    ingress_vlan;
   uint16_t    egress_vlan;
   uint8_t     node;
   uint8_t     in_cos;
   /* information about the ACE that the packet matched */
   uint32_t    entry_num;
   uint32_t    list_type;
   char        list_name[32 + 1];
   struct uuid list_id;
   uint16_t    total_pkt_len;    /* size of the packet received */
   uint16_t    pkt_buffer_len;   /* number of packet bytes in the data buffer */
   /* packet data including the header */
   uint8_t     pkt_data[256];
} acllog_info_t;

/* For tracking acl log events globally. */
struct seq *acllog_pktrx_seq_get(void);

acllog_info_t acllog_pkt_data_get(void);

void acllog_pkt_data_set(acllog_info_t new_pkt);

#endif /* ACLLOG_H */
