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
#define DMM_HANDLER_C
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
//---------------------------------------------------------------------------------------------------------------------
#include "dmm_fsm.h"
#include "handlers.h"
#include "cmd_proc.h"
#include "mar_proc.h"
#include "dmm_msgs.h"
//---------------------------------------------------------------------------------------------------------------------
#include "ndisc.h"
#ifdef ENABLE_VT
#    include "vt.h"
#endif
#include "debug.h"
#include "conf.h"
#include "util.h"
//---------------------------------------------------------------------------------------------------------------------
struct in6_addr *link_local_addr(struct in6_addr *id)
{
    static struct in6_addr ADDR;
    ADDR = in6addr_any;
    ADDR.s6_addr32[0] = htonl(0xfe800000);
    //copy the MN_ID.
    memcpy(&ADDR.s6_addr32[2], &id->s6_addr32[2], sizeof(ip6mnid_t));
    return &ADDR;
}
//---------------------------------------------------------------------------------------------------------------------
struct in6_addr *CONVERT_ID2ADDR(struct in6_addr *result, struct in6_addr *prefix, struct in6_addr *id)
{
    *result = in6addr_any;
    memcpy(&result->s6_addr32[0], &prefix->s6_addr32[0], sizeof(ip6mnid_t));
    memcpy(&result->s6_addr32[2], &id->s6_addr32[2], sizeof(ip6mnid_t));
    return result;
}
//---------------------------------------------------------------------------------------------------------------------
struct in6_addr *get_mn_addr(dmm_entry * bce)
{
    CONVERT_ID2ADDR(&bce->mn_addr, &bce->mn_prefix, &bce->mn_suffix);
    return &bce->mn_addr;
}
//---------------------------------------------------------------------------------------------------------------------
struct in6_addr *solicited_mcast(struct in6_addr *id)
{
    //NUD_ADDR converts an ID into a Multicast Address for NS Unreachability!
    static struct in6_addr ADDR2;
    ADDR2 = in6addr_any;
    ADDR2.s6_addr32[0] = htonl(0xff020000);
    ADDR2.s6_addr32[1] = htonl(0x00000000);
    ADDR2.s6_addr32[2] = htonl(0x00000001);
    ADDR2.s6_addr[12] = 0xff;
    //copy the least 24 bits from the MN_ID.
    memcpy(&ADDR2.s6_addr[13], &id->s6_addr[13], 3 * sizeof(ADDR2.s6_addr[0]));
    return &ADDR2;
}
//---------------------------------------------------------------------------------------------------------------------
void pbu_retx_timer_handler(struct tq_elem *tqe)
{
    int mutex_return_code;

    mutex_return_code = pthread_rwlock_wrlock(&dmm_lock);
    if (mutex_return_code != 0) {
        dbg("pthread_rwlock_wrlock(&dmm_lock) %s\n", strerror(mutex_return_code));
    }
    printf("-------------------------------------\n");
    if (!task_interrupted()) {
        dmm_entry *e = tq_data(tqe, dmm_entry, tqe);
        mutex_return_code = pthread_rwlock_wrlock(&e->lock);
        if (mutex_return_code != 0) {
            dbg("pthread_rwlock_wrlock(&e->lock) %s\n", strerror(mutex_return_code));
        }
        dbg("Retransmissions counter : %d\n", e->n_rets_counter);
        if (e->n_rets_counter == 0) {
            free_iov_data((struct iovec *) &e->mh_vec, e->iovlen);
            dbg("No PBA received from LMA....\n");
            dbg("Abort Trasmitting the PBU....\n");
            mutex_return_code = pthread_rwlock_unlock(&e->lock);
            if (mutex_return_code != 0) {
                dbg("pthread_rwlock_unlock(&e->lock) %s\n", strerror(mutex_return_code));
            }
            dmm_cache_delete(e);
        } else {
            //Decrement the N trasnmissions counter.
            e->n_rets_counter--;
            struct in6_addr_bundle addrs;
            addrs.src = &conf.OurAddress;
            addrs.dst = &conf.CmdAddress;
            //sends a PBU
            dbg("Send PBU again....\n");

            // INCREMENT SEQ NUMBER OF PBU
            e->seqno_out        = get_new_pbu_sequence_number();
            ((struct ip6_mh_binding_update *)(e->mh_vec[0].iov_base))->ip6mhbu_seqno = htons(e->seqno_out);

            pmip_mh_send(&addrs, e->mh_vec, e->iovlen, e->link);
            //add a new task for PBU retransmission.
            struct timespec expires;
            clock_gettime(CLOCK_REALTIME, &e->add_time);
            tsadd(e->add_time, conf.RetransmissionTimeOut, expires);
            add_task_abs(&expires, &e->tqe, pbu_retx_timer_handler);
            dbg("PBU Retransmissions timer is triggered again....\n");
            mutex_return_code = pthread_rwlock_unlock(&e->lock);
            if (mutex_return_code != 0) {
                dbg("pthread_rwlock_unlock(&e->lock) %s\n", strerror(mutex_return_code));
            }
        }
    }
    mutex_return_code = pthread_rwlock_unlock(&dmm_lock);
    if (mutex_return_code != 0) {
        dbg("pthread_rwlock_unlock(&dmm_lock) %s\n", strerror(mutex_return_code));
    }
}
//---------------------------------------------------------------------------------------------------------------------
void bce_expired_handler(struct tq_elem *tqe)
{
    int mutex_return_code;
    mutex_return_code = pthread_rwlock_wrlock(&dmm_lock);
    if (mutex_return_code != 0) {
        dbg("pthread_rwlock_wrlock(&dmm_lock) %s\n", strerror(mutex_return_code));
    }
    printf("-------------------------------------\n");
    if (!task_interrupted()) {
        dmm_entry *e = tq_data(tqe, dmm_entry, tqe);
        mutex_return_code = pthread_rwlock_wrlock(&e->lock);
        if (mutex_return_code != 0) {
            dbg("pthread_rwlock_wrlock(&e->lock) %s\n", strerror(mutex_return_code));
        }
        dbg("Retransmissions counter : %d\n", e->n_rets_counter);
        if (e->n_rets_counter == 0) {
            dbg("Retransmissions counter expired\n");
            free_iov_data((struct iovec *) &e->mh_vec, e->iovlen);
            if (is_mar()) {
                //++e->seqno_out;
                dbg("Calling deregistration\n");
                mar_dereg(e, 1);
                dmm_cache_release(e);
                dmm_cache_delete(e);
                return;
            }
            if (is_cmd()) {
                cmd_dereg(e, 0, 0);
                dmm_cache_release(e);
                dmm_cache_delete(e);
                return;
            }
            mutex_return_code = pthread_rwlock_unlock(&dmm_lock);
            if (mutex_return_code != 0) {
                dbg("pthread_rwlock_unlock(&dmm_lock) %s\n", strerror(mutex_return_code));
            }
            return;
        }
        if (is_mar()) {
            dbg("Send NS for Neighbour Reachability for:%x:%x:%x:%x:%x:%x:%x:%x iif=%d\n", NIP6ADDR(&e->mn_hw_address), e->link);
            //Create NS for Reachability test!
	    ndisc_send_ns(e->link, &in6addr_any, solicited_mcast(&e->mn_suffix), get_mn_addr(e));

            struct timespec expires;
            clock_gettime(CLOCK_REALTIME, &e->add_time);
            tsadd(e->add_time, conf.RetransmissionTimeOut, expires);
            // Add a new task for deletion of entry if No Na is received.
            add_task_abs(&expires, &e->tqe, bce_expired_handler);
            dbg("Start the Timer for Retransmission/Deletion ....\n");
            //Decrements the Retransmissions counter.
            e->n_rets_counter--;
            mutex_return_code = pthread_rwlock_unlock(&e->lock);
            if (mutex_return_code != 0) {
                dbg("pthread_rwlock_unlock(&e->lock) %s\n", strerror(mutex_return_code));
            }
        }
        if (is_cmd()) {
            cmd_dereg(e, 0, 0);
            dmm_cache_release(e);
            dmm_cache_delete(e);
            return;
        }
    }
    mutex_return_code = pthread_rwlock_unlock(&dmm_lock);
    if (mutex_return_code != 0) {
        dbg("pthread_rwlock_unlock(&dmm_lock) %s\n", strerror(mutex_return_code));
    }
}

