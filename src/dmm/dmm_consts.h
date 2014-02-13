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

#ifndef __pmip_consts_h
#    define __pmip_consts_h
#    include <netinet/in.h>

#    define MAX_MOBILES 64

/*
* Mobility Header Message Option Types
* Options for Route Optimization and Flow Binding
*/
#    define IP6_MHOPT_MN_ID		 0x08	/* MOBILE NODE IDENTIFIER */

#    define IP6_MHOPT_DST_MN_ADDR        0x0B   /* Source Mobile Node address */
#    define IP6_MHOPT_SERV_MAG_ADDR      0x0C   /* Serving MAG address */
#    define IP6_MHOPT_SERV_LMA_ADDR      0x0D   /* Source Mobile Node address */
#    define IP6_MHOPT_SRC_MN_ADDR        0x0E   /* Source Mobile Node address */
#    define IP6_MHOPT_SRC_MAG_ADDR       0x0F   /* Serving MAG address */
#    define IP6_MHOPT_HOM_NET_PREX       0x16   /* Home Network Prefix */
#    define IP6_MHOPT_HANDOFF_INDICATOR  0x17   /* Handoff Indicator Option */
#    define IP6_MHOPT_ACCESS_TECHNO_TYPE 0x18   /* Access Technology Type Option */
#    define IP6_MHOPT_LL_ID		 0x19	/* link layer IDENTIFIER */
#    define IP6_MHOPT_LINK_ADDR          0x1A   /* link local address */
#    define IP6_MHOPT_TIME_STAMP         0x1B   /* Timestamp */

/*/* To be used by the CMD */
#    define IP6_MHOPT_S_MAR		 0x1C   /* S-MAR option - value 28 NOT IANA */
#    define IP6_MHOPT_P_MAR		 0x1D   /* P-MAR option - value 29 NOT IANA */
#    define IP6_MHOPT_DLIF_LINKLAYERADDR 0x1E	/* DLIF LLID option - value 30 NOT IANA */
//#    define IP6_MHOPT_DMM_MAX         IP6_MHOPT_SRC_MAG_ADDR
//#    define IP6_MHOPT_DMM_MAX           IP6_MHOPT_TIME_STAMP
#    define IP6_MHOPT_DMM_MAX           IP6_MHOPT_DLIF_LINKLAYERADDR
//must change in mh.h
//#define IP6_MHOPT_MAX                 IP6_MHOPT_DMM_MAX
/*
* New Mobility Header Message Types
* new mobility header  types defined
*/
#    define IP6_MH_TYPE_PBREQ   8   /* Proxy Binding Request */
#    define IP6_MH_TYPE_PBRES   9   /* Proxy Binding Response */

/* *** DMM status RFC 5213 *** */
#define IP6_MH_BAS_PROXY_REG_NOT_ENABLED	152	/* Proxy registration not enabled for the mobile node */
#define IP6_MH_BAS_NOT_LMA_FOR_THIS_MOBILE_NODE	153	/* Not local mobility anchor for this mobile node */
#define IP6_MH_BAS_MAG_NOT_AUTHORIZED_FOR_PROXY_REG	154	/* The mobile access gateway is not authorized to send PBU */
#define IP6_MH_BAS_NOT_AUTHORIZED_FOR_HOME_NETWORK_PREFIX	155	/* The mobile node is not authorized for one or more of the requesting home network prefixes */
#define IP6_MH_BAS_TIMESTAMP_MISMATCH	156	/* Invalid timestamp value (clocks are out of sync) */
#define IP6_MH_BAS_TIMESTAMP_LOWER_THAN_PREV_ACCEPTED	157	/* Invalid timestamp value (lower than previously accepted) */
#define IP6_MH_BAS_MISSING_HOME_NETWORK_PREFIX_OPTION	158	/* Mising home network prefix option */
#define IP6_MH_BAS_BCE_PBU_PREFIX_SET_DO_NOT_MATCH	159	/* The Home network prefixes listed in the BCE do not match all of the received PBU */
#define IP6_MH_BAS_MISSING_MN_IDENTIFIER_OPTION	160	/* Mising mobile node identifier option */
#define IP6_MH_BAS_MISSING_HANDOFF_INDICATOR_OPTION	161	/* Mising handoff indicator option */
#define IP6_MH_BAS_MISSING_ACCESS_TECH_TYPE_OPTION	162	/* Mising access technology type option */

