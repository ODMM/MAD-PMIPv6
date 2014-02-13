/*
 * $Id: conf.c 1.50 06/05/12 11:48:36+03:00 vnuorval@tcs.hut.fi $
 *
 * This file is part of the MIPL Mobile IPv6 for Linux.
 *
 * Authors: Antti Tuominen <anttit@tcs.hut.fi>
 *          Ville Nuorvala <vnuorval@tcs.hut.fi>
 *
 * Copyright 2003-2005 Go-Core Project
 * Copyright 2003-2006 Helsinki University of Technology
 *
 * MIPL Mobile IPv6 for Linux is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; version 2 of
 * the License.
 *
 * MIPL Mobile IPv6 for Linux is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MIPL Mobile IPv6 for Linux; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307 USA.
 */
/*
 * This file is part of the PMIP, Proxy Mobile IPv6 for Linux.
 *
 * Authors: OPENAIR3 <openair_tech@eurecom.fr>
 *
 * Copyright 2010-2011 EURECOM (Sophia-Antipolis, FRANCE)
 * 
 * Proxy Mobile IPv6 (or PMIPv6, or PMIP) is a network-based mobility 
 * management protocol standardized by IETF. It is a protocol for building 
 * a common and access technology independent of mobile core networks, 
 * accommodating various access technologies such as WiMAX, 3GPP, 3GPP2 
 * and WLAN based access architectures. Proxy Mobile IPv6 is the only 
 * network-based mobility management protocol standardized by IETF.
 * 
 * PMIP Proxy Mobile IPv6 for Linux has been built above MIPL free software;
 * which it involves that it is under the same terms of GNU General Public
 * License version 2. See MIPL terms condition if you need more details. 
 */
