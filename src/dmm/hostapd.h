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

#ifndef __DMM_PCAP_H__
#    define __DMM_PCAP_H__
//-----------------------------------------------------------------------------
#    ifdef DMM_PCAP_C
#        define private_dmm_hostapd(x) x
#        define protected_dmm_hostapd(x) x
#        define public_dmm_hostapd(x) x
#    else
#        ifdef DMM
#            define private_dmm_hostapd(x)
#            define protected_dmm_hostapd(x) extern x
#            define public_dmm_hostapd(x) extern x
#        else
#            define private_dmm_hostapd(x)
#            define protected_dmm_hostapd(x)
#            define public_dmm_hostapd(x) extern x
#        endif
#    endif
//-----------------------------------------------------------------------------
#    include <pcap.h>
#    include <netinet/ip6.h>
//-----------------------------------------------------------------------------
#    define PCAPMAXBYTES2CAPTURE 65535
#    define PCAPTIMEDELAYKERNEL2USERSPACE 1000
#    define PCAP_CAPTURE_SYSLOG_MESSAGE_FRAME_OFFSET   42

// IPC with hostapd
#define HOSTAPD_SOCK_PATH "/tmp/pmip_socket"
#define CONNECTED 0
#define DISCONNECTED 1
/*#define DMM_IPC_OFF 0
#define DMM_IPC_ON 1
*/
struct hostapd_mar_msg {
	uint8_t mac_addr[6];
	int 	command;
};

//-PROTOTYPES----------------------------------------------------------------------------

private_dmm_hostapd(void hostapd_associate(uint8_t *mn_ll_id);)

private_dmm_hostapd(void hostapd_listen(void);)

protected_dmm_hostapd(int hostapd_listener_init(void);)

protected_dmm_hostapd(void hostapd_cleanup(void);)

#endif