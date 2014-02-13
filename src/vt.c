/*
 * Copyright (C)2004,2005 USAGI/WIDE Project
 * Copyright (C)2005 Go-Core Project
 * Copyright (C)2005,2006 Helsinki University of Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * Authors:
 *	Noriaki TAKAMIYA @USAGI
 *	Masahide NAKAMURA @USAGI
 *	Ville Nuorvala @HUT
 */

/*
 * VT server performs select(2) and only one client access is allowed.
 * To be accept multiple connect, fix "vt_connect_handle".
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pthread.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <netdb.h>
#include <string.h>
#include <netinet/ip6mh.h>

#include "list.h"
#include "util.h"
#include "debug.h"
#include "conf.h"
#include "vt.h"
#include "binding_cache.h"
#include "dmm_types.h"

#define VT_PKT_BUFLEN		(8192)
#define VT_REPLY_BUFLEN		(LINE_MAX)
#define VT_SERVER_BACKLOG	(1)
#define VT_CMD_PROMPT		("mad-pmip6d> ")
#define VT_CMD_HELP_STR		("help")
#define VT_CMD_HELP_LINE_MAX	(60)

struct vt_server_entry {
	struct list_head list;
	int vse_sock;
	union {
		struct sockaddr sa;
		struct sockaddr_storage ss;
	} vse_sockaddr;
	socklen_t vse_sockaddrlen;

#define vse_sa		vse_sockaddr.sa
#define vse_salen	vse_sockaddrlen
};

static pthread_rwlock_t vt_lock;
static pthread_t vt_listener;

static struct vt_cmd_entry vt_cmd_root;

static LIST_HEAD(vt_server_list);
static struct vt_handle *vt_connect_handle = NULL;

static int vt_server_fini(void);
static int vt_connect_close(struct vt_handle *vh);

/* Find a handle which is able to be modified */
static struct vt_handle *vt_handle_get(const struct vt_handle *vh)
{
	return vt_connect_handle;
}

static int vt_handle_full(void)
{
	if (vt_connect_handle != NULL)
		return 1;
	else
		return 0;
}

static int vt_handle_add(struct vt_handle *vh)
{
	if (vt_connect_handle != NULL) {
		return -EINVAL;
	}
	vt_connect_handle = vh;
	return 0;
}

#define VTDECOR_B_START vh->vh_opt.fancy == VT_BOOL_TRUE ? "\033[1m" : ""
#define VTDECOR_BU_START vh->vh_opt.fancy == VT_BOOL_TRUE ? "\033[1;4m" : ""
#define VTDECOR_END vh->vh_opt.fancy == VT_BOOL_TRUE ? "\033[0m" : ""

ssize_t fprintf_decor(int decor, const struct vt_handle *vh,
		      const char *fmt, ...)
{
	char buf[VT_REPLY_BUFLEN];
	va_list ap;

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);

	return fprintf(vh->vh_stream, "%s%s%s",
		       decor == VTDECOR_B ? VTDECOR_B_START :VTDECOR_BU_START,
		       buf, VTDECOR_END);
}

static const char *yes = "yes";
static const char *no = "no";

static const char *bool_str(vt_bool_t b)
{
	return b == VT_BOOL_TRUE ? yes : no;
}

static int vt_cmd_sys_fancy_off(const struct vt_handle *vh, const char *str)
{
	struct vt_handle *sysvh = vt_handle_get(vh);
	sysvh->vh_opt.fancy = VT_BOOL_FALSE;
	fprintf(vh->vh_stream, "%s\n", bool_str(sysvh->vh_opt.fancy));
	return 0;
}

static int vt_cmd_sys_fancy_on(const struct vt_handle *vh, const char *str)
{
	struct vt_handle *sysvh = vt_handle_get(vh);
	sysvh->vh_opt.fancy = VT_BOOL_TRUE;
	fprintf(vh->vh_stream, "%s\n", bool_str(sysvh->vh_opt.fancy));
	return 0;
}

static int vt_cmd_sys_fancy(const struct vt_handle *vh, const char *str)
{
	struct vt_handle *sysvh = vt_handle_get(vh);
	fprintf(vh->vh_stream, "%s\n", bool_str(sysvh->vh_opt.fancy));
	return 0;
}

