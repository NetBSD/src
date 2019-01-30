/*	$NetBSD: extern.h,v 1.46.6.1 2019/01/30 13:46:25 martin Exp $	*/

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
 * 3. Neither the name of the University nor the names of its contributors
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
#include <stdbool.h>
#include <kvm.h>

#define ADJINETCTR(c, o, n, e)	(c.e = n.e - o.e)
#define xADJINETCTR(c, o, n, e)	(c[e] = n[e] - o[e])
#define MAXFAIL	5

extern struct	command global_commands[];
extern struct	mode *curmode;
extern struct	mode modes[];
extern struct	text *xtext;
extern WINDOW	*wnd;
extern char	*namp, hostname[];
extern double	avenrun[3];
extern float	*dk_mspw;
extern kvm_t	*kd;
extern long	ntext, textp;
extern int	CMDLINE;
extern int	hz, stathz, maxslp;
extern double	naptime;
extern int	nhosts;
extern int	nports;
extern int	protos;
extern int	verbose;
extern int	nflag;
extern char	*memf;
extern int	allflag;
extern int	turns;
extern gid_t	egid;
extern float	hertz;
extern double	etime;

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
void	 closedf(WINDOW *);
void	 closeicmp(WINDOW *);
void	 closeifstat(WINDOW *);
void	 closeiostat(WINDOW *);
void	 closeip(WINDOW *);
void	 closevmstat(WINDOW *);
void	 closesyscall(WINDOW *);
void	 closembufs(WINDOW *);
void	 closenetstat(WINDOW *);
void	 closepigs(WINDOW *);
void	 closeswap(WINDOW *);
void	 closetcp(WINDOW *);
int	 cmdifstat(const char *, const char *);
void	 command(char *);
void	 df_all(char *);
void	 df_some(char *);
void	 die(int) __dead;
void	 disks_add(char *);
void	 disks_remove(char *);
void	 disks_drives(char *);
void	 display(int);
void	 error(const char *, ...) __printflike(1, 2);
void	 clearerror(void);
void	 fetchbufcache(void);
void	 fetchdf(void);
void	 fetchicmp(void);
void	 fetchifstat(void);
void	 fetchiostat(void);
void	 fetchip(void);
void	 fetchvmstat(void);
void	 fetchsyscall(void);
void	 fetchmbufs(void);
void	 fetchnetstat(void);
void	 fetchpigs(void);
void	 fetchswap(void);
void	 fetchtcp(void);
int	 fetch_cptime(u_int64_t *);
void	 global_help(char *);
void	 global_interval(char *);
void	 global_load(char *);
void	 global_quit(char *) __dead;
void	 global_stop(char *);
void	 icmp_boot(char *);
void	 icmp_run(char *);
void	 icmp_time(char *);
void	 icmp_zero(char *);
int	 ifcmd(const char *cmd, const char *args);
void	 ifstat_match(char*);
void	 ifstat_pps(char*);
void	 ifstat_scale(char*);
int	 initbufcache(void);
int	 initdf(void);
int	 initicmp(void);
int	 initifstat(void);
int	 initiostat(void);
int	 initip(void);
int	 initvmstat(void);
int	 initsyscall(void);
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
void	 keyboard(void) __dead;
ssize_t	 kvm_ckread(const void *, void *, size_t, const char *);
void	 labelbufcache(void);
void	 labeldf(void);
void	 labelicmp(void);
void	 labelifstat(void);
void	 labeliostat(void);
void	 labelip(void);
void	 labelvmstat(void);
void	 labelsyscall(void);
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
void	 nlisterr(struct nlist []) __dead;
WINDOW	*openbufcache(void);
WINDOW	*opendf(void);
WINDOW	*openicmp(void);
WINDOW	*openifstat(void);
WINDOW	*openiostat(void);
WINDOW	*openip(void);
WINDOW	*openvmstat(void);
WINDOW	*opensyscall(void);
WINDOW	*openmbufs(void);
WINDOW	*opennetstat(void);
WINDOW	*openpigs(void);
WINDOW	*openswap(void);
WINDOW	*opentcp(void);
int	 prefix(const char *, const char *);
void	 ps_user(char *);
void	 redraw(void);
void	 showbufcache(void);
void	 showdf(void);
void	 showicmp(void);
void	 showifstat(void);
void	 showiostat(void);
void	 showip(void);
void	 showvmstat(void);
void	 showsyscall(void);
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
bool	 toofast(int *);
void	 vmstat_boot(char *);
void	 vmstat_run(char *);
void	 vmstat_time(char *);
void	 vmstat_zero(char *);
void	 syscall_boot(char *);
void	 syscall_run(char *);
void	 syscall_time(char *);
void	 syscall_zero(char *);
void	 syscall_order(char *);
void	 syscall_show(char *);

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
