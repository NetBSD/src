/* $NetBSD: jh7110_trng.c,v 1.2 2025/02/09 09:09:49 skrll Exp $ */

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: jh7110_trng.c,v 1.2 2025/02/09 09:09:49 skrll Exp $");

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/condvar.h>
#include <sys/mutex.h>
#include <sys/rndsource.h>

#include <dev/fdt/fdtvar.h>


struct jh7110_trng_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;

	kmutex_t		sc_lock;
	kcondvar_t		sc_cv;
	void *			sc_ih;
	bool			sc_reseeddone;
	size_t			sc_bytes_wanted;

	krndsource_t		sc_rndsource;
};


#define RD4(sc, reg)							       \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define WR4(sc, reg, val)						       \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))


/* Register definitions */
// https://doc-en.rvspace.org/JH7110/TRM/JH7110_TRM/control_registers_trng.html?hl=trng
#define JH7110_TRNG_CTRL		0x0000
#define  JH7110_TRNG_CTRL_NOP			0x0
#define  JH7110_TRNG_CTRL_RANDOMIZE		0x1
#define  JH7110_TRNG_CTRL_RANDOM_RESEED		0x2
#define  JH7110_TRNG_CTRL_NONCE_RESEED		0x3
#define JH7110_TRNG_STAT		0x0004
#define  JH7110_TRNG_STAT_SEEDED		__BIT(9)

#define JH7110_TRNG_MODE		0x0008
#define  JH7110_TRNG_MODE_R256			__BIT(3)
#define JH7110_TRNG_SMODE		0x000c
#define JH7110_TRNG_IENABLE		0x0010
#define  JH7110_TRNG_IENABLE_GLOBAL		__BIT(31)
#define  JH7110_TRNG_IENABLE_LFSR_LOCKUP	__BIT(4)
#define  JH7110_TRNG_IENABLE_RQST_LOCKUP	__BIT(3)
#define  JH7110_TRNG_IENABLE_AGE_ALARM		__BIT(2)
#define  JH7110_TRNG_IENABLE_SEED_DONE		__BIT(1)
#define  JH7110_TRNG_IENABLE_RAND_RDY		__BIT(0)
#define JH7110_TRNG_ISTATUS		0x0014
#define  JH7110_TRNG_ISTATUS_LFSR_LOCKUP	__BIT(4)
#define  JH7110_TRNG_ISTATUS_RQST_LOCKUP	__BIT(3)
#define  JH7110_TRNG_ISTATUS_AGE_ALARM		__BIT(2)
#define  JH7110_TRNG_ISTATUS_SEED_DONE		__BIT(1)
#define  JH7110_TRNG_ISTATUS_RAND_RDY		__BIT(0)
#define JH7110_TRNG_FEATURES		0x001c
#define  JH7110_TRNG_FEATURES_MM_RESET_STATE	__BIT(3)
#define  JH7110_TRNG_FEATURES_RAND_SEED_AVAIL	__BIT(2)
#define  JH7110_TRNG_FEATURES_MAX_RAND_LENGTH	__BITS(1,0)

#define  JH7110_TRNG_FEATURES_BITS					       \
	"\177\020"	/* New bitmask */				       \
	"f\003\01mode reset state\0"		/* bit  3 (1) */	       \
	    "=\x0" "test mode\0"					       \
	    "=\x1" "mission mode\0"					       \
	"f\002\01ring oscillator\0"		/* bit  2 (1) */	       \
	    "=\x0" "not preset\0"					       \
	    "=\x1" "present\0"						       \
	"f\000\02max rand length\0"		/* bits 0 .. 1 */	       \
	    "=\x0" "128-bit\0"						       \
	    "=\x1" "256-bit\0"						       \
	"\0"


#define JH7110_TRNG_DATA0		0x0020
#define JH7110_TRNG_DATA1		0x0024
#define JH7110_TRNG_DATA2		0x0028
#define JH7110_TRNG_DATA3		0x002c
#define JH7110_TRNG_DATA4		0x0030
#define JH7110_TRNG_DATA5		0x0034
#define JH7110_TRNG_DATA6		0x0038
#define JH7110_TRNG_DATA7		0x003c

