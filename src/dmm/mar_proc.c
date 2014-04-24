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
#define DMM_MAG_PROC_C
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif
#include <sys/sysctl.h>
//---------------------------------------------------------------------------------------------------------------------
#include "mar_proc.h"
#include "handlers.h"
#include "hnp_cache.h"
//---------------------------------------------------------------------------------------------------------------------
#include "rtnl.h"
#include "ndisc.h"
#include "tunnelctl.h"
#ifdef ENABLE_VT
#    include "vt.h"
#endif
#include "debug.h"
#include "conf.h"
#include "proc_sys.h"
#include "util.h"
//---------------------------------------------------------------------------------------------------------------------
void pbu_retx_timer_handler(struct tq_elem *tqe);

struct ingress_if_info ingress_if;

//---------------------------------------------------------------------------------------------------------------------
/**
 * Functions to manipulate a route for the MN:
 * downlink is the interface id for the downstream route
 * uplink is the interface id for the upstream route
 */
int mar_setup_route(struct in6_addr *target_route, int pref_len, int downlink, int uplink)
{
	int res = 0;
	if (conf.TunnelingEnabled)
	{
		if (downlink > 0) {
			dbg("Downlink: Add new route for %x:%x:%x:%x:%x:%x:%x:%x in table %d\n", NIP6ADDR(target_route), RT6_TABLE_MIP6);
			res = route_add(downlink, RT6_TABLE_MIP6, RTPROT_MIP, 0, IP6_RT_PRIO_MIP6_FWD, &in6addr_any, 0, target_route, pref_len, NULL);
		}
		if (uplink > 0) {
			dbg("Uplink: Add new rule for tunneling src=%x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(target_route));
			res |= rule_add(NULL, RT6_TABLE_DMM+(uplink % RT6_TABLE_DMM), IP6_RULE_PRIO_DMM_FWD, RTN_UNICAST, target_route, pref_len, &in6addr_any, 0, 0);
		}
		if (res < 0) {
			dbg("ERROR Add new rule for tunneling");
		}
	} else {
        dbg("WARNING CANNOT ADD ROUTING RULES SINCE TUNNELING DISABLED IN CONFIG\n");
	}
	return res;
}

int mar_remove_route(struct in6_addr *target_route, int pref_len, int downlink, int uplink)
{
    int res = 0;
    if (conf.TunnelingEnabled) {
	if (uplink > 0 ) {//Delete existing rule for the deleted MN
		dbg("Uplink: Delete old rule for tunneling src=%x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(target_route));
		res = rule_del(NULL, RT6_TABLE_DMM+(uplink % RT6_TABLE_DMM), IP6_RULE_PRIO_DMM_FWD, RTN_UNICAST, target_route, pref_len, &in6addr_any, 0, 0);
        }
        if (downlink > 0) {
                dbg("Downlink: Delete old routes for: %x:%x:%x:%x:%x:%x:%x:%x from table %d\n", NIP6ADDR(target_route), RT6_TABLE_MIP6);
		res |= route_del(downlink, RT6_TABLE_MIP6, IP6_RT_PRIO_MIP6_FWD, &in6addr_any, 0, target_route, pref_len, NULL);
	}
        if (res < 0) {
            dbg("ERROR Del downlink route for tunneling\n");
        }
        } else {
        dbg("WARNING CANNOT DELETE ROUTING RULES SINCE TUNNELING DISABLED IN CONFIG\n");
    }
    return res;
}
//---------------------------------------------------------------------------------------------------------------------
/**
 *Functions to manipulate the MN's registration
 */
//---------------------------------------------------------------------------------------------------------------------
/**
 * De-registration of a MN connected to another MAR
 */
int mar_dereg_dmm(dmm_entry * bce)
{
	del_task(&bce->tqe);
	dbg("Deleting routes...\n");
	mar_remove_route(&bce->mn_prefix, 64, bce->tunnel, 0);
	int usercount = tunnel_getusers(bce->tunnel);
	dbg("# of binding entries %d\n", usercount);
	if (usercount == 1) {
		dbg("Deleting routing entry for uplink traffic\n");
		route_del(bce->tunnel, RT6_TABLE_DMM+(bce->tunnel % RT6_TABLE_DMM), IP6_RT_PRIO_MIP6_FWD, &in6addr_any, 0, &in6addr_any, 0, NULL);
	}
	dbg("Deleting tunnel...\n");
	tunnel_del(bce->tunnel, 0, 0);
	dbg("Create PBA for deregistration \n");
	struct in6_addr_bundle addrs;
	struct timespec lifetime = { 0, 0 };
	addrs.src = &conf.OurAddress;
	addrs.dst = &conf.CmdAddress;
	mh_send_pba_to_CMD(&addrs, bce, &lifetime, 0);
	timers_log_dreg_sent();
	bce->type = BCE_NO_ENTRY;
	return 0;
}

/**
De-registration of a MN locally connected
*/
int mar_dereg(dmm_entry * bce, int propagate)
{
	//Delete existing route & rule for the deleted MN
	int res = 0;
	bce->type = BCE_NO_ENTRY;
	res = mar_remove_route(&bce->mn_prefix, 64, bce->link, 0);
	int usercount = 0;
	struct mar_list_entry *ptr = bce->mar_list;
	while (ptr != NULL) {
		usercount = tunnel_getusers(ptr->tunnel_id);
		dbg("# of binding entries %d\n", usercount);
		if (usercount == 1) {
				dbg("Deleting routing entry for uplink traffic\n");
				route_del(ptr->tunnel_id, RT6_TABLE_DMM+(ptr->tunnel_id % RT6_TABLE_DMM), IP6_RT_PRIO_MIP6_FWD, &in6addr_any, 0, &in6addr_any, 0, NULL);
		}
		if (ingress_if.dlif_enabled) {
			mar_remove_route(&ptr->mn_prefix, 64, ptr->dlif_id, ptr->tunnel_id);
			delete_dlif(ptr->dlif_id);
		} else mar_remove_route(&ptr->mn_prefix, 64, bce->link, ptr->tunnel_id);
		tunnel_del(ptr->tunnel_id, 0, 0);
		ptr = ptr->next;
	}
        //decrement users of old tunnel.
	if (ingress_if.dlif_enabled) delete_dlif(bce->link);
	if (propagate) {
		dbg("Propagate the deregistration... \n");
		struct in6_addr_bundle addrs;
		addrs.src = &conf.OurAddress;
		addrs.dst = &conf.CmdAddress;
		struct timespec Lifetime = { 0, 0 };
		dbg("Create PBU for LMA for deregistration....\n");
		mh_send_pbu(&addrs, bce, &Lifetime, 0);
		timers_log_dreg_sent();
	}
	dbg("Delete PBU entry....\n");
	return res;
}

/**
 * Registration attempt of a MN locally connected: the MAR sends a PBU to the CMD
 */
int mar_start_registration(dmm_entry * bce)
{
	//Create PBU and send to the CMD
	struct in6_addr_bundle addrs;
	addrs.src = &conf.MarAddressEgress;
	addrs.dst = &conf.CmdAddress;
	mh_send_pbu(&addrs, bce, &conf.PBULifeTime, 0);
	timers_log_pbu_sent();
	//add a new task for PBU retransmission.
	struct timespec expires;
	clock_gettime(CLOCK_REALTIME, &bce->add_time);
	tsadd(bce->add_time, conf.RetransmissionTimeOut, expires);
	add_task_abs(&expires, &bce->tqe, pbu_retx_timer_handler);
	return 0;
}

/**
 * Finalize registration of a MN locally connected: the MAR stores the BCE and sends a RA
 */
int mar_end_registration(dmm_entry * bce, int iif)
{
	mar_send_ra(&bce->mn_prefix, bce->link, &bce->mn_link_local_addr, 0);
	bce->type = BCE_DMM;
	bce->mn_serv_mar_addr = conf.OurAddress;
	dbg("New DMM cache entry type: %d\n", bce->type);
	bce->n_rets_counter = conf.MaxMessageRetransmissions;

	if (bce->mar_list) {
		struct dlif_entry *dlif;
		int usercount = 0;
		if (!IN6_ARE_ADDR_EQUAL(&bce->mar_list->mar_address, &conf.OurAddress)) {
			bce->mar_list->tunnel_id = tunnel_add(&conf.OurAddress, &bce->mar_list->mar_address, iif, 0, 0);
			usercount = tunnel_getusers(bce->mar_list->tunnel_id);
			dbg("# of binding entries %d\n", usercount);
			dbg("Add routing entry for uplink traffic\n");
			route_add(bce->mar_list->tunnel_id, RT6_TABLE_DMM+(bce->mar_list->tunnel_id % RT6_TABLE_DMM), RTPROT_MIP, 0, IP6_RT_PRIO_MIP6_FWD, &in6addr_any, 0, &in6addr_any, 0, NULL);
			if (ingress_if.dlif_enabled) {
				dlif = setup_dlif(&bce->mn_hw_address, &bce->mar_list->mar_hw_address, &bce->mar_list->mn_prefix, 0);
				if (dlif) {
					mar_setup_route(&bce->mar_list->mn_prefix, 64, dlif->dlif_id, bce->mar_list->tunnel_id);
					bce->mar_list->dlif_id = dlif->dlif_id;
					dbg("test dlif: %d\n", bce->mar_list->dlif_id);
					dbg("Sending RA to deprecate old prefix: %x:%x:%x:%x:%x:%x:%x:%x\n", &bce->mar_list->mn_prefix);
					mar_send_ra(&bce->mar_list->mn_prefix, dlif->dlif_id, &bce->mn_link_local_addr, 1);
				}
			} else {
				mar_setup_route(&bce->mar_list->mn_prefix, 64, bce->link, bce->mar_list->tunnel_id);
				mar_send_ra(&bce->mar_list->mn_prefix, bce->link, &bce->mn_link_local_addr, 1);
			}
		}
		struct mar_list_entry *ptr = bce->mar_list->next;
		while (ptr != NULL) {
			if (!IN6_ARE_ADDR_EQUAL(&ptr->mar_address, &conf.OurAddress)) {
				ptr->tunnel_id = tunnel_add(&conf.OurAddress, &ptr->mar_address, iif, 0, 0);
				usercount = tunnel_getusers(ptr->tunnel_id);
				dbg("# of binding entries %d\n", usercount);
				dbg("Add routing entry for uplink traffic\n");
				route_add(ptr->tunnel_id, RT6_TABLE_DMM+(ptr->tunnel_id % RT6_TABLE_DMM), RTPROT_MIP, 0, IP6_RT_PRIO_MIP6_FWD, &in6addr_any, 0, &in6addr_any, 0, NULL);
				if (ingress_if.dlif_enabled) {
					dlif = setup_dlif(&bce->mn_hw_address, &ptr->mar_hw_address, &ptr->mn_prefix, 0);
					if (dlif) {
						mar_setup_route(&ptr->mn_prefix, 64, dlif->dlif_id, ptr->tunnel_id);
						ptr->dlif_id = dlif->dlif_id;
						dbg("test dlif: %d\n", ptr->dlif_id);
						dbg("Sending RA to deprecate old prefix: %x:%x:%x:%x:%x:%x:%x:%x\n", &ptr->mn_prefix);
						mar_send_ra(&ptr->mn_prefix, dlif->dlif_id, &bce->mn_link_local_addr, 1);
					}
				} else {
					mar_setup_route(&ptr->mn_prefix, 64, bce->link, ptr->tunnel_id);
					mar_send_ra(&ptr->mn_prefix, bce->link, &bce->mn_link_local_addr, 1);
				}
			}
			ptr = ptr->next;
		}
        } else {
	    dbg("No P-MAR(s) found -> no tunnel created\n");
	}
	
	if (bce->tunnel > 0) {
		tunnel_del(bce->tunnel, 0, 0);
		bce->tunnel = 0;
	}
	//Add task for entry expiry.
	dbg("Timer for Expiry is initialized: %d(s)!\n", bce->lifetime.tv_sec);
	dmm_cache_start(bce);
	return 0;
}

/**
 * Update registration of a MN that moved to another MAR
 */
int mar_end_registration_dmm(dmm_entry * bce, int iif)
{
	dmm_cache_stop(bce);
	bce->type = BCE_DMM;
	if (bce->tunnel > 0) {
		dbg("Moving tunnel\n");
		int tmp = tunnel_mod(bce->tunnel, &conf.OurAddress, &bce->mn_serv_mar_addr, iif, 0, 0);
		if (tmp != bce->tunnel) {
			mar_remove_route(&bce->mn_prefix, 64, bce->tunnel, 0);
			bce->tunnel = tmp;
			mar_setup_route(&bce->mn_prefix, 64, bce->tunnel, 0);
		}			
	} else {
		bce->tunnel = tunnel_add(&conf.OurAddress, &bce->mn_serv_mar_addr, iif, 0, 0);
		mar_remove_route(&bce->mn_prefix, 64, bce->link, 0);
		mar_setup_route(&bce->mn_prefix, 64, bce->tunnel, 0);
	}
	
	if (bce->mar_list != NULL) {
		struct mar_list_entry *ptr = bce->mar_list;
		int usercount = 0;
		while (ptr != NULL) {
			usercount = tunnel_getusers(ptr->tunnel_id);
			dbg("# of binding entries %d\n", usercount);
			if (usercount == 1) {
				dbg("Deleting routing entry for uplink traffic\n");
				route_del(ptr->tunnel_id, RT6_TABLE_DMM+(ptr->tunnel_id % RT6_TABLE_DMM), IP6_RT_PRIO_MIP6_FWD, &in6addr_any, 0, &in6addr_any, 0, NULL);
			}
			if (ingress_if.dlif_enabled) {
				mar_remove_route(&ptr->mn_prefix, 64, ptr->dlif_id, ptr->tunnel_id);
				delete_dlif(ptr->dlif_id);
			} else mar_remove_route(&ptr->mn_prefix, 64, bce->link, ptr->tunnel_id);
			tunnel_del(ptr->tunnel_id, 0, 0);
			ptr = ptr->next;
		}
		mar_list_clean(bce->mar_list);
		bce->mar_list = NULL;
        } else {
	    dbg("No P-MAR(s) found -> no tunnel created\n");
	}
	
	if (ingress_if.dlif_enabled) delete_dlif(bce->link); else bce->link = 0;
	struct in6_addr_bundle addrs;
        addrs.src = &conf.OurAddress;
        addrs.dst = &conf.CmdAddress;
        mh_send_pba_to_CMD(&addrs, bce, &bce->lifetime, 0);
	timers_log_pba_sent();
	return 0;
}

/**
 * Finalize refreshing registration of a MN locally connected
 */
int mar_end_registration_no_new_tunnel(dmm_entry * bce, int iif)
{
	bce->type = BCE_DMM;
	dbg("DMM cache entry type: %d\n", bce->type);
	//Reset the Retransmissions counter.
	bce->n_rets_counter = conf.MaxMessageRetransmissions;
	//Add task for entry expiry.
	dmm_cache_start(bce);
	struct mar_list_entry *tmp = bce->mar_list;
	mar_send_ra(&bce->mn_prefix, bce->link, &bce->mn_link_local_addr, 0);
	while (tmp) {
		if (ingress_if.dlif_enabled) mar_send_ra(&tmp->mn_prefix, tmp->dlif_id, &bce->mn_link_local_addr, 1);
		else mar_send_ra(&tmp->mn_prefix, bce->link, &bce->mn_link_local_addr, 1);
    		tmp=tmp->next;
	}
	return 0;
}

/**
 * Start refreshing registration of a MN locally connected
 */
int mar_force_update_registration(dmm_entry * bce, int iif)
{
	if (bce->tqe.task != NULL) {
		dbg("Deleting current BCE task\n");
		del_task(&bce->tqe);
	}
	struct in6_addr_bundle addrs;
	addrs.src = &conf.MarAddressEgress;
	addrs.dst = &conf.CmdAddress;
	mh_send_pbu(&addrs, bce, &conf.PBULifeTime, 0);
	struct timespec expires;
	clock_gettime(CLOCK_REALTIME, &bce->add_time);
	tsadd(bce->add_time, conf.RetransmissionTimeOut, expires);
	add_task_abs(&expires, &bce->tqe, pbu_retx_timer_handler);
	dbg("PBU Retransmissions Timer is registered....\n");
	return 0;
}

//---------------------------------------------------------------------------------------------------------------------
int mar_update_binding_entry(dmm_entry * bce, msg_info_t * info)
{
	int update = 0;
	if (!IN6_ARE_ADDR_EQUAL(&bce->mn_serv_mar_addr, &conf.OurAddress)) {
		dbg("MN previously attached to %x:%x:%x:%x:%x:%x:%x:%x \n", NIP6ADDR(&bce->mn_serv_mar_addr));
		bce->mn_serv_mar_addr = conf.OurAddress;
		bce->type = BCE_TEMP;
		bce->handoffInd = HI_NEW_IF;
		if (ingress_if.dlif_enabled) {
			struct dlif_entry *tmp = setup_dlif(&bce->mn_hw_address, &ingress_if.if_llid, &bce->mn_prefix, 1);
			if (tmp) {
				bce->link = tmp->dlif_id;
				bce->mar_hw_address = tmp->dlif_mac;
			} else {
				dbg("DLIF setup failed, using PHY IF \n");
				bce->link = info->iif;
				bce->mar_hw_address = in6addr_any;
			}
		} else {
			bce->link = info->iif;
			bce->mar_hw_address = in6addr_any;
		}
		if (bce->tunnel > 0) mar_remove_route(&bce->mn_prefix, 64, bce->tunnel, 0);
		dbg("Adding route for : %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(get_mn_addr(bce)));
		mar_setup_route(&bce->mn_prefix, 64, bce->link, 0);
		update = 1;
	} else {
		dbg("Refreshing binding entry\n");
		bce->handoffInd = HI_UNCHANGED;
	}
	bce->lifetime = info->lifetime;
	
	bce->n_rets_counter = conf.MaxMessageRetransmissions;
	//bce->seqno_in = info->seqno;
	return update;
}
//---------------------------------------------------------------------------------------------------------------------
int mar_create_binding_entry(msg_info_t * info, dmm_entry * bce)
{
	if (bce != NULL) {
		dbg("Making BCE entry for MN  %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(&info->mn_iid));
		bce->mn_prefix = info->mn_prefix;
		bce->our_addr = conf.OurAddress;
		bce->mn_suffix = info->mn_iid;
		bce->mn_hw_address = EUI64_to_EUI48(info->mn_iid);
		CONVERT_ID2ADDR(&bce->mn_addr, &bce->mn_prefix, &bce->mn_suffix);
		bce->mn_serv_mar_addr = conf.OurAddress;
		bce->mn_serv_cmd_addr = conf.CmdAddress;
		bce->mar_list = NULL;
		if (!IN6_IS_ADDR_UNSPECIFIED(&info->mn_link_local_addr))
			bce->mn_link_local_addr = info->mn_link_local_addr;
		else {
			struct in6_addr *link_local = link_local_addr(&bce->mn_suffix);
			bce->mn_link_local_addr = *link_local;
		}
		bce->seqno_out = 0;
		bce->accTechType = ATT_WLAN; //ATTENTION we need to check the type of iif to fill this field
		bce->handoffInd = HI_NEW_IF;
		if (ingress_if.dlif_enabled) {
			struct dlif_entry *tmp = setup_dlif(&bce->mn_hw_address, &ingress_if.if_llid, &bce->mn_prefix, 1);
			if (tmp) {
				bce->link = tmp->dlif_id;
				bce->mar_hw_address = tmp->dlif_mac;
				printf("DLIF name: %s, device: %d\n", tmp->dlif_name, tmp->dlif_id);
				printf("MAC: 	    %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(&tmp->dlif_mac));
				printf("Link Local: %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(&tmp->dlif_linklocal));
			} else {
				dbg("DLIF setup failed, using PHY IF \n");
				bce->link = info->iif;
				bce->mar_hw_address = in6addr_any;
			}
		} else {
			bce->link = info->iif;
			bce->mar_hw_address = in6addr_any;
		}
		bce->type = BCE_TEMP;
		
		dbg("Adding route for : %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(get_mn_addr(bce)));
		mar_setup_route(&bce->mn_prefix, 64, bce->link, 0);
	} else {
		dbg("Problems creating BCE\n");
		return -1;
	}
	return 0;
}
//---------------------------------------------------------------------------------------------------------------------
int mar_send_ra(struct in6_addr *mn_prefix, int interface, struct in6_addr *dst, int dup)
{
	struct in6_addr *src;
	src = malloc(sizeof(struct in6_addr));
	memset(src, 0, sizeof(struct in6_addr));
	struct iovec iov;
	struct nd_router_advert *radvert;
	adv_prefix_t prefix;
	unsigned char buff[MSG_SIZE];
	size_t len = 0;
	memset(&buff, 0, sizeof(buff));
	radvert = (struct nd_router_advert *) buff;
	radvert->nd_ra_type = ND_ROUTER_ADVERT;
	radvert->nd_ra_code = 0;
	radvert->nd_ra_cksum = 0;
	radvert->nd_ra_curhoplimit = router_ad_iface.AdvCurHopLimit;
	radvert->nd_ra_flags_reserved = (router_ad_iface.AdvManagedFlag) ? ND_RA_FLAG_MANAGED : 0;
	radvert->nd_ra_flags_reserved |= (router_ad_iface.AdvOtherConfigFlag) ? ND_RA_FLAG_OTHER : 0;
	/* Mobile IPv6 ext */
	radvert->nd_ra_flags_reserved |= (router_ad_iface.AdvHomeAgentFlag) ? ND_RA_FLAG_HOME_AGENT : 0;
	/* if forwarding is disabled, send zero router lifetime */
	radvert->nd_ra_router_lifetime = htons(router_ad_iface.AdvDefaultLifetime);
	radvert->nd_ra_reachable = htonl(router_ad_iface.AdvReachableTime);
	radvert->nd_ra_retransmit = htonl(router_ad_iface.AdvRetransTimer);
	len = sizeof(struct nd_router_advert);
	prefix = router_ad_iface.Adv_Prefix;
	/**
         *  add prefix options
	 */
	struct nd_opt_prefix_info *pinfo;
	pinfo = (struct nd_opt_prefix_info *) (buff + len);
	pinfo->nd_opt_pi_type = ND_OPT_PREFIX_INFORMATION;
	pinfo->nd_opt_pi_len = 4;
	pinfo->nd_opt_pi_prefix_len = prefix.PrefixLen;
	pinfo->nd_opt_pi_flags_reserved = (prefix.AdvOnLinkFlag) ? ND_OPT_PI_FLAG_ONLINK : 0;
	pinfo->nd_opt_pi_flags_reserved |= (prefix.AdvAutonomousFlag) ? ND_OPT_PI_FLAG_AUTO : 0;
	/* Mobile IPv6 ext */
	pinfo->nd_opt_pi_flags_reserved |= (prefix.AdvRouterAddr) ? ND_OPT_PI_FLAG_RADDR : 0;
	pinfo->nd_opt_pi_valid_time = htonl(prefix.AdvValidLifetime);
	if (dup)
		pinfo->nd_opt_pi_preferred_time = 0;
	else
		pinfo->nd_opt_pi_preferred_time = htonl(prefix.AdvPreferredLifetime);
	pinfo->nd_opt_pi_reserved2 = 0;
	memcpy(&pinfo->nd_opt_pi_prefix, mn_prefix, sizeof(struct in6_addr));
	len += sizeof(*pinfo);
	//mobile ip extension
	if (router_ad_iface.AdvHomeAgentInfo
	&& (router_ad_iface.AdvMobRtrSupportFlag || router_ad_iface.HomeAgentPreference != 0 || router_ad_iface.HomeAgentLifetime != router_ad_iface.AdvDefaultLifetime)) {
		home_agent_info_t ha_info;
		ha_info.type = ND_OPT_HOME_AGENT_INFO;
		ha_info.length = 1;
		ha_info.flags_reserved = (router_ad_iface.AdvMobRtrSupportFlag) ? ND_OPT_HAI_FLAG_SUPPORT_MR : 0;
		ha_info.preference = htons(router_ad_iface.HomeAgentPreference);
		ha_info.lifetime = htons(router_ad_iface.HomeAgentLifetime);
		memcpy(buff + len, &ha_info, sizeof(ha_info));
		len += sizeof(ha_info);
	}
	iov.iov_len = len;
	iov.iov_base = (caddr_t) buff;
	int err;
	err = icmp6_send(interface, 255, src, dst, &iov, 1);
    	if (err < 0) {
		dbg("Error: couldn't send a RA message (IF %d)\n", interface);
        } else {
	        dbg("RA LL ADDRESS sent\n");
		timers_log_stop();
    	}
	return err;
}
//---------------------------------------------------------------------------------------------------------------------
int check_ip6_forwarding(void)
{
	int forw_sysctl[] = { SYSCTL_IP6_FORWARDING };
	int value;
	int rc;
	size_t size = sizeof(value);
	FILE *fp = NULL;
#ifdef __linux__
	fp = fopen(PROC_SYS_IP6_FORWARDING, "r");
	if (fp) {
		rc = fscanf(fp, "%d", &value);
		fclose(fp);
		if (rc <= 0) {
			dbg("ERROR reading %s" ,PROC_SYS_IP6_FORWARDING);
		}
	} else
		dbg("Correct IPv6 forwarding procfs entry not found, " "perhaps the procfs is disabled, " "or the kernel interface has changed?");
#endif              /* __linux__ */
	if (!fp && sysctl(forw_sysctl, sizeof(forw_sysctl) / sizeof(forw_sysctl[0]), &value, &size, NULL, 0) < 0) {
		dbg("Correct IPv6 forwarding sysctl branch not found, " "perhaps the kernel interface has changed?");
		return (0);     /* this is of advisory value only */
	}
	if (value != 1) {
		dbg("IPv6 forwarding setting is: %u, should be 1\n", value);
		return (-1);
	}
	return (0);
}

//---------------------------------------------------------------------------------------------------------------------
/*!
 * The following fuctions are related to DLIf
 *
 */

int dlif_list_init(void)
{
	int mutex_return_code;
	if (pthread_rwlock_init(&ingress_if.dlif_lock, NULL)) {
		dbg("DLIF lock not initialized\n");
		return -1;
	}
	mutex_return_code = pthread_rwlock_wrlock(&ingress_if.dlif_lock);
	if (mutex_return_code != 0) {
		dbg("pthread_rwlock_wrlock(&dlif_lock) %s\n", strerror(mutex_return_code));
	}
	ingress_if.dlif_enabled = 1;
	ingress_if.dlif_list_head = NULL;
	
	mutex_return_code = pthread_rwlock_unlock(&ingress_if.dlif_lock);
	if (mutex_return_code != 0) {
		dbg("pthread_rwlock_unlock(&dlif_lock) %s\n", strerror(mutex_return_code));
	}
	if (set_iface_proc_entry(PROC_SYS_IP6_DAD_TRANSMITS,"all",0) < 0)
		dbg("WARNING: Cannot set all/dad_transmits\n");
	if (set_iface_proc_entry(PROC_SYS_IP6_DAD_TRANSMITS,"default",0) < 0)
		dbg("WARNING: Cannot set default/dad_transmits\n");
	if (set_iface_proc_entry(PROC_SYS_IP6_ACCEPT_DAD,"all",0) < 0)
		dbg("WARNING: Cannot set all/accept_dad\n");
	if (set_iface_proc_entry(PROC_SYS_IP6_ACCEPT_DAD,"default",0) < 0)
		dbg("WARNING: Cannot set default/accept_dad\n");
	dbg("DLIF data structure initialized\n");
	return 1;
}
	
int dlif_exists(char *name)
{
	struct dlif_entry *tmp = ingress_if.dlif_list_head; 
	int result = 0;
	while (tmp) {
		if (strcmp(tmp->dlif_name, name) == 0) {
			dbg("Interface %s already exists\n", name);
			result = tmp->dlif_id;
		}
		tmp = tmp->next;
	}
	return result;
}

struct dlif_entry *get_dlif_entry(int iif)
{
	struct dlif_entry *tmp = ingress_if.dlif_list_head;
	while (tmp){
		if (tmp->dlif_id == iif) {
			return tmp;
		}
		tmp = tmp->next;
	}
	return tmp;
}

int delete_dlif(int iif)
{
	dbg("Deleting old DLIF\n");
	struct dlif_entry *tmp = ingress_if.dlif_list_head;
	if (!tmp) {
		dbg("No list of DLIfs\n");
		return -1;
	}
	if (tmp->dlif_id == iif) {
		dlif_del(iif);
		ingress_if.dlif_list_head=ingress_if.dlif_list_head->next; 
		free(tmp);
		return 1;
	}
	while (tmp->next){
		if (tmp->next->dlif_id == iif) {
			dlif_del(iif);
			struct dlif_entry *tmp2 = tmp->next;
			tmp->next=tmp->next->next;
			free(tmp2);
			return 1;
		}
		tmp = tmp->next;
	}
	dbg("ERROR: No entry found\n");
	return -1;
}

void dlif_cleanup(void)
{
	if (set_iface_proc_entry(PROC_SYS_IP6_DAD_TRANSMITS,"all",1) < 0)
		dbg("WARNING: Cannot set all/dad_transmits\n");
	if (set_iface_proc_entry(PROC_SYS_IP6_DAD_TRANSMITS,"default",1) < 0)
		dbg("WARNING: Cannot set default/dad_transmits\n");
	if (set_iface_proc_entry(PROC_SYS_IP6_ACCEPT_DAD,"all",1) < 0)
		dbg("WARNING: Cannot set all/accept_dad\n");
	if (set_iface_proc_entry(PROC_SYS_IP6_ACCEPT_DAD,"default",1) < 0)
		dbg("WARNING: Cannot set default/accept_dad\n");
	
	if (ingress_if.dlif_enabled) {
		struct dlif_entry *tmp = ingress_if.dlif_list_head;
		struct dlif_entry *tmp2;
		if (!tmp) {
			dbg("No list of DLIfs\n");
			return;
		}
		while (tmp) {
			dlif_del(tmp->dlif_id);
			tmp2 =tmp;
			tmp=tmp->next;
			free(tmp2);
		}
		ingress_if.dlif_list_head = NULL;
	}
	return;
}

struct dlif_entry *setup_dlif(struct in6_addr *mn_llid, struct in6_addr *mar_llid, struct in6_addr *prefix, int local)
{	
	int iif = 0;
	char name[14];
	char mac[18];
	uint8_t ll_addr[6];
	
	if (local) {
		sprintf(name, "00%02x%02x%02x%02x%02x", mn_llid->s6_addr[13], mar_llid->s6_addr[14],
				  mn_llid->s6_addr[14], mar_llid->s6_addr[15], mn_llid->s6_addr[15]);
		sprintf(mac, "00:%02x:%02x:%02x:%02x:%02x", mn_llid->s6_addr[13], mar_llid->s6_addr[14],
			             mn_llid->s6_addr[14], mar_llid->s6_addr[15], mn_llid->s6_addr[15]);
		ll_addr[0] = 0x00;
		ll_addr[1] = mn_llid->s6_addr[13];
		ll_addr[2] = mar_llid->s6_addr[14];
		ll_addr[3] = mn_llid->s6_addr[14];
		ll_addr[4] = mar_llid->s6_addr[15];
		ll_addr[5] = mn_llid->s6_addr[15];
	} else {
		sprintf(name, "00%02x%02x%02x%02x%02x", mar_llid->s6_addr[11], mar_llid->s6_addr[12],
				mar_llid->s6_addr[13], mar_llid->s6_addr[14], mar_llid->s6_addr[15]);
		sprintf(mac, "00:%02x:%02x:%02x:%02x:%02x", mar_llid->s6_addr[11], mar_llid->s6_addr[12],
				    mar_llid->s6_addr[13], mar_llid->s6_addr[14], mar_llid->s6_addr[15]);
		ll_addr[0] = 0x00;
		ll_addr[1] = mar_llid->s6_addr[11];
		ll_addr[2] = mar_llid->s6_addr[12];
		ll_addr[3] = mar_llid->s6_addr[13];
		ll_addr[4] = mar_llid->s6_addr[14];
		ll_addr[5] = mar_llid->s6_addr[15];
	}
	
	struct dlif_entry *tmp;
	iif = dlif_exists(name);
	if (!iif){
		iif = dlif_add(ingress_if.eif_id, name, ll_addr);
		if (iif > 0) {
			if (mn_llid) {
				int j;
				struct in6_addr dst_addr = EUI48_to_EUI64(*mn_llid);
				dst_addr.s6_addr[0] = 0xfe;
				dst_addr.s6_addr[1] = 0x80;
				uint8_t mnhw[6];
				for (j=0; j<6; j++) {
					mnhw[j]=mn_llid->s6_addr[j+10];
				}
				printf("\n");
				int neigh = neigh_add_dlif(iif, &dst_addr, mnhw);
				if (prefix) {
					for (j=0; j<8; j++) {
						dst_addr.s6_addr[j]=prefix->s6_addr[j];
					}
					neigh = neigh_add_dlif(iif, &dst_addr, mnhw);
				}
			}
			if (local) dbg("DLIF for MN %x:%x:%x:%x:%x:%x:%x:%x name: %s MAC: %s\n", NIP6ADDR(mn_llid), name, mac);
			else dbg("DLIF for P-MAR(s) - name: %s MAC: %s\n", name, mac);
			tmp = ingress_if.dlif_list_head;
			int i;
			if (!tmp) {
				ingress_if.dlif_list_head = (struct dlif_entry *)malloc(sizeof(struct dlif_entry));
				memset(ingress_if.dlif_list_head, 0, sizeof(struct dlif_entry));
				ingress_if.dlif_list_head->dlif_id = iif;
				ingress_if.dlif_list_head->dlif_mac = in6addr_any;
				for (i = 0; i < 6; i++) {
					ingress_if.dlif_list_head->dlif_mac.s6_addr[i+10] = ll_addr[i];
				}
				ingress_if.dlif_list_head->dlif_linklocal = EUI48_to_EUI64(ingress_if.dlif_list_head->dlif_mac);
				ingress_if.dlif_list_head->dlif_linklocal.s6_addr[0] = 0xfe;
				ingress_if.dlif_list_head->dlif_linklocal.s6_addr[1] = 0x80;
				strcpy(ingress_if.dlif_list_head->dlif_name, name);
				ingress_if.dlif_list_head->next=NULL;
				tmp = ingress_if.dlif_list_head;
			} else {
				while (tmp->next) {
					tmp = tmp->next;
				}
				tmp->next = (struct dlif_entry *)malloc(sizeof(struct dlif_entry));
				memset(tmp->next, 0, sizeof(struct dlif_entry));
				tmp->next->dlif_id = iif;
				tmp->next->dlif_mac = in6addr_any;
				for (i = 0; i < 6; i++) {
					tmp->next->dlif_mac.s6_addr[i+10] = ll_addr[i];
				}
				tmp->next->dlif_linklocal = EUI48_to_EUI64(tmp->next->dlif_mac);
				tmp->next->dlif_linklocal.s6_addr[0] = 0xfe;
				tmp->next->dlif_linklocal.s6_addr[0] = 0x80;
				strcpy(tmp->next->dlif_name, name);
				tmp->next->next=NULL;
				tmp = tmp->next;
			}
		}
	} else {
		tmp = get_dlif_entry(iif);
	}
	return tmp;
}

int mar_get_ingress_info(void)
{	
	FILE *fp;
	char str_addr[INET6_ADDRSTRLEN];
	unsigned int plen, scope, dad_status, if_idx;
	unsigned int ap;
 	char devname[IFNAMSIZ];
	ingress_if.eif_id = 0;
	memset(ingress_if.eif_name,0,IFNAMSIZ*sizeof(char));
	ingress_if.if_llid = in6addr_any;
	ingress_if.if_ll_addr = in6addr_any;	
	
	if ((fp = fopen("/proc/net/if_inet6", "r")) == NULL) {
		dbg("can't open %s:", "/proc/net/if_inet6");
		return -1;
	}
	while (fscanf(fp, "%32s %x %02x %02x %02x %15s\n", str_addr, &if_idx, &plen, &scope, &dad_status, devname) != EOF)
	{
		if (scope == IPV6_ADDR_LINKLOCAL && strcmp(devname, conf.MarDeviceIngress) == 0)
		{
			ingress_if.eif_id = (int)if_idx;
			strcpy(ingress_if.eif_name, conf.MarDeviceIngress);
			for (int i = 0; i < 16; i++) {
				sscanf(str_addr + i * 2, "%02x", &ap);
				ingress_if.if_ll_addr.s6_addr[i] = (unsigned char) ap;
			}
			ingress_if.if_llid = EUI64_to_EUI48(ingress_if.if_ll_addr);
			ingress_if.dlif_enabled = 0;
			ingress_if.dlif_list_head = NULL;
		}
	}
	if (ingress_if.eif_id == 0) {
		dbg("ERROR: No link local address configured on ingress interface\n");
		fclose(fp);
		return -1;
	} else {
		fclose(fp);
		dbg("Ingress interface used: %s, device %d\n", ingress_if.eif_name, ingress_if.eif_id);
		/**
		 * printf("Ingress Interface used: %s, device %d\n", ingress_if.eif_name, ingress_if.eif_id);
		 * printf("MAC address:        %x:%x:%x:%x:%x:%x:%x:%x \n", NIP6ADDR(&ingress_if.if_llid));
		 * printf("Link local address: %x:%x:%x:%x:%x:%x:%x:%x \n", NIP6ADDR(&ingress_if.if_ll_addr));
		 */
		return ingress_if.eif_id;
	}
}

void mar_list_clean_all(struct dmm_entry *bce)
{
	struct mar_list_entry *ptr = bce->mar_list;
	if (!ptr){
		dbg("NO MAR list founf\n");
		return;
	}
	int usercount = 0;
	struct mar_list_entry *ptr2;
	while (ptr) {
		usercount = tunnel_getusers(ptr->tunnel_id);
		dbg("# of binding entries %d\n", usercount);
		if (usercount == 1) {
			dbg("Deleting routing entry for uplink traffic\n");
			route_del(ptr->tunnel_id, RT6_TABLE_DMM+(ptr->tunnel_id % RT6_TABLE_DMM), IP6_RT_PRIO_MIP6_FWD, &in6addr_any, 0, &in6addr_any, 0, NULL);
		}
		if (ingress_if.dlif_enabled) {
			mar_remove_route(&ptr->mn_prefix, 64, ptr->dlif_id, ptr->tunnel_id);
		} else mar_remove_route(&ptr->mn_prefix, 64, bce->link, ptr->tunnel_id);
		tunnel_del(ptr->tunnel_id, 0, 0);
		ptr2 = ptr;
		ptr = ptr->next;
		free(ptr2);
	}
	bce->mar_list = NULL;
	dbg("MAR list cleaned\n");
	return;
}

void mar_list_clean(struct mar_list_entry *ptr)
{
	if (!ptr){
		dbg("NO MAR list found\n");
		return;
	}
	struct mar_list_entry *ptr2;
	while (ptr) {
		ptr2 = ptr;
		ptr = ptr->next;
		free(ptr2);
	}
	dbg("MAR list cleaned\n");
	return;
}
