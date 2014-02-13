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

#ifndef __DMM_CMD_PROC_H__
#    define __DMM_CMD_PROC_H__
//-----------------------------------------------------------------------------
#	ifdef DMM_CMD_PROC_C
#		define private_dmm_cmd_proc(x) x
#		define protected_dmm_cmd_proc(x) x
#		define public_dmm_cmd_proc(x) x
#	else
#		ifdef DMM
#			define private_dmm_cmd_proc(x)
#			define protected_dmm_cmd_proc(x) extern x
#			define public_dmm_cmd_proc(x) extern x
#		else
#			define private_dmm_cmd_proc(x)
#			define protected_dmm_cmd_proc(x)
#			define public_dmm_cmd_proc(x) extern x
#		endif
#	endif
//-----------------------------------------------------------------------------
#	include <netinet/ip6.h>
#	include "binding_cache.h"
#	include "dmm_msgs.h"
//-PROTOTYPES----------------------------------------------------------------------------

protected_dmm_cmd_proc(int cmd_reg(dmm_entry * bce);)

protected_dmm_cmd_proc(int cmd_reg_dmm(dmm_entry * bce);)

protected_dmm_cmd_proc(int cmd_end_reg_dmm(dmm_entry * bce, struct in6_addr * prefix);)

protected_dmm_cmd_proc(int cmd_dereg(dmm_entry * bce, msg_info_t * info, int propagate);)

protected_dmm_cmd_proc(void cmd_refresh_binding_entry(dmm_entry * bce, msg_info_t * info);)

protected_dmm_cmd_proc(int cmd_update_binding_entry(dmm_entry * bce, msg_info_t * info);)

protected_dmm_cmd_proc(int cmd_create_binding_entry(dmm_entry * bce, msg_info_t * info);)
#endif