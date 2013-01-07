/*	$NetBSD: npf.c,v 1.1.8.1 2013/01/07 16:51:07 riz Exp $	*/

/*
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/queue.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <net/pfvar.h>
#include <net/npf_ncode.h>
#include <npf.h>

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include <assert.h>

#include "filter.h"

static void	npf_init_filter(char *, char *, int);
static int	npf_add_filter(uint32_t, uint8_t, struct sockaddr *,
		    struct sockaddr *, uint16_t);
static int	npf_add_nat(uint32_t, struct sockaddr *, struct sockaddr *,
		    uint16_t, struct sockaddr *, uint16_t, uint16_t);
static int	npf_add_rdr(uint32_t, struct sockaddr *, struct sockaddr *,
		    uint16_t, struct sockaddr *, uint16_t);
static int	npf_server_lookup(struct sockaddr *, struct sockaddr *,
		    struct sockaddr *);
static int	npf_prepare_commit(uint32_t);
static int	npf_do_commit(void);
static int	npf_do_rollback(void);

const ftp_proxy_ops_t npf_fprx_ops = {
	.init_filter	= npf_init_filter,
	.add_filter	= npf_add_filter,
	.add_nat	= npf_add_nat,
	.add_rdr	= npf_add_rdr,
	.server_lookup	= npf_server_lookup,
	.prepare_commit	= npf_prepare_commit,
	.do_commit	= npf_do_commit,
	.do_rollback	= npf_do_rollback
};

#define	sa_to_32(sa)		(((struct sockaddr_in *)sa)->sin_addr.s_addr)

#define	NPF_DEV_PATH		"/dev/npf"
#define	NPF_FP_RULE_TAG		"ftp-proxy"

typedef struct fp_ent {
	LIST_ENTRY(fp_ent)	fpe_list;
	uint32_t		fpe_id;
	nl_rule_t *		fpe_rl;
	nl_rule_t *		fpe_nat;
	nl_rule_t *		fpe_rdr;
} fp_ent_t;

char *				npfopts;

static LIST_HEAD(, fp_ent)	fp_ent_list;
static fp_ent_t *		fp_ent_hint;
static struct sockaddr_in	fp_server_sa;
static u_int			fp_if_idx;
static int			npf_fd;

static uint32_t ncode[ ] = {
	/* from <shost> to <dhost> port <dport> */
	NPF_OPCODE_IP4MASK,	0x01,	0xdeadbeef,	0xffffffff,
	NPF_OPCODE_BNE,		15,
	NPF_OPCODE_IP4MASK,	0x00,	0xdeadbeef,	0xffffffff,
	NPF_OPCODE_BNE,		9,
	NPF_OPCODE_TCP_PORTS,	0x00,	0xdeadbeef,
	NPF_OPCODE_BNE,		4,
	/* Success (0x0) and failure (0xff) paths. */
	NPF_OPCODE_RET,		0x00,
	NPF_OPCODE_RET,		0xff
};

static void
ftp_proxy_modify_nc(in_addr_t shost, in_addr_t dhost, in_port_t dport)
{
	/* Source address to match. */
	ncode[2] = shost;
	/* Destination address to match. */
	ncode[8] = shost;
	/* Destination port to match. */
	ncode[14] = ((uint32_t)dport << 16) | dport;
}

static fp_ent_t *
ftp_proxy_lookup(uint32_t id)
{
	fp_ent_t *fpe;

	/* Look for FTP proxy entry.  First, try hint (last used). */
	if (fp_ent_hint && fp_ent_hint->fpe_id == id) {
		return fp_ent_hint;
	}
	LIST_FOREACH(fpe, &fp_ent_list, fpe_list) {
		if (fpe->fpe_id == id)
			break;
	}
	return fpe;
}

static void
npf_init_filter(char *opt_qname, char *opt_tagname, int opt_verbose)
{
	char *netif = npfopts, *saddr, *port;

	/* XXX get rid of this */
	saddr = strchr(netif, ':');
	if (saddr == NULL) {
		errx(EXIT_FAILURE, "invalid -N option string: %s", npfopts);
	}
	*saddr++ = '\0';
	if (saddr == NULL || (port = strchr(saddr, ':')) == NULL) {
		errx(EXIT_FAILURE, "invalid -N option string: %s", npfopts);
	}
	*port++ = '\0';
	if (port == NULL) {
		errx(EXIT_FAILURE, "invalid -N option string: %s", npfopts);
	}

	fp_if_idx = if_nametoindex(netif);
	if (fp_if_idx == 0) {
		errx(EXIT_FAILURE, "invalid network interface '%s'", netif);
	}

	memset(&fp_server_sa, 0, sizeof(struct sockaddr_in));
	fp_server_sa.sin_len = sizeof(struct sockaddr_in);
	fp_server_sa.sin_family = AF_INET;
	fp_server_sa.sin_addr.s_addr = inet_addr(saddr);
	fp_server_sa.sin_port = htons(atoi(port));

	npf_fd = open(NPF_DEV_PATH, O_RDONLY);
	if (npf_fd == -1) {
		err(EXIT_FAILURE, "cannot open '%s'", NPF_DEV_PATH);
	}
	LIST_INIT(&fp_ent_list);
	fp_ent_hint = NULL;
}

