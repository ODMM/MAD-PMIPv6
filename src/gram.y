/*
 * $Id: gram.y 1.88 06/05/12 11:48:36+03:00 vnuorval@tcs.hut.fi $
 *
 * This file is part of the MIPL Mobile IPv6 for Linux.
 *
 * Authors: Antti Tuominen <anttit@tcs.hut.fi>
 *          Ville Nuorvala <vnuorval@tcs.hut.fi>
 *
 * Copyright 2003-2005 Go-Core Project
 * Copyright 2003-2006 Helsinki University of Technology
 *
 * MIPL Mobile IPv6 for Linux is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; version 2 of
 * the License.
 *
 * MIPL Mobile IPv6 for Linux is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MIPL Mobile IPv6 for Linux; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307 USA
 */
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

%{

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <pthread.h>
#include <netinet/in.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <netinet/ip6mh.h>
#include "mipv6.h"
//#include "ha.h"
//#include "mn.h"
#include "conf.h"
//#include "policy.h"
//#include "xfrm.h"
//#include "prefix.h"
#include "util.h"
//#include "ipsec.h"
#include "rtnl.h"

struct net_iface ni = {
	.mip6_if_entity = MIP6_ENTITY_NO,
// 	.mn_if_preference = POL_MN_IF_DEF_PREFERENCE,
};

// struct home_addr_info hai = {
// 	.ro_policies = LIST_HEAD_INIT(hai.ro_policies)
// };

struct policy_bind_acl_entry *bae = NULL;

struct ipsec_policy_set {
	struct in6_addr ha;
	struct list_head hoa_list;
};

struct ipsec_policy_set ipsec_ps = {
	.hoa_list = LIST_HEAD_INIT(ipsec_ps.hoa_list)
};


extern int lineno;
extern char *yytext;

static void yyerror(char *s) {
	fprintf(stderr, "Error in configuration file %s\n", conf.config_file);
	fprintf(stderr, "line %d: %s at '%s'\n", lineno, s, yytext);
}

static void uerror(const char *fmt, ...) {
	char s[1024];
	va_list args;

	fprintf(stderr, "Error in configuration file %s\n", conf.config_file);
	va_start(args, fmt);
	vsprintf(s, fmt, args);
	fprintf(stderr, "line %d: %s\n", lineno, s);
	va_end(args);
}

%}

%union {
	char *string;
	struct in6_addr addr;
	struct in6_addr macaddr;
	char bool;
	unsigned int num;
	unsigned int numpair[2];
	double dec;
}

%token <string> QSTRING
%token <addr>	ADDR
%token <macaddr>	MACADDR
%token <bool>	BOOL
%token <num>	NUMBER
%token <dec>	DECIMAL
%token <numpair>	NUMPAIR;

