/* $NetBSD: dhcpcd.h,v 1.1.1.16 2014/01/03 22:10:44 roy Exp $ */

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2013 Roy Marples <roy@marples.name>
 * All rights reserved

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef DHCPCD_H
#define DHCPCD_H

#include <sys/queue.h>
#include <sys/socket.h>
#include <net/if.h>

#include "control.h"
#include "if-options.h"

#define HWADDR_LEN 20
#define IF_SSIDSIZE 33
#define PROFILE_LEN 64

#define LINK_UP		1
#define LINK_UNKNOWN	0
#define LINK_DOWN	-1

#define IF_DATA_IPV4	0
#define IF_DATA_DHCP	1
#define IF_DATA_IPV6	2
#define IF_DATA_IPV6ND	3
#define IF_DATA_DHCP6	4
#define IF_DATA_MAX	5

struct interface {
	TAILQ_ENTRY(interface) next;
	char name[IF_NAMESIZE];
	unsigned int index;
	int flags;
	sa_family_t family;
	unsigned char hwaddr[HWADDR_LEN];
	size_t hwlen;
	int metric;
	int carrier;
	int wireless;
	char ssid[IF_SSIDSIZE];

	char profile[PROFILE_LEN];
	struct if_options *options;
	void *if_data[IF_DATA_MAX];
};
extern TAILQ_HEAD(if_head, interface) *ifaces;

extern char vendor[VENDORCLASSID_MAX_LEN];
extern sigset_t dhcpcd_sigset;
extern int pidfd;
extern int ifac;
extern char **ifav;
extern int ifdc;
extern char **ifdv;
extern struct if_options *if_options;

extern const int handle_sigs[];

pid_t daemonise(void);
struct interface *find_interface(const char *);
int handle_args(struct fd_list *, int, char **);
void handle_carrier(int, int, const char *);
void handle_interface(int, const char *);
void handle_hwaddr(const char *, const unsigned char *, size_t);
void drop_interface(struct interface *, const char *);
int select_profile(struct interface *, const char *);

void start_interface(void *);

#endif
