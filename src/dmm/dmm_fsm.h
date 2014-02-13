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

#ifndef __DMM_FSM_H__
#    define __DMM_FSM_H__
//-----------------------------------------------------------------------------
#    ifdef DMM_FSM_C
#        define private_dmm_fsm(x) x
#        define protected_dmm_fsm(x) x
#        define public_dmm_fsm(x) x
#    else
#        ifdef DMM
#            define private_dmm_fsm(x)
#            define protected_dmm_fsm(x) extern x
#            define public_dmm_fsm(x) extern x
#        else
#            define private_dmm_fsm(x)
#            define protected_dmm_fsm(x)
#            define public_dmm_fsm(x) extern x
#        endif
#    endif
//-----------------------------------------------------------------------------
#    include <pthread.h>
#    include "dmm_msgs.h"
//-----------------------------------------------------------------------------
protected_dmm_fsm(int mar_init_fsm(void);)

protected_dmm_fsm(int mar_fsm(msg_info_t *info);)

protected_dmm_fsm(int cmd_fsm(msg_info_t *info);)

private_dmm_fsm(pthread_rwlock_t fsm_lock;)

#endif