static int vt_cmd_sys_verbose_off(const struct vt_handle *vh, const char *str)
{
	struct vt_handle *sysvh = vt_handle_get(vh);
	sysvh->vh_opt.verbose = VT_BOOL_FALSE;
	fprintf(vh->vh_stream, "%s\n", bool_str(sysvh->vh_opt.verbose));
	return 0;
}

static int vt_cmd_sys_verbose_on(const struct vt_handle *vh, const char *str)
{
	struct vt_handle *sysvh = vt_handle_get(vh);
	sysvh->vh_opt.verbose = VT_BOOL_TRUE;
	fprintf(vh->vh_stream, "%s\n", bool_str(sysvh->vh_opt.verbose));
	return 0;
}

static int vt_cmd_sys_verbose(const struct vt_handle *vh, const char *str)
{
	struct vt_handle *sysvh = vt_handle_get(vh);
	fprintf(vh->vh_stream, "%s\n", bool_str(sysvh->vh_opt.verbose));
	return 0;
}

static int vt_cmd_sys_prompt_off(const struct vt_handle *vh, const char *str)
{
	struct vt_handle *sysvh = vt_handle_get(vh);
	sysvh->vh_opt.prompt = VT_BOOL_FALSE;
	fprintf(vh->vh_stream, "%s\n", bool_str(sysvh->vh_opt.prompt));
	return 0;
}

static int vt_cmd_sys_prompt_on(const struct vt_handle *vh, const char *str)
{
	struct vt_handle *sysvh = vt_handle_get(vh);
	sysvh->vh_opt.prompt = VT_BOOL_TRUE;
	fprintf(vh->vh_stream, "%s\n", bool_str(sysvh->vh_opt.prompt));
	return 0;
}

static int vt_cmd_sys_prompt(const struct vt_handle *vh, const char *str)
{
	struct vt_handle *sysvh = vt_handle_get(vh);
	fprintf(vh->vh_stream, "%s\n", bool_str(sysvh->vh_opt.prompt));
	return 0;
}

static int vt_cmd_sys_quit(const struct vt_handle *vh, const char *str)
{
	struct vt_handle *sysvh = vt_handle_get(vh);

	if (strlen(str) > 0) {
		int ret = fprintf(vh->vh_stream, "unknown args\n");
		if (ret < 0)
			return ret;
		return 0;
	}

	vt_connect_close(sysvh);
	return 0;
}

static struct vt_cmd_entry vt_cmd_quit = {
	.cmd = "quit",
	.cmd_alias = "exit",
	.parser = vt_cmd_sys_quit,
};

static struct vt_cmd_entry vt_cmd_prompt = {
	.cmd = "prompt",
	.parser = vt_cmd_sys_prompt,
};

static struct vt_cmd_entry vt_cmd_prompt_on = {
	.cmd = "yes",
	.cmd_alias = "y",
	.parser = vt_cmd_sys_prompt_on,
};

static struct vt_cmd_entry vt_cmd_prompt_off = {
	.cmd = "no",
	.cmd_alias = "n",
	.parser = vt_cmd_sys_prompt_off,
};

static struct vt_cmd_entry vt_cmd_verbose = {
	.cmd = "verbose",
	.parser = vt_cmd_sys_verbose,
};

static struct vt_cmd_entry vt_cmd_verbose_on = {
	.cmd = "yes",
	.cmd_alias = "y",
	.parser = vt_cmd_sys_verbose_on,
};

static struct vt_cmd_entry vt_cmd_verbose_off = {
	.cmd = "no",
	.cmd_alias = "n",
	.parser = vt_cmd_sys_verbose_off,
};

static struct vt_cmd_entry vt_cmd_fancy = {
	.cmd = "fancy",
	.parser = vt_cmd_sys_fancy,
};

static struct vt_cmd_entry vt_cmd_fancy_on = {
	.cmd = "yes",
	.cmd_alias = "y",
	.parser = vt_cmd_sys_fancy_on,
};

static struct vt_cmd_entry vt_cmd_fancy_off = {
	.cmd = "no",
	.cmd_alias = "n",
	.parser = vt_cmd_sys_fancy_off,
};

