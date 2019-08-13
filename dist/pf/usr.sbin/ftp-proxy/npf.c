/*	$NetBSD: npf.c,v 1.3 2019/08/13 09:48:24 maxv Exp $	*/

/*
 * Copyright (c) 2011, 2019 The NetBSD Foundation, Inc.
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
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <net/pfvar.h>
#include <net/bpf.h>

#define NPF_BPFCOP
#include <npf.h>

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
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

typedef struct fp_ent {
	LIST_ENTRY(fp_ent)	fpe_list;
	uint32_t		fpe_id;
	nl_rule_t *		fpe_rl;
	nl_rule_t *		fpe_nat;
	nl_rule_t *		fpe_rdr;
} fp_ent_t;

char *				npfopts;

static LIST_HEAD(, fp_ent)	fp_ent_list;
static struct sockaddr_in	fp_server_sa;
static char *			fp_ifname;
static int			npf_fd;

#define	NPF_BPF_SUCCESS		((u_int)-1)
#define	NPF_BPF_FAILURE		0

#define INSN_IPSRC	5
#define INSN_IPDST	7
#define INSN_DPORT	10

static struct bpf_insn insns[13] = {
	/* Want IPv4. */
	BPF_STMT(BPF_LD+BPF_MEM, BPF_MW_IPVER),
	BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 4, 0, 10),
	/* Want TCP. */
	BPF_STMT(BPF_LD+BPF_MEM, BPF_MW_L4PROTO),
	BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, IPPROTO_TCP, 0, 8),
	/* Check source IP. */
	BPF_STMT(BPF_LD+BPF_W+BPF_ABS, offsetof(struct ip, ip_src)),
	BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 0xDEADBEEF, 0, 6),
	/* Check destination IP. */
	BPF_STMT(BPF_LD+BPF_W+BPF_ABS, offsetof(struct ip, ip_dst)),
	BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 0xDEADBEEF, 0, 4),
	/* Check port. */
	BPF_STMT(BPF_LDX+BPF_MEM, BPF_MW_L4OFF),
	BPF_STMT(BPF_LD+BPF_H+BPF_IND, offsetof(struct tcphdr, th_dport)),
	BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 0xDEADBEEF, 0, 1),
	/* Success. */
	BPF_STMT(BPF_RET+BPF_K, NPF_BPF_SUCCESS),
	/* Failure. */
	BPF_STMT(BPF_RET+BPF_K, NPF_BPF_FAILURE),
};

static void
modify_bytecode(in_addr_t shost, in_addr_t dhost, in_port_t dport)
{
	/*
	 * Replace the 0xDEADBEEF by actual values. Note that BPF is in
	 * host order.
	 */

	/* Source address to match. */
	insns[INSN_IPSRC].k = shost;
	/* Destination address to match. */
	insns[INSN_IPDST].k = dhost;
	/* Destination port to match. */
	insns[INSN_DPORT].k = dport;
}