#define JH7110_TRNG_BCONF		0x0068
#define  JH7110_TRNG_BCONF_AUTO_RESEED_LOOPBACK	__BIT(5)
#define  JH7110_TRNG_BCONF_MODE_AFTER_RST	__BIT(4)
#define  JH7110_TRNG_BCONF_PRNG_LEN_AFTER_RST	__BIT(3)
#define  JH7110_TRNG_BCONF_MAX_PRNG_LEN		__BIT(2)
#define  JH7110_TRNG_BCONF_BITS						       \
	"\177\020"	/* New bitmask */				       \
	"f\005\01auto reseed loopback\0"	/* bit  5 (1) */	       \
	    "=\x0" "not present\0"					       \
	    "=\x1" "present\0"						       \
	"f\004\01mode after reset\0"		/* bit  4 (1) */	       \
	    "=\x0" "test mode\0"					       \
	    "=\x1" "mission mode\0"					       \
	"f\003\01PRNG after reset\0"		/* bit  3 (1) */	       \
	    "=\x0" "not preset\0"					       \
	    "=\x1" "present\0"						       \
	"f\002\01max PRNG length\0"		/* bit  2 (1) */	       \
	    "=\x0" "128-bit\0"						       \
	    "=\x1" "256-bit\0"						       \
	"\0"


#define RD4(sc, reg)							       \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define WR4(sc, reg, val)						       \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))


static void
jh7110_trng_irqenable(struct jh7110_trng_softc *sc)
{
	WR4(sc, JH7110_TRNG_IENABLE,
	    JH7110_TRNG_IENABLE_GLOBAL |
	    JH7110_TRNG_IENABLE_SEED_DONE |
	    JH7110_TRNG_IENABLE_RAND_RDY |
	    JH7110_TRNG_IENABLE_LFSR_LOCKUP);
}

static void
jh7110_trng_irqdisable(struct jh7110_trng_softc *sc)
{
	WR4(sc, JH7110_TRNG_IENABLE, 0);
}

static void
jh7110_trng_probe(struct jh7110_trng_softc *sc, uint32_t istat)
{
	KASSERT(mutex_owned(&sc->sc_lock));

	if (sc->sc_bytes_wanted != 0) {
		uint32_t data[8];
		const uint32_t stat = RD4(sc, JH7110_TRNG_STAT);

		if (stat & JH7110_TRNG_STAT_SEEDED) {
			if (istat & JH7110_TRNG_ISTATUS_RAND_RDY) {

				WR4(sc, JH7110_TRNG_ISTATUS,
				    JH7110_TRNG_ISTATUS_RAND_RDY);

				data[0] = RD4(sc, JH7110_TRNG_DATA0);
				data[1] = RD4(sc, JH7110_TRNG_DATA1);
				data[2] = RD4(sc, JH7110_TRNG_DATA2);
				data[3] = RD4(sc, JH7110_TRNG_DATA3);
				data[4] = RD4(sc, JH7110_TRNG_DATA4);
				data[5] = RD4(sc, JH7110_TRNG_DATA5);
				data[6] = RD4(sc, JH7110_TRNG_DATA6);
				data[7] = RD4(sc, JH7110_TRNG_DATA7);

				rnd_add_data_sync(&sc->sc_rndsource, &data,
				    sizeof(data), sizeof(data) * NBBY);

				sc->sc_bytes_wanted -=
				    MIN(sc->sc_bytes_wanted, sizeof(data));

				if (sc->sc_bytes_wanted == 0)
					jh7110_trng_irqdisable(sc);
			}
		} else {
			WR4(sc, JH7110_TRNG_CTRL,
			    JH7110_TRNG_CTRL_RANDOM_RESEED);
		}
		explicit_memset(data, 0, sizeof data);
	}
	if (sc->sc_bytes_wanted != 0) {
		WR4(sc, JH7110_TRNG_CTRL,
		    JH7110_TRNG_CTRL_RANDOMIZE);
	}
}

static void
jh7110_trng_get(size_t bytes_wanted, void *arg)
{
	struct jh7110_trng_softc * const sc = arg;

	mutex_enter(&sc->sc_lock);
	sc->sc_bytes_wanted += bytes_wanted;

	jh7110_trng_irqenable(sc);

	const uint32_t istat = RD4(sc, JH7110_TRNG_ISTATUS);
	jh7110_trng_probe(sc, istat);

	mutex_exit(&sc->sc_lock);
}


