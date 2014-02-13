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

#define DMM
#define DMM_INIT_C
//---------------------------------------------------------------------------------------------------------------------
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <unistd.h>
//---------------------------------------------------------------------------------------------------------------------
#include "binding_cache.h"
#include "cmd_proc.h"
#include "dmm_fsm.h"
#include "dmm_init.h"
#include "dmm_msgs.h"
#include "dmm_types.h"
#include "handlers.h"
#include "hnp_cache.h"
#include "hostapd.h"
#include "mar_proc.h"
//---------------------------------------------------------------------------------------------------------------------
#include "rtnl.h"
#include "tunnelctl.h"
#ifdef ENABLE_VT
#    include "vt.h"
#endif
#include "debug.h"
#include "conf.h"

#define IPV6_ALL_SOLICITED_MCAST_ADDR 68
//---------------------------------------------------------------------------------------------------------------------
extern struct sock icmp6_sock;
//---------------------------------------------------------------------------------------------------------------------
void init_mar_icmp_sock(void)
//---------------------------------------------------------------------------------------------------------------------
{
	if (0) {
		int on = 1;
		dbg("Set SOLRAW, IPV6_ALL_SOLICTED_MCAST_ADDR = %d\n", IPV6_ALL_SOLICITED_MCAST_ADDR);
		if (setsockopt(icmp6_sock.fd, SOL_RAW, IPV6_ALL_SOLICITED_MCAST_ADDR, &on, sizeof(on)) < 0) {
			perror("allow all solicited mcast address\n");
		}
	}
}
//---------------------------------------------------------------------------------------------------------------------
static int dmm_cache_delete_each(void *data, void *arg)
//---------------------------------------------------------------------------------------------------------------------
{
    if (data != NULL) dbg("OK\n"); else dbg("KO\n");
	dmm_entry *bce = (dmm_entry *) data;
    if (is_mar()) {
        mar_remove_route(&bce->mn_prefix, 64, bce->link, 0);
        int usercount = tunnel_getusers(bce->tunnel);
        dbg("# of binding entries %d \n", usercount);
        if (usercount == 1) {
            route_del(bce->tunnel, RT6_TABLE_DMM, IP6_RT_PRIO_MIP6_FWD, &in6addr_any, 0, &in6addr_any, 0, NULL);
        }
	tunnel_del(bce->tunnel,0,0);
	mar_list_clean_all(bce);
    }
    free_iov_data((struct iovec *) &bce->mh_vec, bce->iovlen);
    dmm_cache_delete(bce);
    return 0;
}
//---------------------------------------------------------------------------------------------------------------------
void dmm_cleanup(void)
//---------------------------------------------------------------------------------------------------------------------
{
    //Release the pmip cache ==> deletes the routes and rules and "default route on DMM" and tunnels created.
    dbg("Release all occupied resources...\n");

    dbg("Remove default rule...\n");
    rule_del(NULL, RT6_TABLE_MIP6, IP6_RULE_PRIO_MIP6_FWD, RTN_UNICAST, &in6addr_any, 0, &in6addr_any, 0, 0);

    dbg("Release pmip_cache...\n");
    dmm_cache_iterate(dmm_cache_delete_each, NULL);
	if (conf.DLIFsupportEnabled)
        	dlif_cleanup();
	if (conf.HostapdListenerEnabled)
		hostapd_cleanup();
}

