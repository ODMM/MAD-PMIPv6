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
#define DMM_CMD_PROC_C
#ifdef HAVE_CONFIG_H
#    include <config.h>
#endif
//---------------------------------------------------------------------------------------------------------------------
#include "handlers.h"
#include "hnp_cache.h"
#include "cmd_proc.h"
//---------------------------------------------------------------------------------------------------------------------
#include "rtnl.h"
#ifdef ENABLE_VT
#    include "vt.h"
#endif
#include "debug.h"
#include "conf.h"
#include "util.h"
//---------------------------------------------------------------------------------------------------------------------
int cmd_reg(dmm_entry * bce)
{
	bce->status = 0;        //PBU was Accepted!
	dmm_cache_start(bce);
	//Send a PBA to ack new serving MAR
	dbg("Create PBA to new Serving MAR...\n");
	struct in6_addr_bundle addrs;
	addrs.src = &conf.OurAddress;
	addrs.dst = &bce->mn_serv_mar_addr;
	mh_send_pba(&addrs, bce, &bce->lifetime, 0);
	timers_log_pba_sent();
	return 0;
}
//---------------------------------------------------------------------------------------------------------------------
int cmd_reg_dmm(dmm_entry * bce)
{
	bce->status = 0;
	//Add task for entry expiry.
	dmm_cache_start(bce);
	//Send a PBA to ack new serving MAR
	dbg("Create PBA to new Serving MAR...\n");
	struct in6_addr_bundle addrs;
	addrs.src = &conf.OurAddress;
	addrs.dst = &bce->mn_serv_mar_addr;
	mh_send_pba(&addrs, bce, &bce->lifetime, 0);
	timers_log_pba_sent();
	
	if (!bce->mar_list) {
		dbg("No Previous MAR found\n");
		return -1;
	} else {
		struct mar_list_entry *tmp = bce->mar_list;
		int count = 1;
		while (tmp) {
			if (!IN6_ARE_ADDR_EQUAL(&bce->mn_serv_mar_addr,&tmp->mar_address)){
				addrs.dst = &tmp->mar_address;
				dbg("Sending PBU no %d\n", count);
				mh_send_pbu_to_MAR(&addrs, bce, &bce->lifetime, 0);
				timers_log_pbu_sent();
				count++;
			}
			tmp = tmp->next;
		}
	}
	return 0;
}
//---------------------------------------------------------------------------------------------------------------------
int cmd_end_reg_dmm(dmm_entry * bce, struct in6_addr * prefix)
{
	bce->status = 0;        //PBU was Accepted!
	//Send a PBA to ack new serving MAR
	dbg("Create PBA to new Serving MAR...\n");
	if (!bce->mar_list) {
		dbg("ERROR: No Previous MAR found\n");
		return -1;
	} else {
		struct mar_list_entry *tmp = bce->mar_list;
		int count = 0;
		while (tmp) {
			if (IN6_ARE_ADDR_EQUAL(&tmp->mn_prefix, prefix)) {
				tmp->tunnel_id = TUNNEL_OK;
				count++;
			}
			tmp = tmp->next;
		}
	}
	return 0;
}
//---------------------------------------------------------------------------------------------------------------------
int cmd_dereg(dmm_entry * bce, msg_info_t * info, int propagate)
{
	if (bce != NULL) {
		del_task(&bce->tqe);
		if (propagate) {
			dbg("Create PBA for deregistration for MAR (%x:%x:%x:%x:%x:%x:%x:%x)\n", NIP6ADDR(&bce->mn_serv_mar_addr));
			struct in6_addr_bundle addrs;
			struct timespec lifetime = { 0, 0 };
			addrs.src = &conf.CmdAddress;
			addrs.dst = &bce->mn_serv_mar_addr;
			bce->seqno_in = info->seqno;
			mh_send_pba(&addrs, bce, &lifetime, 0);
			struct mar_list_entry *tmp = bce->mar_list;
			//int count = 0;
			while (tmp) {
				addrs.dst = &tmp->mar_address;
				mh_send_pbu_to_MAR(&addrs, bce, &lifetime, 0);
			//	count++;
				tmp = tmp->next;
			}
		} else {
			dbg("Doing nothing....\n");
		}
		bce->type = BCE_NO_ENTRY;
	} else {
		dbg("WARNING parameter dmm_entry * bce is NULL\n");
		return -1;
	}
	return 0;
}
//---------------------------------------------------------------------------------------------------------------------
void cmd_refresh_binding_entry(dmm_entry * bce, msg_info_t * info)
{
//	int result = 0;
	dbg("Refreshing Binding Entry\n");
	bce->our_addr = conf.OurAddress;
	bce->mn_suffix = info->mn_iid;
	bce->mn_hw_address = EUI64_to_EUI48(info->mn_iid); //This should be taken from the ll id option
	bce->timestamp.first    = info->timestamp.first;
	bce->timestamp.second   = info->timestamp.second;
	bce->mn_prefix          = info->mn_prefix;
	bce->mn_addr            = info->mn_addr;
	bce->mn_link_local_addr = info->mn_link_local_addr;
	bce->mn_serv_mar_addr   = info->src;
	bce->lifetime.tv_sec    = info->lifetime.tv_sec;
	bce->lifetime.tv_nsec   = 0;
	bce->n_rets_counter     = conf.MaxMessageRetransmissions;
	bce->seqno_in           = info->seqno;
	bce->link               = info->iif;
	bce->mar_hw_address	= info->mar_hw_address;
	dbg("Finished updating the binding cache\n");
//	return result;
	return;
}
//---------------------------------------------------------------------------------------------------------------------
int cmd_update_binding_entry(dmm_entry * bce, msg_info_t * info)
{
	int result = 0;
	bce->our_addr = conf.OurAddress;
	bce->mn_suffix = info->mn_iid;
	bce->mn_hw_address = EUI64_to_EUI48(info->mn_iid); 
	dbg("New serving MAR:  %x:%x:%x:%x:%x:%x:%x:%x  (Old MAR: %x:%x:%x:%x:%x:%x:%x:%x)\n", NIP6ADDR(&info->src), NIP6ADDR(&bce->mn_serv_mar_addr));
	if (!bce->mar_list) {
		dbg("First handover\n");
		bce->mar_list = (struct mar_list_entry *)malloc(sizeof(struct mar_list_entry));
		if (!bce->mar_list) {
			dbg("NO memory allocated for previous MAR entry..\n");
			return -1;
		}
		memset(bce->mar_list, 0, sizeof(struct mar_list_entry));
		bce->mar_list->mn_prefix = bce->mn_prefix;
		bce->mar_list->mar_address = bce->mn_serv_mar_addr;
		dbg("Storing previous MAR entry - address: %x:%x:%x:%x:%x:%x:%x:%x - prefix: %x:%x:%x:%x:%x:%x:%x:%x\n",NIP6ADDR(&bce->mn_serv_mar_addr), NIP6ADDR(&bce->mn_prefix));
		bce->mar_list->mar_hw_address	= bce->mar_hw_address;
		bce->mar_list->tunnel_id = TUNNEL_TEMP;
		bce->mar_list->next = NULL;
		result = 1;
	} else {
		int count = 1;
		struct mar_list_entry *tmp = bce->mar_list;
		mar_list_entry *tmp2 = NULL;
		if (IN6_ARE_ADDR_EQUAL(&tmp->mar_address, &info->src)) {
			dbg("MAR %x:%x:%x:%x:%x:%x:%x:%x already visited before\n",NIP6ADDR(&bce->mar_list->mar_address));
			bce->mar_list = bce->mar_list->next;
			tmp2 = tmp;
		} else {
			count++;
			tmp->tunnel_id = TUNNEL_TEMP;
		}
		while (tmp->next) {
			if (IN6_ARE_ADDR_EQUAL(&tmp->next->mar_address, &info->src)) {
				dbg("MAR %x:%x:%x:%x:%x:%x:%x:%x already visited before\n",NIP6ADDR(&tmp->next->mar_address));
				tmp2 = tmp->next;
				tmp->next = tmp->next->next;
			} else {
				tmp = tmp->next;
				tmp->tunnel_id = TUNNEL_TEMP;
				count++;
			}
		}
		dbg("handover no %d\n", count);
		tmp->next = (struct mar_list_entry *)malloc(sizeof(struct mar_list_entry));
		if (!tmp->next) {
			dbg("NO memory allocated for previous MAR entry..\n");
		return -1;
		}
		memset(tmp->next, 0, sizeof(struct mar_list_entry));
		tmp->next->mn_prefix = bce->mn_prefix;
		tmp->next->mar_address = bce->mn_serv_mar_addr;
		dbg("Storing previous MAR entry - address: %x:%x:%x:%x:%x:%x:%x:%x - prefix: %x:%x:%x:%x:%x:%x:%x:%x\n",NIP6ADDR(&bce->mn_serv_mar_addr), NIP6ADDR(&bce->mn_prefix));
		tmp->next->mar_hw_address = bce->mar_hw_address;
		tmp->next->tunnel_id = TUNNEL_TEMP;
		tmp->next->next = NULL;
		//bce->mar_list=tmp;
		if (!bce->mar_list) bce->mar_list = tmp->next;
		if (tmp2) free(tmp2);
		result = 1;
	}
	bce->timestamp.first    = info->timestamp.first;
	bce->timestamp.second   = info->timestamp.second;
	bce->mn_prefix          = info->mn_prefix;
	bce->mn_addr            = info->mn_addr;
	bce->mn_link_local_addr = info->mn_link_local_addr;
	bce->mn_serv_mar_addr   = info->src;
	bce->mar_hw_address	= info->mar_hw_address;
	bce->lifetime.tv_sec    = info->lifetime.tv_sec;
	bce->lifetime.tv_nsec   = 0;
	bce->n_rets_counter     = conf.MaxMessageRetransmissions;
	bce->seqno_in           = info->seqno;
	bce->link               = info->iif;
	dbg("Finished updating the binding cache\n");
	return result;
}