%token		MIP6ENTITY
%token		DEBUGLEVEL
%token		DEBUGLOGFILE
%token		HOMEADDRESS
%token		HOMEAGENTADDRESS
%token		INITIALBINDACKTIMEOUTFIRSTREG
%token		INITIALBINDACKTIMEOUTREREG
%token		LINKNAME
%token		HAMAXBINDINGLIFE
%token		MNMAXHABINDINGLIFE
%token		MNMAXCNBINDINGLIFE
%token		MAXMOBPFXADVINTERVAL
%token		MINMOBPFXADVINTERVAL
%token		MNHOMELINK
%token		HAHOMELINK
%token		NONVOLATILEBINDINGCACHE
%token		SENDMOBPFXSOLS
%token		SENDUNSOLMOBPFXADVS
%token		SENDMOBPFXADVS
%token		IPSECPOLICYSET
%token		IPSECPOLICY
%token		IPSECTYPE
%token		USEALTCOA
%token		USEESP
%token		USEAH
%token		USEIPCOMP
%token		BLOCK
%token		USEMNHAIPSEC
%token		KEYMNGMOBCAPABILITY
%token		HOMEREGBINDING
%token		MH
%token		MOBPFXDISC
%token		TUNNELHOMETESTING
%token		TUNNELMH
%token		TUNNELPAYLOAD
%token		USEMOVEMENTMODULE
%token		USEPOLICYMODULE
%token		INTERNAL
%token		MNROPOLICY
%token		ICMP
%token		ANY
%token		DEFAULTBINDINGACLPOLICY
%token		BINDINGACLPOLICY
%token		MNADDRESS
%token		USECNBUACK
%token		INTERFACE
%token		IFNAME
%token		IFTYPE
%token		MNIFPREFERENCE
%token		MNUSEALLINTERFACES
%token		MNDISCARDHAPARAMPROB
%token		OPTIMISTICHANDOFF
/* PMIP CONF ELEMENTS */
%token      RFC5213TIMESTAMPBASEDAPPROACHINUSE;
%token      RFC5213MOBILENODEGENERATEDTIMESTAMPINUSE;
%token      RFC5213FIXEDMARLINKLOCALADDRESSONALLACCESSLINKS;
%token      RFC5213FIXEDMARLINKLAYERADDRESSONALLACCESSLINKS;
%token      RFC5213MINDELAYBEFOREBCEDELETE;
%token      RFC5213MAXDELAYBEFORENEWBCEASSIGN;
%token      RFC5213TIMESTAMPVALIDITYWINDOW;
%token      RFC5213ENABLEMARLOCALROUTING
%token		DMMCMD
%token		DMMMAR
%token		PROXYDMMCMD
%token		PROXYDMMMAR
%token		CMDADDRESS
%token		CMDDMMNETWORKDEVICE
%token		CMDCORENETWORKADDRESS
%token		CMDCORENETWORKDEVICE
%token		MARADDRESSINGRESS
%token		MARADDRESSEGRESS
%token		MARDEVICEINGRESS
%token		MARDEVICEEGRESS
%token		OURADDRESS
%token		HOMENETWORKPREFIX
%token		PBULIFETIME
%token		PBALIFETIME
%token		RETRANSMISSIONTIMEOUT
%token		MAXMESSAGERETRANSMISSIONS
%token		TUNNELINGENABLED
%token		DYNAMICTUNNELINGENABLED
/*%token		RADIUSPASSWORD
%token      RADIUSCLIENTCONFIGFILE*/
%token      PCAPSYSLOGASSOCIATIONGREPSTRING
%token      PCAPSYSLOGDEASSOCIATIONGREPSTRING

%token		INV_TOKEN

%type <num>	ipsectype
%type <num>	ipsectypeval
%type <num>	ipsecproto
%type <num>	ipsecprotos
%type <numpair>	ipsecreqid

%type <addr>	mnropolicyaddr
%type <num>	bindaclpolval
%type <num>	prefixlen
%type <num>	mip6entity
%type <bool>	xfrmaction

%%

grammar		: topdef
		| grammar topdef
		;

topdef		: MIP6ENTITY mip6entity ';'
		{
			conf.mip6_entity = $2;
		}
		| DEBUGLEVEL NUMBER ';'
		{
			conf.debug_level = $2;
		}
		| DEBUGLOGFILE QSTRING ';'
		{
			conf.debug_log_file = $2;
		}
		| NONVOLATILEBINDINGCACHE BOOL ';'
		{
			conf.NonVolatileBindingCache = $2;
		}
/* 		| INTERFACE ifacedef */
		| SENDMOBPFXSOLS BOOL ';'
		{
			conf.SendMobPfxSols = $2;
		}
		| SENDUNSOLMOBPFXADVS BOOL ';'
		{
			conf.SendUnsolMobPfxAdvs = $2;
		}
		| SENDMOBPFXADVS BOOL ';'
		{
			conf.SendMobPfxAdvs = $2;
		}
		| MAXMOBPFXADVINTERVAL NUMBER ';'
		{
			conf.MaxMobPfxAdvInterval = $2;
		}
		| MINMOBPFXADVINTERVAL NUMBER ';'
		{
			conf.MinMobPfxAdvInterval = $2;
		}
		| HAMAXBINDINGLIFE NUMBER ';'
		{
			if ($2 > MAX_BINDING_LIFETIME) {
				uerror("max allowed binding lifetime is %d",
				       MAX_BINDING_LIFETIME);
				return -1;
			}
			conf.HaMaxBindingLife = $2;
		}
		| MNMAXHABINDINGLIFE NUMBER ';'
		{
			if ($2 > MAX_BINDING_LIFETIME) {
				uerror("max allowed binding lifetime is %d",
				       MAX_BINDING_LIFETIME);
				return -1;
			}
			conf.MnMaxHaBindingLife = $2;
		}
		| MNMAXCNBINDINGLIFE NUMBER ';'
		{
			if ($2 > MAX_RR_BINDING_LIFETIME) {
				uerror("max allowed binding lifetime is %d",
				       MAX_RR_BINDING_LIFETIME);
				return -1;
			}
			conf.MnMaxCnBindingLife = $2;
		}
		| INITIALBINDACKTIMEOUTFIRSTREG DECIMAL ';'
		{
			tssetdsec(conf.InitialBindackTimeoutFirstReg_ts, $2);
		}
		| INITIALBINDACKTIMEOUTREREG DECIMAL ';'
		{
			tssetdsec(conf.InitialBindackTimeoutReReg_ts, $2);
		}
