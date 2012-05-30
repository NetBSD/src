/*	$NetBSD: npf_disassemble.c,v 1.4 2012/05/30 21:30:07 rmind Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: npf_disassemble.c,v 1.4 2012/05/30 21:30:07 rmind Exp $");

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>

#include <util.h>

#define NPF_OPCODES_STRINGS
#include <net/npf_ncode.h>

#include "npfctl.h"

enum {
	NPF_SHOW_SRCADDR,
	NPF_SHOW_DSTADDR,
	NPF_SHOW_SRCPORT,
	NPF_SHOW_DSTPORT,
	NPF_SHOW_ICMP,
	NPF_SHOW_TCPF,
	NPF_SHOW_COUNT,
};

struct nc_inf {
	npfvar_t *	nci_vlist[NPF_SHOW_COUNT];
	bool		nci_srcdst;
};

#define ADVANCE(n, rv) \
	do { \
		if (len < sizeof(*pc) * (n)) { \
			warnx("ran out of bytes"); \
			return rv; \
		} \
		pc += (n); \
		len -= sizeof(*pc) * (n); \
	} while (/*CONSTCOND*/0)

static size_t
npfctl_ncode_get_target(const uint32_t *pc, const uint32_t **t, size_t l)
{
	for (size_t i = 0; i < l; i++)
		if (t[i] == pc)
			return i;
	return ~0;
}

static size_t
npfctl_ncode_add_target(const uint32_t *pc, const uint32_t ***t, size_t *l,
    size_t *m)
{
	size_t q = npfctl_ncode_get_target(pc, *t, *l);

	if (q != (size_t)~0)
		return q;

	if (*l <= *m) {
		*m += 10;
		*t = xrealloc(*t, *m * sizeof(**t));
	}
	q = *l;
	(*t)[(*l)++] = pc;
	return q;
}

static void
npfctl_ncode_add_vp(char *buf, nc_inf_t *nci, unsigned idx)
{
	npfvar_t *vl = nci->nci_vlist[idx];

	if (vl == NULL) {
		vl = npfvar_create(".list");
		nci->nci_vlist[idx] = vl;
	}
	npfvar_t *vp = npfvar_create(".string");
	npfvar_add_element(vp, NPFVAR_STRING, buf, strlen(buf) + 1);
	npfvar_add_elements(vl, vp);
}

