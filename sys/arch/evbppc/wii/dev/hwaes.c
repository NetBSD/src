/* $NetBSD: hwaes.c,v 1.1.2.2 2025/11/14 13:16:33 martin Exp $ */

/*-
 * Copyright (c) 2025 Jared McNeill <jmcneill@invisible.ca>
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

/*
 * A driver for the Nintendo Wii's AES engine. The driver registers an AES
 * implementation for kernel use via aes_md_init(). AES-128 requests are
 * accelerated by hardware and all other requests are passed through to the
 * default (BearSSL aes_ct) implementation.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hwaes.c,v 1.1.2.2 2025/11/14 13:16:33 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/buf.h>

#include <machine/wii.h>
#include <machine/pio.h>
#include "hollywood.h"

#include <crypto/aes/aes.h>
#include <crypto/aes/aes_bear.h>
#include <crypto/aes/aes_impl.h>

/* AES engine registers */
#define AES_CTRL		0x00
#define  AES_CTRL_EXEC		__BIT(31)
#define  AES_CTRL_IRQ		__BIT(30)
#define  AES_CTRL_ERR		__BIT(29)
#define  AES_CTRL_ENA		__BIT(28)
#define  AES_CTRL_DEC		__BIT(27)
#define  AES_CTRL_IV		__BIT(12)
#define  AES_CTRL_BLOCKS	__BITS(11, 0)
#define AES_SRC			0x04
#define AES_DEST		0x08
#define AES_KEY			0x0c
#define AES_IV			0x10

/* Register frame size */
#define AES_REG_SIZE		0x14

/* Device limits */
#define HWAES_BLOCK_LEN		16
#define HWAES_ALIGN		16
#define HWAES_MAX_BLOCKS	4096
#define HWAES_MAX_AES_LEN	(HWAES_BLOCK_LEN * HWAES_MAX_BLOCKS)

static int	hwaes_match(device_t, cfdata_t, void *);
static void	hwaes_attach(device_t, device_t, void *);

struct hwaes_softc;

struct hwaes_dma {
	bus_dmamap_t		dma_map;
	void			*dma_addr;
	size_t			dma_size;
	bus_dma_segment_t	dma_segs[1];
};

struct hwaes_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_dma_tag_t		sc_dmat;
	struct hwaes_dma	sc_dma_bounce;
};

struct hwaes_softc *hwaes_sc;

#define WR4(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define RD4(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))

CFATTACH_DECL_NEW(hwaes, sizeof(struct hwaes_softc),
    hwaes_match, hwaes_attach, NULL, NULL);

static int	hwaes_dma_alloc(struct hwaes_softc *, struct hwaes_dma *,
				size_t, int);
static void	hwaes_register(void);

static int
hwaes_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
hwaes_attach(device_t parent, device_t self, void *aux)
{
	struct hollywood_attach_args *haa = aux;
	struct hwaes_softc *sc = device_private(self);
	int error;

	sc->sc_dev = self;
	sc->sc_dmat = haa->haa_dmat;
	sc->sc_bst = haa->haa_bst;
	error = bus_space_map(sc->sc_bst, haa->haa_addr, AES_REG_SIZE,
	    0, &sc->sc_bsh);
	if (error != 0) {
		aprint_error(": couldn't map registers (%d)\n", error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": AES engine\n");

	hollywood_claim_device(self, IOPAESEN);

	error = hwaes_dma_alloc(sc, &sc->sc_dma_bounce, HWAES_MAX_AES_LEN,
	    BUS_DMA_WAITOK);
	if (error != 0) {
		return;
	}

	WR4(sc, AES_CTRL, 0);
	for (;;) {
		if (RD4(sc, AES_CTRL) == 0) {
			break;
		}
	}

	hwaes_sc = sc;
	hwaes_register();
}

static int
hwaes_dma_alloc(struct hwaes_softc *sc, struct hwaes_dma *dma, size_t size,
    int flags)
{
	int error, nsegs;

	dma->dma_size = size;

	error = bus_dmamem_alloc(sc->sc_dmat, dma->dma_size, HWAES_ALIGN, 0,
	    dma->dma_segs, 1, &nsegs, flags);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "bus_dmamem_alloc failed: %d\n", error);
		goto alloc_failed;
	}
	error = bus_dmamem_map(sc->sc_dmat, dma->dma_segs, nsegs,
	    dma->dma_size, &dma->dma_addr, flags);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "bus_dmamem_map failed: %d\n", error);
		goto map_failed;
	}
	error = bus_dmamap_create(sc->sc_dmat, dma->dma_size, nsegs,
	    dma->dma_size, 0, flags, &dma->dma_map);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "bus_dmamap_create failed: %d\n", error);
		goto create_failed;
	}
	error = bus_dmamap_load(sc->sc_dmat, dma->dma_map, dma->dma_addr,
	    dma->dma_size, NULL, flags);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "bus_dmamap_load failed: %d\n", error);
		goto load_failed;
	}

	return 0;