static inline int ipv6_addr_is_solicited_mcast(const struct in6_addr *addr)
{
    return (addr->s6_addr32[0] == htonl(0xff020000)
            && addr->s6_addr32[1] == htonl(0x00000000)
            && addr->s6_addr32[2] == htonl(0x00000001)
            && addr->s6_addr[12] == 0xff);
}

static inline int ipv6_addr_is_multicast(const struct in6_addr *addr)
{
    return (addr->s6_addr32[0] & htonl(0xFF000000)) == htonl(0xFF000000);
}

static inline int ipv6_addr_is_linklocal(const struct in6_addr *addr)
{
    return IN6_IS_ADDR_LINKLOCAL(addr);
}

/*!
 * handler called when receiving a router solicitation
 */
static void mar_recv_rs(const struct icmp6_hdr *ih, ssize_t len, const struct in6_addr *saddr, const struct in6_addr *daddr, int iif, int hoplimit)
{
	timers_log_start();
	printf("=====================================\n");
	if (hoplimit < 255 || !IN6_IS_ADDR_LINKLOCAL(saddr) ||
	    ih->icmp6_code != 0 || len < sizeof(struct nd_router_solicit)) {
		dbg("Problems receiving RS message\n");
		return;
	}
	dbg("Router Solicitation (RS) Received iif %d\n", iif);
	msg_info_t rs_info;
	bzero(&rs_info, sizeof(rs_info));
	rs_info.length = len;
	int ret = icmp_rs_parse(&rs_info, (struct nd_router_solicit *) ih, saddr, daddr, iif, hoplimit);
	if (ret >= 0) {
		dbg("RS parsed correctly, delivering it to FSM\n");
		mar_fsm(&rs_info);
	}
	else dbg("Problems parsing RS message\n");
}

