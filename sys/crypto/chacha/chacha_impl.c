/*	$NetBSD: chacha_impl.c,v 1.3 2020/07/27 20:49:10 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/module.h>
#include <sys/once.h>
#include <sys/sysctl.h>

#include <lib/libkern/libkern.h>

#include "chacha.h"
#include "chacha_ref.h"

static const struct chacha_impl	*chacha_md_impl __read_mostly;
static const struct chacha_impl	*chacha_impl __read_mostly = &chacha_ref_impl;

static int
sysctl_kern_crypto_chacha_selected(SYSCTLFN_ARGS)
{
	struct sysctlnode node;

	node = *rnode;
	node.sysctl_data = __UNCONST(chacha_impl->ci_name);
	node.sysctl_size = strlen(chacha_impl->ci_name) + 1;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

SYSCTL_SETUP(sysctl_kern_crypto_chacha_setup, "sysctl kern.crypto.chacha setup")
{
	const struct sysctlnode *cnode;
	const struct sysctlnode *chacha_node;

	sysctl_createv(clog, 0, NULL, &cnode, 0, CTLTYPE_NODE, "crypto",
	    SYSCTL_DESCR("Kernel cryptography"),
	    NULL, 0, NULL, 0,
	    CTL_KERN, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &cnode, &chacha_node, 0, CTLTYPE_NODE, "chacha",
	    SYSCTL_DESCR("ChaCha"),
	    NULL, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &chacha_node, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY, CTLTYPE_STRING, "selected",
	    SYSCTL_DESCR("Selected ChaCha implementation"),
	    sysctl_kern_crypto_chacha_selected, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);
}

static int
chacha_select(void)
{

	if (chacha_md_impl) {
		if (chacha_selftest(chacha_md_impl))
			aprint_error("chacha: self-test failed: %s\n",
			    chacha_md_impl->ci_name);
		else
			chacha_impl = chacha_md_impl;
	}

	aprint_verbose("chacha: %s\n", chacha_impl->ci_name);
	return 0;
}

MODULE(MODULE_CLASS_MISC, chacha, NULL);

static int
chacha_modcmd(modcmd_t cmd, void *opaque)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return chacha_select();
	case MODULE_CMD_FINI:
		return 0;
	default:
		return ENOTTY;
	}
}

void
chacha_md_init(const struct chacha_impl *impl)
{

	KASSERT(cold);
	KASSERTMSG(chacha_md_impl == NULL,
	    "ChaCha implementation `%s' already offered, can't offer `%s'",
	    chacha_md_impl->ci_name, impl->ci_name);

	chacha_md_impl = impl;
}

void
chacha_core(uint8_t out[restrict static CHACHA_CORE_OUTBYTES],
    const uint8_t in[static CHACHA_CORE_INBYTES],
    const uint8_t k[static CHACHA_CORE_KEYBYTES],
    const uint8_t c[static CHACHA_CORE_CONSTBYTES],
    unsigned nr)
{

	(*chacha_impl->ci_chacha_core)(out, in, k, c, nr);
}

void
hchacha(uint8_t out[restrict static HCHACHA_OUTBYTES],
    const uint8_t in[static HCHACHA_INBYTES],
    const uint8_t k[static HCHACHA_KEYBYTES],
    const uint8_t c[static HCHACHA_CONSTBYTES],
    unsigned nr)
{

	(*chacha_impl->ci_hchacha)(out, in, k, c, nr);
}

void
chacha_stream(uint8_t *restrict s, size_t nbytes, uint32_t blkno,
    const uint8_t nonce[static CHACHA_STREAM_NONCEBYTES],
    const uint8_t key[static CHACHA_STREAM_KEYBYTES],
    unsigned nr)
{

	(*chacha_impl->ci_chacha_stream)(s, nbytes, blkno, nonce, key, nr);
}

void
chacha_stream_xor(uint8_t *c, const uint8_t *p, size_t nbytes, uint32_t blkno,
    const uint8_t nonce[static CHACHA_STREAM_NONCEBYTES],
    const uint8_t key[static CHACHA_STREAM_KEYBYTES],
    unsigned nr)
{

	(*chacha_impl->ci_chacha_stream_xor)(c, p, nbytes, blkno, nonce, key,
	    nr);
}

void
xchacha_stream(uint8_t *restrict s, size_t nbytes, uint32_t blkno,
    const uint8_t nonce[static XCHACHA_STREAM_NONCEBYTES],
    const uint8_t key[static XCHACHA_STREAM_KEYBYTES],
    unsigned nr)
{

	(*chacha_impl->ci_xchacha_stream)(s, nbytes, blkno, nonce, key, nr);
}

void
xchacha_stream_xor(uint8_t *c, const uint8_t *p, size_t nbytes, uint32_t blkno,
    const uint8_t nonce[static XCHACHA_STREAM_NONCEBYTES],
    const uint8_t key[static XCHACHA_STREAM_KEYBYTES],
    unsigned nr)
{

	(*chacha_impl->ci_xchacha_stream_xor)(c, p, nbytes, blkno, nonce, key,
	    nr);
}