/* 		| MNHOMELINK linksub */
		| USEMNHAIPSEC BOOL ';'
		{
			conf.UseMnHaIPsec = $2;
		}
		| KEYMNGMOBCAPABILITY BOOL  ';'
		{
			conf.KeyMngMobCapability = $2;
		}
		| USEMOVEMENTMODULE movemodule ';'
//		| USEPOLICYMODULE policymodule ';'
		| DEFAULTBINDINGACLPOLICY bindaclpolval ';'
		{
			conf.DefaultBindingAclPolicy = $2;
		}
/* 		| BINDINGACLPOLICY bindaclpolicy ';' */
		{
			bae = NULL;
		}
		| USECNBUACK BOOL ';'
		{
			conf.CnBuAck = $2 ? IP6_MH_BU_ACK : 0;
		}
/* 		| IPSECPOLICYSET '{' ipsecpolicyset '}' */
		| MNUSEALLINTERFACES BOOL ';'
/*		{
		conf.MnUseAllInterfaces = $2 ? POL_MN_IF_DEF_PREFERENCE : 0;
		}*/
		| MNDISCARDHAPARAMPROB BOOL ';'
		{
			conf.MnDiscardHaParamProb = $2;
		}
		| OPTIMISTICHANDOFF BOOL ';'
		{
			conf.OptimisticHandoff = $2;
		}
		| PROXYDMMCMD proxydmmcmddef
		| PROXYDMMMAR proxydmmmardef
		;

mip6entity	: DMMCMD { $$ = MIP6_ENTITY_CMD; }
		| DMMMAR { $$ = MIP6_ENTITY_MAR; }
		;

ifacesub	: '{' ifaceopts '}'
		| ';'
		;

ifaceopts	: ifaceopt
		| ifaceopts ifaceopt
		;

ifaceopt	: IFTYPE mip6entity ';'
		{
			ni.mip6_if_entity = $2;
		}
		| MNIFPREFERENCE NUMBER ';'
		{
			ni.mn_if_preference = $2;
		}
		;

ipsecpolicydefs	: ipsecpolicydef
		| ipsecpolicydefs ipsecpolicydef
		;

ipsecpolicydef	: ipsectype ipsecprotos ipsecreqid xfrmaction ';'
		{
			struct list_head *lp;

			if (IN6_IS_ADDR_UNSPECIFIED(&ipsec_ps.ha)) {
				uerror("HomeAgentAddress missing for IPsecPolicy");
				return -1;
			}
			if (list_empty(&ipsec_ps.hoa_list)) {
				uerror("HomeAddress missing for IPsecPolicy");
				return -1;
			}

			list_for_each(lp, &ipsec_ps.hoa_list) {
				struct home_addr_info *hai;
				struct ipsec_policy_entry *e;

				hai = list_entry(lp, struct home_addr_info,
						 list);

				e = malloc(sizeof(*e));
				if (e == NULL) {
					uerror("out of memory");
					return -1;
				}
				memset(e, 0, sizeof(*e));
				e->ha_addr = ipsec_ps.ha;
				e->mn_addr = hai->hoa.addr;
				e->type = $1;
#ifndef XFRM_MSG_MIGRATE
				switch (e->type) {
				case IPSEC_POLICY_TYPE_TUNNELHOMETESTING:
				case IPSEC_POLICY_TYPE_TUNNELMH:
				case IPSEC_POLICY_TYPE_TUNNELPAYLOAD:
					uerror("cannot use IPsec tunnel because it is not built with MIGRATE");
					return -1;
				default:
					break;
				}
#endif
#ifndef MULTIPROTO_MIGRATE
				if ($2 != IPSEC_PROTO_ESP) {
					uerror("only UseESP is allowed");
					return -1;
				}
#endif
				e->ipsec_protos = $2;
				e->reqid_toha = $3[0];
				e->reqid_tomn = $3[1];
				e->action = $4;

				if (ipsec_policy_entry_check(&e->ha_addr,
							     &e->mn_addr,
							     e->type)) {
					uerror("overlapping IPsec policies "
					       "found for "
					       "HA %x:%x:%x:%x:%x:%x:%x:%x "
					       "MN %x:%x:%x:%x:%x:%x:%x:%x "
					       "pair\n",
					       NIP6ADDR(&e->ha_addr),
					       NIP6ADDR(&e->mn_addr));
					return -1;
				}
				list_add_tail(&e->list, &conf.ipsec_policies);
			}
		}
		;