load_failed:
	bus_dmamap_destroy(sc->sc_dmat, dma->dma_map);
create_failed:
	bus_dmamem_unmap(sc->sc_dmat, dma->dma_addr, dma->dma_size);
map_failed:
	bus_dmamem_free(sc->sc_dmat, dma->dma_segs, nsegs);
alloc_failed:
	return error;
}

static int
hwaes_probe(void)
{
	return 0;
}

static void
hwaes_setenckey(struct aesenc *enc, const uint8_t *key, uint32_t nrounds)
{
	if (nrounds == AES_128_NROUNDS) {
		enc->aese_aes.aes_rk[0] = be32dec(key + 4*0);
		enc->aese_aes.aes_rk[1] = be32dec(key + 4*1);
		enc->aese_aes.aes_rk[2] = be32dec(key + 4*2);
		enc->aese_aes.aes_rk[3] = be32dec(key + 4*3);
	} else {
		aes_bear_impl.ai_setenckey(enc, key, nrounds);
	}
}

static void
hwaes_setdeckey(struct aesdec *dec, const uint8_t *key, uint32_t nrounds)
{
	if (nrounds == AES_128_NROUNDS) {
		dec->aesd_aes.aes_rk[0] = be32dec(key + 4*0);
		dec->aesd_aes.aes_rk[1] = be32dec(key + 4*1);
		dec->aesd_aes.aes_rk[2] = be32dec(key + 4*2);
		dec->aesd_aes.aes_rk[3] = be32dec(key + 4*3);
	} else {
		aes_bear_impl.ai_setdeckey(dec, key, nrounds);
	}
}

static void
hwaes_exec_sync(uint32_t flags, uint16_t blocks)
{
	struct hwaes_softc *sc = hwaes_sc;
	uint32_t ctrl;

	KASSERT(blocks > 0);
	KASSERT(blocks <= HWAES_MAX_BLOCKS);

	WR4(sc, AES_SRC, sc->sc_dma_bounce.dma_segs[0].ds_addr);
	WR4(sc, AES_DEST, sc->sc_dma_bounce.dma_segs[0].ds_addr);

	ctrl = AES_CTRL_EXEC | AES_CTRL_ENA | flags;
	ctrl |= __SHIFTIN(blocks - 1, AES_CTRL_BLOCKS);

	WR4(sc, AES_CTRL, ctrl);
	for (;;) {
		ctrl = RD4(sc, AES_CTRL);
		if ((ctrl & AES_CTRL_ERR) != 0) {
			printf("AES error, AES_CTRL = %#x\n", ctrl);
			break;
		}
		if ((ctrl & AES_CTRL_EXEC) == 0) {
			break;
		}
	}
}

