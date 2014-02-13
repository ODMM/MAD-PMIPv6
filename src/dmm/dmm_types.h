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

#ifndef __DMM_TYPES_H__
#    define __DMM_TYPES_H__
//---------------------------------------------------------------------------------------------------------------------
#    include <linux/types.h>
#    include <netinet/ip6mh.h>
//---------------------------------------------------------------------------------------------------------------------
#    include "mh.h"
//---------------------------------------------------------------------------------------------------------------------

/*
* Mobility Option TLV data structure
*New options defined for Proxy BU & BA
*/

struct ip6_mh_opt_home_net_prefix_t {   /*Home netowork prefix option */
__u8				ip6hnp_type;
__u8				ip6hnp_len;
__u8			ip6hnp_reserved;
__u8			ip6hnp_prefix_len;
struct in6_addr	ip6hnp_prefix;
} __attribute__ ((__packed__));

typedef struct ip6_mh_opt_home_net_prefix_t ip6_mh_opt_home_net_prefix_t;

struct ip6_mh_opt_handoff_indic {   /*Home netowork prefix option */
__u8		ip6hi_type;
__u8		ip6hi_len;
__u8		ip6hi_reserved;
__u8		ip6hi_hi;
} __attribute__ ((__packed__));

typedef struct ip6_mh_opt_handoff_indic ip6_mh_opt_handoff_indic;

struct ip6_mh_opt_access_tech_type {
__u8		ip6att_type;
__u8		ip6att_len;
__u8		ip6att_reserved;
__u8		ip6att_att;
} __attribute__ ((__packed__));

typedef struct ip6_mh_opt_access_tech_type ip6_mh_opt_access_tech_type;

typedef struct {
	__u32				first;
	__u32				second;
} ip6mnid_t;

typedef struct {
	__u32				first;
	__u32				second;
} ip6ts_t;

struct ip6_mh_opt_mn_identifier_t {
	__u8				ip6mnid_type;
	__u8				ip6mnid_len;
	//__u16				ip6mnid_flags;	/*!< \brief Mobile interface identifier flags (NOT RFC4283).*/
	__u8		 		ip6mnid_subtype;
	ip6mnid_t			mn_identifier;
} __attribute__ ((__packed__));
typedef struct ip6_mh_opt_mn_identifier_t ip6_mh_opt_mn_identifier_t;

struct ip6_mh_opt_mobile_node_id {
	__u8	 	ip6mnid_type;
	__u8	 	ip6mnid_len;
	__u8 		ip6mnid_subtype;
	ip6mnid_t	mn_identifier;
} __attribute__ ((__packed__));
typedef struct ip6_mh_opt_mobile_node_id ip6_mh_opt_mobile_node_id;

struct ip6_mh_opt_time_stamp_t {
	__u8				ip6mots_type;
	__u8				ip6mots_len;
	ip6ts_t				time_stamp;
} __attribute__ ((__packed__));
typedef struct ip6_mh_opt_time_stamp_t ip6_mh_opt_time_stamp_t;

struct ip6_mh_opt_link_local_addr {
	__u8			ip6link_type;
	__u8			ip6link_len;
	struct in6_addr		ip6link_addr;
} __attribute__ ((__packed__));
typedef struct ip6_mh_opt_link_local_addr ip6_mh_opt_link_local_addr;

struct ip6_mh_opt_link_layer_id {
	__u8			ip6llid_type;
	__u8			ip6llid_len;
	__u16			ip6llid_reserved;
	__u8			ip6llid_llid[6];
} __attribute__ ((__packed__));
typedef struct ip6_mh_opt_link_layer_id ip6_mh_opt_link_layer_id;

// ******** Extended options for cluster based architecture & Route optimiztion ***********
struct ip6_mh_opt_dst_mn_addr_t {
	__u8				ip6dma_type;
	__u8				ip6dma_len;
	struct in6_addr		dst_mn_addr;    /* Destination MN Address */
} __attribute__ ((__packed__));
typedef struct ip6_mh_opt_dst_mn_addr_t ip6_mh_opt_dst_mn_addr_t;


struct ip6_mh_opt_serv_mar_addr_t {
	__u8				ip6sma_type;
	__u8				ip6sma_len;
	struct in6_addr		serv_mar_addr;  /* Serving MAG Address of the destination */
} __attribute__ ((__packed__));
typedef struct ip6_mh_opt_serv_mar_addr_t ip6_mh_opt_serv_mar_addr_t;