ipsectype	: IPSECPOLICY ipsectypeval { $$ = $2; }
		;

ipsectypeval	: HOMEREGBINDING { $$ = IPSEC_POLICY_TYPE_HOMEREGBINDING; }
		| MH { $$ = IPSEC_POLICY_TYPE_MH; }
		| MOBPFXDISC { $$ = IPSEC_POLICY_TYPE_MOBPFXDISC; }
		| TUNNELHOMETESTING { $$ = IPSEC_POLICY_TYPE_TUNNELHOMETESTING; }
		| TUNNELMH { $$ = IPSEC_POLICY_TYPE_TUNNELMH; }
		| TUNNELPAYLOAD { $$ = IPSEC_POLICY_TYPE_TUNNELPAYLOAD; }
		| ICMP { $$ = IPSEC_POLICY_TYPE_ICMP; }
		| ANY { $$ = IPSEC_POLICY_TYPE_ANY; }
		;

ipsecprotos	:
		{
			uerror("IPsecPolicy must set at least one protocol");
			return -1;
		}
		| ipsecproto { $$ = $1; }
		| ipsecproto ipsecproto { $$ = $1 | $2; }
		| ipsecproto ipsecproto ipsecproto { $$ = $1 | $2 | $3; }
		;

ipsecproto	: USEESP { $$ = IPSEC_PROTO_ESP; }
		| USEAH { $$ = IPSEC_PROTO_AH; }
		| USEIPCOMP { $$ = IPSEC_PROTO_IPCOMP; }
		;

ipsecreqid	: { $$[0] = $$[1] = 0; }
		| NUMBER { $$[0] = $$[1] = $1; }
		| NUMBER NUMBER { $$[0] = $1; $$[1] = $2; }
		;

xfrmaction	: { $$ = XFRM_POLICY_ALLOW; }
 		| BOOL { $$ = $1 ? XFRM_POLICY_ALLOW : XFRM_POLICY_BLOCK; }
		;

mnropolicyaddr	: { $$ = in6addr_any; }
		| ADDR { $$ = $1; }
		;

movemodule	: INTERNAL
		{
			conf.MoveModulePath = NULL;
		}
		| QSTRING
		{
			conf.MoveModulePath = NULL;
		}
		;

/*policymodule	: QSTRING
		{
			if (pmgr_init($1, &conf.pmgr) < 0) {
				uerror("error loading shared object %s", $1);
				return -1;
			}
		}
		;*/

bindaclpolval	: BOOL
		{
			if ($1)
				$$ = IP6_MH_BAS_ACCEPTED;
			else
				$$ = IP6_MH_BAS_PROHIBIT;
		}
		| NUMBER { $$ = $1; }
		;

/*bindaclpolicy	: ADDR bindaclpolval
		{
			bae = malloc(sizeof(struct policy_bind_acl_entry));
			if (bae == NULL) {
				uerror("out of memory");
				return -1;
			}
			memset(bae, 0, sizeof(struct policy_bind_acl_entry));
			bae->hoa = $1;
			bae->plen = 128;
			bae->bind_policy = $2;
			list_add_tail(&bae->list, &conf.bind_acl);
		}
		;*/

prefixlen	: NUMBER
		{
			if ($1 > 128) {
				uerror("invalid prefix length %d", $1);
				return -1;
			}
			$$ = $1;
		}
		;