static void
hwaes_enc(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], uint32_t nrounds)
{
	struct hwaes_softc *sc = hwaes_sc;
	unsigned n;
	int s;

	if (nrounds != AES_128_NROUNDS) {
		aes_bear_impl.ai_enc(enc, in, out, nrounds);
		return;
	}

	s = splvm();

	for (n = 0; n < 4; n++) {
		WR4(sc, AES_IV, 0);
	}
	for (n = 0; n < 4; n++) {
		WR4(sc, AES_KEY, enc->aese_aes.aes_rk[n]);
	}
	memcpy(sc->sc_dma_bounce.dma_addr, in, HWAES_BLOCK_LEN);
	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_bounce.dma_map,
	    0, HWAES_BLOCK_LEN, BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	hwaes_exec_sync(0, 1);
	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_bounce.dma_map,
	    0, HWAES_BLOCK_LEN, BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
	memcpy(out, sc->sc_dma_bounce.dma_addr, HWAES_BLOCK_LEN);

	splx(s);
}

static void
hwaes_encN(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nblocks)
{
	for (size_t n = 0; n < nblocks; n++) {
		hwaes_enc(enc, &in[n * HWAES_BLOCK_LEN],
		    &out[n * HWAES_BLOCK_LEN], AES_128_NROUNDS);
	}
}

static void
hwaes_dec(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], uint32_t nrounds)
{
	struct hwaes_softc *sc = hwaes_sc;
	unsigned n;
	int s;

	if (nrounds != AES_128_NROUNDS) {
		aes_bear_impl.ai_dec(dec, in, out, nrounds);
		return;
	}

	s = splvm();

	for (n = 0; n < 4; n++) {
		WR4(sc, AES_IV, 0);
	}
	for (n = 0; n < 4; n++) {
		WR4(sc, AES_KEY, dec->aesd_aes.aes_rk[n]);
	}
	memcpy(sc->sc_dma_bounce.dma_addr, in, HWAES_BLOCK_LEN);
	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_bounce.dma_map,
	    0, HWAES_BLOCK_LEN, BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	hwaes_exec_sync(AES_CTRL_DEC, 1);
	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_bounce.dma_map,
	    0, HWAES_BLOCK_LEN, BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
	memcpy(out, sc->sc_dma_bounce.dma_addr, HWAES_BLOCK_LEN);

	splx(s);
}

static void
hwaes_decN(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nblocks)
{
	for (size_t n = 0; n < nblocks; n++) {
		hwaes_dec(dec, &in[n * HWAES_BLOCK_LEN],
		    &out[n * HWAES_BLOCK_LEN], AES_128_NROUNDS);
	}
}

static void
hwaes_cbc_enc(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t iv[static 16],
    uint32_t nrounds)
{
	struct hwaes_softc *sc = hwaes_sc;
	const uint8_t *inp = in;
	uint8_t *outp = out;
	uint32_t flags;
	unsigned n;
	int s;

	if (nrounds != AES_128_NROUNDS) {
		aes_bear_impl.ai_cbc_enc(enc, in, out, nbytes, iv, nrounds);
		return;
	}

	KASSERT(nbytes % HWAES_BLOCK_LEN == 0);
	if (nbytes == 0) {
		return;
	}

	s = splvm();

	for (n = 0; n < 4; n++) {
		WR4(sc, AES_IV, be32dec(&iv[n * 4]));
	}
	for (n = 0; n < 4; n++) {
		WR4(sc, AES_KEY, enc->aese_aes.aes_rk[n]);
	}
	flags = 0;
	while (nbytes > 0) {
		const size_t blocks = MIN(nbytes / HWAES_BLOCK_LEN,
					  HWAES_MAX_BLOCKS);

		memcpy(sc->sc_dma_bounce.dma_addr, inp,
		    blocks * HWAES_BLOCK_LEN);
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_bounce.dma_map,
		    0, blocks * HWAES_BLOCK_LEN,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
		hwaes_exec_sync(flags, blocks);
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_bounce.dma_map,
		    0, blocks * HWAES_BLOCK_LEN,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
		memcpy(outp, sc->sc_dma_bounce.dma_addr,
		    blocks * HWAES_BLOCK_LEN);

		nbytes -= blocks * HWAES_BLOCK_LEN;
		inp += blocks * HWAES_BLOCK_LEN;
		outp += blocks * HWAES_BLOCK_LEN;
		flags |= AES_CTRL_IV;
	}

	memcpy(iv, outp - HWAES_BLOCK_LEN, HWAES_BLOCK_LEN);

	splx(s);
}

