/*	$NetBSD: extern.h,v 1.30 2003/08/03 11:56:57 jdolecek Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)extern.h	8.1 (Berkeley) 6/6/93
 */

#include <sys/cdefs.h>
#include <fcntl.h>
#include <kvm.h>

#define ADJINETCTR(c, o, n, e)	(c.e = n.e - o.e)

extern struct	command global_commands[];
extern struct	mode *curmode;
extern struct	mode modes[];
extern struct	text *xtext;
extern WINDOW	*wnd;
extern char	c, *namp, hostname[];
extern double	avenrun[3];
extern float	*dk_mspw;
extern kvm_t	*kd;
extern long	ntext, textp;
extern int	CMDLINE;
extern int	hz, stathz, maxslp;
extern int	naptime, col;
extern int	nhosts;
extern int	nports;
extern int	protos;
extern int	verbose;
extern int	nflag;
extern char	*memf;
extern int	allflag;
extern int	turns;
extern gid_t	egid;

struct inpcb;
#ifdef INET6
struct in6pcb;
#endif

int	 checkhost(struct inpcb *);
int	 checkport(struct inpcb *);
#ifdef INET6
int	 checkhost6(struct in6pcb *);
int	 checkport6(struct in6pcb *);
#endif
void	 closebufcache(WINDOW *);
void	 closeicmp(WINDOW *);
void	 closeiostat(WINDOW *);
void	 closeip(WINDOW *);
void	 closevmstat(WINDOW *);
void	 closembufs(WINDOW *);
void	 closenetstat(WINDOW *);
void	 closepigs(WINDOW *);
void	 closeswap(WINDOW *);
void	 closetcp(WINDOW *);
void	 command(char *);
void	 die(int);
void	 disks_add(char *);
void	 disks_delete(char *);
void	 disks_drives(char *);
void	 display(int);
void	 error(const char *, ...)
     __attribute__((__format__(__printf__, 1, 2)));
void	 fetchbufcache(void);
void	 fetchicmp(void);
void	 fetchiostat(void);
void	 fetchip(void);
void	 fetchvmstat(void);
void	 fetchmbufs(void);
void	 fetchnetstat(void);
void	 fetchpigs(void);
void	 fetchswap(void);
void	 fetchtcp(void);
int	 fetch_cptime(u_int64_t *);
void	 global_help(char *);
void	 global_interval(char *);
void	 global_load(char *);
void	 global_quit(char *);
void	 global_stop(char *);
void	 icmp_boot(char *);
void	 icmp_run(char *);
void	 icmp_time(char *);
void	 icmp_zero(char *);
int	 initbufcache(void);
int	 initicmp(void);
int	 initiostat(void);
int	 initip(void);
int	 initvmstat(void);
int	 initmbufs(void);
int	 initnetstat(void);
int	 initpigs(void);
int	 initswap(void);
int	 inittcp(void);
void	 iostat_bars(char *);
void	 iostat_numbers(char *);
void	 iostat_secs(char *);
void	 iostat_rw(char *);
void	 iostat_all(char *);
void	 ip_boot(char *);
void	 ip_run(char *);
void	 ip_time(char *);
void	 ip_zero(char *);
int	 keyboard(void) __attribute__((__noreturn__));
ssize_t	 kvm_ckread(const void *, void *, size_t);
void	 labelbufcache(void);
void	 labelicmp(void);
void	 labeliostat(void);
void	 labelip(void);
void	 labelvmstat(void);
void	 labelmbufs(void);
void	 labelnetstat(void);
void	 labelpigs(void);
void	 labelps(void);
void	 labels(void);
void	 labelswap(void);
void	 labeltcp(void);
void	 labeltcpsyn(void);
void	 netstat_all(char *);
void	 netstat_display(char *);
void	 netstat_ignore(char *);
void	 netstat_names(char *);
void	 netstat_numbers(char *);
void	 netstat_reset(char *);
void	 netstat_show(char *);
void	 netstat_tcp(char *);
void	 netstat_udp(char *);
void	 nlisterr(struct nlist []) __attribute__((__noreturn__));
WINDOW	*openbufcache(void);
WINDOW	*openicmp(void);
WINDOW	*openiostat(void);
WINDOW	*openip(void);
WINDOW	*openvmstat(void);
WINDOW	*openmbufs(void);
WINDOW	*opennetstat(void);
WINDOW	*openpigs(void);
WINDOW	*openswap(void);
WINDOW	*opentcp(void);
void	 ps_user(char *);
void	 redraw(int);
void	 showbufcache(void);
void	 showicmp(void);
void	 showiostat(void);
void	 showip(void);
void	 showvmstat(void);
void	 showmbufs(void);
void	 shownetstat(void);
void	 showpigs(void);
void	 showps(void);
void	 showswap(void);
void	 showtcp(void);
void	 showtcpsyn(void);
void	 status(void);
void	 switch_mode(struct mode *);
void	 tcp_boot(char *);
void	 tcp_run(char *);
void	 tcp_time(char *);
void	 tcp_zero(char *);
void	 vmstat_boot(char *);
void	 vmstat_run(char *);
void	 vmstat_time(char *);
void	 vmstat_zero(char *);

#ifdef INET6
void	 closeip6(WINDOW *);
void	 fetchip6(void);
int	 initip6(void);
void	 labelip6(void);
WINDOW	*openip6(void);
void	 showip6(void);
void	 ip6_boot(char *);
void	 ip6_run(char *);
void	 ip6_time(char *);
void	 ip6_zero(char *);
#endif

#ifdef IPSEC
void	 closeipsec(WINDOW *);
void	 fetchipsec(void);
int	 initipsec(void);
void	 labelipsec(void);
WINDOW	*openipsec(void);
void	 showipsec(void);
void	 ipsec_boot(char *);
void	 ipsec_run(char *);
void	 ipsec_time(char *);
void	 ipsec_zero(char *);
#endif
