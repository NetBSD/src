/*	$NetBSD: rk_v1crypto.c,v 1.9 2022/04/08 23:14:21 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
 * rk_v1crypto -- Rockchip crypto v1 driver
 *
 * This is just the RNG for now.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: rk_v1crypto.c,v 1.9 2022/04/08 23:14:21 riastradh Exp $");

#include <sys/types.h>

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/mutex.h>
#include <sys/rndsource.h>
#include <sys/sysctl.h>

#include <dev/fdt/fdtvar.h>

#include <arm/rockchip/rk_v1crypto.h>

struct rk_v1crypto_softc {
	device_t			sc_dev;
	bus_space_tag_t			sc_bst;
	bus_space_handle_t		sc_bsh;
	kmutex_t			sc_lock;
	struct krndsource		sc_rndsource;
	struct rk_v1crypto_sysctl {
		struct sysctllog		*cy_log;
		const struct sysctlnode		*cy_root_node;
	}				sc_sysctl;
};

static int rk_v1crypto_match(device_t, cfdata_t, void *);
static void rk_v1crypto_attach(device_t, device_t, void *);
static int rk_v1crypto_selftest(struct rk_v1crypto_softc *);
static void rk_v1crypto_rndsource_attach(struct rk_v1crypto_softc *);
static void rk_v1crypto_rng_get(size_t, void *);
static void rk_v1crypto_sysctl_attach(struct rk_v1crypto_softc *);
static int rk_v1crypto_sysctl_rng(SYSCTLFN_ARGS);
static int rk_v1crypto_rng(struct rk_v1crypto_softc *,
    uint32_t[static RK_V1CRYPTO_TRNG_NOUT]);

static uint32_t
RKC_READ(struct rk_v1crypto_softc *sc, bus_addr_t reg)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, reg);
}

static void
RKC_WRITE(struct rk_v1crypto_softc *sc, bus_addr_t reg, uint32_t v)
{
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, reg, v);
}

static inline void
RKC_CTRL(struct rk_v1crypto_softc *sc, uint16_t m, uint16_t v)
{
	uint32_t c = 0;

	c |= __SHIFTIN(m, RK_V1CRYPTO_CTRL_MASK);
	c |= __SHIFTIN(v, m);
	RKC_WRITE(sc, RK_V1CRYPTO_CTRL, c);
}

CFATTACH_DECL_NEW(rk_v1crypto, sizeof(struct rk_v1crypto_softc),
    rk_v1crypto_match, rk_v1crypto_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "rockchip,rk3288-crypto" },
	DEVICE_COMPAT_EOL
};

static int
rk_v1crypto_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct fdt_attach_args *const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
rk_v1crypto_attach(device_t parent, device_t self, void *aux)
{
	static const char *const clks[] = {"aclk", "hclk", "sclk", "apb_pclk"};
	struct rk_v1crypto_softc *const sc = device_private(self);
	const struct fdt_attach_args *const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	const int phandle = faa->faa_phandle;
	struct fdtbus_reset *rst;
	unsigned i;
	uint32_t ctrl;

	fdtbus_clock_assign(phandle);

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SOFTSERIAL);

	/* Get and map device registers.  */
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	/* Enable the clocks.  */
	for (i = 0; i < __arraycount(clks); i++) {
		if (fdtbus_clock_enable(phandle, clks[i], true) != 0) {
			aprint_error(": couldn't enable %s clock\n", clks[i]);
			return;
		}
	}

	/* Get a reset handle if we need and try to deassert it.  */
	if ((rst = fdtbus_reset_get_index(phandle, 0)) != NULL) {
		if (fdtbus_reset_deassert(rst) != 0) {
			aprint_error(": couldn't de-assert reset\n");
			return;
		}
	}

	aprint_naive("\n");
	aprint_normal(": Crypto v1\n");

	/*
	 * Enable ring oscillator to start gathering entropy, and set
	 * up the crypto clock to sample it once every 100 cycles.
	 *
	 * The ring oscillator can run even when the clock is gated or
	 * flush is asserted, and the longer we run it, the less it
	 * will be synchronized with the main clock owing to jitter
	 * ideally from unpredictable thermal noise.
	 */
	ctrl = RK_V1CRYPTO_TRNG_CTRL_OSC_ENABLE;
	ctrl |= __SHIFTIN(100, RK_V1CRYPTO_TRNG_CTRL_CYCLES);
	RKC_WRITE(sc, RK_V1CRYPTO_TRNG_CTRL, ctrl);

	if (rk_v1crypto_selftest(sc))
		return;
	rk_v1crypto_rndsource_attach(sc);
	rk_v1crypto_sysctl_attach(sc);
}

