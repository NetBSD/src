/*	$NetBSD: rtsold.h,v 1.10.8.1 2014/08/20 00:05:13 tls Exp $	*/
/*	$KAME: rtsold.h,v 1.14 2002/05/31 10:10:03 itojun Exp $	*/

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
	char ifname[IF_NAMESIZE]; /* interface name */
	u_int32_t linkid;	/* link ID of this interface */
	int active;		/* interface status */
	int probeinterval;	/* interval of probe timer(if necessary) */
	int probetimer;		/* rest of probe timer */
	int mediareqok;		/* whether the IF supports SIOCGIFMEDIA */
	int state;
	int probes;
	int dadcount;
	struct timeval timer;
	struct timeval expire;
	int errors;		/* # of errors we've got - detect wedge */

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

/* rtsold.c */
extern struct timeval tm_max;
extern int dflag;
extern int aflag;
extern struct ifinfo *iflist;
extern int rssock;

int ifconfig(char *);
void iflist_init(void);
struct ifinfo *find_ifinfo(size_t);
void rtsol_timer_update(struct ifinfo *);
void warnmsg(int, const char *, const char *, ...) __printflike(3, 4);
char **autoifprobe(void);

/* if.c */
int ifinit(void);
int interface_up(char *);
int interface_status(struct ifinfo *);
size_t lladdropt_length(struct sockaddr_dl *);
void lladdropt_fill(struct sockaddr_dl *, struct nd_opt_hdr *);
struct sockaddr_dl *if_nametosdl(char *);
int getinet6sysctl(int);

/* rtsol.c */
int sockopen(void);
void sendpacket(struct ifinfo *);
void rtsol_input(int);

/* probe.c */
int probe_init(void);
void defrouter_probe(struct ifinfo *);

/* dump.c */
void rtsold_dump_file(const char *);

/* rtsock.c */
int rtsock_open(void);
int rtsock_input(int);
