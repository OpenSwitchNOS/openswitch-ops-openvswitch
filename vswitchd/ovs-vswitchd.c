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

#include <config.h>

#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_MLOCKALL
#include <sys/mman.h>
#endif

#include "bridge.h"
#include "command-line.h"
#include "compiler.h"
#include "daemon.h"
#include "dirs.h"
#include "dpif.h"
#include "dummy.h"
#include "fatal-signal.h"
#include "memory.h"
#include "netdev.h"
#include "openflow/openflow.h"
#include "ovsdb-idl.h"
#include "plugins.h"
#include "poll-loop.h"
#include "simap.h"
#include "stream-ssl.h"
#include "stream.h"
#include "svec.h"
#include "timeval.h"
#include "unixctl.h"
#include "util.h"
#include "openvswitch/vconn.h"
#include "openvswitch/vlog.h"
#include "lib/vswitch-idl.h"
#include "lib/netdev-dpdk.h"
#ifdef OPS
#include "subsystem.h"
#include "bufmon-provider.h"
#endif

VLOG_DEFINE_THIS_MODULE(vswitchd);

/* --mlockall: If set, locks all process memory into physical RAM, preventing
 * the kernel from paging any of its memory to disk. */
static bool want_mlockall;

static unixctl_cb_func ovs_vswitchd_exit;

static char *parse_options(int argc, char *argv[], char **unixctl_path,
                           char **plugins_path);
OVS_NO_RETURN static void usage(void);

int
main(int argc, char *argv[])
{
    char *unixctl_path = NULL;
    struct unixctl_server *unixctl;
    char *plugins_path = NULL;
    char *remote;
    bool exiting;
    int retval;

    set_program_name(argv[0]);
    retval = dpdk_init(argc,argv);
    argc -= retval;
    argv += retval;

    ovs_cmdl_proctitle_init(argc, argv);
    service_start(&argc, &argv);
    remote = parse_options(argc, argv, &unixctl_path, &plugins_path);

    fatal_ignore_sigpipe();
    ovsrec_init();

    daemonize_start();

    if (want_mlockall) {
#ifdef HAVE_MLOCKALL
        if (mlockall(MCL_CURRENT | MCL_FUTURE)) {
            VLOG_ERR("mlockall failed: %s", ovs_strerror(errno));
        }
#else
        VLOG_ERR("mlockall not supported on this system");
#endif
    }

    retval = unixctl_server_create(unixctl_path, &unixctl);
    if (retval) {
        exit(EXIT_FAILURE);
    }
    unixctl_command_register("exit", "", 0, 0, ovs_vswitchd_exit, &exiting);

    plugins_init(plugins_path);

    bridge_init(remote);
#ifdef OPS
    subsystem_init();

    bufmon_init();

    wait_for_config_complete();

#endif

    free(remote);

    exiting = false;
    while (!exiting) {
        memory_run();
        if (memory_should_report()) {
            struct simap usage;

            simap_init(&usage);
            bridge_get_memory_usage(&usage);
            memory_report(&usage);
            simap_destroy(&usage);
        }
        bridge_run();
#ifdef OPS
        subsystem_run();
        bufmon_run();
#endif
        unixctl_server_run(unixctl);
        netdev_run();
        plugins_run();

        memory_wait();
        bridge_wait();
#ifdef OPS
        subsystem_wait();
        bufmon_wait();
#endif
        unixctl_server_wait(unixctl);
        netdev_wait();
        plugins_wait();
        if (exiting) {
            poll_immediate_wake();
        }
        poll_block();
        if (should_service_stop()) {
            exiting = true;
        }
    }
    bridge_exit();
#ifdef OPS
    subsystem_exit();
#endif
    unixctl_server_destroy(unixctl);
    plugins_destroy();
    service_stop();

    return 0;
}

