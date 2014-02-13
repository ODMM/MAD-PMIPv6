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

#ifndef __DMM_HANDLER_H__
#    define __DMM_HANDLER_H__
//-----------------------------------------------------------------------------
#    ifdef DMM_HANDLER_C
#        define private_dmm_handler(x) x
#        define protected_dmm_handler(x) x
#        define public_dmm_handler(x) x
#    else
#        ifdef DMM
#            define private_dmm_handler(x)
#            define protected_dmm_handler(x) extern x
#            define public_dmm_handler(x) extern x
#        else
#            define private_dmm_handler(x)
#            define protected_dmm_handler(x)
#            define public_dmm_handler(x) extern x
#        endif
#    endif
//-----------------------------------------------------------------------------
#    include <netinet/ip6mh.h>
#    include "binding_cache.h"
//-----------------------------------------------------------------------------
protected_dmm_handler(struct icmp6_handler mar_rs_handler;)
protected_dmm_handler(struct mh_handler    mar_pba_handler;)
protected_dmm_handler(struct mh_handler    mar_pbu_handler;)
protected_dmm_handler(struct mh_handler    cmd_pba_handler;)
protected_dmm_handler(struct mh_handler    cmd_pbu_handler;)
protected_dmm_handler(struct icmp6_handler mar_na_handler;)
//-PROTOTYPES----------------------------------------------------------------------------

protected_dmm_handler(struct in6_addr  *link_local_addr(struct in6_addr *id);)

protected_dmm_handler(struct in6_addr  *CONVERT_ID2ADDR(struct in6_addr *result, struct in6_addr *prefix, struct in6_addr *id);)

protected_dmm_handler(struct in6_addr  *get_mn_addr(dmm_entry * bce);)

protected_dmm_handler(struct in6_addr  *solicited_mcast(struct in6_addr *id);)

private_dmm_handler(void pbu_retx_timer_handler(struct tq_elem *tqe);)

protected_dmm_handler(void bce_expired_handler(struct tq_elem *tqe);)
#endif