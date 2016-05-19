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
#include <arpa/inet.h>
//---------------------------------------------------------------------------------------------------------------------
#include "hnp_cache.h"
#include "dmm_consts.h"
//---------------------------------------------------------------------------------------------------------------------

#include "util.h"
#ifdef ENABLE_VT
#    include "vt.h"
#endif
#include "debug.h"
#include "conf.h"
//---------------------------------------------------------------------------------------------------------------------

static mnid_hnp_t g_mn_hn_map[MAX_MOBILES];
static int g_mn_count = 0;
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
int dmm_mn_to_hnp_cache_init (void)
{
    FILE               *fp;
    char                str_hn_prefix[INET6_ADDRSTRLEN];
    uint8_t		temp[6];
    int                 j;
    memset(g_mn_hn_map, 0, sizeof(mnid_hnp_t) * MAX_MOBILES);
    memset(temp, 0, 6*sizeof(uint8_t));
    j = 0;
    if ((fp = fopen (DMM_USERS, "r")) == NULL) {
        printf ("can't open %s \n", DMM_USERS);
        exit (0);
    }
    while ((fscanf (fp, "%2x:%2x:%2x:%2x:%2x:%2x %s\n", &temp[0], &temp[1], &temp[2], &temp[3], &temp[4], &temp[5], str_hn_prefix) != EOF) && (j < MAX_MOBILES)) {
	inet_pton(AF_INET6, str_hn_prefix, &g_mn_hn_map[j].mn_prefix);
	for (int i = 0; i < 6; i++) {
		g_mn_hn_map[j].mn_iid.s6_addr[i+10] = temp[i];
	}
	g_mn_count++;
	dbg ("USER %d: %x:%x:%x:%x:%x:%x:%x:%x <=> %x:%x:%x:%x:%x:%x:%x:%x\n", g_mn_count, NIP6ADDR (&g_mn_hn_map[j].mn_prefix), NIP6ADDR (&g_mn_hn_map[j].mn_iid));
	j++;
    }
    fclose (fp);
    if (j >= MAX_MOBILES) {
        dbg ("ERROR TOO MANY MAPPINGS DEFINED IN CONFIG FILE\n");
        exit (0);
    }
    return 0;
}
// int dmm_mn_to_hnp_cache_init (void)
// {
//     FILE               *fp;
//     char                str_addr[40], str_addr_iid[40];
//     struct in6_addr     addr, addr1;
//     unsigned int        ap, ap1;
//     int                 i, j;
// 
//     memset(g_mn_hn_map, 0, sizeof(mnid_hnp_t) * MAX_MOBILES);
//     j = 0;
//     if ((fp = fopen (DMM_USERS, "r")) == NULL) {
//         printf ("can't open %s \n", DMM_USERS);
//         exit (0);
//     }
//     while ((fscanf (fp, "%32s %32s\n", str_addr, str_addr_iid) != EOF) && (j < MAX_MOBILES)) {
//         for (i = 0; i < 16; i++) {
//             sscanf (str_addr + i * 2, "%02x", &ap);
//             addr.s6_addr[i] = (unsigned char) ap;
//             g_mn_hn_map[j].mn_prefix = addr;
//             sscanf (str_addr_iid + i * 2, "%02x", &ap1);
//             addr1.s6_addr[i] = (unsigned char) ap1;
//             g_mn_hn_map[j].mn_iid = addr1;
//         }
//         dbg ("%x:%x:%x:%x:%x:%x:%x:%x <=> %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR (&g_mn_hn_map[j].mn_prefix), NIP6ADDR (&g_mn_hn_map[j].mn_iid));
//         j++;
//         g_mn_count = g_mn_count + 1;
//     }
//     fclose (fp);
//     if (j >= MAX_MOBILES) {
//         dbg ("ERROR TOO MANY MAPPINGS DEFINED IN CONFIG FILE\n");
//         exit (0);
//     }
//     return 0;
// }
//-----------------------------------------------------------------------------

struct in6_addr *mnid_hnp_map(struct in6_addr * mnid)
{
    int j = 0;
    while (j < g_mn_count) {
        if (IN6_ARE_ADDR_EQUAL(&g_mn_hn_map[j].mn_iid, mnid)) {
	    dbg("USER %d: prefix to assign: %x:%x:%x:%x:%x:%x:%x:%x \n",j+1, NIP6ADDR(&g_mn_hn_map[j].mn_prefix));
            return (&g_mn_hn_map[j].mn_prefix);}
        j++;
    }
    dbg("MN-ID not found\n");
    return NULL;
}