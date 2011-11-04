/*	$NetBSD: npfctl.h,v 1.7 2011/11/04 01:00:28 zoltan Exp $	*/

/*-
 * Copyright (c) 2009-2011 The NetBSD Foundation, Inc.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _NPFCTL_H_
#define _NPFCTL_H_

#include <sys/types.h>
#include <stdio.h>
#include <stdbool.h>

#include <net/npf_ncode.h>
#include <net/npf.h>

#define	_NPF_PRIVATE
#include <npf.h>

#ifdef DEBUG
#define	DPRINTF(x)	printf x
#else
#define	DPRINTF(x)
#endif

#define	NPF_DEV_PATH	"/dev/npf"
#define	NPF_CONF_PATH	"/etc/npf.conf"
#define	NPF_SESSDB_PATH	"/var/db/npf_sessions.db"

typedef struct {
	char *		e_data;
	void *		e_next;
} element_t;

#define	VAR_SINGLE	1
#define	VAR_ARRAY	2
#define	VAR_TABLE	3

typedef struct {
	char *		v_key;
	element_t *	v_elements;
	int		v_type;
	int		v_count;
	void *		v_next;
} var_t;

extern nl_config_t *	npf_conf;

void *		zalloc(size_t);
char *		xstrdup(const char *);

void		npfctl_init_data(void);
int		npfctl_ioctl_send(int);

struct ifaddrs *npfctl_getif(char *, unsigned int *, bool, sa_family_t);
void		npfctl_create_mask(sa_family_t, u_int, npf_addr_t *);
sa_family_t	npfctl_get_addrfamily(const char *);
sa_family_t	npfctl_parse_cidr(char *, sa_family_t, npf_addr_t *, npf_netmask_t *);
bool		npfctl_parse_port(char *, bool *, in_port_t *, in_port_t *);

void		npfctl_fill_table(nl_table_t *, char *);

void		npfctl_rule_ncode(nl_rule_t *, char *, char *,
		    int, int, var_t *, sa_family_t, var_t *, var_t *, var_t *);

size_t		npfctl_calc_ncsize(int []);
size_t		npfctl_failure_offset(int []);

void		npfctl_gennc_ether(void **, int, uint16_t);
void		npfctl_gennc_v4cidr(void **, int,
		    const npf_addr_t *, const npf_netmask_t, bool);
void		npfctl_gennc_v6cidr(void **, int,
		    const npf_addr_t *, const npf_netmask_t, bool);
void		npfctl_gennc_icmp(void **, int, int, int);
void		npfctl_gennc_tcpfl(void **, int , uint8_t, uint8_t);
void		npfctl_gennc_ports(void **, int,
		    in_port_t, in_port_t, bool, bool);
void		npfctl_gennc_tbl(void **, int, u_int , bool);
void		npfctl_gennc_complete(void **);

int		npf_parseline(char *);

#endif