/*!
 * handler called when receiving a proxy binding acknowledgment
 */
static void mar_recv_pba(const struct ip6_mh *mh, ssize_t len, const struct in6_addr_bundle *in_addrs, int iif)
{
	timers_log_pba_recv();
    printf("=====================================\n");
    dbg("Proxy Binding Acknowledgement (PBA) Received\n");
    dbg("Received PBA Src Addr: %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(in_addrs->src));
    dbg("Received PBA Dst addr: %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(in_addrs->dst));
    //define the values for calling the parsing function
    //call the parsing function
    struct ip6_mh_binding_ack *pba;
    //call the fsm function.
    msg_info_t info;
    memset(&info, 0, sizeof(struct msg_info_t));
    pba = (struct ip6_mh_binding_ack *) ((void *) mh);
    info.length = len;
    info.pba_status = mh_pba_parse(&info, pba, len, in_addrs, iif);
    if (info.pba_status == 0) {
	    dbg("PBA parsed correctly, delivering it to FSM\n");
	    mar_fsm(&info);
    }
    else dbg ("Problems parsing PBA message\n");
}

/*!
 * handler called when receiving a proxy binding acknowledgment
 */
static void cmd_recv_pba(const struct ip6_mh *mh, ssize_t len, const struct in6_addr_bundle *in_addrs, int iif)
{
	timers_log_pba_recv();
    printf("=====================================\n");
    dbg("Proxy Binding Acknowledgement (PBA) Received\n");
    dbg("Received PBA Src Addr: %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(in_addrs->src));
    dbg("Received PBA Dst addr: %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(in_addrs->dst));
    //define the values for calling the parsing function
    //call the parsing function
    struct ip6_mh_binding_ack *pba;
    //call the fsm function.
    msg_info_t info;
    pba = (struct ip6_mh_binding_ack *) ((void *) mh);
    info.length = len;
    info.pba_status = mh_pba_parse(&info, pba, len, in_addrs, iif);
    if (info.pba_status >= 0) {
	    dbg("PBA parsed correctly, delivering it to FSM\n");
	    cmd_fsm(&info);
    }
    else dbg ("Problems parsing PBA message\n");
}

/*!
 * handler called when receiving a proxy binding update
 */
static void cmd_recv_pbu(const struct ip6_mh *mh, ssize_t len, const struct in6_addr_bundle *in_addrs, int iif)
{
	timers_log_pbu_recv();
    printf("=====================================\n");
    dbg("Proxy Binding Update (PBU) Received\n");
    dbg("Received PBU Src Addr: %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(in_addrs->src));
    dbg("Received PBU Dst addr: %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(in_addrs->dst));
    //define the values for the parsing function
    //call the parsing function
    struct ip6_mh_binding_update *pbu = (struct ip6_mh_binding_update *) mh;
    //call the fsm function.
    msg_info_t info;
    bzero(&info, sizeof(info));
    info.length = len;
    info.pbu_status = mh_pbu_parse(&info, pbu, len, in_addrs, iif);
    if (info.pbu_status == 0) {
	    dbg("PBU parsed correctly, delivering it to FSM\n");
	    cmd_fsm(&info);
    }
    else
	dbg("Problems parsing PBU message\n");
}

/*!
 * handler called when receiving a proxy binding update
 */
static void mar_recv_pbu(const struct ip6_mh *mh, ssize_t len, const struct in6_addr_bundle *in_addrs, int iif)
{
	timers_log_pbu_recv();
    printf("=====================================\n");
    dbg("Proxy Binding Update (PBU) Received\n");
    dbg("Received PBU Src Addr: %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(in_addrs->src));
    dbg("Received PBU Dst addr: %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(in_addrs->dst));
    //define the values for the parsing function
    //call the parsing function
    struct ip6_mh_binding_update *pbu = (struct ip6_mh_binding_update *) mh;
    //call the fsm function.
    msg_info_t info;
    bzero(&info, sizeof(info));
    info.length = len;
    info.pbu_status = mh_pbu_parse(&info, pbu, len, in_addrs, iif);
    if (info.pbu_status >= 0) {
	    dbg("PBU parsed correctly, delivering it to FSM\n");
	    mar_fsm(&info);
    }
    else
	dbg("Problems parsing PBU message\n");
}

/*!
 * handler called when MAR receive a neighbor advertisement
 */
static void mar_recv_na(const struct icmp6_hdr *ih, ssize_t len, const struct in6_addr *saddr, const struct in6_addr *daddr, int iif, int hoplimit)
{
    struct nd_neighbor_advert *msg = (struct nd_neighbor_advert *) ih;
    //Check target is not link local address.
    if (ipv6_addr_is_linklocal(&msg->nd_na_target)) {
        return;
    }
    //Check target is not multicast.
    if (ipv6_addr_is_multicast(&msg->nd_na_target)) {
        return;
    }
    if (len - sizeof(struct nd_neighbor_advert) > 0) {
        printf("=====================================\n");
        dbg("Neighbor Advertisement (NA) Received\n");
        dbg("Received NA Src Addr: %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(saddr));
        dbg("Received NA Dst addr: %x:%x:%x:%x:%x:%x:%x:%x\n", NIP6ADDR(daddr));
        msg_info_t na_info;
        bzero(&na_info, sizeof(na_info));
        icmp_na_parse(&na_info, (struct nd_neighbor_advert *) ih, saddr, daddr, iif, hoplimit);
        mar_fsm(&na_info);
    }
    return;
}

struct icmp6_handler mar_rs_handler = {
        .recv = mar_recv_rs
};

struct mh_handler mar_pba_handler = {
        .recv = mar_recv_pba
};

struct mh_handler mar_pbu_handler = {
        .recv = mar_recv_pbu
};

struct mh_handler cmd_pbu_handler = {
        .recv = cmd_recv_pbu
};

struct mh_handler cmd_pba_handler = {
        .recv = cmd_recv_pba
};

struct icmp6_handler mar_na_handler = {
        .recv = mar_recv_na
};