/*
 * This file is part of MAD-PMIPv6,
 * Mobility Anchors Distribution for PMIPv6,
 * released by the Open Platform for DMM solutions (ODMM).
 *
 * Authors: Fabio Giust <odmm-support@odmm.net>
 *          within the NETCOM working group,
 *          Department of Telematics Engineering
 *          University Carlos III of Madrid, Spain
 *
 * Copyright 2012-2014 NETCOM - UC3M (Madrid, SPAIN)
 *
 * MAD-PMIPv6 for Linux has been built above MIPL free software;
 * which it involves that it is under the same terms of GNU General Public
 * License version 2. See MIPL terms condition if you need more details. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include <netinet/in.h>
#include <netinet/ip6mh.h>
#include <arpa/inet.h>
#include "defpath.h"
#include "conf.h"
#include "debug.h"
#include "util.h"
#include "mipv6.h"
#ifdef ENABLE_VT
#include "vt.h"
#endif

static void conf_usage(char *exec_name)
{
    fprintf(stderr,
            "Usage: %s [options]\nOptions:\n"
            "  -V, --version                Display version information and copyright\n"
            "  -?, -h, --help               Display this help text\n"
            "  -c <file>                    Read configuration from <file>\n"
#ifdef ENABLE_VT
            "      --vt-service <serv>      Set VT service (default=" VT_DEFAULT_SERVICE ")\n"
#endif
            "\n These options override values read from config file:\n"
            "  -d <number>                  Set debug level (0-10)\n"
            "  -l <file>                    Write debug log to <file> instead of stderr\n"
            "  -C, --correspondent-node     Node is CN\n"
            "  -H, --home-agent             Node is HA\n"
            "  -M, --mobile-node            Node is MN\n\n"
            "  -m, --mobile-access-gateway  Node is MAR (DMM-Proxy MIP architecture)\n"
            "  -a, --local-mobility-anchor  Node is CMD (DMM-Proxy MIP architecture)\n"
            "  -i, --pmip-tunneling         With IPv6-in-IPv6 tunneling      (Proxy MIP architecture)\n"
            "  -p, --pmip-dyn-tunneling     Dynamicaly create/delete tunnels (Proxy MIP architecture)\n"
            "  -L, --cmd-address            CMD address exposed to MARs\n"
            "  -N, --mar-ingress-address    MAR ingress address (towards MNs)\n"
            "  -E, --mar-egress-address     MAR egress address  (towards CMD)\n"
            "  -D,                          DLIF support enabled\n"
            "  -W,                          hostapd listener enabled\n"
            "For bug reporting, see %s.\n", exec_name, PACKAGE_BUGREPORT);
}

static void conf_version(void)
{
    fprintf(stderr,
            "%s (%s) %s\n"
            "%s\n"
            "This is free software; see the source for copying conditions.  There is NO\n"
            "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n",
            PACKAGE, PACKAGE_NAME, PACKAGE_VERSION, PACKAGE_COPYRIGHT);
}

static int conf_alt_file(char *filename, int argc, char **argv)
{
    int args_left = argc;
    char **cur_arg = argv;

    while (args_left--) {
        if (strcmp(*cur_arg, "-c") == 0 && args_left > 0) {
            cur_arg++;
            if (**cur_arg == '-')
                return -EINVAL;
            if (strlen(*cur_arg) >= MAXPATHLEN)
                return -ENAMETOOLONG;
            strcpy(filename, *cur_arg);
            return 0;
        }
        cur_arg++;
    }

    return 1;
}

static int conf_file(struct mip6_config *c, char *filename)
{
    extern FILE *yyin;
    int ret;

    yyin = fopen(filename, "r");
    if (yyin == NULL)
        return -ENOENT;

    c->config_file = malloc(strlen(filename) + 1);
    if (c->config_file == NULL)
        return -ENOMEM;
    strcpy(c->config_file, filename);

    ret = yyparse();

    fclose(yyin);
    if (ret) return -EINVAL;

    return 0;
}

static int conf_cmdline(struct mip6_config *cfg, int argc, char **argv)
{
    static struct option long_opts[] = {
            {"version", 0, 0, 'V'},
            {"help", 0, 0, 'h'},
            {"correspondent-node", 0, 0, 'C'},
            {"home-agent", 0, 0, 'H'},
            {"mobile-node", 0, 0, 'M'},
            {"mobile-access-gateway", optional_argument, 0, 'm'},
            {"local-mobility-anchor", optional_argument, 0, 'a'},
            {"pmip-tunneling", optional_argument, 0, 'i'},
            {"pmip-dyn-tunneling", optional_argument, 0, 'd'},
            {"cmd-address", optional_argument, 0, 'L'},
            {"mar-ingress-address", optional_argument, 0, 'N'},
            {"mar-egress-address", optional_argument, 0, 'E'},
            {"show-config", 0, 0, 0},
            {"radius-client-cfg-file", optional_argument, 0, 'R'},
            {"radius-password", 0, 0, 'P'},
#ifdef ENABLE_VT
            {"vt-service", 1, 0, 0 },
#endif
            {0, 0, 0, 0}
    };

    /* parse all other cmd line parameters than -c */
    while (1) {
        int idx, c;
        c = getopt_long(argc, argv, "c:d:l:L:N:E:DWVh?CMHmaip", long_opts, &idx);
        if (c == -1) break;

        switch (c) {
        case 0:
#ifdef ENABLE_VT
            if (strcmp(long_opts[idx].name, "vt-service") == 0) {
                cfg->vt_service = optarg;
                break;
            }
#endif
            if (idx == 5)
                conf_show(cfg);
            return -1;
        case 'V':
            conf_version();
            return -1;
        case '?':
        case 'h':
            conf_usage(basename(argv[0]));
            return -1;
        case 'd':
            cfg->debug_level = atoi(optarg);
            break;
        case 'l':
            cfg->debug_log_file = optarg;
            break;
        case 'C':
            cfg->mip6_entity = MIP6_ENTITY_CN;
            break;
        case 'H':
            cfg->mip6_entity = MIP6_ENTITY_HA;
            break;
        case 'M':
            cfg->mip6_entity = MIP6_ENTITY_MN;
            break;
        case 'L':
            if (strchr(optarg, ':')) {
                if (inet_pton(AF_INET6, optarg, (char *) &cfg->CmdAddress) <= 0) {
                    fprintf(stderr, "invalid  address %s\n", optarg);
                    exit(2);
                }
            }
            break;
        case 'N':
            if (strchr(optarg, ':')) {
                if (inet_pton(AF_INET6, optarg, (char *) &cfg->MarAddressIngress) <= 0) {
                    fprintf(stderr, "invalid  address %s\n", optarg);
                    exit(2);
                }
            }
            break;
        case 'E':
            if (strchr(optarg, ':')) {
                if (inet_pton(AF_INET6, optarg, (char *) &cfg->MarAddressEgress) <= 0) {
                    fprintf(stderr, "invalid  address %s\n", optarg);
                    exit(2);
                }
            }
            break;
        case 'm':
            cfg->mip6_entity = MIP6_ENTITY_MAR;
            break;
        case 'a':
            cfg->mip6_entity = MIP6_ENTITY_CMD;
            break;
        case 'i':
            cfg->TunnelingEnabled = 1;
            break;
        case 'p':
            cfg->DynamicTunnelingEnabled = 1;
            break;
	case 'D':
		cfg->DLIFsupportEnabled = 1;
		break;
	case 'W':
		cfg->HostapdListenerEnabled = 1;
		break;
        default:
            break;
        };
    }
    return 0;
}

