/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

struct ifinfo {
	struct ifinfo *next;	/* pointer to the next interface */

	struct sockaddr_dl *sdl; /* link-layer address */
	char ifname[16];	/* interface name */
	int active;		/* interface status */
	int probeinterval;	/* interval of probe timer(if necessary) */
	int probetimer;		/* rest of probe timer */
	int mediareqok;		/* wheter the IF supports SIOCGIFMEDIA */
	int state;
	int probes;
	int dadcount;
	struct timeval timer;
	struct timeval expire;

	int racnt;		/* total # of valid RAs it have got */

	size_t rs_datalen;
	u_char *rs_data;
};

/* per interface status */
#define IFS_IDLE	0
#define IFS_DELAY	1
#define IFS_PROBE	2
#define IFS_DOWN	3
#define IFS_TENTATIVE	4

struct ifinfo *find_ifinfo(int ifindex);
void rtsol_timer_update(struct ifinfo *ifinfo);

extern int interface_status(struct ifinfo*);
#ifdef __STDC__
extern void warnmsg(int, const char *, const char *, ...);
#else
extern void warnmsg(int, const char *, const char *, va_list);
#endif
extern int getinet6sysctl(int code);
