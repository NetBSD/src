/*	$NetBSD: bpf_stub.c,v 1.3 2010/01/19 23:11:10 pooka Exp $	*/

/*
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bpf_stub.c,v 1.3 2010/01/19 23:11:10 pooka Exp $");

#include <sys/param.h>
#include <sys/mbuf.h>

#include <net/bpf.h>

static void
bpf_stub_attach(struct ifnet *ipf, u_int dlt, u_int hlen, struct bpf_if **drvp)
{

	*drvp = NULL;
}

static void
bpf_stub_null(void)
{

}

static void
bpf_stub_warn(void)
{

#ifdef DEBUG
	panic("bpf method called without attached bpf_if");
#endif
#ifdef DIAGNOSTIC
	printf("bpf method called without attached bpf_if\n");
#endif
}

struct bpf_ops bpf_ops_stub = {
	.bpf_attach =		bpf_stub_attach,
	.bpf_detach =		(void *)bpf_stub_null,
	.bpf_change_type =	(void *)bpf_stub_null,

	.bpf_tap = 		(void *)bpf_stub_warn,
	.bpf_mtap = 		(void *)bpf_stub_warn,
	.bpf_mtap2 = 		(void *)bpf_stub_warn,
	.bpf_mtap_af = 		(void *)bpf_stub_warn,
	.bpf_mtap_et = 		(void *)bpf_stub_warn,
	.bpf_mtap_sl_in = 	(void *)bpf_stub_warn,
	.bpf_mtap_sl_out =	(void *)bpf_stub_warn,
};

struct bpf_ops *bpf_ops;

void bpf_setops_stub(void);
void
bpf_setops_stub()
{

	bpf_ops = &bpf_ops_stub;
}
__weak_alias(bpf_setops,bpf_setops_stub);