int cmd_create_binding_entry(dmm_entry * bce, msg_info_t * info)
{
	int result = 0;
	if (bce != NULL) {
		if (info != NULL) {
			//int result;
			//struct in6_addr r_tmp, r_tmp1;
			//memset(&r_tmp1, 0, sizeof(struct in6_addr));
			dbg("Store Binding Entry\n");
			bce->our_addr = conf.OurAddress;
			bce->mn_suffix = info->mn_iid;
			bce->mn_hw_address = EUI64_to_EUI48(info->mn_iid); //This should be taken from the ll id option
			bce->timestamp.first    = info->timestamp.first;
			bce->timestamp.second   = info->timestamp.second;
			bce->mn_prefix          = info->mn_prefix;
			//bce->mn_addr            = info->mn_addr;
			bce->mn_link_local_addr = info->mn_link_local_addr;
			bce->mn_serv_mar_addr   = info->src;
			bce->lifetime.tv_sec    = info->lifetime.tv_sec;
			bce->lifetime.tv_nsec   = 0;
			bce->n_rets_counter     = conf.MaxMessageRetransmissions;
			bce->seqno_in           = info->seqno;
			bce->link               = info->iif;
			bce->accTechType	= info->techType;
			bce->handoffInd		= info->handoffInd;
			bce->mar_list 		= NULL;
			bce->mar_hw_address	= info->mar_hw_address;
			dbg("Binding cache created\n");
			
			return result;
        } else {
            dbg("WARNING parameter msg_info_t* info is NULL\n");
            result = -1;
            return result;
        }
    } else {
        dbg("WARNING parameter dmm_entry * bce is NULL\n");
        result = -1;
        return result;
    }
}