static const char *
npfctl_ncode_operand(char *buf, size_t bufsiz, uint8_t op, const uint32_t *st,
    const uint32_t *ipc, const uint32_t **pcv, size_t *lenv,
    const uint32_t ***t, size_t *l, size_t *m, nc_inf_t *nci)
{
	const uint32_t *pc = *pcv;
	size_t len = *lenv;
	struct sockaddr_storage ss;

	switch (op) {
	case NPF_OPERAND_NONE:
		abort();

	case NPF_OPERAND_REGISTER:
		if (*pc & ~0x3) {
			warnx("invalid register operand 0x%x at offset %td",
			    *pc, pc - st);
			return NULL;
		}
		snprintf(buf, bufsiz, "R%d", *pc);
		ADVANCE(1, NULL);
		break;

	case NPF_OPERAND_KEY:
		snprintf(buf, bufsiz, "key=<0x%x>", *pc);
		ADVANCE(1, NULL);
		break;

	case NPF_OPERAND_VALUE:
		snprintf(buf, bufsiz, "value=<0x%x>", *pc);
		ADVANCE(1, NULL);
		break;

	case NPF_OPERAND_SD:
		if (*pc & ~0x1) {
			warnx("invalid src/dst operand 0x%x at offset %td",
			    *pc, pc - st);
			return NULL;
		}
		bool srcdst = (*pc == NPF_OPERAND_SD_SRC);
		if (nci) {
			nci->nci_srcdst = srcdst;
		}
		snprintf(buf, bufsiz, "%s", srcdst ? "SRC" : "DST");
		ADVANCE(1, NULL);
		break;

	case NPF_OPERAND_REL_ADDRESS:
		snprintf(buf, bufsiz, "+%zu",
		    npfctl_ncode_add_target(ipc + *pc, t, l, m));
		ADVANCE(1, NULL);
		break;

	case NPF_OPERAND_NET_ADDRESS4: {
		struct sockaddr_in *sin = (void *)&ss;
		sin->sin_len = sizeof(*sin);
		sin->sin_family = AF_INET;
		sin->sin_port = 0;
		memcpy(&sin->sin_addr, pc, sizeof(sin->sin_addr));
		sockaddr_snprintf(buf, bufsiz, "%a", (struct sockaddr *)sin);
		if (nci) {
			npfctl_ncode_add_vp(buf, nci, nci->nci_srcdst ?
			    NPF_SHOW_SRCADDR : NPF_SHOW_DSTADDR);
		}
		ADVANCE(sizeof(sin->sin_addr) / sizeof(*pc), NULL);
		break;
	}
	case NPF_OPERAND_NET_ADDRESS6: {
		struct sockaddr_in6 *sin6 = (void *)&ss;
		sin6->sin6_len = sizeof(*sin6);
		sin6->sin6_family = AF_INET6;
		sin6->sin6_port = 0;
		memcpy(&sin6->sin6_addr, pc, sizeof(sin6->sin6_addr));
		sockaddr_snprintf(buf, bufsiz, "%a", (struct sockaddr *)sin6);
		if (nci) {
			npfctl_ncode_add_vp(buf, nci, nci->nci_srcdst ?
			    NPF_SHOW_SRCADDR : NPF_SHOW_DSTADDR);
		}
		ADVANCE(sizeof(sin6->sin6_addr) / sizeof(*pc), NULL);
		break;
	}
	case NPF_OPERAND_ETHER_TYPE:
		snprintf(buf, bufsiz, "ether=0x%x", *pc);
		ADVANCE(1, NULL);
		break;

	case NPF_OPERAND_SUBNET: {
		snprintf(buf, bufsiz, "/%d", *pc);
		if (nci) {
			npfctl_ncode_add_vp(buf, nci, nci->nci_srcdst ?
			    NPF_SHOW_SRCADDR : NPF_SHOW_DSTADDR);
		}
		ADVANCE(1, NULL);
		break;
	}
	case NPF_OPERAND_LENGTH:
		snprintf(buf, bufsiz, "length=%d", *pc);
		ADVANCE(1, NULL);
		break;

	case NPF_OPERAND_TABLE_ID:
		if (nci) {
			snprintf(buf, bufsiz, "<%d>", *pc);
			npfctl_ncode_add_vp(buf, nci, nci->nci_srcdst ?
			    NPF_SHOW_SRCADDR : NPF_SHOW_DSTADDR);
		}
		snprintf(buf, bufsiz, "id=%d", *pc);
		ADVANCE(1, NULL);
		break;

	case NPF_OPERAND_ICMP4_TYPE_CODE:
		if (*pc & ~0xffff) {
			warnx("invalid icmp/type operand 0x%x at offset %td",
			    *pc, pc - st);
			return NULL;
		}
		snprintf(buf, bufsiz, "type=%d, code=%d", *pc >> 8, *pc & 0xff);
		if (nci) {
			npfctl_ncode_add_vp(buf, nci, NPF_SHOW_ICMP);
		}
		ADVANCE(1, NULL);
		break;

	case NPF_OPERAND_TCP_FLAGS_MASK:
		if (*pc & ~0xffff) {
			warnx("invalid flags/mask operand 0x%x at offset %td",
			    *pc, pc - st);
			return NULL;
		}
		snprintf(buf, bufsiz, "type=%d, code=%d", *pc >> 8, *pc & 0xff);
		if (nci) {
			npfctl_ncode_add_vp(buf, nci, NPF_SHOW_TCPF);
		}
		ADVANCE(1, NULL);
		break;

	case NPF_OPERAND_PORT_RANGE: {
		in_port_t p1 = ntohs(*pc >> 16), p2 = ntohs(*pc & 0xffff);

		if (p1 == p2) {
			snprintf(buf, bufsiz, "%d", p1);
		} else {
			snprintf(buf, bufsiz, "%d-%d", p1, p2);
		}
		if (nci) {
			npfctl_ncode_add_vp(buf, nci, nci->nci_srcdst ?
			    NPF_SHOW_SRCPORT : NPF_SHOW_DSTPORT);
		}
		ADVANCE(1, NULL);
		break;
	}
	default:
		warnx("invalid operand %d at offset %td", op, pc - st);
		return NULL;
	}

	*pcv = pc;
	*lenv = len;
	return buf;
}

int
npfctl_ncode_disassemble(FILE *fp, const void *v, size_t len, nc_inf_t *nci)
{
	const uint32_t *ipc, *pc = v;
	const uint32_t *st = v;
	const struct npf_instruction *ni;
	char buf[256];
	const uint32_t **targ;
	size_t tlen, mlen, target;
	int error = -1;

	targ = NULL;
	mlen = tlen = 0;
	while (len) {
		/* Get the opcode */
		if (*pc & ~0xff) {
			warnx("invalid opcode 0x%x at offset (%td)", *pc,
			    pc - st);
			goto out;
		}
		ni = &npf_instructions[*pc];
		if (ni->name == NULL) {
			warnx("invalid opcode 0x%x at offset (%td)", *pc,
			    pc - st);
			goto out;
		}

		ipc = pc;
		target = npfctl_ncode_get_target(pc, targ, tlen);
		if (fp) {
			if (target != (size_t)~0)
				fprintf(fp, "%zu:", target);
			fprintf(fp, "\t%s", ni->name);
		}
		ADVANCE(1, -1);

		for (size_t i = 0; i < __arraycount(ni->op); i++) {
			const char *op;
			if (ni->op[i] == NPF_OPERAND_NONE)
				break;
			op = npfctl_ncode_operand(buf, sizeof(buf), ni->op[i],
			    st, ipc, &pc, &len, &targ, &tlen, &mlen, nci);
			if (op == NULL)
				goto out;
			if (fp) {
				fprintf(fp, "%s%s", i == 0 ? " " : ", ", op);
			}
		}
		if (fp) {
			fprintf(fp, "\n");
		}
	}
	error = 0;
out:
	free(targ);
	return error;
}