static char *
parse_options(int argc, char *argv[], char **unixctl_pathp, char **plugins_pathp)
{
    enum {
        OPT_PEER_CA_CERT = UCHAR_MAX + 1,
        OPT_MLOCKALL,
        OPT_UNIXCTL,
        OPT_PLUGINS,
        VLOG_OPTION_ENUMS,
        OPT_BOOTSTRAP_CA_CERT,
        OPT_ENABLE_DUMMY,
        OPT_DISABLE_SYSTEM,
        DAEMON_OPTION_ENUMS,
        OPT_DPDK,
    };
    static const struct option long_options[] = {
        {"help",        no_argument, NULL, 'h'},
        {"version",     no_argument, NULL, 'V'},
        {"mlockall",    no_argument, NULL, OPT_MLOCKALL},
        {"unixctl",     required_argument, NULL, OPT_UNIXCTL},
        {"plugins-path",     required_argument, NULL, OPT_PLUGINS},
        DAEMON_LONG_OPTIONS,
        VLOG_LONG_OPTIONS,
        STREAM_SSL_LONG_OPTIONS,
        {"peer-ca-cert", required_argument, NULL, OPT_PEER_CA_CERT},
        {"bootstrap-ca-cert", required_argument, NULL, OPT_BOOTSTRAP_CA_CERT},
        {"enable-dummy", optional_argument, NULL, OPT_ENABLE_DUMMY},
        {"disable-system", no_argument, NULL, OPT_DISABLE_SYSTEM},
        {"dpdk", required_argument, NULL, OPT_DPDK},
        {NULL, 0, NULL, 0},
    };
    char *short_options = ovs_cmdl_long_options_to_short_options(long_options);

    for (;;) {
        int c;

        c = getopt_long(argc, argv, short_options, long_options, NULL);
        if (c == -1) {
            break;
        }

        switch (c) {
        case 'h':
            usage();

        case 'V':
            ovs_print_version(OFP10_VERSION, OFP10_VERSION);
            exit(EXIT_SUCCESS);

        case OPT_MLOCKALL:
            want_mlockall = true;
            break;

        case OPT_UNIXCTL:
            *unixctl_pathp = optarg;
            break;

        case OPT_PLUGINS:
            *plugins_pathp = optarg;
            break;

        VLOG_OPTION_HANDLERS
        DAEMON_OPTION_HANDLERS
        STREAM_SSL_OPTION_HANDLERS

        case OPT_PEER_CA_CERT:
            stream_ssl_set_peer_ca_cert_file(optarg);
            break;

        case OPT_BOOTSTRAP_CA_CERT:
            stream_ssl_set_ca_cert_file(optarg, true);
            break;

        case OPT_ENABLE_DUMMY:
            dummy_enable(optarg);
            break;

        case OPT_DISABLE_SYSTEM:
            dp_blacklist_provider("system");
            break;

        case '?':
            exit(EXIT_FAILURE);

        case OPT_DPDK:
            ovs_fatal(0, "--dpdk must be given at beginning of command line.");
            break;

        default:
            abort();
        }
    }
    free(short_options);

    argc -= optind;
    argv += optind;

    switch (argc) {
    case 0:
        return xasprintf("unix:%s/db.sock", ovs_rundir());

    case 1:
        return xstrdup(argv[0]);

    default:
        VLOG_FATAL("at most one non-option argument accepted; "
                   "use --help for usage");
    }
}

static void
usage(void)
{
    printf("%s: Open vSwitch daemon\n"
           "usage: %s [OPTIONS] [DATABASE]\n"
           "where DATABASE is a socket on which ovsdb-server is listening\n"
           "      (default: \"unix:%s/db.sock\").\n",
           program_name, program_name, ovs_rundir());
    stream_usage("DATABASE", true, false, true);
    daemon_usage();
    vlog_usage();
    printf("\nDPDK options:\n"
           "  --dpdk options          Initialize DPDK datapath.\n");
    printf("\nOther options:\n"
           "  --unixctl=SOCKET        override default control socket name\n"
           "  --plugins-path=path          override default path to plugins directory\n"
           "  -h, --help              display this help message\n"
           "  -V, --version           display version information\n");
    exit(EXIT_SUCCESS);
}

static void
ovs_vswitchd_exit(struct unixctl_conn *conn, int argc OVS_UNUSED,
                  const char *argv[] OVS_UNUSED, void *exiting_)
{
    bool *exiting = exiting_;
    *exiting = true;
    unixctl_command_reply(conn, NULL);
}
