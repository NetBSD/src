/*	$NetBSD: aes_impl.c,v 1.1 2020/06/29 23:27:52 riastradh Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: aes_impl.c,v 1.1 2020/06/29 23:27:52 riastradh Exp $");

#include <sys/types.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/once.h>
#include <sys/systm.h>

#include <crypto/aes/aes.h>
#include <crypto/aes/aes_bear.h> /* default implementation */

static const struct aes_impl	*aes_md_impl	__read_mostly;
static const struct aes_impl	*aes_impl	__read_mostly;

/*
 * The timing of AES implementation selection is finicky:
 *
 *	1. It has to be done _after_ cpu_attach for implementations,
 *	   such as AES-NI, that rely on fpu initialization done by
 *	   fpu_attach.
 *
 *	2. It has to be done _before_ the cgd self-tests or anything
 *	   else that might call AES.
 *
 * For the moment, doing it in module init works.  However, if a
 * driver-class module depended on the aes module, that would break.
 */

static int
aes_select(void)
{

	KASSERT(aes_impl == NULL);

	if (aes_md_impl) {
		if (aes_selftest(aes_md_impl))
			aprint_error("aes: self-test failed: %s\n",
			    aes_md_impl->ai_name);
		else
			aes_impl = aes_md_impl;
	}
	if (aes_impl == NULL) {
		if (aes_selftest(&aes_bear_impl))
			aprint_error("aes: self-test failed: %s\n",
			    aes_bear_impl.ai_name);
		else
			aes_impl = &aes_bear_impl;
	}
	if (aes_impl == NULL)
		panic("AES self-tests failed");

	aprint_normal("aes: %s\n", aes_impl->ai_name);
	return 0;
}

MODULE(MODULE_CLASS_MISC, aes, NULL);

static int
aes_modcmd(modcmd_t cmd, void *opaque)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return aes_select();
	case MODULE_CMD_FINI:
		return 0;
	default:
		return ENOTTY;
	}
}

static void
aes_guarantee_selected(void)
{
#if 0
	static once_t once;
	int error;

	error = RUN_ONCE(&once, aes_select);
	KASSERT(error == 0);
#endif
}

void
aes_md_init(const struct aes_impl *impl)
{

	KASSERT(cold);
	KASSERTMSG(aes_impl == NULL,
	    "AES implementation `%s' already chosen, can't offer `%s'",
	    aes_impl->ai_name, impl->ai_name);
	KASSERTMSG(aes_md_impl == NULL,
	    "AES implementation `%s' already offered, can't offer `%s'",
	    aes_md_impl->ai_name, impl->ai_name);

	aes_md_impl = impl;
}

static void
aes_setenckey(struct aesenc *enc, const uint8_t key[static 16],
    uint32_t nrounds)
{

	aes_guarantee_selected();
	aes_impl->ai_setenckey(enc, key, nrounds);
}

uint32_t
aes_setenckey128(struct aesenc *enc, const uint8_t key[static 16])
{
	uint32_t nrounds = AES_128_NROUNDS;

	aes_setenckey(enc, key, nrounds);
	return nrounds;
}

uint32_t
aes_setenckey192(struct aesenc *enc, const uint8_t key[static 24])
{
	uint32_t nrounds = AES_192_NROUNDS;

	aes_setenckey(enc, key, nrounds);
	return nrounds;
}

uint32_t
aes_setenckey256(struct aesenc *enc, const uint8_t key[static 32])
{
	uint32_t nrounds = AES_256_NROUNDS;

	aes_setenckey(enc, key, nrounds);
	return nrounds;
}

static void
aes_setdeckey(struct aesdec *dec, const uint8_t key[static 16],
    uint32_t nrounds)
{

	aes_guarantee_selected();
	aes_impl->ai_setdeckey(dec, key, nrounds);
}

uint32_t
aes_setdeckey128(struct aesdec *dec, const uint8_t key[static 16])
{
	uint32_t nrounds = AES_128_NROUNDS;

	aes_setdeckey(dec, key, nrounds);
	return nrounds;
}

uint32_t
aes_setdeckey192(struct aesdec *dec, const uint8_t key[static 24])
{
	uint32_t nrounds = AES_192_NROUNDS;

	aes_setdeckey(dec, key, nrounds);
	return nrounds;
}

uint32_t
aes_setdeckey256(struct aesdec *dec, const uint8_t key[static 32])
{
	uint32_t nrounds = AES_256_NROUNDS;

	aes_setdeckey(dec, key, nrounds);
	return nrounds;
}

void
aes_enc(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], uint32_t nrounds)
{

	aes_guarantee_selected();
	aes_impl->ai_enc(enc, in, out, nrounds);
}

void
aes_dec(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], uint32_t nrounds)
{

	aes_guarantee_selected();
	aes_impl->ai_dec(dec, in, out, nrounds);
}

void
aes_cbc_enc(struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t iv[static 16],
    uint32_t nrounds)
{

	aes_guarantee_selected();
	aes_impl->ai_cbc_enc(enc, in, out, nbytes, iv, nrounds);
}

void
aes_cbc_dec(struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t iv[static 16],
    uint32_t nrounds)
{

	aes_guarantee_selected();
	aes_impl->ai_cbc_dec(dec, in, out, nbytes, iv, nrounds);
}

void
aes_xts_enc(struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t tweak[static 16],
    uint32_t nrounds)
{

	aes_guarantee_selected();
	aes_impl->ai_xts_enc(enc, in, out, nbytes, tweak, nrounds);
}

void
aes_xts_dec(struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t tweak[static 16],
    uint32_t nrounds)
{

	aes_guarantee_selected();
	aes_impl->ai_xts_dec(dec, in, out, nbytes, tweak, nrounds);
}