proxydmmcmddef	: QSTRING proxydmmcmdsub
		{
			conf.HomeNetworkPrefix = in6addr_any;
			conf.OurAddress        = in6addr_loopback;
		}
		;

proxydmmcmdsub	: '{' proxydmmcmdopts '}'
		| ';'
		;

proxydmmcmdopts	: proxydmmcmdopt
		| proxydmmcmdopts proxydmmcmdopt
		;

proxydmmcmdopt	: CMDADDRESS ADDR ';'
		{
			memcpy(&conf.CmdAddress, &$2, sizeof(struct in6_addr));
		}
		| CMDDMMNETWORKDEVICE QSTRING ';'
		{
			conf.CmdDmmNetworkDevice = $2;
		}
                | CMDCORENETWORKADDRESS ADDR ';'
		{
			memcpy(&conf.CmdCoreNetworkAddress, &$2, sizeof(struct in6_addr));
		}
		| CMDCORENETWORKDEVICE QSTRING ';'
		{
			conf.CmdCoreNetworkDevice = $2;
		}
		| RFC5213TIMESTAMPBASEDAPPROACHINUSE BOOL ';'
		{
			conf.RFC5213TimestampBasedApproachInUse = $2;
		}
		| RFC5213MOBILENODEGENERATEDTIMESTAMPINUSE BOOL ';'
		{
			conf.RFC5213MobileNodeGeneratedTimestampInUse = $2;
		}
		| RFC5213FIXEDMARLINKLOCALADDRESSONALLACCESSLINKS ADDR ';'
		{
			memcpy(&conf.RFC5213FixedMARLinkLocalAddressOnAllAccessLinks, &$2, sizeof(struct in6_addr));
		}
		| RFC5213FIXEDMARLINKLAYERADDRESSONALLACCESSLINKS  MACADDR ';'
		{
			memcpy(&conf.RFC5213FixedMARLinkLayerAddressOnAllAccessLinks, &$2, sizeof(struct in6_addr));
		}
		| RFC5213MINDELAYBEFOREBCEDELETE NUMBER ';'
		{
			struct timespec lifetime;
			lifetime.tv_sec = $2/1000;
            lifetime.tv_nsec = ($2 % 1000)*1000000;
			conf.RFC5213MinDelayBeforeBCEDelete = lifetime;
		}
		| RFC5213MAXDELAYBEFORENEWBCEASSIGN NUMBER ';'
		{
			struct timespec lifetime;
            lifetime.tv_sec = $2/1000;
            lifetime.tv_nsec = ($2 % 1000)*1000000;
			conf.RFC5213MaxDelayBeforeNewBCEAssign = lifetime;		
		}
		| RFC5213TIMESTAMPVALIDITYWINDOW NUMBER ';'
		{
			struct timespec lifetime;
            lifetime.tv_sec = $2/1000;
            lifetime.tv_nsec = ($2 % 1000)*1000000;
			conf.RFC5213TimestampValidityWindow = lifetime;
		}
		| OURADDRESS ADDR ';'
		{
			memcpy(&conf.OurAddress, &$2, sizeof(struct in6_addr));
		}
		| HOMENETWORKPREFIX ADDR ';'
		{
			memcpy(&conf.HomeNetworkPrefix, &$2, sizeof(struct in6_addr));
		}
		| TUNNELINGENABLED BOOL ';'
		{
			conf.TunnelingEnabled = $2;
		}
		| DYNAMICTUNNELINGENABLED BOOL ';'
		{
			conf.DynamicTunnelingEnabled = $2;
		}
		| PBULIFETIME NUMBER ';'
		{
			struct timespec lifetime;
            lifetime.tv_sec = $2/1000;
            lifetime.tv_nsec = ($2 % 1000)*1000000;
			conf.PBULifeTime = lifetime;
		}
		| PBALIFETIME NUMBER ';'
		{
			struct timespec lifetime;
            lifetime.tv_sec = $2/1000;
            lifetime.tv_nsec = ($2 % 1000)*1000000;
			conf.PBALifeTime = lifetime;
		}
		| RETRANSMISSIONTIMEOUT NUMBER ';'
		{
			struct timespec lifetime;
            lifetime.tv_sec = $2/1000;
            lifetime.tv_nsec = ($2 % 1000)*1000000;
			conf.RetransmissionTimeOut = lifetime;
		}
		| MAXMESSAGERETRANSMISSIONS NUMBER ';'
		{
			conf.MaxMessageRetransmissions = $2;
		}
		;