static int vt_cmd_sys_init(void)
{
	int ret;

	INIT_LIST_HEAD(&vt_cmd_root.list);
	vt_cmd_root.parent = NULL;
	INIT_LIST_HEAD(&vt_cmd_root.child_list);

	ret = vt_cmd_add_root(&vt_cmd_quit);
	if (ret < 0)
		return ret;

	ret = vt_cmd_add_root(&vt_cmd_prompt);
	if (ret < 0)
		return ret;
	ret = vt_cmd_add(&vt_cmd_prompt, &vt_cmd_prompt_on);
	if (ret < 0)
		return ret;
	ret = vt_cmd_add(&vt_cmd_prompt, &vt_cmd_prompt_off);
	if (ret < 0)
		return ret;

	ret = vt_cmd_add_root(&vt_cmd_verbose);
	if (ret < 0)
		return ret;
	ret = vt_cmd_add(&vt_cmd_verbose, &vt_cmd_verbose_on);
	if (ret < 0)
		return ret;
	ret = vt_cmd_add(&vt_cmd_verbose, &vt_cmd_verbose_off);
	if (ret < 0)
		return ret;

	ret = vt_cmd_add_root(&vt_cmd_fancy);
	if (ret < 0)
		return ret;
	ret = vt_cmd_add(&vt_cmd_fancy, &vt_cmd_fancy_on);
	if (ret < 0)
		return ret;
	ret = vt_cmd_add(&vt_cmd_fancy, &vt_cmd_fancy_off);
	if (ret < 0)
		return ret;

	return 0;
}

static int vt_cmd_dump_candidates(const struct vt_handle *vh,
				  const struct vt_cmd_entry *ent, int line_max)
{
	const char sep[] = " ";
	const int seplen = strlen(sep);
	int llen = 0;
	int n = 0;
	int ret;
	struct list_head *lp;

	list_for_each (lp, &ent->child_list) {
		const struct vt_cmd_entry *e;
		int cmdlen;

		e = list_entry(lp, const struct vt_cmd_entry, list);

		if (!e->cmd)
			continue;

		cmdlen = strlen(e->cmd);

		/* check whether new-line is needed or not */
		if (n > 0) {
			if (llen + seplen + cmdlen >= line_max) {
				ret = fprintf(vh->vh_stream, "\n");
				llen = 0;
			} else {
				/* add a separator */
				ret = fprintf(vh->vh_stream, sep);
				llen += seplen;
			}
			if (ret < 0)
				return ret;
		}
		llen += cmdlen;

		ret = fprintf(vh->vh_stream, e->cmd);
		if (ret < 0)
			return ret;

		n++;
	}

	ret = fprintf(vh->vh_stream, "\n");
	if (ret < 0)
		return ret;

	return 0;
}

//////////////////////////////////////////////////
//Defined for PMIP///////////////////////////////
/////////////////////////////////////////////////
struct dmm_cache_vt_arg {
    const struct vt_handle *vh;
};