static void
hwaes_cbc_dec(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t iv[static 16],
    uint32_t nrounds)
{
	struct hwaes_softc *sc = hwaes_sc;
	const uint8_t *inp = in;
	uint8_t *outp = out;
	uint32_t flags;
	unsigned n;
	int s;

	if (nrounds != AES_128_NROUNDS) {
		aes_bear_impl.ai_cbc_dec(dec, in, out, nbytes, iv, nrounds);
		return;
	}

	KASSERT(nbytes % HWAES_BLOCK_LEN == 0);
	if (nbytes == 0) {
		return;
	}

	s = splvm();

	for (n = 0; n < 4; n++) {
		WR4(sc, AES_IV, be32dec(&iv[n * 4]));
	}

	memcpy(iv, inp + nbytes - HWAES_BLOCK_LEN, HWAES_BLOCK_LEN);

	for (n = 0; n < 4; n++) {
		WR4(sc, AES_KEY, dec->aesd_aes.aes_rk[n]);
	}
	flags = AES_CTRL_DEC;
	while (nbytes > 0) {
		const size_t blocks = MIN(nbytes / HWAES_BLOCK_LEN,
					  HWAES_MAX_BLOCKS);

		memcpy(sc->sc_dma_bounce.dma_addr, inp,
		    blocks * HWAES_BLOCK_LEN);
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_bounce.dma_map,
		    0, blocks * HWAES_BLOCK_LEN,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
		hwaes_exec_sync(flags, blocks);
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_bounce.dma_map,
		    0, blocks * HWAES_BLOCK_LEN,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
		memcpy(outp, sc->sc_dma_bounce.dma_addr,
		    blocks * HWAES_BLOCK_LEN);

		nbytes -= blocks * HWAES_BLOCK_LEN;
		inp += blocks * HWAES_BLOCK_LEN;
		outp += blocks * HWAES_BLOCK_LEN;
		flags |= AES_CTRL_IV;
	}

	splx(s);
}

static void
hwaes_xts_update(uint32_t *t0, uint32_t *t1, uint32_t *t2, uint32_t *t3)
{
	uint32_t s0, s1, s2, s3;

	s0 = *t0 >> 31;
	s1 = *t1 >> 31;
	s2 = *t2 >> 31;
	s3 = *t3 >> 31;
	*t0 = (*t0 << 1) ^ (-s3 & 0x87);
	*t1 = (*t1 << 1) ^ s0;
	*t2 = (*t2 << 1) ^ s1;
	*t3 = (*t3 << 1) ^ s2;
}

static void
hwaes_xts_enc(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t tweak[static 16],
    uint32_t nrounds)
{
	uint8_t block[16];
	uint8_t tle[16];
	uint32_t t[4];
	const uint8_t *inp = in;
	uint8_t *outp = out;

	if (nrounds != AES_128_NROUNDS) {
		aes_bear_impl.ai_xts_enc(enc, in, out, nbytes, tweak, nrounds);
		return;
	}

	KASSERT(nbytes % 16 == 0);

	t[0] = le32dec(tweak + 4*0);
	t[1] = le32dec(tweak + 4*1);
	t[2] = le32dec(tweak + 4*2);
	t[3] = le32dec(tweak + 4*3);

	while (nbytes > 0) {
		le32enc(tle + 4*0, t[0]);
		le32enc(tle + 4*1, t[1]);
		le32enc(tle + 4*2, t[2]);
		le32enc(tle + 4*3, t[3]);

		for (unsigned n = 0; n < 16; n++) {
			block[n] = inp[n] ^ tle[n];
		}

		hwaes_encN(enc, block, block, 1);

		for (unsigned n = 0; n < 16; n++) {
			outp[n] = block[n] ^ tle[n];
		}

		hwaes_xts_update(&t[0], &t[1], &t[2], &t[3]);

		nbytes -= HWAES_BLOCK_LEN;
		inp += HWAES_BLOCK_LEN;
		outp += HWAES_BLOCK_LEN;
	}

	le32enc(tweak + 4*0, t[0]);
	le32enc(tweak + 4*1, t[1]);
	le32enc(tweak + 4*2, t[2]);
	le32enc(tweak + 4*3, t[3]);
	
	explicit_memset(t, 0, sizeof(t));
	explicit_memset(block, 0, sizeof(block));
	explicit_memset(tle, 0, sizeof(tle));
}

