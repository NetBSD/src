/*	$NetBSD: timed-extern.h,v 1.11 2018/01/16 07:57:58 maya Exp $	*/

/*-
 * Copyright (c) 1993 The Regents of the University of California.
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
 *	@(#)extern.h	5.1 (Berkeley) 5/11/93
 */

struct hosttbl;
struct netinfo;
struct sockaddr_in;
struct timeval;
struct tsp;

struct hosttbl *addmach(char *, struct sockaddr_in *, struct netinfo *);
struct hosttbl *findhost(char *);
struct hosttbl *remmach(struct hosttbl *);

struct tsp *readmsg(int, char *, struct timeval *, struct netinfo *);
struct tsp *acksend(struct tsp *,
		    struct sockaddr_in *, char *, int, struct netinfo *, int);

void	 addnetname(char *);
void	 adj_msg_time(struct tsp *, struct timeval *);
void	 bytehostorder(struct tsp *);
void	 bytenetorder(struct tsp *);
void	 byteorder(struct tsp *);
long	 casual(long, long);
int	 cksum(u_short *, int);
void	 correct(long);
char	*date(void);
void	 doquit(struct tsp *);
int	 election(struct netinfo *);
void	 get_goodgroup(int);
int	 good_host_name(char *);
void	 ignoreack(void);
int	 in_cksum(const void *, int);
void	 lookformaster(struct netinfo *);
void	 makeslave(struct netinfo *);
void	 master(void) __dead;
void	 masterack(void);
void	 masterup(struct netinfo *);
int	 measure(u_long, u_long, const char *, const struct sockaddr_in *, int);
void	 msterup(struct netinfo *);
void	 mstotvround(struct timeval *, long);
long	 networkdelta(void);
void	 newslave(struct tsp *);
void	 print(struct tsp *, struct sockaddr_in *);
void	 prthp(clock_t);
void	 rmnetmachs(struct netinfo *);
void	 setstatus(void);
void	 slave(void) __dead;
void	 slaveack(void);
void	 spreadtime(void);
void	 suppress(struct sockaddr_in *, char *, struct netinfo *);
void	 synch(long);
void	 traceoff(const char *);
void	 traceon(void);
void	 xmit(int, u_short, struct sockaddr_in *);
void	 set_tsp_name(struct tsp *, const char *);
void	 get_tsp_name(const struct tsp *, char *, size_t);
void	 update_time(struct timeval *, const struct tsp *);
int	 sendtsp(int , struct tsp *, struct sockaddr_in *);