static void conf_default(struct mip6_config *c)
{
    memset(c, 0, sizeof(*c));
    // Common options
#ifdef ENABLE_VT
    c->vt_hostname = VT_DEFAULT_HOSTNAME;
    c->vt_service = VT_DEFAULT_SERVICE;
#endif
    c->mip6_entity = MIP6_ENTITY_CN;
    //pmgr_init(NULL, &conf.pmgr);
    INIT_LIST_HEAD(&c->net_ifaces);
    INIT_LIST_HEAD(&c->bind_acl);
    c->DefaultBindingAclPolicy = IP6_MH_BAS_ACCEPTED;

    // IPsec options
    c->UseMnHaIPsec = 0;
    INIT_LIST_HEAD(&c->ipsec_policies);

    // MN options
    c->MnMaxHaBindingLife = MAX_BINDING_LIFETIME;
    c->MnMaxCnBindingLife = MAX_RR_BINDING_LIFETIME;
    tssetdsec(c->InitialBindackTimeoutFirstReg_ts, 1.5);//seconds
    tssetsec(c->InitialBindackTimeoutReReg_ts, INITIAL_BINDACK_TIMEOUT);//seconds
    INIT_LIST_HEAD(&c->home_addrs);
    c->MoveModulePath = NULL; // internal
    c->SendMobPfxSols = 1;
    c->OptimisticHandoff = 0;

    /* PMIP global options */
    c->RFC5213TimestampBasedApproachInUse = 1;
    c->RFC5213MobileNodeGeneratedTimestampInUse = 1;
    c->RFC5213FixedMARLinkLocalAddressOnAllAccessLinks = in6addr_any;
    c->RFC5213FixedMARLinkLayerAddressOnAllAccessLinks = in6addr_any;

    /* PMIP LMA options */
    struct timespec lifetime1;
    lifetime1.tv_sec = 10;
    lifetime1.tv_nsec = 0;
    c->RFC5213MinDelayBeforeBCEDelete         = lifetime1; // 10000 milliseconds
    struct timespec lifetime2;
    lifetime2.tv_sec = 1;
    lifetime2.tv_nsec = 500000000;
    c->RFC5213MaxDelayBeforeNewBCEAssign      = lifetime2; // 1500 milliseconds
    struct timespec lifetime3;
    lifetime3.tv_sec = 0;
    lifetime3.tv_nsec = 300000000;
    c->RFC5213TimestampValidityWindow         = lifetime3; // 300 milliseconds

    // HA options
    c->SendMobPfxAdvs       = 1;
    c->SendUnsolMobPfxAdvs  = 1;
    c->MaxMobPfxAdvInterval = 86400; // seconds
    c->MinMobPfxAdvInterval = 600; // seconds
    c->HaMaxBindingLife     = MAX_BINDING_LIFETIME;

    /* PMIP MAR options */
    c->HomeNetworkPrefix             = in6addr_any;
    c->MarAddressIngress             = in6addr_loopback;
    c->MarAddressEgress              = in6addr_loopback;
    c->MarDeviceIngress              = "";
    c->MarDeviceEgress               = "";
    c->CmdAddress                    = in6addr_loopback;
    c->OurAddress                    = in6addr_loopback;
    //Lifetime for PB entry
    struct timespec lifetime4;
    lifetime4.tv_sec  = 40;
    lifetime4.tv_nsec = 0;
    c->PBULifeTime    = lifetime4;
    struct timespec lifetime5;
    lifetime5.tv_sec  = 40;
    lifetime5.tv_nsec = 0;
    c->PBALifeTime    = lifetime5;
    //Time for N_Retransmissions
    struct timespec lifetime6;
    lifetime6.tv_sec  = 1;
    lifetime6.tv_nsec = 0;
    c->RetransmissionTimeOut = lifetime6;
    //Define the maximum # of message retransmissions.
    int Max_rets = 5;
    c->MaxMessageRetransmissions = Max_rets;
    c->TunnelingEnabled          = 0;
    c->DynamicTunnelingEnabled   = 0;
}