static int
jh7110_trng_intr(void *priv)
{
	struct jh7110_trng_softc * const sc = priv;

	mutex_enter(&sc->sc_lock);

	const uint32_t istat = RD4(sc, JH7110_TRNG_ISTATUS);

	if (istat & JH7110_TRNG_ISTATUS_RAND_RDY) {
		KASSERT(RD4(sc, JH7110_TRNG_STAT) & JH7110_TRNG_STAT_SEEDED);
		jh7110_trng_probe(sc, istat);
		//sc->sc_randready = true;

	}

	if (istat & JH7110_TRNG_ISTATUS_SEED_DONE)
		sc->sc_reseeddone = true;

#if 0
	if (istat & JH7110_TRNG_ISTATUS_LFSR_LOCKUP) {
		sc->sc_reseeddone = false;
	}
#endif
	WR4(sc, JH7110_TRNG_ISTATUS, istat);

	if (sc->sc_reseeddone)
		cv_broadcast(&sc->sc_cv);

	mutex_exit(&sc->sc_lock);

	return 1;
}


static void
jh7110_trng_init(struct jh7110_trng_softc *sc)
{
	/* Mask and clear all interrupts. */
	WR4(sc, JH7110_TRNG_IENABLE,  0U);
	WR4(sc, JH7110_TRNG_ISTATUS, ~0U);

	WR4(sc, JH7110_TRNG_MODE, JH7110_TRNG_MODE_R256);

	mutex_enter(&sc->sc_lock);

	jh7110_trng_irqenable(sc);

	sc->sc_reseeddone = false;
	WR4(sc, JH7110_TRNG_CTRL, JH7110_TRNG_CTRL_RANDOM_RESEED);

	while (!sc->sc_reseeddone) {
		const int error = cv_timedwait(&sc->sc_cv, &sc->sc_lock, 1);
		if (error) {
			printf("%s: timedout\n", __func__);
			mutex_exit(&sc->sc_lock);
			return;
		}
	}
	mutex_exit(&sc->sc_lock);
}

static void
jh7110_trng_attach_i(device_t self)
{
	struct jh7110_trng_softc * const sc = device_private(self);

	jh7110_trng_init(sc);

	/* set up an rndsource */
	rndsource_setcb(&sc->sc_rndsource, &jh7110_trng_get, sc);
	rnd_attach_source(&sc->sc_rndsource, device_xname(self), RND_TYPE_RNG,
	    RND_FLAG_COLLECT_VALUE | RND_FLAG_HASCB);
}

/* Compat string(s) */
static const struct device_compatible_entry compat_data[] = {
	{ .compat = "starfive,jh7110-trng" },
	DEVICE_COMPAT_EOL
};


static int
jh7110_trng_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
jh7110_trng_attach(device_t parent, device_t self, void *aux)
{
	struct jh7110_trng_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_space_tag_t bst = faa->faa_bst;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	error = bus_space_map(bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#" PRIxBUSADDR ": %d", addr,
		    error);
		return;
	}

	/* Enable the hclk clock.  */
	error = fdtbus_clock_enable(phandle, "hclk", true);
	if (error) {
		aprint_error(": couldn't enable 'hclk' clock\n");
		return;
	}

	/* Enable the hclk clock.  */
	error = fdtbus_clock_enable(phandle, "ahb", true);
	if (error) {
		aprint_error(": couldn't enable 'ahb' clock\n");
		return;
	}

	/* Get a reset handle if we need and try to deassert it.  */
	struct fdtbus_reset * const rst = fdtbus_reset_get_index(phandle, 0);
	if (rst != NULL) {
		if (fdtbus_reset_deassert(rst) != 0) {
			aprint_error(": couldn't de-assert reset\n");
			return;
		}
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = bst;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);
	cv_init(&sc->sc_cv, "jh7110trng");

	aprint_naive("\n");
	aprint_normal(": JH7110 TRNG\n");

	char buf[256];

	snprintb(buf, sizeof(buf), JH7110_TRNG_FEATURES_BITS,
	    RD4(sc, JH7110_TRNG_FEATURES));
	aprint_verbose_dev(sc->sc_dev, "Features    : %s\n", buf);

	snprintb(buf, sizeof(buf), JH7110_TRNG_BCONF_BITS,
	    RD4(sc, JH7110_TRNG_BCONF));
	aprint_verbose_dev(sc->sc_dev, "Build config: %s\n", buf);

	char intrstr[128];
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish_xname(phandle, 0, IPL_VM,
	    FDT_INTR_MPSAFE, jh7110_trng_intr, sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	config_interrupts(self, jh7110_trng_attach_i);
}

CFATTACH_DECL_NEW(jh7110_trng, sizeof(struct jh7110_trng_softc),
	jh7110_trng_match, jh7110_trng_attach, NULL, NULL);