static void
hwaes_xts_dec(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t tweak[static 16],
    uint32_t nrounds)
{
	uint8_t block[16];
	uint8_t tle[16];
	uint32_t t[4];
	const uint8_t *inp = in;
	uint8_t *outp = out;

	if (nrounds != AES_128_NROUNDS) {
		aes_bear_impl.ai_xts_dec(dec, in, out, nbytes, tweak, nrounds);
		return;
	}

	KASSERT(nbytes % 16 == 0);

	t[0] = le32dec(tweak + 4*0);
	t[1] = le32dec(tweak + 4*1);
	t[2] = le32dec(tweak + 4*2);
	t[3] = le32dec(tweak + 4*3);

	while (nbytes > 0) {
		le32enc(tle + 4*0, t[0]);
		le32enc(tle + 4*1, t[1]);
		le32enc(tle + 4*2, t[2]);
		le32enc(tle + 4*3, t[3]);

		for (unsigned n = 0; n < 16; n++) {
			block[n] = inp[n] ^ tle[n];
		}

		hwaes_decN(dec, block, block, 1);

		for (unsigned n = 0; n < 16; n++) {
			outp[n] = block[n] ^ tle[n];
		}

		hwaes_xts_update(&t[0], &t[1], &t[2], &t[3]);

		nbytes -= HWAES_BLOCK_LEN;
		inp += HWAES_BLOCK_LEN;
		outp += HWAES_BLOCK_LEN;
	}

	le32enc(tweak + 4*0, t[0]);
	le32enc(tweak + 4*1, t[1]);
	le32enc(tweak + 4*2, t[2]);
	le32enc(tweak + 4*3, t[3]);
	
	explicit_memset(t, 0, sizeof(t));
	explicit_memset(block, 0, sizeof(block));
	explicit_memset(tle, 0, sizeof(tle));
}

static void
hwaes_cbcmac_update1(const struct aesenc *enc, const uint8_t in[static 16],
    size_t nbytes, uint8_t auth0[static 16], uint32_t nrounds)
{
	const uint8_t *inp = in;

	if (nrounds != AES_128_NROUNDS) {
		aes_bear_impl.ai_cbcmac_update1(enc, in, nbytes, auth0, nrounds);
		return;
	}

	KASSERT(nbytes % 16 == 0);

	while (nbytes > 0) {
		for (unsigned n = 0; n < 16; n++) {
			auth0[n] = auth0[n] ^ inp[n];
		}

		hwaes_encN(enc, auth0, auth0, 1);

		nbytes -= HWAES_BLOCK_LEN;
		inp += HWAES_BLOCK_LEN;
	}
}