static int
rk_v1crypto_selftest(struct rk_v1crypto_softc *sc)
{
	static const uint32_t key[4] = {0};
	static const uint32_t input[4] = {0};
	static const uint32_t expected[4] = {
		0x66e94bd4, 0xef8a2c3b, 0x884cfa59, 0xca342b2e,
	};
	uint32_t output[4];
	uint32_t ctrl;
	unsigned i, timo;

	/* Program the key and input block.  */
	for (i = 0; i < 4; i++)
		RKC_WRITE(sc, RK_V1CRYPTO_AES_DIN(i), key[i]);
	for (i = 0; i < 4; i++)
		RKC_WRITE(sc, RK_V1CRYPTO_AES_DIN(i), input[i]);

	/*
	 * Set up the AES unit to do AES-128 `ECB' (i.e., just the raw
	 * AES permutation) in the encryption direction.
	 */
	ctrl = 0;
	ctrl |= RK_V1CRYPTO_AES_CTRL_KEYCHANGE;
	ctrl |= __SHIFTIN(RK_V1CRYPTO_AES_CTRL_MODE_ECB,
	    RK_V1CRYPTO_AES_CTRL_MODE);
	ctrl |= __SHIFTIN(RK_V1CRYPTO_AES_CTRL_KEYSIZE_128,
	    RK_V1CRYPTO_AES_CTRL_KEYSIZE);
	ctrl |= __SHIFTIN(RK_V1CRYPTO_AES_CTRL_DIR_ENC,
	    RK_V1CRYPTO_AES_CTRL_DIR);
	RKC_WRITE(sc, RK_V1CRYPTO_AES_CTRL, ctrl);

	/* Kick it off.  */
	RKC_CTRL(sc, RK_V1CRYPTO_CTRL_AES_START, 1);

	/* Wait up to 1ms for it to complete.  */
	timo = 1000;
	while (RKC_READ(sc, RK_V1CRYPTO_CTRL) & RK_V1CRYPTO_CTRL_AES_START) {
		if (--timo == 0) {
			device_printf(sc->sc_dev, "AES self-test timed out\n");
			return -1;
		}
		DELAY(1);
	}

	/* Read the output.  */
	for (i = 0; i < 4; i++)
		output[i] = RKC_READ(sc, RK_V1CRYPTO_AES_DOUT(i));

	/* Verify the output.  */
	for (i = 0; i < 4; i++) {
		if (output[i] != expected[i]) {
			device_printf(sc->sc_dev, "AES self-test failed\n");
			return -1;
		}
	}

	/* Success!  */
	return 0;
}

static void
rk_v1crypto_rndsource_attach(struct rk_v1crypto_softc *sc)
{
	device_t self = sc->sc_dev;

	rndsource_setcb(&sc->sc_rndsource, rk_v1crypto_rng_get, sc);
	rnd_attach_source(&sc->sc_rndsource, device_xname(self),
	    RND_TYPE_RNG, RND_FLAG_DEFAULT|RND_FLAG_HASCB);
}

static void
rk_v1crypto_rng_get(size_t nbytes, void *cookie)
{
	struct rk_v1crypto_softc *sc = cookie;
	device_t self = sc->sc_dev;
	uint32_t buf[RK_V1CRYPTO_TRNG_NOUT];
	uint32_t entropybits = NBBY*sizeof(buf)/2; /* be conservative */
	unsigned n = RK_V1CRYPTO_TRNG_NOUT;
	int error;
	size_t nbits = NBBY*nbytes;

	while (nbits) {
		CTASSERT((RK_V1CRYPTO_TRNG_NOUT % 2) == 0);

		error = rk_v1crypto_rng(sc, buf);
		if (error) {
			device_printf(self, "timed out\n");
			break;
		}
		if (consttime_memequal(buf, buf + n/2, n/2)) {
			device_printf(self, "failed repeated output test\n");
			break;
		}
		rnd_add_data_sync(&sc->sc_rndsource, buf, sizeof buf,
		    entropybits);
		nbits -= MIN(nbits, MAX(1, entropybits));
	}
	explicit_memset(buf, 0, sizeof buf);
}