int conf_parse(struct mip6_config *c, int argc, char **argv)
{
    char cfile[MAXPATHLEN];
    int ret;

    /* set config defaults */
    conf_default(c);

    if ((ret = conf_alt_file(cfile, argc, argv)) != 0) {
        if (ret == -EINVAL) {
            fprintf(stderr,
                    "%s: option requires an argument -- c\n",
                    argv[0]);
            conf_usage(basename(argv[0]));
            return -1;
        } else if (ret == -ENAMETOOLONG) {
            fprintf(stderr,
                    "%s: argument too long -- c <file>\n",
                    argv[0]);
            return -1;
        }
        strcpy(cfile, DEFAULT_CONFIG_FILE);
    }

    if (conf_file(c, cfile) < 0 && ret == 0)
        return -1;

    if (conf_cmdline(c, argc, argv) < 0)
        return -1;

    return 0;
}

#define CONF_BOOL_STR(x) ((x) ? "enabled" : "disabled")

void conf_show(struct mip6_config *c)
{
    /* Common options */
    dbg("config_file = %s\n", c->config_file);
#ifdef ENABLE_VT
    dbg("vt_hostname = %s\n", c->vt_hostname);
    dbg("vt_service = %s\n", c->vt_service);
#endif
    dbg("mip6_entity = %u\n", c->mip6_entity);
    dbg("debug_level = %u\n", c->debug_level);
    dbg("debug_log_file = %s\n", (c->debug_log_file ? c->debug_log_file :"stderr"));
    //if (c->pmgr.so_path)
        //dbg("PolicyModulePath = %s\n", c->pmgr.so_path);
    dbg("DefaultBindingAclPolicy = %u\n", c->DefaultBindingAclPolicy);
    dbg("NonVolatileBindingCache = %s\n", CONF_BOOL_STR(c->NonVolatileBindingCache));

    /* IPsec options */
    dbg("KeyMngMobCapability = %s\n", CONF_BOOL_STR(c->KeyMngMobCapability));
    dbg("UseMnHaIPsec = %s\n", CONF_BOOL_STR(c->UseMnHaIPsec));

    /* MN options */
    dbg("MnMaxHaBindingLife = %u\n", c->MnMaxHaBindingLife);
    dbg("MnMaxCnBindingLife = %u\n", c->MnMaxCnBindingLife);
    dbg("MnRouterProbes = %u\n", c->MnRouterProbes);
    dbg("MnRouterProbeTimeout = %f\n", tstodsec(c->MnRouterProbeTimeout_ts));
    dbg("InitialBindackTimeoutFirstReg = %f\n",  tstodsec(c->InitialBindackTimeoutFirstReg_ts));
    dbg("InitialBindackTimeoutReReg = %f\n",	tstodsec(c->InitialBindackTimeoutReReg_ts));
    if (c->MoveModulePath)
        dbg("MoveModulePath = %s\n", c->MoveModulePath);
    dbg("UseCnBuAck = %s\n", CONF_BOOL_STR(c->CnBuAck));
    dbg("MnUseAllInterfaces = %s\n", CONF_BOOL_STR(c->MnUseAllInterfaces));
    dbg("MnDiscardHaParamProb = %s\n",
            CONF_BOOL_STR(c->MnDiscardHaParamProb));
    dbg("SendMobPfxSols = %s\n", CONF_BOOL_STR(c->SendMobPfxSols));
    dbg("OptimisticHandoff = %s\n", CONF_BOOL_STR(c->OptimisticHandoff));

    /* HA options */
    dbg("SendMobPfxAdvs = %s\n", CONF_BOOL_STR(c->SendMobPfxAdvs));
    dbg("SendUnsolMobPfxAdvs = %s\n",
            CONF_BOOL_STR(c->SendUnsolMobPfxAdvs));
    dbg("MaxMobPfxAdvInterval = %u\n", c->MaxMobPfxAdvInterval);
    dbg("MinMobPfxAdvInterval = %u\n", c->MinMobPfxAdvInterval);
    dbg("HaMaxBindingLife = %u\n", c->HaMaxBindingLife);

    /* PMIP options */
    dbg("RFC5213TimestampBasedApproachInUse                = %s\n",CONF_BOOL_STR(c->RFC5213TimestampBasedApproachInUse));
    dbg("RFC5213MobileNodeGeneratedTimestampInUse          = %s\n",CONF_BOOL_STR(c->RFC5213MobileNodeGeneratedTimestampInUse));
    dbg("RFC5213FixedMARLinkLocalAddressOnAllAccessLinks   = %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(&c->RFC5213FixedMARLinkLocalAddressOnAllAccessLinks));
    dbg("RFC5213FixedMARLinkLayerAddressOnAllAccessLinks   = %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(&c->RFC5213FixedMARLinkLayerAddressOnAllAccessLinks));

    /* PMIP LMA options */
    dbg("RFC5213MinDelayBeforeBCEDelete                    = %u.%9u seconds\n",c->RFC5213MinDelayBeforeBCEDelete.tv_sec,c->RFC5213MinDelayBeforeBCEDelete.tv_nsec);
    dbg("RFC5213MaxDelayBeforeNewBCEAssign                 = %u.%9u seconds\n",c->RFC5213MaxDelayBeforeNewBCEAssign.tv_sec,c->RFC5213MaxDelayBeforeNewBCEAssign.tv_nsec);
    dbg("RFC5213TimestampValidityWindow                    = %u.%9u seconds\n",c->RFC5213TimestampValidityWindow.tv_sec,c->RFC5213TimestampValidityWindow.tv_nsec);

    /* PMIP MAR options */
    dbg("CmdAddress                    = %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(&c->CmdAddress));
    if (is_mar()) {
        dbg("MarAddressIngress     = %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(&c->MarAddressIngress));
        dbg("MarAddressEgress      = %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(&c->MarAddressEgress));
    }
    dbg("MarDeviceIngress      = %s\n", (c->MarDeviceIngress ? c->MarDeviceIngress :
    "No device"));
    dbg("MarDeviceEgress      = %s\n", (c->MarDeviceEgress ? c->MarDeviceEgress :
    "No device"));
    dbg("OurAddress                    = %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(&c->OurAddress));
    dbg("HomeNetworkPrefix             = %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(&c->HomeNetworkPrefix));

    dbg("PBULifeTime                   = %u.%9u seconds\n",c->PBULifeTime.tv_sec,c->PBULifeTime.tv_nsec);
    dbg("RetransmissionTimeOut         = %u.%9u seconds\n",c->RetransmissionTimeOut.tv_sec,c->RetransmissionTimeOut.tv_nsec);
    dbg("MaxMessageRetransmissions     = %u\n", c->MaxMessageRetransmissions);
    dbg("TunnelingEnabled              = %s\n", CONF_BOOL_STR(c->TunnelingEnabled));
    dbg("DynamicTunnelingEnabled       = %s\n", CONF_BOOL_STR(c->DynamicTunnelingEnabled));
    dbg("DLIFsupport = %s\n",CONF_BOOL_STR(c->DLIFsupportEnabled));
    dbg("HostapdListener = %s\n",CONF_BOOL_STR(c->HostapdListenerEnabled));
}
