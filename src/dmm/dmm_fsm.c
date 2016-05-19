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
#define DMM_FSM_C
#define USE_L2_DETECTION 0
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
//---------------------------------------------------------------------------------------------------------------------
#include "dmm_fsm.h"
#include "hnp_cache.h"
#include "cmd_proc.h"
#include "mar_proc.h"
//---------------------------------------------------------------------------------------------------------------------
#ifdef ENABLE_VT
#    include "vt.h"
#endif
#include "debug.h"
#include "conf.h"
//---------------------------------------------------------------------------------------------------------------------
int mar_init_fsm(void)
{
	if (pthread_rwlock_init(&fsm_lock, NULL))
		return -1;
	else
		return 0;
}
//---------------------------------------------------------------------------------------------------------------------
void mar_fsm(msg_info_t * info)
{
	int aaa_result = 0;
	dmm_entry *bce = NULL;
	struct in6_addr *prefix;
	struct in6_addr hw_address = EUI64_to_EUI48(info->mn_iid);
	int mutex_return_code;

	mutex_return_code = pthread_rwlock_wrlock(&fsm_lock);
	if (mutex_return_code != 0) {
		dbg("pthread_rwlock_wrlock(&fsm_lock) %s\n", strerror(mutex_return_code));
	}
	//dbg("Entered FSM\n");
	bce = dmm_cache_get(&conf.OurAddress, &hw_address);
	switch (info->msg_event) {
	//--------------------------------------
	case hasRS:
		if (!bce) {
			dbg("New MN is found sending RS, checking AAA ...\n");
			prefix = mnid_hnp_map(&hw_address);
			if (prefix) {
				//dbg("AAA success: prefix to assign: %x:%x:%x:%x:%x:%x:%x:%x \n", NIP6ADDR(prefix));
				memcpy(&info->mn_prefix, prefix, sizeof(struct in6_addr));
				/* allocate memory for BCE */
				bce = dmm_cache_alloc(BCE_TEMP);
				if (bce) {				
					/* Fill BCE with MN's parameters */
					mar_create_binding_entry(info, bce);
					/* Send PBU to CMD for registration */
					mar_start_registration(bce);
					dmm_cache_add(bce);
					dbg("BCE registered\n");
				} else {
					dbg("Problems creating BCE\n");
					break;
				}
			} else {
				dbg("AAA failed\n");
			}
		} else {
			switch (bce->type) {
				case BCE_TEMP:
					dbg("BCE already existing but not finalized -> Request Rejected\n");
					dmm_cache_release(bce);
				break;
				case BCE_DMM:
					dbg("Router solicitation received for existing MN:\n");
					int update = mar_update_binding_entry(bce, info);
					if (update == 1) mar_start_registration(bce);
					else {
						mar_force_update_registration(bce, info);
					}
					dmm_cache_release(bce);
					break;
				default:
					dbg("No action for this event (%d) at current state (%d) !\n", info->msg_event, bce->type);
					dmm_cache_release(bce);
				}
		}
	break;
		//--------------------------------------
	case hasPBA:
		if (!bce) {
			dbg("PBA received for not registered MN -> discarded\n");
			break;
		} else {
			switch (bce->type) {
			case BCE_TEMP:
				if (is_pba_is_response_to_last_pbu_sent(info, bce)) {
					dbg("Handling PBA. Moving from BCE_TEMP to BCE_DMM\n");
					
					del_task(&bce->tqe);    //Delete timer retransmission PBU (if any)
					bce->PBA_flags = info->PBA_flags;
					if (info->lifetime.tv_sec > 30) {
						bce->lifetime.tv_sec = info->lifetime.tv_sec - 8;
						dbg("Set BCE lifetime to PBA lifetime (%d seconds) - 8 second = %d seconds\n", info->lifetime.tv_sec, bce->lifetime.tv_sec);
					} else if (info->lifetime.tv_sec > 5) {
						bce->lifetime.tv_sec = info->lifetime.tv_sec - 3;
						dbg("Set BCE lifetime to PBA lifetime (%d seconds) - 3 second = %d seconds\n", info->lifetime.tv_sec, bce->lifetime.tv_sec);
					} else if (info->lifetime.tv_sec > 1) {
							bce->lifetime.tv_sec = info->lifetime.tv_sec - 1;
							dbg("Set BCE lifetime to PBA lifetime (%d seconds) - 1 second = %d seconds\n", info->lifetime.tv_sec, bce->lifetime.tv_sec);
					} else {
						bce->lifetime.tv_sec = info->lifetime.tv_sec;
						dbg("Set BCE lifetime to PBA lifetime (%d seconds) =  %d seconds\n", info->lifetime.tv_sec, bce->lifetime.tv_sec);
					}
					if (info->mn_p_mar_addr) {
						mar_list_clean(bce->mar_list);
						bce->mar_list = info->mn_p_mar_addr;
					}
					mar_end_registration(bce, info->iif);
				}
				dmm_cache_release(bce);
			break;
		//--------------------------------------
			case BCE_DMM:
				if (is_pba_is_response_to_last_pbu_sent(info, bce)) {
					del_task(&bce->tqe);    //Delete timer retransmission PBU (if any)
					bce->PBA_flags = info->PBA_flags;
					bce->lifetime.tv_nsec = 0;
					if (info->lifetime.tv_sec > 10) {
						bce->lifetime.tv_sec = info->lifetime.tv_sec - 8;
						dbg("Set BCE lifetime to PBA lifetime (%d seconds) - 8 second = %d seconds\n", info->lifetime.tv_sec, bce->lifetime.tv_sec);
					} else if (info->lifetime.tv_sec > 5) {
						bce->lifetime.tv_sec = info->lifetime.tv_sec - 3;
						dbg("Set BCE lifetime to PBA lifetime (%d seconds) - 3 second = %d seconds\n", info->lifetime.tv_sec, bce->lifetime.tv_sec);
					} else if (info->lifetime.tv_sec > 1) {
						bce->lifetime.tv_sec = info->lifetime.tv_sec - 1;
						dbg("Set BCE lifetime to PBA lifetime (%d seconds) - 1 second = %d seconds\n", info->lifetime.tv_sec, bce->lifetime.tv_sec);
					} else {
						bce->lifetime.tv_sec = info->lifetime.tv_sec;
						dbg("Set BCE lifetime to PBA lifetime (%d seconds) =  %d seconds\n", info->lifetime.tv_sec, bce->lifetime.tv_sec);
					}
					mar_end_registration_no_new_tunnel(bce, info->iif);
				}
				dmm_cache_release(bce);
			break;
			default:
				dbg("No action for this event (%d) at current state (%d) !\n", info->msg_event, bce->type);
				dmm_cache_release(bce);
			}
		}
	break;
	case hasNA:
		//Reset counter, Delete task for entry deletion  & Add a new task for NS expiry.
		if (!bce) {
			dbg("Received NA for a not registered MN -> reject\n");
			break;
		} else if (bce->type == BCE_DMM) {
			if (bce->n_rets_counter == conf.MaxMessageRetransmissions)
				dbg("Received NA for not expired BCE");
			else {
				bce->n_rets_counter = conf.MaxMessageRetransmissions;    //Reset the Retransmissions Counter.
				dbg("Reset the Reachability Counter = %d for %x:%x:%x:%x:%x:%x:%x:%x\n", bce->n_rets_counter, NIP6ADDR(&info->mn_iid));
				del_task(&bce->tqe);
				mar_force_update_registration(bce, info->iif);
			}
			dmm_cache_release(bce);
		} else {
			dbg("Cannot accept NA for not registered MN\n");
			dmm_cache_release(bce);
		}
	break;
	case hasPBU:
		if (!bce) {
			dbg("Received PBU for a not registered MN -> reject\n");
			break;
		} else if (bce->type == BCE_DMM) {
			/* 
			Install routes and tunnel for the MN
			Send a PBA to the CMD with the P-MAR option
			*/
			if (!IN6_ARE_ADDR_EQUAL(&conf.CmdAddress, &info->src)){
				dbg("Received PBU from a non-CMD node -> rejected\n");
				break;
			} else {
				if (info->lifetime.tv_sec > 0) {
					if (!IN6_IS_ADDR_UNSPECIFIED(&info->mn_serv_mar_addr)) {
						dbg("Stopping BCE timer, no longer needed\n");
						bce->mn_serv_mar_addr = info->mn_serv_mar_addr;
						bce->timestamp.first    = info->timestamp.first;
						bce->timestamp.second   = info->timestamp.second;
						mar_end_registration_dmm(bce, info->iif);
					} else
						dbg("ERROR: No Serving MAR found\n");
				} else if (info->lifetime.tv_sec == 0 && info->lifetime.tv_nsec == 0) {
					bce->seqno_in = info->seqno;
					mar_dereg_dmm(bce);
					dmm_cache_delete(bce);
				}
			}
			dmm_cache_release(bce);
		} else	{
			dbg("Cannot accept PBU for not registered MN\n");
			dmm_cache_release(bce);
		}
	break;
	default:
		dbg("No action for this event (%d) at current state !\n", info->msg_event);
	}
	mutex_return_code = pthread_rwlock_unlock(&fsm_lock);
	if (mutex_return_code != 0) {
		dbg("pthread_rwlock_unlock(&fsm_lock) %s\n", strerror(mutex_return_code));
	}
	dbg("exiting FSM - Waiting for new messages ...\n\n\n");
	return;
}
//---------------------------------------------------------------------------------------------------------------------
int cmd_fsm(msg_info_t * info)
{
	if (!info) {
		dbg("Input parameter fail: Cannot proceed\n");
		return -1;
	}
	int result = 0;
	dmm_entry *bce = NULL;
	struct in6_addr hw_address = EUI64_to_EUI48(info->mn_iid);
	
	bce = dmm_cache_get(&conf.OurAddress, &hw_address);
	switch (info->msg_event) {
	/*
	 * Two possible events:
	 * PBU reception;
	 * PBA reception.
	 * All other received messages are discarded
	 */
	case hasPBU:
		if (bce) {
			if (info->lifetime.tv_sec > 0) {
				if (IN6_ARE_ADDR_EQUAL(&bce->mn_serv_mar_addr, &info->src)) {
					dbg("Same serving MAR: %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(&info->src));
					cmd_refresh_binding_entry(bce, info);
					cmd_reg(bce);
					dmm_cache_release(bce);
				} else {					
					dbg("PBU for an existing MN ... update serving MAR\n");
					cmd_update_binding_entry(bce, info);
					cmd_reg_dmm(bce);
					dmm_cache_release(bce);
				}
			} else if (info->lifetime.tv_sec == 0 && info->lifetime.tv_nsec == 0) {
				dbg("PBU with Lifetime = 0... start Location Deregistration\n");
				if (IN6_ARE_ADDR_EQUAL(&info->src, &bce->mn_serv_mar_addr)) //Received PBU from serving MAG
				{		
					dbg("Deregistration case...\n");
					cmd_dereg(bce, info, 1);
					dmm_cache_release(bce);
					dmm_cache_delete(bce);
				} else { //Received PBU from an already unregistered MAG
					dbg("Deregistration for not registered MAG, doing nothing\n");
					dmm_cache_release(bce);
					}
				}
		} else {
			dbg("No DMM entry found for %x:%x:%x:%x:%x:%x:%x:%x ... \n", NIP6ADDR(&info->mn_iid));
			if (info->lifetime.tv_sec > 0) {
				dbg("PBU for a new MN ... Location Registration starting now...\n");
				bce = dmm_cache_alloc(BCE_DMM);
				if (bce) {
					insert_into_hnp_cache(hw_address, info->mn_prefix);
					result = cmd_create_binding_entry(bce, info);
					if (result >= 0)
						cmd_reg(bce);
					else break;
				dmm_cache_add(bce);
				}
			} else if ((info->lifetime.tv_sec == 0) && (info->lifetime.tv_nsec == 0)) {
				dbg("PBU with Lifetime = 0 for a not-registered MN. PBU discarded\n");
				}
			}
	break;
	case hasPBA:
			if (bce) {
				dbg("Received PBA from Previous MAR: BCE updated\n");
				if (is_pba_is_response_to_last_pbu_sent(info, bce)) {
					dbg("Finish Location Registration\n");
					bce->PBA_flags = info->PBA_flags;
					bce->lifetime.tv_nsec = 0;
					if (!IN6_IS_ADDR_UNSPECIFIED(&info->mn_serv_mar_addr)) {
						cmd_end_reg_dmm(bce, &info->mn_prefix);
					}
				}
				dmm_cache_release(bce);
			} else {
				dbg("WARNING: Received PBA for a not registered MN\n");
			}
	break;
	default:
		dbg("No action for this event (%d) at current state!\n", info->msg_event);
	}
	return result;
}