struct ip6_mh_opt_serv_cmd_addr_t {
	__u8				ip6sla_type;
	__u8				ip6sla_len;
	struct in6_addr		serv_cmd_addr;  /* Serving LMA Address of the destination */
} __attribute__ ((__packed__));
typedef struct ip6_mh_opt_serv_cmd_addr_t ip6_mh_opt_serv_cmd_addr_t;


struct ip6_mh_opt_src_mn_addr {
	__u8				ip6sma_type;
	__u8				ip6sma_len;
	struct in6_addr		src_mn_addr;    /* Source MN Address */
} __attribute__ ((__packed__));
typedef struct ip6_mh_opt_src_mn_addr_t ip6_mh_opt_src_mn_addr_t;


struct ip6_mh_opt_src_mar_addr_t {
	__u8				ip6sma_type;
	__u8				ip6sma_len;
	struct in6_addr		src_mar_addr;   /* Source MAG Address */
} __attribute__ ((__packed__));
typedef struct ip6_mh_opt_src_mar_addr_t ip6_mh_opt_src_mar_addr_t;

/* Fabio begin */
struct ip6_mh_opt_p_mar {
	__u8			ip6pmar_type;
	__u8			ip6pmar_len;
	struct in6_addr		ip6pmar_addr;   /* Previous MAR Address */
	struct in6_addr		ip6mar_mn_prefix;    /* HNP anchored at the previous MAR*/
	struct in6_addr		ip6pmar_hw_addr;
} __attribute__ ((__packed__));
typedef struct ip6_mh_opt_p_mar ip6_mh_opt_p_mar;

struct ip6_mh_opt_s_mar {
	__u8			ip6smar_type;
	__u8			ip6smar_len;
	struct in6_addr		ip6smar_addr;   /* Serving MAR Address */
} __attribute__ ((__packed__));
typedef struct ip6_mh_opt_s_mar ip6_mh_opt_s_mar;

struct ip6_mh_opt_dlif_link_layer_addr {
	__u8			ip6dlif_type;
	__u8			ip6dlif_len;
	__u16			ip6dlif_reserved;
	struct in6_addr		ip6dlif_addr;  
} __attribute__ ((__packed__));
typedef struct ip6_mh_opt_dlif_link_layer_addr ip6_mh_opt_dlif_link_layer_addr;

typedef struct mar_list_entry {
	struct in6_addr		mn_prefix;
	struct in6_addr		mar_address;
	struct in6_addr		mar_hw_address;
	int			tunnel_id;
	int			dlif_id;
	struct mar_list_entry	*next;
} mar_list_entry;

#define TUNNEL_OK	1
#define TUNNEL_TEMP	0
#define TUNNEL_NA	-1

typedef struct msg_info_t {
/*pbu*/	struct in6_addr			src;
/*pbu*/	struct in6_addr			dst;
/*pbu*/	struct in6_addr_bundle		addrs;
	int 				length;
/*pbu*/	int				iif;
/*pbu*/	uint32_t			msg_event;
/*pbu*/	uint8_t				pbu_status;
	uint8_t				pba_status;
/*pbu*/	struct in6_addr			mn_iid;
	struct in6_addr			mn_addr;
/*pbu*/	struct in6_addr			mn_prefix;
	struct in6_addr			remote_mn_prefix;
	struct in6_addr			mn_serv_mar_addr;
	struct in6_addr			mn_serv_cmd_addr;
	struct mar_list_entry		*mn_p_mar_addr;
	struct in6_addr			mn_link_local_addr;
	uint8_t 			mn_llid[6];
	struct in6_addr			mar_hw_address;
	struct timespec			addtime;
/*pbu*/	struct timespec			lifetime;
/*pbu*/	uint16_t			seqno;
/*pbu*/	uint16_t			PBU_flags;
	uint8_t				PBA_flags;
/*pbu*/	ip6ts_t				timestamp;
/*pbu*/	uint8_t				techType;
/*pbu*/	uint8_t				handoffInd;
	struct in6_addr			src_mar_addr;
	struct in6_addr			src_mn_addr;
	struct in6_addr			na_target;
	struct in6_addr			ns_target;
	int				is_dad;
	int				hoplimit;
} msg_info_t;
#endif