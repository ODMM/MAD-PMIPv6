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

#ifndef __DMM_MSGS_H__
#    define __DMM_MSGS_H__
//-----------------------------------------------------------------------------
#    ifdef DMM_MSGS_C
#        define private_dmm_msgs(x) x
#        define protected_dmm_msgs(x) x
#        define public_dmm_msgs(x) x
#    else
#        ifdef DMM
#            define private_dmm_msgs(x)
#            define protected_dmm_msgs(x) extern x
#            define public_dmm_msgs(x) extern x
#        else
#            define private_dmm_msgs(x)
#            define protected_dmm_msgs(x)
#            define public_dmm_msgs(x) extern x
#        endif
#    endif
//-----------------------------------------------------------------------------
#include <netinet/ip6mh.h>
#include "icmp6.h"
#include "mh.h"
//-----------------------------------------------------------------------------
#include "binding_cache.h"
//-GLOBAL VARIABLES----------------------------------------------------------------------------

private_dmm_msgs(int g_mar_sequence_number;)

//-PROTOTYPES----------------------------------------------------------------------------

protected_dmm_msgs(void init_pbu_sequence_number(void);)

protected_dmm_msgs(int get_new_pbu_sequence_number(void);)

protected_dmm_msgs(int is_pba_is_response_to_last_pbu_sent(msg_info_t * pba_info, dmm_entry *bce);)

private_dmm_msgs(struct in6_addr get_node_id(struct in6_addr *mn_addr);)

protected_dmm_msgs(struct in6_addr get_node_prefix(struct in6_addr *mn_addr);)

private_dmm_msgs(int mh_create_opt_home_net_prefix(struct iovec *iov, struct in6_addr *Home_Network_Prefix);)

private_dmm_msgs(int mh_create_opt_handoffIndic(struct iovec *iov, __u8 hi);)

private_dmm_msgs(int mh_create_opt_accTechType(struct iovec *iov, __u8 att);)

private_dmm_msgs(int mh_create_opt_mn_identifier(struct iovec *iov, ip6mnid_t * MN_ID);)

private_dmm_msgs(int mh_create_opt_time_stamp(struct iovec *iov, ip6ts_t * Timestamp);)

private_dmm_msgs(int mh_create_opt_link_local_add(struct iovec *iov, struct in6_addr *LinkLocal);)

private_dmm_msgs(int mh_create_opt_serv_mar_addr(struct iovec *iov, struct in6_addr *serv_MAR_addr);)

private_dmm_msgs(int mh_create_opt_prev_mar_addr(struct iovec *iov, struct in6_addr *prev_MAR_addr, struct in6_addr *mn_prefix, struct in6_addr *mar_hw_address);)

private_dmm_msgs(int mh_create_opt_dlif_link_layer_addr(struct iovec *iov, struct in6_addr *mar_hw_addr);)

protected_dmm_msgs(int mh_pbu_parse(msg_info_t * info, struct ip6_mh_binding_update *pbu, ssize_t len, const struct in6_addr_bundle *in_addrs, int iif);)

protected_dmm_msgs(int mh_pba_parse(msg_info_t * info, struct ip6_mh_binding_ack *pba, ssize_t len, const struct in6_addr_bundle *in_addrs, int iif);)

protected_dmm_msgs(int icmp_rs_parse(msg_info_t * info, struct nd_router_solicit *rs, const struct in6_addr *saddr, const struct in6_addr *daddr, int iif, int hoplimit);)

protected_dmm_msgs(int icmp_na_parse(msg_info_t * info, struct nd_neighbor_advert *na, const struct in6_addr *saddr, const struct in6_addr *daddr, int iif, int hoplimit);)

protected_dmm_msgs(int pmip_mh_send(const struct in6_addr_bundle *addrs, const struct iovec *mh_vec, int iovlen, int oif);)

protected_dmm_msgs(int mh_send_pbu(const struct in6_addr_bundle *addrs, dmm_entry * bce, struct timespec *lifetime, int oif);)

protected_dmm_msgs(int mh_send_pbu_to_MAR(const struct in6_addr_bundle *addrs, dmm_entry * bce, struct timespec *lifetime, int oif);)

protected_dmm_msgs(int mh_send_pba(const struct in6_addr_bundle *addrs, dmm_entry * bce, struct timespec *lifetime, int oif);)

protected_dmm_msgs(int mh_send_pba_to_CMD(const struct in6_addr_bundle *addrs, dmm_entry * bce, struct timespec *lifetime, int oif);)

#endif