static int
npf_prepare_commit(uint32_t id)
{
	fp_ent_t *fpe;

	/* Check if already exists. */
	fpe = ftp_proxy_lookup(id);
	if (fpe) {
		/* Destroy existing rules and reset the values. */
		npf_rule_destroy(fpe->fpe_rl);
		npf_rule_destroy(fpe->fpe_nat);
		npf_rule_destroy(fpe->fpe_rdr);
		goto reset;
	}
	/* Create a new one, if not found. */
	fpe = malloc(sizeof(fp_ent_t));
	if (fpe == NULL) {
		return -1;
	}
	LIST_INSERT_HEAD(&fp_ent_list, fpe, fpe_list);
	fpe->fpe_id = id;
reset:
	fpe->fpe_rl = NULL;
	fpe->fpe_nat = NULL;
	fpe->fpe_rdr = NULL;
	return 0;
}

static int
npf_add_filter(uint32_t id, uint8_t pf_dir, struct sockaddr *src,
    struct sockaddr *dst, uint16_t dport)
{
	fp_ent_t *fpe;
	nl_rule_t *rl;
	int di;

	if (!src || !dst || !dport) {
		errno = EINVAL;
		return -1;
	}
	fpe = ftp_proxy_lookup(id);
	assert(fpe != NULL);

	di = (pf_dir == PF_OUT) ? NPF_RULE_OUT : NPF_RULE_IN;
	rl = npf_rule_create(NULL, di | NPF_RULE_PASS | NPF_RULE_FINAL, 0);
	if (rl == NULL) {
		errno = ENOMEM;
		return -1;
	}
	ftp_proxy_modify_nc(sa_to_32(src), sa_to_32(dst), htons(dport));
	errno = npf_rule_setcode(rl, NPF_CODE_NCODE, ncode, sizeof(ncode));
	if (errno) {
		npf_rule_destroy(rl);
		return -1;
	}
	assert(fpe->fpe_rl == NULL);
	fpe->fpe_rl = rl;
	return 0;
}

static int
npf_add_nat(uint32_t id, struct sockaddr *src, struct sockaddr *dst,
    uint16_t dport, struct sockaddr *snat, uint16_t plow, uint16_t phigh)
{
	fp_ent_t *fpe;
	nl_nat_t *nt;
	npf_addr_t addr;

	if (!src || !dst || !dport || !snat || !plow ||
	    (src->sa_family != snat->sa_family)) {
		errno = EINVAL;
		return (-1);
	}
	fpe = ftp_proxy_lookup(id);
	assert(fpe != NULL);

	memcpy(&addr, &sa_to_32(snat), sizeof(struct in_addr));
	nt = npf_nat_create(NPF_NATOUT, NPF_NAT_PORTS | NPF_NAT_PORTMAP, 0,
	    &addr, AF_INET, htons(plow));
	if (nt == NULL) {
		errno = ENOMEM;
		return -1;
	}
	ftp_proxy_modify_nc(sa_to_32(src), sa_to_32(dst), htons(dport));
	errno = npf_rule_setcode(nt, NPF_CODE_NCODE, ncode, sizeof(ncode));
	if (errno) {
		npf_rule_destroy(nt);
		return -1;
	}
	assert(fpe->fpe_nat == NULL);
	fpe->fpe_nat = nt;
	return 0;
}

static int
npf_add_rdr(uint32_t id, struct sockaddr *src, struct sockaddr *dst,
    uint16_t dport, struct sockaddr *rdr, uint16_t rdr_port)
{
	fp_ent_t *fpe;
	nl_nat_t *nt;
	npf_addr_t addr;

	if (!src || !dst || !dport || !rdr || !rdr_port ||
	    (src->sa_family != rdr->sa_family)) {
		errno = EINVAL;
		return -1;
	}
	fpe = ftp_proxy_lookup(id);
	assert(fpe != NULL);

	memcpy(&addr, &sa_to_32(rdr), sizeof(struct in_addr));
	nt = npf_nat_create(NPF_NATIN, NPF_NAT_PORTS, 0,
	    &addr, AF_INET, htons(rdr_port));
	if (nt == NULL) {
		errno = ENOMEM;
		return -1;
	}
	ftp_proxy_modify_nc(sa_to_32(src), sa_to_32(dst), htons(dport));
	errno = npf_rule_setcode(nt, NPF_CODE_NCODE, ncode, sizeof(ncode));
	if (errno) {
		npf_rule_destroy(nt);
		return -1;
	}
	assert(fpe->fpe_rdr == NULL);
	fpe->fpe_rdr = nt;
	return 0;
}

static int
npf_server_lookup(struct sockaddr *c, struct sockaddr *proxy,
    struct sockaddr *server)
{

	memcpy(server, &fp_server_sa, sizeof(struct sockaddr_in));
	return 0;
}

static int
npf_do_commit(void)
{
#if 0
	nl_rule_t *group;
	fp_ent_t *fpe;
	pri_t pri;

	group = npf_rule_create(NPF_FP_RULE_TAG, NPF_RULE_PASS | NPF_RULE_IN |
	    NPF_RULE_OUT | NPF_RULE_FINAL, fp_if_idx);
	if (group == NULL) {
		return -1;
	}
	pri = 1;
	LIST_FOREACH(fpe, &fp_ent_list, fpe_list) {
		npf_rule_insert(NULL, group, fpe->fpe_rl, pri++);
	}
	npf_update_rule(npf_fd, NPF_FP_RULE_TAG, group);
	npf_rule_destroy(group);
	return 0;
#else
	errno = ENOTSUP;
	return -1;
#endif
}

static int
npf_do_rollback(void)
{
	/* None. */
	return 0;
}