proxydmmmardef	: QSTRING proxydmmmarsub
		{
			conf.HomeNetworkPrefix = in6addr_any;
			conf.OurAddress        = in6addr_loopback;
		}
		;

proxydmmmarsub	: '{' proxydmmmaropts '}'
		| ';'
		;

proxydmmmaropts	: proxydmmmaropt
		| proxydmmmaropts proxydmmmaropt
		;

proxydmmmaropt	: CMDADDRESS ADDR ';'
		{
			memcpy(&conf.CmdAddress, &$2, sizeof(struct in6_addr));
		}
		| RFC5213TIMESTAMPBASEDAPPROACHINUSE BOOL ';'
		{
			conf.RFC5213TimestampBasedApproachInUse = $2;
		}
		| RFC5213MOBILENODEGENERATEDTIMESTAMPINUSE BOOL ';'
		{
			conf.RFC5213MobileNodeGeneratedTimestampInUse = $2;
		}
		| RFC5213FIXEDMARLINKLOCALADDRESSONALLACCESSLINKS ADDR ';'
		{
			memcpy(&conf.RFC5213FixedMARLinkLocalAddressOnAllAccessLinks, &$2, sizeof(struct in6_addr));
		}
		| RFC5213FIXEDMARLINKLAYERADDRESSONALLACCESSLINKS MACADDR ';'
		{
			memcpy(&conf.RFC5213FixedMARLinkLayerAddressOnAllAccessLinks, &$2, sizeof(struct in6_addr));
		}
		| OURADDRESS ADDR ';'
		{
			memcpy(&conf.OurAddress, &$2, sizeof(struct in6_addr));
		}
		| MARADDRESSINGRESS ADDR ';'
		{
			memcpy(&conf.MarAddressIngress, &$2, sizeof(struct in6_addr));
		}
		| MARADDRESSEGRESS ADDR ';'
		{
			memcpy(&conf.MarAddressEgress, &$2, sizeof(struct in6_addr));
		}
		| MARDEVICEINGRESS QSTRING ';'
		{
			conf.MarDeviceIngress = $2;		
		}
		| MARDEVICEEGRESS QSTRING ';'
		{
			conf.MarDeviceEgress = $2;
		}
		| HOMENETWORKPREFIX ADDR ';'
		{
			memcpy(&conf.HomeNetworkPrefix, &$2, sizeof(struct in6_addr));
		}
		| TUNNELINGENABLED BOOL ';'
		{
			conf.TunnelingEnabled = $2;
		}
		| DYNAMICTUNNELINGENABLED BOOL ';'
		{
			conf.DynamicTunnelingEnabled = $2;
		}
		| PBULIFETIME NUMBER ';'
		{
			struct timespec lifetime;
            lifetime.tv_sec = $2/1000;
            lifetime.tv_nsec = ($2 % 1000)*1000000;
			conf.PBULifeTime = lifetime;
		}
		| PBALIFETIME NUMBER ';'
		{
			struct timespec lifetime;
            lifetime.tv_sec = $2/1000;
            lifetime.tv_nsec = ($2 % 1000)*1000000;
			conf.PBALifeTime = lifetime;
		}
		| RETRANSMISSIONTIMEOUT NUMBER ';'
		{
			struct timespec lifetime;
            lifetime.tv_sec = $2/1000;
            lifetime.tv_nsec = ($2 % 1000)*1000000;
			conf.RetransmissionTimeOut = lifetime;
		}
		| MAXMESSAGERETRANSMISSIONS NUMBER ';'
		{
			conf.MaxMessageRetransmissions = $2;
		}
/*		| RADIUSPASSWORD QSTRING ';'
		{
			conf.RadiusPassword = $2;
		}
		| RADIUSCLIENTCONFIGFILE QSTRING ';'
		{
			conf.RadiusClientConfigFile = $2;
		}
		| PCAPSYSLOGASSOCIATIONGREPSTRING QSTRING ';'
		{
			conf.PcapSyslogAssociationGrepString = $2;
		}
		| PCAPSYSLOGDEASSOCIATIONGREPSTRING QSTRING ';'
		{
			conf.PcapSyslogDeAssociationGrepString = $2;
		}*/
		;

%%
