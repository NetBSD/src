/*	$NetBSD: chacha_impl.c,v 1.1 2020/07/25 22:46:34 riastradh Exp $	*/

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

static const struct chacha_impl	*chacha_md_impl	__read_mostly;
static const struct chacha_impl	*chacha_impl	__read_mostly;

static int
sysctl_hw_chacha_impl(SYSCTLFN_ARGS)
{
	struct sysctlnode node;

	KASSERTMSG(chacha_impl != NULL,
	    "sysctl ran before ChaCha implementation was selected");

	node = *rnode;
	node.sysctl_data = __UNCONST(chacha_impl->ci_name);
	node.sysctl_size = strlen(chacha_impl->ci_name) + 1;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

SYSCTL_SETUP(sysctl_hw_chacha_setup, "sysctl hw.chacha_impl setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY, CTLTYPE_STRING, "chacha_impl",
	    SYSCTL_DESCR("Selected ChaCha implementation"),
	    sysctl_hw_chacha_impl, 0, NULL, 0,
	    CTL_HW, CTL_CREATE, CTL_EOL);
}

static int
chacha_select(void)
{

	KASSERT(chacha_impl == NULL);

	if (chacha_md_impl) {
		if (chacha_selftest(chacha_md_impl))
			aprint_error("chacha: self-test failed: %s\n",
			    chacha_md_impl->ci_name);
		else
			chacha_impl = chacha_md_impl;
	}
	if (chacha_impl == NULL) {
		if (chacha_selftest(&chacha_ref_impl))
			aprint_error("chacha: self-test failed: %s\n",
			    chacha_ref_impl.ci_name);
		else
			chacha_impl = &chacha_ref_impl;
	}
	if (chacha_impl == NULL)
		panic("ChaCha self-tests failed");

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

static void
chacha_guarantee_selected(void)
{
#if 0
	static once_t once;
	int error;

	error = RUN_ONCE(&once, chacha_select);
	KASSERT(error == 0);
#endif
}

void
chacha_md_init(const struct chacha_impl *impl)
{

	KASSERT(cold);
	KASSERTMSG(chacha_impl == NULL,
	    "ChaCha implementation `%s' already chosen, can't offer `%s'",
	    chacha_impl->ci_name, impl->ci_name);
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

	chacha_guarantee_selected();
	(*chacha_impl->ci_chacha_core)(out, in, k, c, nr);
}

void
hchacha(uint8_t out[restrict static HCHACHA_OUTBYTES],
    const uint8_t in[static HCHACHA_INBYTES],
    const uint8_t k[static HCHACHA_KEYBYTES],
    const uint8_t c[static HCHACHA_CONSTBYTES],
    unsigned nr)
{

	chacha_guarantee_selected();
	(*chacha_impl->ci_hchacha)(out, in, k, c, nr);
}

void
chacha_stream(uint8_t *restrict s, size_t nbytes, uint32_t blkno,
    const uint8_t nonce[static CHACHA_STREAM_NONCEBYTES],
    const uint8_t key[static CHACHA_STREAM_KEYBYTES],
    unsigned nr)
{

	chacha_guarantee_selected();
	(*chacha_impl->ci_chacha_stream)(s, nbytes, blkno, nonce, key, nr);
}

void
chacha_stream_xor(uint8_t *c, const uint8_t *p, size_t nbytes, uint32_t blkno,
    const uint8_t nonce[static CHACHA_STREAM_NONCEBYTES],
    const uint8_t key[static CHACHA_STREAM_KEYBYTES],
    unsigned nr)
{

	chacha_guarantee_selected();
	(*chacha_impl->ci_chacha_stream_xor)(c, p, nbytes, blkno, nonce, key,
	    nr);
}

void
xchacha_stream(uint8_t *restrict s, size_t nbytes, uint32_t blkno,
    const uint8_t nonce[static XCHACHA_STREAM_NONCEBYTES],
    const uint8_t key[static XCHACHA_STREAM_KEYBYTES],
    unsigned nr)
{

	chacha_guarantee_selected();
	(*chacha_impl->ci_xchacha_stream)(s, nbytes, blkno, nonce, key, nr);
}

void
xchacha_stream_xor(uint8_t *c, const uint8_t *p, size_t nbytes, uint32_t blkno,
    const uint8_t nonce[static XCHACHA_STREAM_NONCEBYTES],
    const uint8_t key[static XCHACHA_STREAM_KEYBYTES],
    unsigned nr)
{

	chacha_guarantee_selected();
	(*chacha_impl->ci_xchacha_stream_xor)(c, p, nbytes, blkno, nonce, key,
	    nr);
}