//---------------------------------------------------------------------------------------------------------------------
int dmm_common_init(void)
//---------------------------------------------------------------------------------------------------------------------
{
    /**
    * Probe for the local address
	**/
    int probe_fd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (probe_fd < 0) {
        perror("socket");
        exit(2);
    }
    unsigned int alen;
    struct sockaddr_in6 host;
    struct sockaddr_in6 firsthop;

    memset(&firsthop, 0, sizeof(firsthop));
    firsthop.sin6_port = htons(1025);
    firsthop.sin6_family = AF_INET6;
    if (connect(probe_fd, (struct sockaddr *) &firsthop, sizeof(firsthop)) == -1) {
        perror("connect");
        return -1;;
    }
    alen = sizeof(host);
    if (getsockname(probe_fd, (struct sockaddr *) &host, &alen) == -1) {
        perror("probe getsockname");
        return -1;;
    }
    close(probe_fd);


    /**
    * Initializes DMM cache.
    **/
    if (dmm_cache_init() < 0) {
        dbg("DMM Binding Cache initialization failed! \n");
        return -1;
    } else {
        dbg("DMM Binding Cache is initialized!\n");
    }
    /**
    * Adds a default rule for RT6_TABLE_MIP6.
    */
    dbg("Add default rule for RT6_TABLE_MIP6\n");
    if (rule_add(NULL, RT6_TABLE_MIP6, IP6_RULE_PRIO_MIP6_FWD, RTN_UNICAST, &in6addr_any, 0, &in6addr_any, 0, 0) < 0) {
        dbg("Add default rule for RT6_TABLE_MIP6 failed, insufficient privilege/kernel options missing!\n");
        return -1;
    }
	return 0;
}
//---------------------------------------------------------------------------------------------------------------------
int mar_init(void)
//---------------------------------------------------------------------------------------------------------------------
{
	dmm_common_init();
	conf.OurAddress = conf.MarAddressEgress;
	conf.HomeNetworkPrefix = get_node_prefix(&conf.MarAddressIngress); //copy Home network prefix.
	dbg("Running as MAR entity\n");
	dbg("Entity Egress Address: %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(&conf.OurAddress));
	dbg("Entity Ingress Address: %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(&conf.MarAddressIngress));
	dbg("Home Network Prefix Address: %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(&conf.HomeNetworkPrefix));
	if (mar_init_fsm() < 0) {
		dbg("Initialization of FSM failed...exit\n");
		exit(-1);
	}

	init_pbu_sequence_number();

	init_iface_ra();
	init_mar_icmp_sock();
	dbg("Initializing the NA handler\n");
	icmp6_handler_reg(ND_NEIGHBOR_ADVERT, &mar_na_handler);
	dbg("Initializing the RS handler\n");
	icmp6_handler_reg(ND_ROUTER_SOLICIT, &mar_rs_handler);
	dbg("Initializing the PBU/PBA handler\n");
	mh_handler_reg(IP6_MH_TYPE_BACK, &mar_pba_handler);
	mh_handler_reg(IP6_MH_TYPE_BU, &mar_pbu_handler);
	/**
	* Deletes the default route for MN prefix so routing is per unicast MN address!
	**/
	route_del((int) NULL, RT6_TABLE_MAIN, IP6_RT_PRIO_ADDRCONF, &in6addr_any, 0, &conf.HomeNetworkPrefix, 64, NULL);
	dbg("Initializing the HNP cache\n");
	if (dmm_mn_to_hnp_cache_init() < 0) {
		exit (-1);
	}
	if (!conf.TunnelingEnabled) {
		dbg("Tunneling not enabled\n");
		exit(-1);
	}
	if (check_ip6_forwarding() < 0) {
		exit(-1);
	}

	/**
	 * This is to pick the egress IF's MAC address
	 */
	if (mar_get_ingress_info() <= 0) {
		exit (-1);
	}
	/**
	 * Start DLIF support if configured
	 */
	if (conf.DLIFsupportEnabled)
		if (dlif_list_init() < 0){
			dbg("Could not initialize DLIF list\n");
	}
	/**
	 * Start hostapd IPC for L2 detection
	 */
	if (conf.HostapdListenerEnabled)
		if (hostapd_listener_init() < 0){
			dbg("Could not initialize hostapd IPC\n");
	}
	dbg("MAR ready - Waiting for new messages ...\n\n\n");
        return 0;
}
//---------------------------------------------------------------------------------------------------------------------
int cmd_init(void)
{
	dmm_common_init();
	cmd_mn_to_hnp_cache_init();
	conf.OurAddress = conf.CmdAddress;
	dbg("Entity Address: %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(&conf.OurAddress));
	dbg("Initializing the PBU/PBA handler\n");
	mh_handler_reg(IP6_MH_TYPE_BU, &cmd_pbu_handler);
	mh_handler_reg(IP6_MH_TYPE_BACK, &cmd_pba_handler);
	dbg("CMD ready - Waiting for new messages ...\n\n\n");
	return 0;
}