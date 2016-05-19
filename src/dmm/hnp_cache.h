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

#ifndef __DMM_HNP_CACHE_H__
#    define __DMM_HNP_CACHE_H__
//-----------------------------------------------------------------------------
#    ifdef DMM_HNP_CACHE_C
#        define private_dmm_hnp_cache(x) x
#        define protected_dmm_hnp_cache(x) x
#        define public_dmm_hnp_cache(x) x
#    else
#        ifdef DMM
#            define private_dmm_hnp_cache(x)
#            define protected_dmm_hnp_cache(x) extern x
#            define public_dmm_hnp_cache(x) extern x
#        else
#            define private_dmm_hnp_cache(x)
#            define protected_dmm_hnp_cache(x)
#            define public_dmm_hnp_cache(x) extern x
#        endif
#    endif
//-----------------------------------------------------------------------------
#include <netinet/ip6.h>

#define DMM_USERS "/usr/local/etc/mad-users.conf"
//-----------------------------------------------------------------------------
typedef struct mnid_hnp_t {
	struct in6_addr mn_prefix;
	struct in6_addr mn_iid;
} mnid_hnp_t;
//-PROTOTYPES----------------------------------------------------------------------------

protected_dmm_hnp_cache(struct in6_addr EUI64_to_EUI48(struct in6_addr iid);)

protected_dmm_hnp_cache(struct in6_addr EUI48_to_EUI64(struct in6_addr macaddr);)

protected_dmm_hnp_cache(void insert_into_hnp_cache(struct in6_addr mn_iid, struct in6_addr addr);)

protected_dmm_hnp_cache(void cmd_mn_to_hnp_cache_init(void);)

protected_dmm_hnp_cache(int dmm_mn_to_hnp_cache_init(void);)

protected_dmm_hnp_cache(struct in6_addr *mnid_hnp_map(struct in6_addr *mnid);)
/*protected_dmm_hnp_cache(struct in6_addr mnid_hnp_map(struct in6_addr mnid, int *aaa_result);)*/
#endif