//#define IP6_MH_TYPE_MAX               IP6_MH_TYPE_PBRE //Defined in mh.h
/* PBU_flags */
#    if BYTE_ORDER == BIG_ENDIAN
#        define IP6_MH_PBU      0x0200  /* Proxy Binding Update */
#    else           /* BYTE_ORDER == LITTLE_ENDIAN */
#        define IP6_MH_PBU      0x0020  /* Proxy Binding Update */
#    endif
/* PBA_flags */
#    if BYTE_ORDER == BIG_ENDIAN
#        define IP6_MH_PBA      0x20    /* Proxy Binding Ack */
#    else           /* BYTE_ORDER == LITTLE_ENDIAN */
#        define IP6_MH_PBA      0x20    /* Proxy Binding Ack */
#    endif
/* MN_ID option_flags */
#    if BYTE_ORDER == BIG_ENDIAN
#        define IP6_MH_MNID     0x8000  /* MN_ID flag */
#    else           /* BYTE_ORDER == LITTLE_ENDIAN */
#        define IP6_MH_MNID     0x0080  /* MN_ID flag */
#    endif
/* NDP NA_flags */
/* Router flag */
#    if BYTE_ORDER == BIG_ENDIAN
#        define NDP_NA_ROUTER       0x80000000  /* Router flag */
#    else           /* BYTE_ORDER == LITTLE_ENDIAN */
#        define NDP_NA_ROUTER       0x00000080  /* Router flag */
#    endif
/* Solicited flag */
#    if BYTE_ORDER == BIG_ENDIAN
#        define NDP_NA_SOLICITED    0x40000000  /* Solicited flag */
#    else           /* BYTE_ORDER == LITTLE_ENDIAN */
#        define NDP_NA_SOLICITED    0x00000040  /* Solicited flag */
#    endif
/* Override flag */
#    if BYTE_ORDER == BIG_ENDIAN
#        define NDP_NA_OVERRIDE     0x20000000  /* Override flag */
#    else           /* BYTE_ORDER == LITTLE_ENDIAN */
#        define NDP_NA_OVERRIDE     0x00000020  /* Override flag */
#    endif

/* Subtype for Mobile Node Identifier option */
#define IP6_MNID_MHOPT_SUB_NAI	0x01	/* MN ID = NAI */
#define IP6_MNID_MHOPT_SUB_MAC	0x02	/* MN ID = MAC address (no IANA number...)*/

//Define list of ACC_TECH_TYPE Values
#define ATT_RESERVED	0
#define ATT_VIRTUAL	1
#define ATT_PPP		2
#define ATT_ETHERNET	3
#define ATT_WLAN	4
#define ATT_WIMAX	5
#define ATT_3GPP	6

//Define list of Handoff Indicator Values
#define HI_RESERVED	0	/*Reserved*/
#define HI_NEW_IF	1	/*Attachment over a new interface*/
#define HI_VERT		2	/*Handoff between two different interfaces of the mobile node*/
#define HI_HOR		3	/*Handoff between mobile access gateways for the same interface*/
#define HI_UNKNOWN	4	/*Handoff state unknown*/
#define HI_UNCHANGED	5	/*Handoff state not changed (Re-registration)*/

//Define STATUS FLAGS for FSM.
#    define hasDEREG        0x00000050  /* Has a DEREG */
#    define hasWLCCP        0x00000040  /* Has a WLCCP CISCO protocol */
#    define hasRS           0x00000030  /* Has a RS */
#    define hasNA           0x00000020  /* Has a NA */
#    define hasNS           0x00000010  /* Has a NS */
#    define hasPBU          0x00000008  /* Has a PBU */
#    define hasPBA          0x00000004  /* Has a PBA */
#    define hasPBREQ        0x00000002  /* Has a PBRR */
#    define hasPBRES        0x00000001  /* Has a PBRE */
#    define PLEN    64
#    define PBREQ_LOCATE                    0
#    define PBREQ_RO_TEST_INIT              1
#    define PBREQ_RO_INIT                   2
#    define PBRES_OK                        0
#    define PBRES_REASON_UNSPECIFIED        128
#	define UL	0
#	define DL	1
#	define UL_DL	2
#endif