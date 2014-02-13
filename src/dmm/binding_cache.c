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
#define DMM_CACHE_C
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif
//---------------------------------------------------------------------------------------------------------------------
#include "binding_cache.h"
#include "handlers.h"
#include "mar_proc.h"
//---------------------------------------------------------------------------------------------------------------------
#ifdef ENABLE_VT
#    include "vt.h"
#endif
#include "debug.h"
#include "conf.h"
//---------------------------------------------------------------------------------------------------------------------
static struct hash	g_dmm_hash;
static int		g_dmm_cache_count = 0;
//---------------------------------------------------------------------------------------------------------------------
int dmm_cache_init(void)
{
	int ret;
	int mutex_return_code;
	if (pthread_rwlock_init(&dmm_lock, NULL)) {
		return -1;
	}
	mutex_return_code = pthread_rwlock_wrlock(&dmm_lock);
	if (mutex_return_code != 0) {
		dbg("pthread_rwlock_wrlock(&dmm_lock) %s\n", strerror(mutex_return_code));
	}

	ret = hash_init(&g_dmm_hash, DOUBLE_ADDR, DMM_CACHE_BUCKETS);

	mutex_return_code = pthread_rwlock_unlock(&dmm_lock);
	if (mutex_return_code != 0) {
		dbg("pthread_rwlock_unlock(&dmm_lock) %s\n", strerror(mutex_return_code));
	}

#ifdef ENABLE_VT
	if (ret < 0)
		return ret;
	ret = vt_pbc_init();
#endif
	return ret;
}
//---------------------------------------------------------------------------------------------------------------------
void init_iface_ra()
{
	router_ad_iface.AdvSendAdvert = DFLT_AdvSendAdv;
	router_ad_iface.MaxRtrAdvInterval = DFLT_MaxRtrAdvInterval;
	router_ad_iface.MinRtrAdvInterval = 1;  //changed from -1
	router_ad_iface.AdvIntervalOpt = DFLT_AdvIntervalOpt;
	router_ad_iface.AdvCurHopLimit = DFLT_AdvCurHopLimit;
	router_ad_iface.AdvHomeAgentFlag = DFLT_AdvHomeAgentFlag;
	router_ad_iface.AdvHomeAgentInfo = DFLT_AdvHomeAgentInfo;
	router_ad_iface.HomeAgentPreference = DFLT_HomeAgentPreference;
	router_ad_iface.HomeAgentLifetime = 10000;  //changed from -1
	router_ad_iface.AdvReachableTime = DFLT_AdvReachableTime;
	router_ad_iface.AdvRetransTimer = DFLT_AdvRetransTimer;
	router_ad_iface.AdvDefaultLifetime = conf.PBULifeTime.tv_sec;
	router_ad_iface.AdvManagedFlag = 0;
	router_ad_iface.AdvOtherConfigFlag = 0;
	// default values for Prefix.
	router_ad_iface.Adv_Prefix.AdvOnLinkFlag = DFLT_AdvOnLinkFlag;
	router_ad_iface.Adv_Prefix.AdvAutonomousFlag = DFLT_AdvAutonomousFlag;
	router_ad_iface.Adv_Prefix.AdvRouterAddr = DFLT_AdvRouterAddr;
	router_ad_iface.Adv_Prefix.PrefixLen = 64;
	router_ad_iface.Adv_Prefix.AdvValidLifetime = conf.PBULifeTime.tv_sec;
	router_ad_iface.Adv_Prefix.AdvPreferredLifetime = conf.PBULifeTime.tv_sec;
}
//---------------------------------------------------------------------------------------------------------------------
dmm_entry *dmm_cache_alloc(int type)
{
	dmm_entry *tmp;
	tmp = (dmm_entry *)malloc(sizeof(dmm_entry));
	if (tmp == NULL) {
		dbg("NO memory allocated for DMM cache entry..\n");
		return NULL;
	}
	memset(tmp, 0, sizeof(*tmp));
	if (pthread_rwlock_init(&tmp->lock, NULL)) {
		dbg("ERROR lock init\n");
		free(tmp);
		return NULL;
	}
	INIT_LIST_HEAD(&tmp->tqe.list);
	tmp->type = type;
	return tmp;
}
//---------------------------------------------------------------------------------------------------------------------
static int __dmmcache_insert(dmm_entry * bce)
{
	int ret;
	ret = hash_add(&g_dmm_hash, bce, &bce->our_addr, &bce->mn_hw_address);
	if (ret) {
		return ret;
	}
	g_dmm_cache_count++;
	return 0;
}
//---------------------------------------------------------------------------------------------------------------------
int dmm_cache_start(dmm_entry * bce)
{
	struct timespec expires;
	clock_gettime(CLOCK_REALTIME, &bce->add_time);
	tsadd(bce->add_time, bce->lifetime, expires);

	add_task_abs(&expires, &bce->tqe,  bce_expired_handler);
	return 0;
}
//---------------------------------------------------------------------------------------------------------------------
int dmm_cache_stop(dmm_entry * bce)
{
	//dbg("DMM cache timer is stopped and lifetime set to 0\n");
	clock_gettime(CLOCK_REALTIME, &bce->add_time);
	del_task(&bce->tqe);
	bce->lifetime.tv_sec = 0;
	bce->lifetime.tv_nsec = 0;
	return 0;
}
//---------------------------------------------------------------------------------------------------------------------
dmm_entry *dmm_cache_add(dmm_entry * bce)
{
	int ret = 1;
	int mutex_return_code;
	assert(bce);
	bce->unreach = 0;
	mutex_return_code = pthread_rwlock_wrlock(&dmm_lock);
	if (mutex_return_code != 0) {
		dbg("pthread_rwlock_wrlock(&dmm_lock) %s\n", strerror(mutex_return_code));
	}
	if ((ret = __dmmcache_insert(bce)) != 0) {
		pthread_rwlock_unlock(&dmm_lock);
		dbg("WARNING: DMM ENTRY NOT INSERTED..\n");
		return NULL;
	}
	//dbg("Making Entry\n");
	dbg("DMM cache entry for: %x:%x:%x:%x:%x:%x:%x:%x with type %d is added\n", NIP6ADDR(&bce->mn_hw_address), bce->type);
	bce->n_rets_counter = conf.MaxMessageRetransmissions;
	dbg("Retransmissions counter intialized: %d\n", bce->n_rets_counter);
	if (bce->type == BCE_DMM) {
		dmm_cache_start(bce);
	}
	mutex_return_code = pthread_rwlock_unlock(&dmm_lock);
	if (mutex_return_code != 0) {
		dbg("pthread_rwlock_unlock(&dmm_lock) %s\n", strerror(mutex_return_code));
	}
	return bce;
}
//---------------------------------------------------------------------------------------------------------------------
dmm_entry *dmm_cache_get(const struct in6_addr * our_addr, const struct in6_addr * peer_addr)
{
	dmm_entry *bce;
	int mutex_return_code;

	assert(peer_addr && our_addr);
	mutex_return_code = pthread_rwlock_rdlock(&dmm_lock);
	if (mutex_return_code != 0) {
		dbg("pthread_rwlock_rdlock(&dmm_lock) %s\n", strerror(mutex_return_code));
	}
	bce = hash_get(&g_dmm_hash, our_addr, peer_addr);
	if (bce) {
		mutex_return_code = pthread_rwlock_wrlock(&bce->lock);
		if (mutex_return_code != 0) {
			dbg("pthread_rwlock_wrlock(&bce->lock) %s\n", strerror(mutex_return_code));
		}
		//dbg("DMM cache entry is found for: %x:%x:%x:%x:%x:%x:%x:%x with type %d\n", NIP6ADDR(&bce->mn_hw_address), (bce->type));
	} else {
		mutex_return_code = pthread_rwlock_unlock(&dmm_lock);
		if (mutex_return_code != 0) {
			dbg("pthread_rwlock_unlock(&dmm_lock) %s\n", strerror(mutex_return_code));
		}
		//dbg("DMM cache entry is NOT found for %x:%x:%x:%x:%x:%x:%x:%x <-> %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(our_addr), NIP6ADDR(peer_addr));
	}
	return bce;
}
//---------------------------------------------------------------------------------------------------------------------
void dmm_cache_release(dmm_entry * bce)
{
	int mutex_return_code;
	assert(bce);
	mutex_return_code = pthread_rwlock_unlock(&bce->lock);
	if (mutex_return_code != 0) {
		dbg("pthread_rwlock_unlock(&bce->lock) %s\n", strerror(mutex_return_code));
	}
    mutex_return_code = pthread_rwlock_unlock(&dmm_lock);
 	if (mutex_return_code != 0) {
 		dbg("pthread_rwlock_unlock(&dmm_lock) %s\n", strerror(mutex_return_code));
 	}
}
//---------------------------------------------------------------------------------------------------------------------
int dmm_cache_exists(const struct in6_addr *our_addr, const struct in6_addr *peer_addr)
{
	dmm_entry *bce;
	int type;
	bce = dmm_cache_get(our_addr, peer_addr);
	if (bce == NULL) {
		return -1;
	}
	dbg("DMM cache entry does exist with type: %d\n", (bce->type));
	type = bce->type;
	dmm_cache_release(bce);
	return type;
}
//---------------------------------------------------------------------------------------------------------------------
void dmm_cache_free(dmm_entry * bce)
{
/* This function should really return allocated space to free
* pool. */
	int mutex_return_code;
	mutex_return_code = pthread_rwlock_destroy(&bce->lock);
	if (mutex_return_code != 0) {
		dbg("pthread_rwlock_destroy(&bce->lock) %s\n", strerror(mutex_return_code));
	}
	free(bce);
	//dbg("DMM cache entry is free\n");
}
//---------------------------------------------------------------------------------------------------------------------
void dmm_cache_delete(dmm_entry * bce)
{
	int mutex_return_code;

	mutex_return_code = pthread_rwlock_wrlock(&bce->lock);
	if (mutex_return_code != 0) {
		dbg("pthread_rwlock_wrlock(&bce->lock) %s\n", strerror(mutex_return_code));
	}
	if (bce->mar_list) {
		mar_list_clean(bce->mar_list);
	}

	del_task(&bce->tqe);

	if (bce->cleanup) {
		bce->cleanup(bce);
	}

	g_dmm_cache_count--;

	hash_delete(&g_dmm_hash, &bce->our_addr, &bce->mn_hw_address);
	mutex_return_code = pthread_rwlock_unlock(&bce->lock);
	if (mutex_return_code != 0) {
		dbg("pthread_rwlock_unlock(&bce->lock) %s\n", strerror(mutex_return_code));
	}
	dmm_cache_free(bce);
	dbg("DMM cache entry is deleted!\n");
}
//---------------------------------------------------------------------------------------------------------------------
// void pmip_cache_delete(const struct in6_addr *our_addr, const struct in6_addr *peer_addr)
// {
// 	int mutex_return_code;
// 	dmm_entry *bce;
// 	mutex_return_code = pthread_rwlock_wrlock(&dmm_lock);
// 	if (mutex_return_code != 0) {
// 		dbg("pthread_rwlock_wrlock(&dmm_lock) %s\n", strerror(mutex_return_code));
// 	}
// 	bce = hash_get(&g_dmm_hash, our_addr, peer_addr);
// 	if (bce) {
// 		dmm_cache_delete(bce);
// 	}
// 	mutex_return_code = pthread_rwlock_unlock(&dmm_lock);
// 	if (mutex_return_code != 0) {
// 		dbg("pthread_rwlock_unlock(&dmm_lock) %s\n", strerror(mutex_return_code));
// 	}
// }
//---------------------------------------------------------------------------------------------------------------------
int dmm_cache_iterate(int (*func) (void *, void *), void *arg)
{
	int err;
	int mutex_return_code;
	mutex_return_code = pthread_rwlock_rdlock(&dmm_lock);
	if (mutex_return_code != 0) {
		dbg("pthread_rwlock_rdlock(&dmm_lock) %s\n", strerror(mutex_return_code));
	}
	err = hash_iterate(&g_dmm_hash, func, arg);
	mutex_return_code = pthread_rwlock_unlock(&dmm_lock);
	if (mutex_return_code != 0) {
		dbg("pthread_rwlock_unlock(&dmm_lock) %s\n", strerror(mutex_return_code));
	}
	return err;
}