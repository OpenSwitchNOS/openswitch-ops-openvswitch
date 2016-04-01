#include <config.h>
#include "acllog.h"
#include "ovs-thread.h"
#include "poll-loop.h"
#include "seq.h"

static struct seq *acllog_pktrx_seq;

static struct ovs_mutex acllog_mutex = OVS_MUTEX_INITIALIZER;

static acllog_info_t info OVS_GUARDED_BY(acllog_mutex) = { .valid_fields = 0 };

/* Provides a global seq for acl logging events.
 *
 * ACL logging modules should call seq_change() on the returned object whenever
 * a packet is received for ACL logging.
 *
 * Clients can seq_wait() on this object to do the logging and tell all ASICs
 * to stop copying packets to the CPU. */
struct seq *
acllog_pktrx_seq_get(void)
{
    static struct ovsthread_once once = OVSTHREAD_ONCE_INITIALIZER;

    if (ovsthread_once_start(&once)) {
        acllog_pktrx_seq = seq_create();
        ovsthread_once_done(&once);
    }

    return acllog_pktrx_seq;
}

acllog_info_t
acllog_pkt_data_get(void)
{
   acllog_info_t to_return;

   /* take the mutex */
   ovs_mutex_lock(&acllog_mutex);

   /* copy the static value into the data to be returned */
   to_return = info;
   memcpy(to_return.pkt_data, info.pkt_data,
          MIN(sizeof(to_return.pkt_data), to_return.total_pkt_len));

   /* give the mutex */
   ovs_mutex_unlock(&acllog_mutex);

   /* return the value */
   return to_return;
}

void
acllog_pkt_data_set(acllog_info_t new_pkt)
{
   /* take the mutex */
   ovs_mutex_lock(&acllog_mutex);

   /* copy the argument into the static value */
   info = new_pkt;
   memcpy(info.pkt_data, new_pkt.pkt_data,
          MIN(sizeof(info.pkt_data), info.total_pkt_len));

   /* give the mutex */
   ovs_mutex_unlock(&acllog_mutex);
}
