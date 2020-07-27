/*	$NetBSD: aes_impl.c,v 1.9 2020/07/27 20:45:15 riastradh Exp $	*/

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
__KERNEL_RCSID(1, "$NetBSD: aes_impl.c,v 1.9 2020/07/27 20:45:15 riastradh Exp $");

#include <sys/types.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/once.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#include <crypto/aes/aes.h>
#include <crypto/aes/aes_cbc.h>
#include <crypto/aes/aes_bear.h> /* default implementation */
#include <crypto/aes/aes_impl.h>
#include <crypto/aes/aes_xts.h>

static int aes_selftest_stdkeysched(void);

static const struct aes_impl	*aes_md_impl	__read_mostly;
static const struct aes_impl	*aes_impl	__read_mostly;

static int
sysctl_kern_crypto_aes_selected(SYSCTLFN_ARGS)
{
	struct sysctlnode node;

	KASSERTMSG(aes_impl != NULL,
	    "sysctl ran before AES implementation was selected");

	node = *rnode;
	node.sysctl_data = __UNCONST(aes_impl->ai_name);
	node.sysctl_size = strlen(aes_impl->ai_name) + 1;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

SYSCTL_SETUP(sysctl_kern_crypto_aes_setup, "sysctl kern.crypto.aes setup")
{
	const struct sysctlnode *cnode;
	const struct sysctlnode *aes_node;

	sysctl_createv(clog, 0, NULL, &cnode, 0, CTLTYPE_NODE, "crypto",
	    SYSCTL_DESCR("Kernel cryptography"),
	    NULL, 0, NULL, 0,
	    CTL_KERN, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &cnode, &aes_node, 0, CTLTYPE_NODE, "aes",
	    SYSCTL_DESCR("AES -- Advanced Encryption Standard"),
	    NULL, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &aes_node, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY, CTLTYPE_STRING, "selected",
	    SYSCTL_DESCR("Selected AES implementation"),
	    sysctl_kern_crypto_aes_selected, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);
}

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

	if (aes_selftest_stdkeysched())
		panic("AES is busted");

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

	aprint_verbose("aes: %s\n", aes_impl->ai_name);
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

void
aes_cbcmac_update1(const struct aesenc *enc, const uint8_t in[static 16],
    size_t nbytes, uint8_t auth[static 16], uint32_t nrounds)
{

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	aes_guarantee_selected();
	aes_impl->ai_cbcmac_update1(enc, in, nbytes, auth, nrounds);
}

void
aes_ccm_enc1(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t authctr[static 32],
    uint32_t nrounds)
{

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	aes_guarantee_selected();
	aes_impl->ai_ccm_enc1(enc, in, out, nbytes, authctr, nrounds);
}

void
aes_ccm_dec1(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t authctr[static 32],
    uint32_t nrounds)
{

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	aes_guarantee_selected();
	aes_impl->ai_ccm_dec1(enc, in, out, nbytes, authctr, nrounds);
}

/*
 * Known-answer self-tests for the standard key schedule.
 */
static int
aes_selftest_stdkeysched(void)
{
	static const uint8_t key[32] = {
		0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
		0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
		0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
		0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
	};
	static const uint32_t rk128enc[] = {
		0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c,
		0xfd74aad6, 0xfa72afd2, 0xf178a6da, 0xfe76abd6,
		0x0bcf92b6, 0xf1bd3d64, 0x00c59bbe, 0xfeb33068,
		0x4e74ffb6, 0xbfc9c2d2, 0xbf0c596c, 0x41bf6904,
		0xbcf7f747, 0x033e3595, 0xbc326cf9, 0xfd8d05fd,
		0xe8a3aa3c, 0xeb9d9fa9, 0x57aff350, 0xaa22f6ad,
		0x7d0f395e, 0x9692a6f7, 0xc13d55a7, 0x6b1fa30a,
		0x1a70f914, 0x8ce25fe3, 0x4ddf0a44, 0x26c0a94e,
		0x35874347, 0xb9651ca4, 0xf4ba16e0, 0xd27abfae,
		0xd1329954, 0x685785f0, 0x9ced9310, 0x4e972cbe,
		0x7f1d1113, 0x174a94e3, 0x8ba707f3, 0xc5302b4d,
	};
	static const uint32_t rk192enc[] = {
		0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c,
		0x13121110, 0x17161514, 0xf9f24658, 0xfef4435c,
		0xf5fe4a54, 0xfaf04758, 0xe9e25648, 0xfef4435c,
		0xb349f940, 0x4dbdba1c, 0xb843f048, 0x42b3b710,
		0xab51e158, 0x55a5a204, 0x41b5ff7e, 0x0c084562,
		0xb44bb52a, 0xf6f8023a, 0x5da9e362, 0x080c4166,
		0x728501f5, 0x7e8d4497, 0xcac6f1bd, 0x3c3ef387,
		0x619710e5, 0x699b5183, 0x9e7c1534, 0xe0f151a3,
		0x2a37a01e, 0x16095399, 0x779e437c, 0x1e0512ff,
		0x880e7edd, 0x68ff2f7e, 0x42c88f60, 0x54c1dcf9,
		0x235f9f85, 0x3d5a8d7a, 0x5229c0c0, 0x3ad6efbe,
		0x781e60de, 0x2cdfbc27, 0x0f8023a2, 0x32daaed8,
		0x330a97a4, 0x09dc781a, 0x71c218c4, 0x5d1da4e3,
	};
	static const uint32_t rk256enc[] = {
		0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c,
		0x13121110, 0x17161514, 0x1b1a1918, 0x1f1e1d1c,
		0x9fc273a5, 0x98c476a1, 0x93ce7fa9, 0x9cc072a5,
		0xcda85116, 0xdabe4402, 0xc1a45d1a, 0xdeba4006,
		0xf0df87ae, 0x681bf10f, 0xfbd58ea6, 0x6715fc03,
		0x48f1e16d, 0x924fa56f, 0x53ebf875, 0x8d51b873,
		0x7f8256c6, 0x1799a7c9, 0xec4c296f, 0x8b59d56c,
		0x753ae23d, 0xe7754752, 0xb49ebf27, 0x39cf0754,
		0x5f90dc0b, 0x48097bc2, 0xa44552ad, 0x2f1c87c1,
		0x60a6f545, 0x87d3b217, 0x334d0d30, 0x0a820a64,
		0x1cf7cf7c, 0x54feb4be, 0xf0bbe613, 0xdfa761d2,
		0xfefa1af0, 0x7929a8e7, 0x4a64a5d7, 0x40e6afb3,
		0x71fe4125, 0x2500f59b, 0xd5bb1388, 0x0a1c725a,
		0x99665a4e, 0xe04ff2a9, 0xaa2b577e, 0xeacdf8cd,
		0xcc79fc24, 0xe97909bf, 0x3cc21a37, 0x36de686d,
	};
	static const uint32_t rk128dec[] = {
		0x7f1d1113, 0x174a94e3, 0x8ba707f3, 0xc5302b4d,
		0xbe29aa13, 0xf6af8f9c, 0x80f570f7, 0x03bff700,
		0x63a46213, 0x4886258f, 0x765aff6b, 0x834a87f7,
		0x74fc828d, 0x2b22479c, 0x3edcdae4, 0xf510789c,
		0x8d09e372, 0x5fdec511, 0x15fe9d78, 0xcbcca278,
		0x2710c42e, 0xd2d72663, 0x4a205869, 0xde323f00,
		0x04f5a2a8, 0xf5c7e24d, 0x98f77e0a, 0x94126769,
		0x91e3c6c7, 0xf13240e5, 0x6d309c47, 0x0ce51963,
		0x9902dba0, 0x60d18622, 0x9c02dca2, 0x61d58524,
		0xf0df568c, 0xf9d35d82, 0xfcd35a80, 0xfdd75986,
		0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c,
	};
	static const uint32_t rk192dec[] = {
		0x330a97a4, 0x09dc781a, 0x71c218c4, 0x5d1da4e3,
		0x0dbdbed6, 0x49ea09c2, 0x8073b04d, 0xb91b023e,
		0xc999b98f, 0x3968b273, 0x9dd8f9c7, 0x728cc685,
		0xc16e7df7, 0xef543f42, 0x7f317853, 0x4457b714,
		0x90654711, 0x3b66cf47, 0x8dce0e9b, 0xf0f10bfc,
		0xb6a8c1dc, 0x7d3f0567, 0x4a195ccc, 0x2e3a42b5,
		0xabb0dec6, 0x64231e79, 0xbe5f05a4, 0xab038856,
		0xda7c1bdd, 0x155c8df2, 0x1dab498a, 0xcb97c4bb,
		0x08f7c478, 0xd63c8d31, 0x01b75596, 0xcf93c0bf,
		0x10efdc60, 0xce249529, 0x15efdb62, 0xcf20962f,
		0xdbcb4e4b, 0xdacf4d4d, 0xc7d75257, 0xdecb4949,
		0x1d181f1a, 0x191c1b1e, 0xd7c74247, 0xdecb4949,
		0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c,
	};
	static const uint32_t rk256dec[] = {
		0xcc79fc24, 0xe97909bf, 0x3cc21a37, 0x36de686d,
		0xffd1f134, 0x2faacebf, 0x5fe2e9fc, 0x6e015825,
		0xeb48165e, 0x0a354c38, 0x46b77175, 0x84e680dc,
		0x8005a3c8, 0xd07b3f8b, 0x70482743, 0x31e3b1d9,
		0x138e70b5, 0xe17d5a66, 0x4c823d4d, 0xc251f1a9,
		0xa37bda74, 0x507e9c43, 0xa03318c8, 0x41ab969a,
		0x1597a63c, 0xf2f32ad3, 0xadff672b, 0x8ed3cce4,
		0xf3c45ff8, 0xf3054637, 0xf04d848b, 0xe1988e52,
		0x9a4069de, 0xe7648cef, 0x5f0c4df8, 0x232cabcf,
		0x1658d5ae, 0x00c119cf, 0x0348c2bc, 0x11d50ad9,
		0xbd68c615, 0x7d24e531, 0xb868c117, 0x7c20e637,
		0x0f85d77f, 0x1699cc61, 0x0389db73, 0x129dc865,
		0xc940282a, 0xc04c2324, 0xc54c2426, 0xc4482720,
		0x1d181f1a, 0x191c1b1e, 0x15101712, 0x11141316,
		0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c,
	};
	static const struct {
		unsigned	len;
		unsigned	nr;
		const uint32_t	*enc, *dec;
	} C[] = {
		{ 16, AES_128_NROUNDS, rk128enc, rk128dec },
		{ 24, AES_192_NROUNDS, rk192enc, rk192dec },
		{ 32, AES_256_NROUNDS, rk256enc, rk256dec },
	};
	uint32_t rk[60];
	unsigned i;

	for (i = 0; i < __arraycount(C); i++) {
		if (br_aes_ct_keysched_stdenc(rk, key, C[i].len) != C[i].nr)
			return -1;
		if (memcmp(rk, C[i].enc, 4*(C[i].nr + 1)))
			return -1;
		if (br_aes_ct_keysched_stddec(rk, key, C[i].len) != C[i].nr)
			return -1;
		if (memcmp(rk, C[i].dec, 4*(C[i].nr + 1)))
			return -1;
	}

	return 0;
}
