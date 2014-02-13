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

#ifndef __DMM_CACHE_H__
#    define __DMM_CACHE_H__
//-----------------------------------------------------------------------------
#    ifdef DMM_CACHE_C
#        define private_dmm_cache(x) x
#        define protected_dmm_cache(x) x
#        define public_dmm_cache(x) x
#    else
#        ifdef DMM
#            define private_dmm_cache(x)
#            define protected_dmm_cache(x) extern x
#            define public_dmm_cache(x) extern x
#        else
#            define private_dmm_cache(x)
#            define protected_dmm_cache(x)
#            define public_dmm_cache(x) extern x
#        endif
#    endif
//-----------------------------------------------------------------------------
#    include <pthread.h>
#    include <arpa/inet.h>
#    include <netinet/in.h>
//-----------------------------------------------------------------------------
#    include "tqueue.h"
#    include "util.h"
#    include "hash.h"
//-----------------------------------------------------------------------------
#    include "dmm_types.h"

#    define DMM_CACHE_BUCKETS				128

#    ifndef ND_OPT_PI_FLAG_RADDR
#        define ND_OPT_PI_FLAG_RADDR		0x20
#    endif
#    define DFLT_AdvValidLifetime			86400   /* seconds */
#    define DFLT_AdvOnLinkFlag				1
#    define DFLT_AdvPreferredLifetime		14400    /* seconds */
#    define DFLT_AdvAutonomousFlag			1
#    ifndef ND_OPT_HAI_FLAG_SUPPORT_MR
#        if BYTE_ORDER== BIG_ENDIAN
#            define ND_OPT_HAI_FLAG_SUPPORT_MR  0x8000
#        else
#            define ND_OPT_HAI_FLAG_SUPPORT_MR  0x0080
#        endif
#    endif
#    define DFLT_AdvSendAdv				1
#    define DFLT_MaxRtrAdvInterval		1.5
#    define DFLT_MinRtrAdvInterval		1 //(iface) (0.33 * (iface)->MaxRtrAdvInterval)
#    define DFLT_AdvCurHopLimit			64  /* as per RFC 1700 or the next incarnation of it :) */
#    define DFLT_AdvReachableTime		0
#    define DFLT_AdvRetransTimer		0
#    define DFLT_HomeAgentPreference	20
#    define DFLT_AdvHomeAgentFlag		1
#    define DFLT_AdvIntervalOpt			1
#    define DFLT_AdvHomeAgentInfo		1
#    define DFLT_AdvRouterAddr			1
#    define MSG_SIZE 					4096

typedef struct AdvPrefix_t {
	struct in6_addr		Prefix;
	uint8_t			PrefixLen;
	int			AdvOnLinkFlag;
	int			AdvAutonomousFlag;
	uint32_t		AdvValidLifetime;
	uint32_t		AdvPreferredLifetime;
	/* Mobile IPv6 extensions */
	int			AdvRouterAddr;
} adv_prefix_t;

typedef struct HomeAgentInfo_t {
	uint8_t 		type;
	uint8_t 		length;
	uint16_t 		flags_reserved;
	uint16_t 		preference;
	uint16_t 		lifetime;
} home_agent_info_t;

typedef struct ra_iface_t {
	int			AdvSendAdvert;
	double			MaxRtrAdvInterval;
	double			MinRtrAdvInterval;
	uint32_t		AdvReachableTime;
	uint32_t		AdvRetransTimer;
	int32_t			AdvDefaultLifetime;
	int			AdvMobRtrSupportFlag;
	uint8_t			AdvCurHopLimit;
    /* Mobile IPv6 extensions */
	int			AdvIntervalOpt;
	int			AdvHomeAgentInfo;
	int			AdvHomeAgentFlag;
	uint16_t		HomeAgentPreference;
	int32_t			HomeAgentLifetime;
	int			AdvManagedFlag;
	int			AdvOtherConfigFlag;
	adv_prefix_t		Adv_Prefix;
} router_ad_iface_t;

typedef struct dmm_entry {
	struct in6_addr			mn_prefix;
	struct in6_addr			our_addr;
	struct in6_addr			mn_suffix;
	struct in6_addr			mn_hw_address;
	struct in6_addr			mn_addr;
	struct in6_addr			mn_serv_mar_addr;
	struct in6_addr			mn_serv_cmd_addr;
	struct in6_addr			mar_hw_address;
	struct mar_list_entry		*mar_list;
	struct in6_addr			mn_link_local_addr;
	struct timespec			add_time;
	struct timespec			lifetime;
	uint16_t			seqno_in;
	uint16_t			seqno_out;
	uint16_t			PBU_flags;
	uint8_t				PBA_flags;
	uint8_t				accTechType;
	uint8_t				handoffInd;
	int 				type;
	int				unreach;
	int				tunnel;
	int				link;
	/* PBU/PBRR message for retransmissions */
	struct iovec 			mh_vec[2 * (IP6_MHOPT_MAX + 1)];          /* was 7 */
	int				iovlen;
	/* info_block status flags */
	uint8_t				status;
	ip6ts_t				timestamp;
	uint32_t			msg_event;
	struct timespec			br_lastsent;
	int 				br_count;
	int 				n_rets_counter;
	pthread_rwlock_t 		lock;
	struct tq_elem 			tqe;
	void (*cleanup) (struct dmm_entry * bce);
} dmm_entry;

#    define BCE_NO_ENTRY (-1)
#    define BCE_DMM 5
#    define BCE_TEMP 6

//-GLOBAL VARIABLES----------------------------------------------------------------------------
protected_dmm_cache(pthread_rwlock_t dmm_lock;)

protected_dmm_cache(router_ad_iface_t router_ad_iface;)

//-PROTOTYPES----------------------------------------------------------------------------

protected_dmm_cache(int dmm_cache_init(void);)

protected_dmm_cache(void init_iface_ra(void);)

protected_dmm_cache(dmm_entry * dmm_cache_alloc(int type);)

protected_dmm_cache(int dmm_cache_start(dmm_entry * bce);)

protected_dmm_cache(int dmm_cache_stop(dmm_entry * bce);)

protected_dmm_cache(dmm_entry * dmm_cache_add(dmm_entry * bce);)

protected_dmm_cache(dmm_entry * dmm_cache_get(const struct in6_addr *our_addr, const struct in6_addr *peer_addr);)

protected_dmm_cache(void dmm_cache_release(dmm_entry * bce);)

protected_dmm_cache(int dmm_cache_exists(const struct in6_addr *our_addr, const struct in6_addr *peer_addr);)

private_dmm_cache(void dmm_cache_free(dmm_entry * bce);)

protected_dmm_cache(void dmm_cache_delete(dmm_entry * bce);)

//protected_dmm_cache(void dmm_cache_delete(const struct in6_addr *our_addr, const struct in6_addr *peer_addr);)

public_dmm_cache(int dmm_cache_iterate(int (*func) (void *, void *), void *arg);)

#endif