/*	$NetBSD: if.h,v 1.13 2021/03/23 18:16:21 christos Exp $	*/
/*	$KAME: if.h,v 1.12 2003/09/21 07:17:03 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, 1998, and 1999 WIDE Project.
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

#define RTADV_TYPE2BITMASK(type) (0x1 << type)

struct nd_opt_hdr;
struct sockaddr_dl *if_nametosdl(const char *);
int if_getmtu(const char *);
int if_getflags(unsigned int, int);
int lladdropt_length(struct sockaddr_dl *);
void lladdropt_fill(struct sockaddr_dl *, struct nd_opt_hdr *);
char *get_next_msg(char *, char *, unsigned int, size_t *, int);
const struct in6_addr *get_addr(const void *);
unsigned int get_rtm_ifindex(const void *);
unsigned int get_ifm_ifindex(const void *);
unsigned int get_ifam_ifindex(const void *);
int get_ifm_flags(const void *);
#ifdef RTM_IFANNOUNCE
unsigned int get_ifan_ifindex(const void *);
int get_ifan_what(const void *);
#endif
int get_prefixlen(const void *);
int prefixlen(const unsigned char *, const unsigned char *);
const char *rtmsg_typestr(const void *);
int rtmsg_type(const void *);
int rtmsg_len(const void *);
