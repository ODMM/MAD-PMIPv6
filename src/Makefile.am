## $Id: Makefile.am 1.55 06/05/07 17:37:11+03:00 anttit@tcs.hut.fi $
## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -g -Wpointer-arith -Wreturn-type -pedantic -std=gnu99
AM_CPPFLAGS = -isystem ../include -I../src -I./dmm -I/usr/local/include
AM_YFLAGS = -d

defpath.h: Makefile.am
	echo "#define DEFAULT_CONFIG_FILE \"$(sysconfdir)/mad-pmip6d.conf\"" > $@

pmgr.c: mk-pmgr_c.sh pmgr.c.in policy.h
	sh mk-pmgr_c.sh

pmgr.h: mk-pmgr_h.sh pmgr.h.in policy.h
	sh mk-pmgr_h.sh

BUILT_SOURCES = defpath.h
## pmgr.c pmgr.h
EXTRA_DIST = mk-pmgr_c.sh mk-pmgr_h.sh pmgr.c.in pmgr.h.in

sbin_PROGRAMS = mad-pmip6d

mad_pmip6d_SOURCES = defpath.h \
	main.c vars.c mipv6.h \
	conf.c conf.h \
	gram.y scan.l \
	tqueue.c tqueue.h \
	icmp6.c icmp6.h \
	list.h util.h \
	hash.c hash.h \
	debug.c debug.h \
	ndisc.c ndisc.h \
	mh.c mh.h \
	tunnelctl.c tunnelctl.h \
	rtnl.c rtnl.h \
	proc_sys.c proc_sys.h \
        ./dmm/binding_cache.c \
        ./dmm/binding_cache.h \
        ./dmm/dmm_consts.h \
        ./dmm/dmm_fsm.c \
        ./dmm/dmm_fsm.h \
        ./dmm/handlers.c \
        ./dmm/handlers.h \
        ./dmm/hnp_cache.c \
        ./dmm/hnp_cache.h \
        ./dmm/dmm_init.c \
        ./dmm/dmm_init.h \
        ./dmm/cmd_proc.c \
        ./dmm/cmd_proc.h \
        ./dmm/mar_proc.c \
        ./dmm/mar_proc.h \
        ./dmm/dmm_msgs.c \
        ./dmm/dmm_msgs.h \
        ./dmm/hostapd.c \
        ./dmm/hostapd.h \
        ./dmm/dmm_types.h

#  cn.c cn.h \ ./dmm/dmm.h \ xfrm.c xfrm.h \ mn.c mn.h \ dhaad_ha.c dhaad_ha.h \ dhaad_mn.c dhaad_mn.h \ prefix.c prefix.h \ ha.c ha.h \
# 	keygen.c keygen.h \
# 	movement.c movement.h \
# # 	policy.c policy.h \
# 	bul.c bul.h \
# 	bcache.c bcache.h \
# 	retrout.c retrout.h \
# 	mpdisc_ha.c mpdisc_ha.h \
# 	mpdisc_mn.c mpdisc_mn.h \
# ipsec.c ipsec.h \
# pmgr.c pmgr.h 
if ENABLE_VT
mad_pmip6d_SOURCES += vt.c vt.h
endif

if BUILTIN_CRYPTO
mad_pmip6d_SOURCES += crypto.c crypto.h
endif

mad_pmip6d_LDADD =  -lpcap
## mad_pmip6d_LDADD =

if MISSING
mad_pmip6d_LDADD += -L../libmissing -lmissing
endif

if LOCAL_NETLINK
mad_pmip6d_LDADD += -L../libnetlink -lnetlink
endif

noinst_HEADERS = ../include/netinet/icmp6.h \
		 ../include/netinet/in.h \
		 ../include/netinet/ip6.h \
		 ../include/netinet/ip6mh.h