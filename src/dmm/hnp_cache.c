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
#define DMM_HNP_CACHE_C
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
//---------------------------------------------------------------------------------------------------------------------
#include "hnp_cache.h"
#include "dmm_consts.h"
//---------------------------------------------------------------------------------------------------------------------
#ifdef USE_RADIUS
#	include "freeradius-client.h"
#endif
#include "util.h"
#ifdef ENABLE_VT
#    include "vt.h"
#endif
#include "debug.h"
#include "conf.h"
//---------------------------------------------------------------------------------------------------------------------

static mnid_hnp_t g_mn_hn_map[MAX_MOBILES];

static int g_mn_count = 0;
#ifdef USE_RADIUS
#	define RADIUS_MSG_MAX_SIZE      4096
#	define RADIUS_USERNAME_MAX_SIZE 256
static rc_handle*			g_rh       = NULL;
static char*				g_default_realm = NULL;

static char 				msg[RADIUS_MSG_MAX_SIZE];
static char					g_username[RADIUS_USERNAME_MAX_SIZE];
static char					g_username_realm[RADIUS_USERNAME_MAX_SIZE];
static char					g_passwd[AUTH_PASS_LEN + 1];
#endif
//-----------------------------------------------------------------------------
struct in6_addr EUI64_to_EUI48(struct in6_addr iid)
{
	struct in6_addr macaddr;
	macaddr.s6_addr[0]  = 0;
	macaddr.s6_addr[1]  = 0;
	macaddr.s6_addr[2]  = 0;
	macaddr.s6_addr[3]  = 0;
	macaddr.s6_addr[4]  = 0;
	macaddr.s6_addr[5]  = 0;
	macaddr.s6_addr[6]  = 0;
	macaddr.s6_addr[7]  = 0;
	macaddr.s6_addr[8]  = 0;
	macaddr.s6_addr[9]  = 0;
	macaddr.s6_addr[10] = iid.s6_addr[8] ^ 0x02;
	macaddr.s6_addr[11] = iid.s6_addr[9];
	macaddr.s6_addr[12] = iid.s6_addr[10];
	macaddr.s6_addr[13] = iid.s6_addr[13];
	macaddr.s6_addr[14] = iid.s6_addr[14];
	macaddr.s6_addr[15] = iid.s6_addr[15];
	return macaddr;
}
//-----------------------------------------------------------------------------
struct in6_addr EUI48_to_EUI64(struct in6_addr macaddr)
{
	struct in6_addr iid;
	iid.s6_addr[0]  = 0;
	iid.s6_addr[1]  = 0;
	iid.s6_addr[2]  = 0;
	iid.s6_addr[3]  = 0;
	iid.s6_addr[4]  = 0;
	iid.s6_addr[5]  = 0;
	iid.s6_addr[6]  = 0;
	iid.s6_addr[7]  = 0;
	iid.s6_addr[8]  = macaddr.s6_addr[10] ^ 0x02;
	iid.s6_addr[9]  = macaddr.s6_addr[11];
	iid.s6_addr[10] = macaddr.s6_addr[12];
	iid.s6_addr[11] = 0xFF;
	iid.s6_addr[12] = 0xFE;
	iid.s6_addr[13] = macaddr.s6_addr[13];
	iid.s6_addr[14] = macaddr.s6_addr[14];
	iid.s6_addr[15] = macaddr.s6_addr[15];
	return iid;
}
//-----------------------------------------------------------------------------
void insert_into_hnp_cache(struct in6_addr mn_iid, struct in6_addr addr)
{
    int j = 0;
    while (j < g_mn_count) {
        if (IN6_ARE_ADDR_EQUAL(&g_mn_hn_map[j].mn_iid, &mn_iid)) {
            dbg("mnid %x:%x:%x:%x:%x:%x:%x:%x already in cache, updating addr %x:%x:%x:%x:%x:%x:%x:%x -> %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(&g_mn_hn_map[j].mn_iid), NIP6ADDR(&g_mn_hn_map[j].mn_prefix), NIP6ADDR(&addr));
            g_mn_hn_map[j].mn_prefix = addr;
            return;
        }
        j++;
    }
    if (g_mn_count < MAX_MOBILES) {
        g_mn_hn_map[g_mn_count].mn_prefix = addr;
        g_mn_hn_map[g_mn_count].mn_iid = mn_iid;
        dbg("new entry in cache %x:%x:%x:%x:%x:%x:%x:%x -> %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(&g_mn_hn_map[g_mn_count].mn_prefix), NIP6ADDR(&g_mn_hn_map[g_mn_count].mn_iid));
        g_mn_count = g_mn_count + 1;
    } else {
        dbg("ERROR COULD NOT PUT NEW ENTRY IN CACHE %x:%x:%x:%x:%x:%x:%x:%x -> %x:%x:%x:%x:%x:%x:%x:%x MAX ENTRIES REACHED\n", NIP6ADDR(&addr), NIP6ADDR(&mn_iid));
    }
}
//-----------------------------------------------------------------------------
void cmd_mn_to_hnp_cache_init(void)
{
    memset((void*)g_mn_hn_map, 0, sizeof(mnid_hnp_t) * MAX_MOBILES);
}
//-----------------------------------------------------------------------------

#ifdef USE_RADIUS
int dmm_mn_to_hnp_cache_init(void)
{
	dbg ("\n");
    memset(g_mn_hn_map, 0, sizeof(mnid_hnp_t) * MAX_MOBILES);
    rc_openlog("pmip_radius_client");
	if ((g_rh = rc_read_config(conf.RadiusClientConfigFile)) == NULL) {
		dbg ("ERROR INIT CLIENT RADIUS\n");
        	return ERROR_RC;
	} else {
        dbg ("RADIUS client radius read config file OK\n");
    }

    if (rc_read_dictionary(g_rh, rc_conf_str(g_rh, "dictionary")) != 0) {
        dbg ("ERROR INIT CLIENT RADIUS DICTIONNARY\n");
        return ERROR_RC;
    } else {
        dbg ("client radius read dictionnary file OK\n");
    }
    g_default_realm = rc_conf_str(g_rh, "default_realm");
    return 0;
}
#else
int dmm_mn_to_hnp_cache_init (void)
{
    FILE               *fp;
    char                str_addr[40], str_addr_iid[40];
    struct in6_addr     addr, addr1;
    unsigned int        ap, ap1;
    int                 i, j;

    memset(g_mn_hn_map, 0, sizeof(mnid_hnp_t) * MAX_MOBILES);
    j = 0;
    if ((fp = fopen (DMM_USERS, "r")) == NULL) {
        printf ("can't open %s \n", DMM_USERS);
        exit (0);
    }
    while ((fscanf (fp, "%32s %32s\n", str_addr, str_addr_iid) != EOF) && (j < MAX_MOBILES)) {
        for (i = 0; i < 16; i++) {
            sscanf (str_addr + i * 2, "%02x", &ap);
            addr.s6_addr[i] = (unsigned char) ap;
            g_mn_hn_map[j].mn_prefix = addr;
            sscanf (str_addr_iid + i * 2, "%02x", &ap1);
            addr1.s6_addr[i] = (unsigned char) ap1;
            g_mn_hn_map[j].mn_iid = addr1;
        }
        dbg ("%x:%x:%x:%x:%x:%x:%x:%x <=> %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR (&g_mn_hn_map[j].mn_prefix), NIP6ADDR (&g_mn_hn_map[j].mn_iid));
        j++;
        g_mn_count = g_mn_count + 1;
    }
    fclose (fp);
    if (j >= MAX_MOBILES) {
        dbg ("ERROR TOO MANY MAPPINGS DEFINED IN CONFIG FILE\n");
        exit (0);
    }
    return 0;
}
#endif
//-----------------------------------------------------------------------------

struct in6_addr mnid_hnp_map(struct in6_addr mnid, int *aaa_result)
{
    int l_flag = 0;
#if !defined (USE_RADIUS) || defined(CACHE_RADIUS)
    int j = 0;
    while (j < g_mn_count) {
        if (IN6_ARE_ADDR_EQUAL(&g_mn_hn_map[j].mn_iid, &mnid)) {
            l_flag = 1;
            *aaa_result = 10;
            return (g_mn_hn_map[j].mn_prefix);
        }
        j++;
    }
    #ifdef CACHE_RADIUS
    dbg("MNID not in cache  %x:%x:%x:%x:%x:%x:%x:%x  \n", NIP6ADDR(&mnid));
    #endif
#endif

#if defined (USE_RADIUS)

    VALUE_PAIR *send, *received;
    VALUE_PAIR *vp;
    struct in6_addr prefix;
    uint32_t service;

    *aaa_result = 0;
     send = NULL;

    sprintf(g_username, "%04x%04x%04x%04x", ntohs(mnid.s6_addr16[4]), ntohs(mnid.s6_addr16[5]), ntohs(mnid.s6_addr16[6]), ntohs(mnid.s6_addr16[7]));
	g_username[16] = 0;
    memset(g_passwd, '\0', AUTH_PASS_LEN + 1);
	strncpy(g_passwd, conf.RadiusPassword, strlen(conf.RadiusPassword));
	g_passwd[strlen(conf.RadiusPassword)] = '\0';
// Fill in User-Name

    strncpy(g_username_realm, g_username, sizeof(g_username_realm));
// Append default realm
    if ((strchr(g_username_realm, '@') == NULL) && g_default_realm && (*g_default_realm != '\0')) {
        strncat(g_username_realm, "@", sizeof(g_username_realm) - strlen(g_username_realm) - 1);
        strncat(g_username_realm, g_default_realm, sizeof(g_username_realm) - strlen(g_username_realm) - 1);
    }
    dbg("RADIUS USER NAME %s\n", g_username_realm);
    dbg("RADIUS PASSWORD  %s\n", g_passwd);
    if (rc_avpair_add(g_rh, &send, PW_USER_NAME, g_username_realm, -1, 0) == NULL) {
        fprintf(stderr, "[RADIUS] ERROR rc_avpair_add PW_USER_NAME\n");
    } else {
//
// Fill in User-Password

    if (rc_avpair_add(g_rh, &send, PW_USER_PASSWORD, g_passwd, -1, 0) == NULL) {
        fprintf(stderr, "[RADIUS] ERROR rc_avpair_add PW_USER_PASSWORD\n");
    } else {

// Fill in Service-Type

        service = PW_AUTHENTICATE_ONLY;
        if (rc_avpair_add(g_rh, &send, PW_SERVICE_TYPE, &service, -1, 0) == NULL) {
            fprintf(stderr, "[RADIUS] ERROR rc_avpair_add PW_SERVICE_TYPE\n");
        } else {
            // result = RESULT always < 0 !!!
            rc_auth(g_rh, 0, send, &received, msg);
            {
                *aaa_result = 0;
                if (received != NULL) {
                    if ((vp = rc_avpair_get(received, PW_FRAMED_IPV6_PREFIX, 0)) != NULL) {
                        *aaa_result += 1;
                        int netmask = vp->strvalue[1];
                        int num_bytes = netmask / 8;
                        int i;
                        for (i = 0; i < num_bytes; i++) {
                            prefix.s6_addr[i] = vp->strvalue[2 + i];
                        }
                        for (i = num_bytes; i < 16; i++) {
                            prefix.s6_addr[i] = 0;
                        }
                    }
                    if ((vp = rc_avpair_get(received, PW_FRAMED_INTERFACE_ID, 0)) != NULL) {
                        *aaa_result += 1;
                        int i;
                        for (i = 0; i < 8; i++) {
                            prefix.s6_addr[8 + i] = prefix.s6_addr[8 + i] | vp->strvalue[i];
                        }
                    }
                    rc_avpair_free(received);
                }
                if (*aaa_result >= 2) {
                    l_flag = 1;
                    dbg("[RADIUS] Assigned IPv6 @ for MN UID %x:%x:%x:%x:%x:%x:%x:%x <=> %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(&mnid), NIP6ADDR(&prefix));
                    dbg("[RADIUS] \"%s\" Authentication OK\n", g_username);
                    insert_into_hnp_cache(mnid, prefix);
                    return prefix;
                }
            }
        }
    }
    }
#endif
    dbg("MN-ID not found\n");
    struct in6_addr tmp;
    memset(&tmp, 0, sizeof(struct in6_addr));
    *aaa_result = -1;
    return tmp;
}