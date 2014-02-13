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

#ifndef __DMM_MAG_PROC_H__
#    define __DMM_MAG_PROC_H__
//-----------------------------------------------------------------------------
#    ifdef DMM_MAG_PROC_C
#        define private_mar_proc(x) x
#        define protected_mar_proc(x) x
#        define public_mar_proc(x) x
#    else
#        ifdef DMM
#            define private_mar_proc(x)
#            define protected_mar_proc(x) extern x
#            define public_mar_proc(x) extern x
#        else
#            define private_mar_proc(x)
#            define protected_mar_proc(x)
#            define public_mar_proc(x) extern x
#        endif
#    endif
//-----------------------------------------------------------------------------

#    ifndef IPV6_ADDR_LINKLOCAL
#        define IPV6_ADDR_LINKLOCAL 0x0020U
#    endif
//-----------------------------------------------------------------------------
#	include <netinet/ip6.h>
#	include "binding_cache.h"
#	include "dmm_msgs.h"

#include <net/if.h>

struct dlif_entry {
	int 			dlif_id;
	struct in6_addr		dlif_mac;
	struct in6_addr		dlif_linklocal;
	char			dlif_name[14];
	struct dlif_entry	*next;
};

struct ingress_if_info {
	int			eif_id;
	char			eif_name[IFNAMSIZ];
	struct in6_addr		if_llid;
	struct in6_addr		if_ll_addr;
	pthread_rwlock_t	dlif_lock;
	uint8_t			dlif_enabled;
	struct dlif_entry	*dlif_list_head;
};

//-PROTOTYPES----------------------------------------------------------------------------

private_mar_proc(int mar_setup_route(struct in6_addr *target_route, int pref_len, int downlink, int uplink);)

protected_mar_proc(int mar_remove_route(struct in6_addr *target_route, int pref_len, int downlink, int direction);)

protected_mar_proc(int mar_dereg_dmm(dmm_entry * bce);)

protected_mar_proc(int mar_dereg(dmm_entry * bce, int propagate);)

protected_mar_proc(int mar_start_registration(dmm_entry * bce);)

protected_mar_proc(int mar_end_registration(dmm_entry * bce, int iif);)

protected_mar_proc(struct in6_addr_list *MAR_check(dmm_entry * bce);)

protected_mar_proc(int mar_start_registration_dmm(dmm_entry * bce, struct in6_addr *alt_dst);)

protected_mar_proc(int mar_end_registration(dmm_entry * bce, int iif);)

protected_mar_proc(int mar_end_registration_dmm(dmm_entry * bce, int iif);)

protected_mar_proc(int mar_end_registration_no_new_tunnel(dmm_entry * bce, int iif);)

protected_mar_proc(int mar_force_update_registration(dmm_entry * bce, int iif);)

protected_mar_proc(int mar_update_binding_entry(dmm_entry * bce, msg_info_t * info);)

protected_mar_proc(int mar_create_binding_entry(msg_info_t * info, dmm_entry * bce);)

private_mar_proc(int mar_send_ra(struct in6_addr *mn_prefix, int interface, struct in6_addr *dst, int dup);)

protected_mar_proc(int check_ip6_forwarding(void);)

protected_mar_proc(int dlif_list_init(void);)
	
private_mar_proc(int dlif_exists(char *name);)

private_mar_proc(struct dlif_entry *get_dlif_entry(int iif);)

private_mar_proc(int delete_dlif(int iif);)

protected_mar_proc(void dlif_cleanup(void);)

private_mar_proc(struct dlif_entry *setup_dlif(struct in6_addr *mn_llid, struct in6_addr *mar_llid, struct in6_addr *prefix, int local);)

protected_mar_proc(int mar_get_ingress_info(void);)

protected_mar_proc(void mar_list_clean_all(struct dmm_entry *bce);)

protected_mar_proc(void mar_list_clean(struct mar_list_entry *ptr);)

#endif