static int dmm_cache_vt_dump(void *data, void *arg)
{
    dmm_entry *bce = (dmm_entry *)data;
    struct dmm_cache_vt_arg *bva = (struct dmm_cache_vt_arg *)arg;
    const struct vt_handle *vh = bva->vh;
    struct timespec ts_now;
    tsclear(ts_now);

	int local = IN6_ARE_ADDR_EQUAL(&bce->mn_serv_mar_addr, &bce->our_addr);
    fprintf_bl(vh, "MN-ID %x:%x:%x:%x:%x:%x:%x:%x",
           NIP6ADDR(&bce->mn_suffix));
    fprintf_b(vh, " Status: %s - %s",
          (bce->type == BCE_DMM) ? "Registered for DMM" :
          (bce->type == BCE_TEMP) ? "TEMP" :
          "(unknown)", local ? "On link" : "Remotely");
    fprintf(vh->vh_stream, "\n");

	char buf[IF_NAMESIZE + 1];
        char *dev;

	if (local) {
		fprintf(vh->vh_stream, "Local HNP %x:%x:%x:%x:%x:%x:%x:%x",
		NIP6ADDR(&bce->mn_prefix));
		if (bce->link) {
			fprintf(vh->vh_stream, " on link ");
			dev = if_indextoname(bce->link, buf);
			if (!dev || strlen(dev) == 0)
				fprintf(vh->vh_stream, "(%d)", bce->link);
			else
				fprintf(vh->vh_stream, "%s", dev);
		}
		fprintf(vh->vh_stream, "\n");
		if (bce->mar_list == NULL) 
			fprintf(vh->vh_stream, "No other (remote) registrations");
		else {
			fprintf(vh->vh_stream, "Other registrations:");
			struct mar_list_entry *tmp = bce->mar_list;
			while (tmp) {
				fprintf(vh->vh_stream, "\nRemote NHP %x:%x:%x:%x:%x:%x:%x:%x", NIP6ADDR(&tmp->mn_prefix));
				fprintf(vh->vh_stream, " Anchor MAR %x:%x:%x:%x:%x:%x:%x:%x", NIP6ADDR(&tmp->mar_address));
				fprintf(vh->vh_stream, " on tunnel %d - ", tmp->tunnel_id);
				dev = if_indextoname(tmp->tunnel_id, buf);
				if (!dev || strlen(dev) == 0)
					fprintf(vh->vh_stream, "(%d)", tmp->tunnel_id);
				else
					fprintf(vh->vh_stream, "%s", dev);
				tmp=tmp->next;
			}
		}
	} else {
		fprintf(vh->vh_stream, "Local HNP %x:%x:%x:%x:%x:%x:%x:%x",
		NIP6ADDR(&bce->mn_prefix));
		if (bce->tunnel) {
			fprintf(vh->vh_stream, " on tunnel %d - ",bce->tunnel);
			dev = if_indextoname(bce->tunnel, buf);
			if (!dev || strlen(dev) == 0)
				fprintf(vh->vh_stream, "(%d)", bce->tunnel);
			else
				fprintf(vh->vh_stream, "%s", dev);
		}
		fprintf(vh->vh_stream, " Serving MAR %x:%x:%x:%x:%x:%x:%x:%x",
		NIP6ADDR(&bce->mn_serv_mar_addr));
	}
	fprintf(vh->vh_stream, "\n");

    if (vh->vh_opt.verbose == VT_BOOL_TRUE){
	fprintf(vh->vh_stream, "MAR's address %x:%x:%x:%x:%x:%x:%x:%x",
		NIP6ADDR(&bce->our_addr));
	fprintf(vh->vh_stream, " CMD's address %x:%x:%x:%x:%x:%x:%x:%x",
		NIP6ADDR(&bce->mn_serv_cmd_addr));
	fprintf(vh->vh_stream, "\n");
    }

    fprintf(vh->vh_stream, " lifetime ");
    if (clock_gettime(CLOCK_REALTIME, &ts_now) != 0)
        fprintf(vh->vh_stream, "(error)");
    else {
        if (tsafter(ts_now, bce->add_time))
            fprintf(vh->vh_stream, "(broken)");
        else {
            struct timespec ts;

            tssub(ts_now, bce->add_time, ts);
            /* "ts" is now time how log it alives */
            if (tsafter(bce->lifetime, ts)) {
                tssub(ts, bce->lifetime, ts);
                fprintf(vh->vh_stream, "-%ld", ts.tv_sec);
            } else {
                tssub(bce->lifetime, ts, ts);
                fprintf(vh->vh_stream, "%ld", ts.tv_sec);
            }
        }
    }
    fprintf(vh->vh_stream, " / %ld", bce->lifetime.tv_sec);
    fprintf(vh->vh_stream, " seq %u", bce->seqno_out);
    fprintf(vh->vh_stream, "\n");

    return 0;
}

static int dmm_cache_vt_cmd_pbc(const struct vt_handle *vh, const char *str)
{
    struct dmm_cache_vt_arg bva;
    bva.vh = vh;

    if (strlen(str) > 0) {
        fprintf(vh->vh_stream, "unknown args\n");
        return 0;
    }

    dmm_cache_iterate(dmm_cache_vt_dump, &bva);
    return 0;
}

static struct vt_cmd_entry vt_cmd_pbc = {
    .cmd = "dmm",
    .parser = dmm_cache_vt_cmd_pbc,
};
////////////////////////////////////////////////////////////