static void
npfctl_show_fromto(const char *name, npfvar_t *vl, bool showany)
{
	size_t count = npfvar_get_count(vl), last = count - 1;
	bool one = (count == 1);

	if (count == 0) {
		if (showany) {
			printf("%s all ", name);
		}
		return;
	}
	printf("%s%s ", name, one ? "" : " {");

	for (size_t i = 0; i < count; i++) {
		char *s = npfvar_get_data(vl, NPFVAR_STRING, i);
		printf("%s%s ", s, i == last ? (one ? "" : " }") : ",");
	}
	npfvar_destroy(vl);
}

static void
npfctl_show_ncode(const void *nc, size_t len)
{
	nc_inf_t nci;

	memset(&nci, 0, sizeof(nc_inf_t));

	if (npfctl_ncode_disassemble(NULL, nc, len, (void *)&nci) == 0) {
		npfctl_show_fromto("from", nci.nci_vlist[NPF_SHOW_SRCADDR], true);
		npfctl_show_fromto("port", nci.nci_vlist[NPF_SHOW_SRCPORT], false);
		npfctl_show_fromto("to", nci.nci_vlist[NPF_SHOW_DSTADDR], true);
		npfctl_show_fromto("port", nci.nci_vlist[NPF_SHOW_DSTPORT], false);
	} else {
		printf("<< ncode >> ");
	}
}

#define	NPF_RSTICMP		(NPF_RULE_RETRST | NPF_RULE_RETICMP)

static const struct attr_keyword_mapent {
	uint32_t	mask;
	uint32_t	flags;
	const char *	onmatch;
	const char *	nomatch;
} attr_keyword_map[] = {
	{ NPF_RULE_PASS,	NPF_RULE_PASS,	"pass",		"block"	},
	{ NPF_RSTICMP,		NPF_RSTICMP,	"return",	NULL	},
	{ NPF_RSTICMP,		NPF_RULE_RETRST,"return-rst",	NULL	},
	{ NPF_RSTICMP,		NPF_RULE_RETICMP,"return-icmp",	NULL	},
	{ NPF_RULE_KEEPSTATE,	NPF_RULE_KEEPSTATE,"stateful",	NULL	},
	{ NPF_RULE_DIMASK,	NPF_RULE_IN,	"in",		NULL	},
	{ NPF_RULE_DIMASK,	NPF_RULE_OUT,	"out",		NULL	},
	{ NPF_RULE_FINAL,	NPF_RULE_FINAL,	"final",	NULL	},
};

static void
npfctl_show_rule(nl_rule_t *nrl, unsigned nlevel)
{
	rule_group_t rg;
	const char *rproc;
	const void *nc;
	size_t nclen;

	_npf_rule_getinfo(nrl, &rg.rg_name, &rg.rg_attr, &rg.rg_ifnum);

	/* Get the interface, if any. */
	char ifnamebuf[IFNAMSIZ], *ifname = NULL;
	if (rg.rg_ifnum) {
		ifname = if_indextoname(rg.rg_ifnum, ifnamebuf);
	}

	/*
	 * If zero level, then it is a group.
	 */
	if (nlevel == 0) {
		static bool ingroup = false;
		const char *rname = rg.rg_name;

		if (ingroup) {
			puts("}");
		}
		ingroup = true;
		if (rg.rg_attr & NPF_RULE_DEFAULT) {
			puts("group (default) {");
			return;
		}
		printf("group (name \"%s\"", rname == NULL ? "" : rname);
		if (ifname) {
			printf(", interface %s", ifname);
		}
		puts(") {");
		return;
	}

	/*
	 * Rule case.  First, unparse the attributes.
	 */
	while (nlevel--) {
		printf("\t");
	}
	for (unsigned i = 0; i < __arraycount(attr_keyword_map); i++) {
		const struct attr_keyword_mapent *ak = &attr_keyword_map[i];

		if ((rg.rg_attr & ak->mask) == ak->flags) {
			printf("%s ", ak->onmatch);
		} else if (ak->nomatch) {
			printf("%s ", ak->nomatch);
		}
	}

	if (ifname) {
		printf("on %s ", ifname);
	}

	nc = _npf_rule_ncode(nrl, &nclen);
	if (nc) {
		npfctl_show_ncode(nc, nclen);
	} else {
		printf("all ");
	}
	/* apply <rproc> */

	if ((rproc = _npf_rule_rproc(nrl)) != NULL) {
		printf("apply \"%s\"", rproc);
	}
	puts("");
}

int
npfctl_config_show(int fd)
{
	nl_config_t *ncf;
	bool active, loaded;
	int error = 0;

	ncf = npf_config_retrieve(fd, &active, &loaded);
	if (ncf == NULL) {
		return errno;
	}
	printf("Filtering:\t%s\nConfiguration:\t%s\n\n",
	    active ? "active" : "inactive",
	    loaded ? "loaded" : "empty");

	if (loaded) {
		error = _npf_rule_foreach(ncf, npfctl_show_rule);
		puts("}");
	}
	npf_config_destroy(ncf);
	return error;
}
