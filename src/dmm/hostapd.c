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
#define DMM_PCAP_C
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
//---------------------------------------------------------------------------------------------------------------------
#include "dmm_fsm.h"
#include "hnp_cache.h"
#include "hostapd.h"
#include "dmm_msgs.h"
#include "mar_proc.h"
//---------------------------------------------------------------------------------------------------------------------
#ifdef ENABLE_VT
#    include "vt.h"
#endif
#include "debug.h"
#include "conf.h"

//#include <stdlib.h>

/*
 * Following code serves to listen to hostapd customized to send association events. This features is enabled with the "-W" option
 */

int rcv_fd;
int pmip_socket_fd;
pthread_t hostapd_listener;

void hostapd_associate(uint8_t *mn_ll_id)
{
	printf("=====================================\n");
	dbg("hostapd_associate\n");
	msg_info_t msg;
	memset(&msg, 0, sizeof(msg_info_t));
	struct in6_addr temp = in6addr_any;
	for (int i = 0; i < 6; i++) {
		temp.s6_addr[i+10] = mn_ll_id[i];
	}
	msg.mn_iid = EUI48_to_EUI64(temp);
	struct ingress_if_info ingress_if;
	msg.iif = ingress_if.eif_id;
	msg.msg_event = hasRS;
	mar_fsm(&msg);
}

void hostapd_listen(void)
{
	struct sockaddr_un remote;
	int rcv;
	int s;
	int done;
	struct hostapd_mar_msg *rcv_msg;
	char rcv_buff[sizeof(struct hostapd_mar_msg)];
	for (;;){
		dbg("Waiting for hostapd connection...\n");
		s = sizeof(remote);
		if ((rcv_fd = accept(pmip_socket_fd, (struct sockaddr *)&remote, &s)) == -1) {
			dbg("Cannot accept hostapd connection");
			pthread_exit(0);
		}
		dbg("Connected to hostapd socket\n");
		done = 0;
		do {
			memset(rcv_buff, 0, sizeof(struct hostapd_mar_msg));
			rcv = recv(rcv_fd, rcv_buff, sizeof(rcv_buff), 0);
			if (rcv <= 0) {
				dbg("Cannot receive hostapd message\n");
				done = 1;
			} else {
				rcv_msg = (struct hostapd_mar_msg *)rcv_buff;
				if (rcv_msg->command == CONNECTED) {
					timers_log_start();
					hostapd_associate(rcv_msg->mac_addr);
				} else  dbg("Station disconnected: %02x:%02x:%02x:%02x:%02x:%02x\n",
							rcv_msg->mac_addr[0],rcv_msg->mac_addr[1],
 							rcv_msg->mac_addr[2],rcv_msg->mac_addr[3],
 							rcv_msg->mac_addr[4],rcv_msg->mac_addr[5]);
			}
		} while (!done);
		close(rcv_fd);
	}
}

int hostapd_listener_init(void)
{
	int len;
	struct sockaddr_un local;
	if ((pmip_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		dbg("Cannot initialize hostapd socket\n");
		return -1;
	}
	local.sun_family = AF_UNIX;
	strcpy(local.sun_path, HOSTAPD_SOCK_PATH);
	unlink(local.sun_path);
	len = strlen(local.sun_path) + sizeof(local.sun_family);
	if (bind(pmip_socket_fd, (struct sockaddr *)&local, len) == -1) {
		dbg("Cannot bind hostapd socket\n");
		return -1;
	}
	if (listen(pmip_socket_fd, 1) == -1) {
		dbg("Cannot listen to socket\n");
		return -1;
	}
	pthread_create(&hostapd_listener, NULL, hostapd_listen, NULL);
	return 0;
}

void hostapd_cleanup(void)
{		
	close(rcv_fd);
	close(pmip_socket_fd);
	pthread_cancel(hostapd_listener);
	pthread_join(hostapd_listener, NULL);
}