static int vt_str_to_uint32(const struct vt_handle *vh, const char *str,
			    uint32_t *val)
{
	long int v;
	char *ptr = NULL;

	v = strtoul(str, &ptr, 0);
	if (!ptr || ptr == str || ptr[0] != '\0') {
		fprintf(vh->vh_stream, "invalid integer:%s\n", str);
		return -EINVAL;
	}
	if (v == ULONG_MAX) {
		fprintf(vh->vh_stream, "overflow:%s\n", str);
		return -ERANGE;
	}

	*val = (uint32_t)v;
	return 0;
}

/* get the first address with given string */
static int vt_str_to_addr6(const struct vt_handle *vh, const char *str,
			   struct in6_addr *addr)
{
	struct addrinfo hints;
	struct addrinfo *res = NULL;
	struct sockaddr_in6 *sin6;
	int err;

	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_RAW;
	hints.ai_family = AF_INET6;
	hints.ai_flags = AI_PASSIVE;

	err = getaddrinfo(str, NULL, &hints, &res);
	if (err != 0) {
		fprintf(vh->vh_stream,
			"getaddrinfo: \"%s\" :%s\n", str, gai_strerror(err));
		goto end;
	}
	if (res->ai_addrlen < sizeof(*sin6)) {
		fprintf(vh->vh_stream,
			"getaddrinfo: \"%s\" : sockaddr too short:%d\n",
			str, res->ai_addrlen);
		err = -ENOBUFS;
		goto end;
	}
	if (res->ai_family != AF_INET6) {
		fprintf(vh->vh_stream,
			"getaddrinfo: \"%s\" : family is not AF_INET6:%u\n",
			str, res->ai_family);
		err = -EINVAL;
		goto end;
	}
	sin6 = (struct sockaddr_in6 *)res->ai_addr;
	memcpy(addr, &sin6->sin6_addr, sizeof(*addr));

 end:
	if (res)
		freeaddrinfo(res);

	return err;
}

static const char *vt_str_space_skip(const char *str);
static char *vt_str_space_chop(const char *str, char *buf, int bufsize);

int vt_cmd_add(struct vt_cmd_entry *parent, struct vt_cmd_entry *e)
{
	int err = 0;
	struct list_head *lp;

	INIT_LIST_HEAD(&e->list);
	e->parent = NULL;
	INIT_LIST_HEAD(&e->child_list);

	pthread_rwlock_wrlock(&vt_lock);

	if (!parent || !e) {
		err = -EINVAL;
		goto fin;
	}
	if (e == &vt_cmd_root) {
		err = -EINVAL;
		goto fin;
	}
	if (parent != &vt_cmd_root && parent->parent == NULL) {
		err = -EINVAL;
		goto fin;
	}
	if (e->parent != NULL) {
		err = -EINVAL;
		goto fin;
	}

	/* insert the entry to the list */
	/* XXX: TODO: it should be checked infinite loop */
	list_for_each (lp, &parent->child_list) {
		struct vt_cmd_entry *ce;
		ce = list_entry(lp, struct vt_cmd_entry, list);
		if (strcmp(ce->cmd, e->cmd) > 0) {
			list_add_tail(&e->list, lp);
			goto inserted;
		}
	}

	list_add_tail(&e->list, &parent->child_list);

 inserted:
	e->parent = parent;

 fin:
	pthread_rwlock_unlock(&vt_lock);

	return err;
}

int vt_cmd_add_root(struct vt_cmd_entry *e)
{
	return vt_cmd_add(&vt_cmd_root, e);
}

static int vt_cmd_has_child(struct vt_cmd_entry *e)
{
	return !list_empty(&e->child_list);
}

static const char *vt_str_space_skip(const char *str)
{
	int len = strlen(str);
	int i = 0;

	for (i = 0; i < len; i++) {
		if (isspace(str[i]) == 0)
			break;
	}

	return &str[i];
}

static char *vt_str_space_chop(const char *str, char *buf, int bufsize)
{
	int len = strlen(str);
	int i = 0;

	if (len > bufsize - 1)
		len = bufsize - 1;

	for (i = 0; i < len; i++) {
		if (isspace(str[i]) == 0)
			continue;

		strncpy(buf, str, i);
		buf[i] = '\0';
		return buf;
	}

	return NULL;
}

