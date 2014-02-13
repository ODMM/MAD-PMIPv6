/* $Id: conf.h 1.39 06/05/12 11:48:36+03:00 vnuorval@tcs.hut.fi $ */

#ifndef __CONF_H__
#define __CONF_H__ 1

#include <time.h>
#include <net/if.h>
#include "list.h"
//#include "pmgr.h"

struct mip6_config {
    /* Common options */
    char            *config_file;
#ifdef ENABLE_VT
    char            *vt_hostname;
    char            *vt_service;
#endif
    unsigned int    mip6_entity;
    unsigned int    debug_level;
    char *          debug_log_file;
    //struct pmgr_cb  pmgr;
    struct list_head net_ifaces;
    struct list_head bind_acl;
    uint8_t         DefaultBindingAclPolicy;
    char            NonVolatileBindingCache;

    /* IPsec options */
    char            KeyMngMobCapability;
    char            UseMnHaIPsec;
    struct list_head ipsec_policies;

    /* MN options */
    unsigned int    MnMaxHaBindingLife;
    unsigned int    MnMaxCnBindingLife;
    unsigned int    MnRouterProbes;
    struct timespec MnRouterProbeTimeout_ts;
    struct timespec InitialBindackTimeoutFirstReg_ts;
    struct timespec InitialBindackTimeoutReReg_ts;
    struct list_head home_addrs;
    char *          MoveModulePath;
    uint16_t        CnBuAck;
    char            MnUseAllInterfaces;
    char            MnDiscardHaParamProb;
    char            SendMobPfxSols;
    char            OptimisticHandoff;

    /* HA options */
    char            SendMobPfxAdvs;
    char            SendUnsolMobPfxAdvs;
    unsigned int    MaxMobPfxAdvInterval;
    unsigned int    MinMobPfxAdvInterval;
    unsigned int    HaMaxBindingLife;


    /* PMIP global options */
    unsigned int    RFC5213TimestampBasedApproachInUse;
    unsigned int    RFC5213MobileNodeGeneratedTimestampInUse;
    struct in6_addr RFC5213FixedMARLinkLocalAddressOnAllAccessLinks;
    struct in6_addr RFC5213FixedMARLinkLayerAddressOnAllAccessLinks;

    /* PMIP LMA options */
    struct timespec RFC5213MinDelayBeforeBCEDelete;
    struct timespec RFC5213MaxDelayBeforeNewBCEAssign;
    struct timespec RFC5213TimestampValidityWindow;

    /* PMIP MAG options */
    struct in6_addr CmdAddress;                 // address of CMD, DMM network side.
    char*           CmdDmmNetworkDevice;        // CMD device, DMM network side.
    struct in6_addr CmdCoreNetworkAddress;      // address of CMD, core network side.
    char*           CmdCoreNetworkDevice;       // PMIP CMD device, core network side.
    struct in6_addr MarAddressIngress;          // ingress address of MAG.
    struct in6_addr MarAddressEgress;           // egress address of MAG.
    char*           MarDeviceIngress;           // ingress device.
    char*           MarDeviceEgress;            // egress device.
    struct in6_addr OurAddress;
    struct in6_addr HomeNetworkPrefix;          // home network address common for domain!
    struct timespec PBULifeTime;                // Life time of Proxy Binding Update.
    struct timespec PBALifeTime;                // Life time MR side.
    struct timespec RetransmissionTimeOut;      // Time-out before retransmission of a message.
    int             MaxMessageRetransmissions;  //indicates the maximum number of message retransmissions
    char            TunnelingEnabled;
    char            DynamicTunnelingEnabled;
    unsigned int    DLIFsupportEnabled;
    unsigned int    HostapdListenerEnabled;
};

struct net_iface {
    struct list_head list;
    char name[IF_NAMESIZE];
    int ifindex;
    int is_rtr;
    int mip6_if_entity;
    int mn_if_preference;
};

extern struct mip6_config conf;

#define MIP6_ENTITY_NO -1
#define MIP6_ENTITY_CN 0
#define MIP6_ENTITY_MN 1
#define MIP6_ENTITY_HA 2
#define MIP6_ENTITY_MAR 3
#define MIP6_ENTITY_CMD 4

static inline int is_cn(void)
{
    return conf.mip6_entity == MIP6_ENTITY_CN;
}

static inline int is_mn(void)
{
    return conf.mip6_entity == MIP6_ENTITY_MN;
}

static inline int is_ha(void)
{
    return conf.mip6_entity == MIP6_ENTITY_HA;
}

static inline int is_mar(void)
{
    return conf.mip6_entity == MIP6_ENTITY_MAR;
}

static inline int is_cmd(void)
{
    return conf.mip6_entity == MIP6_ENTITY_CMD;
}

static inline int is_if_entity_set(struct net_iface *i)
{
    return i->mip6_if_entity != MIP6_ENTITY_NO;

}

static inline int is_if_cn(struct net_iface *i)
{
    return (is_cn() &&
            (!is_if_entity_set(i) || i->mip6_if_entity == MIP6_ENTITY_CN));

}

static inline int is_if_mn(struct net_iface *i)
{
    return (is_mn() &&
            (!is_if_entity_set(i) || i->mip6_if_entity == MIP6_ENTITY_MN));
}

static inline int is_if_ha(struct net_iface *i)
{
    return (is_ha() &&
            (!is_if_entity_set(i) || i->mip6_if_entity == MIP6_ENTITY_HA));
}

static inline int is_if_lma(struct net_iface *i)
{
    return (is_cmd() &&
            (!is_if_entity_set(i) || i->mip6_if_entity == MIP6_ENTITY_CMD));
}

static inline int is_if_mag(struct net_iface *i)
{
    return (is_mar() &&
            (!is_if_entity_set(i) || i->mip6_if_entity == MIP6_ENTITY_MAR));
}

int conf_parse(struct mip6_config *c, int argc, char **argv);

void conf_show(struct mip6_config *c);

int yyparse(void);

int yylex(void);

#endif