static void
hwaes_ccm_enc1(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t authctr0[static 32],
    uint32_t nrounds)
{
	const uint8_t *inp = in;
	uint8_t *outp = out;
	uint32_t c[4];

	if (nrounds != AES_128_NROUNDS) {
		aes_bear_impl.ai_ccm_enc1(enc, in, out, nbytes, authctr0, nrounds);
		return;
	}

	KASSERT(nbytes % 16 == 0);

	c[0] = le32dec(authctr0 + 16 + 4*0);
	c[1] = le32dec(authctr0 + 16 + 4*1);
	c[2] = le32dec(authctr0 + 16 + 4*2);
	c[3] = be32dec(authctr0 + 16 + 4*3);

	while (nbytes > 0) {
		for (unsigned n = 0; n < 16; n++) {
			authctr0[n] = authctr0[n] ^ inp[n];
		}

		le32enc(authctr0 + 16 + 4*0, c[0]);
		le32enc(authctr0 + 16 + 4*1, c[1]);
		le32enc(authctr0 + 16 + 4*2, c[2]);
		be32enc(authctr0 + 16 + 4*3, ++c[3]);

		hwaes_encN(enc, authctr0, authctr0, 2);

		for (unsigned n = 0; n < 16; n++) {
			outp[n] = inp[n] ^ authctr0[n + 16];
		}

		nbytes -= HWAES_BLOCK_LEN;
		inp += HWAES_BLOCK_LEN;
		outp += HWAES_BLOCK_LEN;
	}

	le32enc(authctr0 + 16 + 4*0, c[0]);
	le32enc(authctr0 + 16 + 4*1, c[1]);
	le32enc(authctr0 + 16 + 4*2, c[2]);
	be32enc(authctr0 + 16 + 4*3, c[3]);
}

static void
hwaes_ccm_dec1(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t authctr0[static 32],
    uint32_t nrounds)
{
	const uint8_t *inp = in;
	uint8_t *outp = out;
	uint32_t c[4];

	if (nrounds != AES_128_NROUNDS) {
		aes_bear_impl.ai_ccm_dec1(enc, in, out, nbytes, authctr0, nrounds);
		return;
	}

	KASSERT(nbytes % 16 == 0);

	c[0] = le32dec(authctr0 + 16 + 4*0);
	c[1] = le32dec(authctr0 + 16 + 4*1);
	c[2] = le32dec(authctr0 + 16 + 4*2);
	c[3] = be32dec(authctr0 + 16 + 4*3);

	be32enc(authctr0 + 16 + 4*3, ++c[3]);
	hwaes_encN(enc, authctr0 + 16, authctr0 + 16, 1);

	while (nbytes > 0) {
		for (unsigned n = 0; n < 16; n++) {
			outp[n] = authctr0[n + 16] ^ inp[n];
			authctr0[n] = authctr0[n] ^ outp[n];
		}
		nbytes -= HWAES_BLOCK_LEN;
		if (nbytes == 0) {
			break;
		}

		inp += HWAES_BLOCK_LEN;
		outp += HWAES_BLOCK_LEN;

		le32enc(authctr0 + 16 + 4*0, c[0]);
		le32enc(authctr0 + 16 + 4*1, c[1]);
		le32enc(authctr0 + 16 + 4*2, c[2]);
		be32enc(authctr0 + 16 + 4*3, ++c[3]);
		hwaes_encN(enc, authctr0, authctr0, 2);
	}
	hwaes_encN(enc, authctr0, authctr0, 1);

	le32enc(authctr0 + 16 + 4*0, c[0]);
	le32enc(authctr0 + 16 + 4*1, c[1]);
	le32enc(authctr0 + 16 + 4*2, c[2]);
	be32enc(authctr0 + 16 + 4*3, c[3]);

}

static struct aes_impl aes_hwaes_impl = {
	.ai_name = "Hollywood AES engine",
	.ai_probe = hwaes_probe,
	.ai_setenckey = hwaes_setenckey,
	.ai_setdeckey = hwaes_setdeckey,
	.ai_enc = hwaes_enc,
	.ai_dec = hwaes_dec,
	.ai_cbc_enc = hwaes_cbc_enc,
	.ai_cbc_dec = hwaes_cbc_dec,
	.ai_xts_enc = hwaes_xts_enc,
	.ai_xts_dec = hwaes_xts_dec,
	.ai_cbcmac_update1 = hwaes_cbcmac_update1,
	.ai_ccm_enc1 = hwaes_ccm_enc1,
	.ai_ccm_dec1 = hwaes_ccm_dec1,
};

static void
hwaes_register(void)
{
	aes_md_init(&aes_hwaes_impl);
}