static int vt_str_match(const char *def, const char *str)
{
	int def_len = strlen(def);
	int len = strlen(str);

	if (def_len == len) {
		if (strncmp(def, str, def_len) == 0)
			return 1;
	} else if (def_len < len) {
		if (strncmp(def, str, def_len) == 0 &&
		    isspace(str[def_len]) != 0)
			return 1;
	}
	return 0;
}

static int vt_cmd_match(struct vt_cmd_entry *e, const char *cmd)
{
	return (vt_str_match(e->cmd, cmd) ||
		(e->cmd_alias && vt_str_match(e->cmd_alias, cmd)));
}

/*
 * It is only used the parser which is the final level away from root.
 */
static int vt_cmd_input(const struct vt_handle *vh, char *line, ssize_t len)
{
	struct vt_cmd_entry *ce = &vt_cmd_root;
	const char *p;
	int ret;

	pthread_rwlock_rdlock(&vt_lock);

	p = line;

	while (1) {
		const char *p_next = NULL;
		struct vt_cmd_entry *e = NULL;
		struct list_head *lp;

		p = vt_str_space_skip(p);
		/* command has no character */
		if (strlen(p) == 0)
			goto fin;

		list_for_each (lp, &ce->child_list) {
			e = list_entry(lp, struct vt_cmd_entry, list);

			if (vt_cmd_match(e, p) == 0)
				continue;

			p_next = p + strlen(e->cmd);
			p_next = vt_str_space_skip(p_next);

			if (strlen(p_next) > 0 && vt_cmd_has_child(e))
				break;

			if (!e->parser) {
				fprintf(vh->vh_stream, "do nothing\n");
				goto fin;
			}

			pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
			ret = e->parser(vh, p_next);
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
			if (ret != 0) {
				fprintf(vh->vh_stream, "command parse failed\n");
			}

			goto fin;
		}

		if (p_next) {
			p = p_next;
			ce = e;
		} else if (vt_str_match(VT_CMD_HELP_STR, p)) {
			/* try to show help message when no matching command */
			vt_cmd_dump_candidates(vh, ce, VT_CMD_HELP_LINE_MAX);
			goto fin;
		} else {
			char buf[VT_PKT_BUFLEN];
			fprintf(vh->vh_stream, "unknown command: \"%s\"\n",
				vt_str_space_chop(p, buf, sizeof(buf)));
			goto fin;
		}
	}

 fin:
	if (vh->vh_sock < 0) {
		/* socket is closed during paring (normal operation) */
		goto closed;
	}

	if (vh->vh_opt.prompt == VT_BOOL_TRUE) {
		/* send prompt */
		fprintf(vh->vh_stream, VT_CMD_PROMPT);
	}

	fprintf(vh->vh_stream, "%c", 0);
	fflush(vh->vh_stream);

	pthread_rwlock_unlock(&vt_lock);

	return 0;

 closed:
	pthread_rwlock_unlock(&vt_lock);

	return 1;

}

static int vt_connect_input(struct vt_handle *vh, void *data, ssize_t len)
{
	char *line = NULL;
	int ret;

	line = (char *)malloc(len);
	if (!line) {
		ret = -errno;
		goto fin;
	}
	memcpy(line, data, len);
	line[len - 1] = 0;

	ret = vt_cmd_input(vh, line, len);

	if (ret != 0)
		goto fin;

	free(line);

	return 0;

 fin:
	if (line)
		free(line);

	vt_connect_close(vh);
	return ret;
}

static int vt_connect_recv(struct vt_handle *vh)
{
	while (1) {
		char buf[VT_PKT_BUFLEN];
		struct msghdr msg;
		struct iovec iov = { buf, sizeof(buf) };
		ssize_t len;
		int ret;

		memset(buf, 0, sizeof(buf));
		memset(&msg, 0, sizeof(msg));

		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
		msg.msg_flags = 0;

		if (vh->vh_sock < 0)
			return -EBADFD;

		ret = recvmsg(vh->vh_sock, &msg, 0);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		} else if (ret == 0) {
			goto disconnect;
		}
		len = ret;

		ret = vt_connect_input(vh, buf, len);

		if (ret != 0)
			goto disconnect;

		if (msg.msg_flags & MSG_TRUNC)
			continue;

		if (msg.msg_flags & MSG_CTRUNC)
			continue;

		break;
	}

	return 0;

 disconnect:
	vt_connect_close(vh);
	return 1;
}