static fp_ent_t *
proxy_lookup(uint32_t id)
{
	fp_ent_t *fpe;

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
	int kernver, idx;

	/* XXX get rid of this */
	if ((saddr = strchr(netif, ':')) == NULL) {
		errx(EXIT_FAILURE, "invalid -N option string: %s", npfopts);
	}
	*saddr++ = '\0';
	if ((port = strchr(saddr, ':')) == NULL) {
		errx(EXIT_FAILURE, "invalid -N option string: %s", npfopts);
	}
	*port++ = '\0';

	fp_ifname = netif;
	idx = if_nametoindex(fp_ifname);
	if (idx == 0) {
		errx(EXIT_FAILURE, "invalid network interface '%s'", fp_ifname);
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
	if (ioctl(npf_fd, IOC_NPF_VERSION, &kernver) == -1) {
		err(EXIT_FAILURE, "ioctl failed on IOC_NPF_VERSION");
	}
	if (kernver != NPF_VERSION) {
		errx(EXIT_FAILURE,
		    "incompatible NPF interface version (%d, kernel %d)\n"
		    "Hint: update %s?", NPF_VERSION, kernver,
		    kernver > NPF_VERSION ? "userland" : "kernel");
	}

	LIST_INIT(&fp_ent_list);
}

static int
npf_prepare_commit(uint32_t id)
{
	fp_ent_t *fpe;

	/* Check if already exists. */
	fpe = proxy_lookup(id);
	if (fpe) {
		/* Destroy existing rules and reset the values. */
		npf_rule_destroy(fpe->fpe_rl);
		npf_rule_destroy(fpe->fpe_nat);
		npf_rule_destroy(fpe->fpe_rdr);
		goto reset;
	}

	/* Create a new one, if not found. */
	fpe = malloc(sizeof(fp_ent_t));
	if (fpe == NULL)
		return -1;
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
	fpe = proxy_lookup(id);
	assert(fpe != NULL);

	di = (pf_dir == PF_OUT) ? NPF_RULE_OUT : NPF_RULE_IN;
	rl = npf_rule_create(NULL, di | NPF_RULE_PASS | NPF_RULE_FINAL, NULL);
	if (rl == NULL) {
		errno = ENOMEM;
		return -1;
	}

	modify_bytecode(sa_to_32(src), sa_to_32(dst), dport);
	errno = npf_rule_setcode(rl, NPF_CODE_BPF, insns, sizeof(insns));
	if (errno) {
		npf_rule_destroy(rl);
		return -1;
	}

	assert(fpe->fpe_rl == NULL);
	fpe->fpe_rl = rl;
	return 0;
}

/*
 * Note: we don't use plow and phigh. In NPF they are not per-rule, but global,
 * under the "portmap.min_port" and "portmap.max_port" params.
 */
static int
npf_add_nat(uint32_t id, struct sockaddr *src, struct sockaddr *dst,
    uint16_t dport, struct sockaddr *snat, uint16_t plow, uint16_t phigh)
{
	npf_addr_t addr;
	fp_ent_t *fpe;
	nl_nat_t *nt;

	if (!src || !dst || !dport || !snat || !plow ||
	    (src->sa_family != snat->sa_family)) {
		errno = EINVAL;
		return -1;
	}
	fpe = proxy_lookup(id);
	assert(fpe != NULL);

	memset(&addr, 0, sizeof(npf_addr_t));
	memcpy(&addr, &sa_to_32(snat), sizeof(struct in_addr));

	nt = npf_nat_create(NPF_NATOUT, NPF_NAT_PORTS | NPF_NAT_PORTMAP, NULL);
	if (nt == NULL) {
		errno = ENOMEM;
		return -1;
	}
	errno = npf_nat_setaddr(nt, AF_INET, &addr, 0);
	if (errno) {
		goto err;
	}

	modify_bytecode(sa_to_32(src), sa_to_32(dst), dport);
	errno = npf_rule_setcode(nt, NPF_CODE_BPF, insns, sizeof(insns));
	if (errno) {
		goto err;
	}

	assert(fpe->fpe_nat == NULL);
	fpe->fpe_nat = nt;
	return 0;

err:
	npf_rule_destroy(nt);
	return -1;
}

static int
npf_add_rdr(uint32_t id, struct sockaddr *src, struct sockaddr *dst,
    uint16_t dport, struct sockaddr *rdr, uint16_t rdr_port)
{
	npf_addr_t addr;
	fp_ent_t *fpe;
	nl_nat_t *nt;

	if (!src || !dst || !dport || !rdr || !rdr_port ||
	    (src->sa_family != rdr->sa_family)) {
		errno = EINVAL;
		return -1;
	}
	fpe = proxy_lookup(id);
	assert(fpe != NULL);

	memset(&addr, 0, sizeof(npf_addr_t));
	memcpy(&addr, &sa_to_32(rdr), sizeof(struct in_addr));

	nt = npf_nat_create(NPF_NATIN, NPF_NAT_PORTS, NULL);
	if (nt == NULL) {
		errno = ENOMEM;
		return -1;
	}
	errno = npf_nat_setaddr(nt, AF_INET, &addr, 0);
	if (errno) {
		goto err;
	}
	errno = npf_nat_setport(nt, htons(rdr_port));
	if (errno) {
		goto err;
	}

	modify_bytecode(sa_to_32(src), sa_to_32(dst), dport);
	errno = npf_rule_setcode(nt, NPF_CODE_BPF, insns, sizeof(insns));
	if (errno) {
		goto err;
	}

	assert(fpe->fpe_rdr == NULL);
	fpe->fpe_rdr = nt;
	return 0;

err:
	npf_rule_destroy(nt);
	return -1;
}

static int
npf_server_lookup(struct sockaddr *client, struct sockaddr *proxy,
    struct sockaddr *server)
{

	memcpy(server, &fp_server_sa, sizeof(struct sockaddr_in));
	return 0;
}

static int
npf_do_commit(void)
{
	nl_config_t *ncf;
	nl_rule_t *group;
	fp_ent_t *fpe;
	pri_t pri;

	ncf = npf_config_create();
	if (ncf == NULL) {
		errno = ENOMEM;
		return -1;
	}

	group = npf_rule_create(NULL, NPF_RULE_GROUP | NPF_RULE_PASS |
	    NPF_RULE_IN | NPF_RULE_OUT | NPF_RULE_FINAL, fp_ifname);
	if (group == NULL) {
		errno = ENOMEM;
		goto err;
	}

	pri = 1;
	LIST_FOREACH(fpe, &fp_ent_list, fpe_list) {
		if (fpe->fpe_rl == NULL) {
			/* Empty. */
			continue;
		}
		npf_rule_setprio(fpe->fpe_rl, pri++);
		npf_rule_insert(NULL, group, fpe->fpe_rl);
		/*
		 * XXX: Mmh, aren't we supposed to insert fpe_nat and fpe_rdr
		 * too here?
		 */
	}
	npf_rule_insert(ncf, NULL, group);

	errno = npf_config_submit(ncf, npf_fd, NULL);
	if (errno != 0)
		goto err;

	npf_config_destroy(ncf);
	return 0;

err:
	if (ncf != NULL)
		npf_config_destroy(ncf);
	return -1;
}

static int
npf_do_rollback(void)
{
	/* None. */
	return 0;
}
