/*	$NetBSD: npf_disassemble.c,v 1.3.4.2 2012/04/17 00:09:50 yamt Exp $	*/

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
__RCSID("$NetBSD: npf_disassemble.c,v 1.3.4.2 2012/04/17 00:09:50 yamt Exp $");

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <util.h>

#define NPF_OPCODES_STRINGS
#include <net/npf_ncode.h>

#include "npfctl.h"

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

static const char *
npfctl_ncode_operand(char *buf, size_t bufsiz, uint8_t op, const uint32_t *st,
    const uint32_t *ipc, const uint32_t **pcv, size_t *lenv,
    const uint32_t ***t, size_t *l, size_t *m)
{
	const uint32_t *pc = *pcv;
	size_t len = *lenv;
	struct sockaddr_storage ss;

	switch (op) {
	case NPF_OPERAND_NONE:
		abort();

	case NPF_OPERAND_REGISTER:
		if (*pc & ~0x3) {
			warnx("Bad register operand 0x%x at offset %td",
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
			warnx("Bad src/dst operand 0x%x at offset %td",
			    *pc, pc - st);
			return NULL;
		}
		snprintf(buf, bufsiz, "%s", *pc == NPF_OPERAND_SD_SRC ?
		    "SRC" : "DST");
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
		sockaddr_snprintf(buf, bufsiz, "%a", (struct sockaddr *)(void *)
		    sin);
		ADVANCE(sizeof(sin->sin_addr) / sizeof(*pc), NULL);
		break;
	}
	case NPF_OPERAND_NET_ADDRESS6: {
		struct sockaddr_in6 *sin6 = (void *)&ss;
		sin6->sin6_len = sizeof(*sin6);
		sin6->sin6_family = AF_INET6;
		sin6->sin6_port = 0;
		memcpy(&sin6->sin6_addr, pc, sizeof(sin6->sin6_addr));
		sockaddr_snprintf(buf, bufsiz, "%a", (struct sockaddr *)(void *)
		    sin6);
		ADVANCE(sizeof(sin6->sin6_addr) / sizeof(*pc), NULL);
		break;
	}
	case NPF_OPERAND_ETHER_TYPE:
		snprintf(buf, bufsiz, "ether=0x%x", *pc);
		ADVANCE(1, NULL);
		break;

	case NPF_OPERAND_SUBNET:
		snprintf(buf, bufsiz, "/%d", *pc);
		ADVANCE(1, NULL);
		break;

	case NPF_OPERAND_LENGTH:
		snprintf(buf, bufsiz, "length=%d", *pc);
		ADVANCE(1, NULL);
		break;

	case NPF_OPERAND_TABLE_ID:
		snprintf(buf, bufsiz, "id=%d", *pc);
		ADVANCE(1, NULL);
		break;

	case NPF_OPERAND_ICMP4_TYPE_CODE:
		if (*pc & ~0xffff) {
			warnx("Bad icmp/type operand 0x%x at offset %td",
			    *pc, pc - st);
			return NULL;
		}
		snprintf(buf, bufsiz, "type=%d, code=%d", *pc >> 8,
		    *pc & 0xff);
		ADVANCE(1, NULL);
		break;

	case NPF_OPERAND_TCP_FLAGS_MASK:
		if (*pc & ~0xffff) {
			warnx("Bad flags/mask operand 0x%x at offset %td",
			    *pc, pc - st);
			return NULL;
		}
		snprintf(buf, bufsiz, "type=%d, code=%d", *pc >> 8,
		    *pc & 0xff);
		ADVANCE(1, NULL);
		break;

	case NPF_OPERAND_PORT_RANGE:
		snprintf(buf, bufsiz, "%d-%d", (*pc >> 16), *pc & 0xffff);
		ADVANCE(1, NULL);
		break;
	default:
		warnx("invalid operand %d at offset %td", op, pc - st);
		return NULL;
	}

	*pcv = pc;
	*lenv = len;
	return buf;
}

int
npfctl_ncode_disassemble(FILE *fp, const void *v, size_t len)
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
			warnx("bad opcode 0x%x at offset (%td)", *pc,
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
		if (target != (size_t)~0)
			printf("%zu:", target);
		fprintf(fp, "\t%s", ni->name);
		ADVANCE(1, -1);
		for (size_t i = 0; i < __arraycount(ni->op); i++) {
			const char *op;
			if (ni->op[i] == NPF_OPERAND_NONE)
				break;
			op = npfctl_ncode_operand(buf, sizeof(buf), ni->op[i],
			    st, ipc, &pc, &len, &targ, &tlen, &mlen);
			if (op == NULL)
				goto out;
			fprintf(fp, "%s%s", i == 0 ? " " : ", ", op);
		}
		fprintf(fp, "\n");
	}
	error = 0;
out:
	free(targ);
	return error;
}