static int vt_connect_close(struct vt_handle *vh)
{
	if (vh->vh_sock < 0)
		return 0;

	close(vh->vh_sock);

	vh->vh_sock = -1;

	return 0;
}

static int vt_connect_fini(struct vt_handle *vh)
{
	if (vh->vh_sock >= 0)
		vt_connect_close(vh);

	memset(vh, 0, sizeof(*vh)); /* for fail-safe */
	vh->vh_sock = -1;

	free(vh);

	vt_connect_handle = NULL; /* XXX: remove from global pointer */

	return 0;
}

static int vt_connect_init(const struct vt_server_entry *vse)
{
	int sock;
	FILE *stream;
	struct vt_handle *vh = NULL;
	int ret;

	sock = accept(vse->vse_sock, NULL, NULL);
	if (sock < 0)
		goto error;

	stream = fdopen(sock, "w");

	if (vt_handle_full()) {
		/* send error message */
		ret = fprintf(stream, "Too many connections\n");
		if (ret < 0) {
			/* ignore error here*/
		}
		close(sock);
		goto error;
	}

	vh = (struct vt_handle *)malloc(sizeof(*vh));
	if (vh == NULL) {
		ret = -errno;

		/* send error message */
		if (fprintf(stream, "Server cannot make connection\n") < 0) {
			/* ignore error here*/
		}
		close(sock);
		fclose(stream);
		goto error;
	}

	memset(vh, 0, sizeof(*vh));
	vh->vh_sock = sock;
	vh->vh_stream = stream;

	/* Apply default values to option per server */
	switch (vse->vse_sa.sa_family) {
	case AF_LOCAL:
		vh->vh_opt.prompt = VT_BOOL_FALSE;
		vh->vh_opt.verbose = VT_BOOL_FALSE;
		vh->vh_opt.fancy = VT_BOOL_FALSE;
		break;
	default:
		vh->vh_opt.prompt = VT_BOOL_TRUE;
		vh->vh_opt.verbose = VT_BOOL_FALSE;
		vh->vh_opt.fancy = VT_BOOL_TRUE;
		break;
	}

	ret = vt_handle_add(vh);
	if (ret != 0) {
		goto error;
	}

	if (vh->vh_opt.prompt == VT_BOOL_TRUE) {
		/* send prompt */
		ret = fprintf(vh->vh_stream, VT_CMD_PROMPT);
		if (ret < 0)
			goto error;
		fflush(vh->vh_stream);
	}

	return 0;

 error:
	if (vh)
		vt_connect_fini(vh);

	return 0; /* ignore error here */
}

static void *vt_server_recv(void *arg)
{
	while (1) {
		int ret;
		int sock_max = 0;
		fd_set fds;
		struct list_head *lp;

		FD_ZERO(&fds);
		list_for_each (lp, &vt_server_list) {
			struct vt_server_entry *e;
			e = list_entry(lp, struct vt_server_entry, list);
			FD_SET(e->vse_sock, &fds);

			if (sock_max < e->vse_sock)
				sock_max = e->vse_sock;
		}
		if (sock_max == 0)
			break;
		if (vt_connect_handle != NULL &&
		    vt_connect_handle->vh_sock >= 0) {
			FD_SET(vt_connect_handle->vh_sock, &fds);
			if (sock_max < vt_connect_handle->vh_sock)
				sock_max = vt_connect_handle->vh_sock;
		}

		ret = select(sock_max+1, &fds, NULL, NULL, NULL);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			break;
		}

		ret = 0;
		list_for_each (lp, &vt_server_list) {
			struct vt_server_entry *e;
			e = list_entry(lp, struct vt_server_entry, list);

			if (FD_ISSET(e->vse_sock, &fds)) {
				ret = vt_connect_init(e);
				if (ret != 0)
					break;
			}
		}
		if (ret != 0)
			break;

		if (vt_connect_handle != NULL &&
		    vt_connect_handle->vh_sock >= 0) {
			if (FD_ISSET(vt_connect_handle->vh_sock, &fds)) {
				ret = vt_connect_recv(vt_connect_handle);
				if (ret != 0)
					vt_connect_fini(vt_connect_handle);
			}
		}
	}

	if (vt_connect_handle != NULL)
		vt_connect_fini(vt_connect_handle);
	vt_server_fini();

	pthread_exit(NULL);
}

