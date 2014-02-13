/*
 * $Id: debug.c 1.23 06/05/07 21:52:42+03:00 anttit@tcs.hut.fi $
 *
 * This file is part of the MIPL Mobile IPv6 for Linux.
 * 
 * Author: Antti Tuominen <anttit@tcs.hut.fi>
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "debug.h"

static FILE *sdbg;

static const char *dbg_strdate(char *str)
{
	struct timespec ts;
	time_t t;
	int ret;

	memset(&ts, 0, sizeof(ts));

	ret = clock_gettime(CLOCK_REALTIME, &ts);
	if (ret != 0)
		sprintf(str, "(clock_gettime error=%d)", errno);
	else {
		t = (time_t)ts.tv_sec; /* XXX: fix it! */
		if (t == 0) {
			strcpy(str, "(undefined)");
		} else {
			char buf[1024];
			sprintf(str, "%.19s",
				(ctime_r(&t, buf) ? buf : "(ctime_r error)"));
		}
	}

	return str;
}

void dbgprint(const char *fname, const char *fmt, ...)
{
        char s[1024];
        char stime[1024];
        va_list args;
 
        va_start(args, fmt);
        vsprintf(s, fmt, args);
        va_end(args);

	memset(stime, '\0', sizeof(stime));
	fprintf(sdbg, "%s ", dbg_strdate(stime));

	if (fname)
		fprintf(sdbg, "%s: ", fname);
	fprintf(sdbg, "%s", s);
	fflush(sdbg);
}

void debug_print_buffer(const void *data, int len, const char *fname, 
			const char *fmt, ...)
{ 
	int i; 
	char s[1024];
        va_list args;
 
        va_start(args, fmt);
        vsprintf(s, fmt, args);
        fprintf(sdbg, "%s: %s", fname, s);
        va_end(args);
	for (i = 0; i < len; i++) { 
		if (i % 16 == 0) fprintf(sdbg, "\n%04x: ", i);
		fprintf(sdbg, "%02x ", ((unsigned char *)data)[i]);
	} 
	fprintf(sdbg, "\n\n");
	fflush(sdbg);
}

void debug_print_func(void *arg, void (*func)(void *arg, void *stream))
{
	func(arg, sdbg);
	fflush(sdbg);
}

int debug_open(const char *path)
{
	FILE *fp;

	if (!path)
		return -EINVAL;
	if (sdbg && sdbg != stderr)
		return -EALREADY;

	fp = fopen(path, "a");
	if (!fp)
		return -errno;
	sdbg = fp;

	return 0;
}

void debug_close(void)
{
	if (sdbg && sdbg != stderr)
		fclose(sdbg);
	debug_init();
}

void debug_init(void)
{
	sdbg = stderr;
}

FILE *timers_log_fd = NULL;

void timers_log_init(void){
	timers_log_fd = fopen(TIMERS_LOG_FILE, "a+");
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	if (timers_log_fd) {
		dbg("Timers log enabled\n"); 
		fprintf(timers_log_fd, "LOG started at time %d:%09d\n", now.tv_sec, now.tv_nsec);
	} else dbg("Could not initialize timers log\n");
	return;
}

void timers_log_msg_recv(void){
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	if (timers_log_fd) {
		fprintf(timers_log_fd, "MSG R  %d:%09d\n", now.tv_sec, now.tv_nsec);
	}
	return;
}

void timers_log_icmp_recv(void){
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	if (timers_log_fd) {
		fprintf(timers_log_fd, "ICMP R %d:%09d\n", now.tv_sec, now.tv_nsec);
	}
	return;
}

void timers_log_start(void){
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	if (timers_log_fd) {
		fprintf(timers_log_fd, "START! %d:%09d\n", now.tv_sec, now.tv_nsec);
	}
	return;
}

void timers_log_pbu_sent(void){
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	if (timers_log_fd) {
		fprintf(timers_log_fd, "PBU S  %d:%09d\n", now.tv_sec, now.tv_nsec);
	}
	return;
}

void timers_log_pbu_recv(void){
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	if (timers_log_fd) {
		fprintf(timers_log_fd, "PBU R  %d:%09d\n", now.tv_sec, now.tv_nsec);
	}
	return;
}

void timers_log_pba_sent(void){
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	if (timers_log_fd) {
		fprintf(timers_log_fd, "PBA S  %d:%09d\n", now.tv_sec, now.tv_nsec);
	}
	return;
}

void timers_log_pba_recv(void){
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	if (timers_log_fd) {
		fprintf(timers_log_fd, "PBA R  %d:%09d\n", now.tv_sec, now.tv_nsec);
	}
	return;
}

void timers_log_dreg_sent(void){
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	if (timers_log_fd) {
		fprintf(timers_log_fd, "DER S  %d:%09d\n", now.tv_sec, now.tv_nsec);
	}
	return;
}

void timers_log_dreg_recv(void){
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	if (timers_log_fd) {
		fprintf(timers_log_fd, "DER R  %d:%09d\n", now.tv_sec, now.tv_nsec);
	}
	return;
}

void timers_log_stop(void){
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	if (timers_log_fd) {
		fprintf(timers_log_fd, "STOP!  %d:%09d\n", now.tv_sec, now.tv_nsec);
	}
	return;
}

void timers_log_clean(void){
	if (timers_log_fd) {
		fprintf(timers_log_fd, "LOG closed\n");
		fclose(timers_log_fd);
	}
}