static void
rk_v1crypto_sysctl_attach(struct rk_v1crypto_softc *sc)
{
	device_t self = sc->sc_dev;
	struct rk_v1crypto_sysctl *cy = &sc->sc_sysctl;
	int error;

	/* hw.rkv1cryptoN (node) */
	error = sysctl_createv(&cy->cy_log, 0, NULL, &cy->cy_root_node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, device_xname(self),
	    SYSCTL_DESCR("rk crypto v1 engine knobs"),
	    NULL, 0, NULL, 0,
	    CTL_HW, CTL_CREATE, CTL_EOL);
	if (error) {
		aprint_error_dev(self,
		    "failed to set up sysctl hw.%s: %d\n",
		    device_xname(self), error);
		return;
	}

	/* hw.rkv1cryptoN.rng (`struct', 32-byte array) */
	error = sysctl_createv(&cy->cy_log, 0, &cy->cy_root_node, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY|CTLFLAG_PRIVATE, CTLTYPE_STRUCT,
	    "rng", SYSCTL_DESCR("Read up to 32 bytes out of the TRNG"),
	    &rk_v1crypto_sysctl_rng, 0, sc, 0, CTL_CREATE, CTL_EOL);
	if (error) {
		aprint_error_dev(self,
		    "failed to set up sysctl hw.%s.rng: %d\n",
		    device_xname(self), error);
		return;
	}
}

static int
rk_v1crypto_sysctl_rng(SYSCTLFN_ARGS)
{
	uint32_t buf[RK_V1CRYPTO_TRNG_NOUT];
	struct sysctlnode node = *rnode;
	struct rk_v1crypto_softc *sc = node.sysctl_data;
	size_t size;
	int error;

	/* If oldp == NULL, the caller wants to learn the size.  */
	if (oldp == NULL) {
		*oldlenp = sizeof buf;
		return 0;
	}

	/* Verify the output buffer size is reasonable.  */
	size = *oldlenp;
	if (size > sizeof buf)	/* size_t, so never negative */
		return E2BIG;
	if (size == 0)
		return 0;	/* nothing to do */

	/* Generate data.  */
	error = rk_v1crypto_rng(sc, buf);
	if (error)
		return error;

	/* Copy out the data.  */
	node.sysctl_data = buf;
	node.sysctl_size = size;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	/* Clear the buffer.  */
	explicit_memset(buf, 0, sizeof buf);

	/* Return the sysctl_lookup error, if any.  */
	return error;
}

static int
rk_v1crypto_rng(struct rk_v1crypto_softc *sc,
    uint32_t buf[static RK_V1CRYPTO_TRNG_NOUT])
{
	unsigned i, timo;
	int error;

	/* Acquire lock to serialize access to TRNG.  */
	mutex_enter(&sc->sc_lock);

	/*
	 * Query TRNG and wait up to 1ms for it to post.  Empirically,
	 * this takes around 120us.
	 */
	RKC_CTRL(sc, RK_V1CRYPTO_CTRL_TRNG_START, 1);
	timo = 1000;
	while (RKC_READ(sc, RK_V1CRYPTO_CTRL) & RK_V1CRYPTO_CTRL_TRNG_START) {
		if (--timo == 0) {
			error = ETIMEDOUT;
			goto out;
		}
		DELAY(1);
	}

	/* Read out the data.  */
	for (i = 0; i < RK_V1CRYPTO_TRNG_NOUT; i++)
		buf[i] = RKC_READ(sc, RK_V1CRYPTO_TRNG_DOUT(i));

	/* Success!  */
	error = 0;
out:	mutex_exit(&sc->sc_lock);
	return error;
}