static int vt_server_clean(const struct sockaddr *sa, int salen)
{
	if (sa->sa_family == AF_LOCAL) {
		const struct sockaddr_un *sun;
		if (salen >= sizeof(*sun)) {
			sun = (const struct sockaddr_un *)sa;
			if (unlink(sun->sun_path))
				errno = 0; /* ignore error here */
		}
	}

	return 0;
}

static void vt_server_close(struct vt_server_entry *e)
{
	if (e->vse_sock) {
		close(e->vse_sock);
		e->vse_sock = -1;
	}
}


static int vt_server_fini(void)
{
	struct list_head *lp, *tmp;

	list_for_each_safe (lp, tmp,  &vt_server_list) {
		struct vt_server_entry *e;

		list_del(lp);
		e = list_entry(lp, struct vt_server_entry, list);

		if (e->vse_sock >= 0)
			vt_server_clean(&e->vse_sa, e->vse_salen);
		vt_server_close(e);
		memset(e, 0, sizeof(*e)); /* for fail-safe */
		free(e);
	}

	return 0;
}

static int vt_server_setsockopt(int sock, struct addrinfo *ai)
{
	int ret;

	ret = vt_server_clean(ai->ai_addr, ai->ai_addrlen);
	if (ret != 0)
		return ret;

	if (ai->ai_protocol == IPPROTO_TCP) {
		const int on = 1;

		ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
				 &on, sizeof(on));
		if (ret != 0)
			return ret;
	}
	return 0;
}

static int vt_server_init(const char *node, const char *service,
			  struct addrinfo *hints)
{
	struct addrinfo *res = NULL;
	struct addrinfo *ai;
	int ret;
	int n = 0;

	ret = getaddrinfo(node, service, hints, &res);
	if (ret != 0)
		goto error;

	errno = 0;

	for (ai = res; ai != NULL; ai = ai->ai_next) {
		int sock;
		struct vt_server_entry *e;

		sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (sock < 0)
			continue;

		ret = vt_server_setsockopt(sock, ai);
		if (ret != 0) {
			close(sock);
			continue;
		}

		ret = bind(sock, ai->ai_addr, ai->ai_addrlen);
		if (ret != 0) {
			close(sock);
			continue;
		}

		ret = listen(sock, VT_SERVER_BACKLOG);
		if (ret != 0) {
			close(sock);
			continue;
		}

		e = (struct vt_server_entry *)malloc(sizeof(*e));
		if (e == NULL) {
			ret = -errno;
			close(sock);
			goto error;
		}
		memset(e, 0, sizeof(*e));
		e->vse_sock = sock;
		memcpy(&e->vse_sa, ai->ai_addr, ai->ai_addrlen);
		e->vse_salen = ai->ai_addrlen;

		list_add(&e->list, &vt_server_list);

		n ++;
	}
	if (n == 0) {
		ret = -1;
		goto error;
	}
	errno = 0;

	if (res != NULL)
		freeaddrinfo(res);

	return 0;
 error:
	if (res != NULL)
		freeaddrinfo(res);
	vt_server_fini();
	return ret;
}

int vt_start(const char *vthost, const char *vtservice)
{
	struct addrinfo hints;

	INIT_LIST_HEAD(&vt_server_list);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if (vt_server_init(vthost, vtservice, &hints) != 0)
		return -1;

	if (pthread_create(&vt_listener, NULL, vt_server_recv, NULL))
		return -1;
	return 0;
}

//Defined for PMIP////////////////
int vt_pbc_init(void)
{
    int ret;
    ret = vt_cmd_add_root(&vt_cmd_pbc);
    if (ret < 0)
    return ret;

    return 0;
}
//////////////////////////////////

int vt_init(void)
{
	if (pthread_rwlock_init(&vt_lock, NULL))
		return -1;

	return vt_cmd_sys_init();
}

void vt_fini(void)
{
	struct list_head *lp;

	list_for_each(lp,  &vt_server_list) {
		struct vt_server_entry *e;
		e = list_entry(lp, struct vt_server_entry, list);
		vt_server_close(e);
	}
	pthread_cancel(vt_listener);
	pthread_join(vt_listener, NULL);

	vt_server_fini();
}