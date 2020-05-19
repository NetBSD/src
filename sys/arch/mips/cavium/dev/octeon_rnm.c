/*	$NetBSD: octeon_rnm.c,v 1.2.4.1 2020/05/19 17:35:50 martin Exp $	*/

/*
 * Copyright (c) 2007 Internet Initiative Japan, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Cavium Octeon Random Number Generator / Random Number Memory `RNM'
 *
 *	The RNM unit consists of:
 *
 *	1. 128 ring oscillators
 *	2. an LFSR/SHA-1 conditioner
 *	3. a 512-byte FIFO
 *
 *	When the unit is enabled, there are three modes of operation:
 *
 *	(a) deterministic: the ring oscillators are disabled and the
 *	    LFSR/SHA-1 conditioner operates on fixed inputs to give
 *	    reproducible results for testing,
 *
 *	(b) conditioned entropy: the ring oscillators are enabled and
 *	    samples from them are fed through the LFSR/SHA-1
 *	    conditioner before being put into the FIFO, and
 *
 *	(c) raw entropy: the ring oscillators are enabled, and a group
 *	    of eight of them selected at any one time is sampled and
 *	    fed into the FIFO.
 *
 *	Details:
 *
 *	- The FIFO is refilled whenever we read out of it, either with
 *	  a load address or an IOBDMA operation.
 *
 *	- The conditioner takes 81 cycles to produce a 64-bit block of
 *	  output in the FIFO whether in deterministic or conditioned
 *	  entropy mode, each block consisting of the first 64 bits of a
 *	  SHA-1 hash.
 *
 *	- A group of eight ring oscillators take 8 cycles to produce a
 *	  64-bit block of output in the FIFO in raw entropy mode, each
 *	  block consisting of eight consecutive samples from each RO in
 *	  parallel.
 *
 *	The first sample of each RO always seems to be zero.  Further,
 *	consecutive samples from a single ring oscillator are not
 *	independent, so naive debiasing like a von Neumann extractor
 *	falls flat on its face.  And parallel ring oscillators powered
 *	by the same source may not be independent either, if they end
 *	up locked.
 *
 *	We read out one FIFO's worth of raw samples from groups of 8
 *	ring oscillators at a time, of 128 total, by going through them
 *	round robin.  We take 32 consecutive samples from each ring
 *	oscillator in a group of 8 in parallel before we count one bit
 *	of entropy.  To get 256 bits of entropy, we read 4Kbit of data
 *	from each of two 8-RO groups.
 *
 *	We could use the on-board LFSR/SHA-1 conditioner like the Linux
 *	driver written by Cavium does, but it's not clear how many RO
 *	samples go into the conditioner, and our entropy pool is a
 *	perfectly good conditioner itself, so it seems there is little
 *	advantage -- other than expedience -- to using the LFSR/SHA-1
 *	conditioner.  All the manual says is that it samples 125 of the
 *	128 ROs.  But the Cavium SHA-1 CPU instruction is advertised to
 *	have a latency of 100 cycles, so it seems implausible that much
 *	more than one sample from each RO could be squeezed in there.
 *
 *	The hardware exposes only 64 bits of each SHA-1 hash, and the
 *	Linux driver uses 32 bits of that -- which, if treated as full
 *	entropy, would mean an assessment of 3.9 bits of RO samples to
 *	get 1 bit of entropy, whereas we take 256 bits of RO samples to
 *	get one bit of entropy, so this seems reasonably conservative.
 *
 * Reference: Cavium Networks OCTEON Plus CN50XX Hardware Reference
 * Manual, CN50XX-HM-0.99E PRELIMINARY, July 2008.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon_rnm.c,v 1.2.4.1 2020/05/19 17:35:50 martin Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/rndsource.h>
#include <sys/systm.h>

#include <mips/locore.h>
#include <mips/cavium/include/iobusvar.h>
#include <mips/cavium/dev/octeon_rnmreg.h>
#include <mips/cavium/dev/octeon_corereg.h>
#include <mips/cavium/octeonvar.h>

#include <sys/bus.h>

//#define	OCTEON_RNM_DEBUG

#define	ENT_DELAY_CLOCK 8	/* cycles for each 64-bit RO sample batch */
#define	RNG_DELAY_CLOCK 81	/* cycles for each SHA-1 output */
#define	NROGROUPS	16
#define	RNG_FIFO_WORDS	(512/sizeof(uint64_t))

struct octeon_rnm_softc {
	bus_space_tag_t		sc_bust;
	bus_space_handle_t	sc_regh;
	kmutex_t		sc_lock;
	krndsource_t		sc_rndsrc;	/* /dev/random source */
	unsigned		sc_rogroup;
};

static int octeon_rnm_match(device_t, struct cfdata *, void *);
static void octeon_rnm_attach(device_t, device_t, void *);
static void octeon_rnm_rng(size_t, void *);
static void octeon_rnm_reset(struct octeon_rnm_softc *);
static void octeon_rnm_conditioned_deterministic(struct octeon_rnm_softc *);
static void octeon_rnm_conditioned_entropy(struct octeon_rnm_softc *);
static void octeon_rnm_raw_entropy(struct octeon_rnm_softc *, unsigned);
static uint64_t octeon_rnm_load(struct octeon_rnm_softc *);
static void octeon_rnm_iobdma(struct octeon_rnm_softc *, uint64_t *, unsigned);
static void octeon_rnm_delay(uint32_t);

CFATTACH_DECL_NEW(octeon_rnm, sizeof(struct octeon_rnm_softc),
    octeon_rnm_match, octeon_rnm_attach, NULL, NULL);

static int
octeon_rnm_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct iobus_attach_args *aa = aux;

	if (strcmp(cf->cf_name, aa->aa_name) != 0)
		return 0;
	if (cf->cf_unit != aa->aa_unitno)
		return 0;
	return 1;
}

static void
octeon_rnm_attach(device_t parent, device_t self, void *aux)
{
	struct octeon_rnm_softc *sc = device_private(self);
	struct iobus_attach_args *aa = aux;
	uint64_t bist_status, sample, expected = UINT64_C(0xd654ff35fadf866b);

	aprint_normal("\n");

	/* Map the device registers, all two of them.  */
	sc->sc_bust = aa->aa_bust;
	if (bus_space_map(aa->aa_bust, aa->aa_unit->addr, RNM_SIZE,
	    0, &sc->sc_regh) != 0) {
		aprint_error_dev(self, "unable to map device\n");
		return;
	}

	/* Verify that the built-in self-test succeeded.  */
	bist_status = bus_space_read_8(sc->sc_bust, sc->sc_regh,
	    RNM_BIST_STATUS_OFFSET);
	if (bist_status) {
		aprint_error_dev(self, "RNG built in self test failed: %#lx\n",
		    bist_status);
		return;
	}

	/* Create a mutex to serialize access to the FIFO.  */
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);

	/*
	 * Reset the core, enable the RNG engine without entropy, wait
	 * 81 cycles for it to produce a single sample, and draw the
	 * deterministic sample to test.
	 *
	 * XXX Verify that the output matches the SHA-1 computation
	 * described by the data sheet, not just a known answer.
	 */
	octeon_rnm_reset(sc);
	octeon_rnm_conditioned_deterministic(sc);
	octeon_rnm_delay(RNG_DELAY_CLOCK*1);
	sample = octeon_rnm_load(sc);
	if (sample != expected)
		aprint_error_dev(self, "self-test: read %016"PRIx64","
		    " expected %016"PRIx64, sample, expected);

	/*
	 * Reset the core again to clear the FIFO, and enable the RNG
	 * engine with entropy exposed directly.  Start from the first
	 * group of ring oscillators; as we gather samples we will
	 * rotate through the rest of them.
	 */
	octeon_rnm_reset(sc);
	sc->sc_rogroup = 0;
	octeon_rnm_raw_entropy(sc, sc->sc_rogroup);
	octeon_rnm_delay(ENT_DELAY_CLOCK*RNG_FIFO_WORDS);

	/* Attach the rndsource.  */
	rndsource_setcb(&sc->sc_rndsrc, octeon_rnm_rng, sc);
	rnd_attach_source(&sc->sc_rndsrc, device_xname(self), RND_TYPE_RNG,
	    RND_FLAG_DEFAULT | RND_FLAG_HASCB);
}

static void
octeon_rnm_rng(size_t nbytes, void *vsc)
{
	const unsigned BPB = 256; /* bits of data per bit of entropy */
	uint64_t sample[32];
	struct octeon_rnm_softc *sc = vsc;
	size_t needed = NBBY*nbytes;
	unsigned i;

	/* Sample the ring oscillators round-robin.  */
	mutex_enter(&sc->sc_lock);
	while (needed) {
		/*
		 * Switch to the next RO group once we drain the FIFO.
		 * By the time rnd_add_data is done, we will have
		 * processed all 512 bytes of the FIFO.  We assume it
		 * takes at least one cycle per byte (realistically,
		 * more like ~80cpb to draw from the FIFO and then
		 * process it with rnd_add_data), so there is no need
		 * for any other delays.
		 */
		sc->sc_rogroup++;
		sc->sc_rogroup %= NROGROUPS;
		octeon_rnm_raw_entropy(sc, sc->sc_rogroup);

		/*
		 * Gather half the FIFO at a time -- we are limited to
		 * 256 bytes because of limits on the CVMSEG buffer.
		 */
		CTASSERT(sizeof sample == 256);
		CTASSERT(2*__arraycount(sample) == RNG_FIFO_WORDS);
		for (i = 0; i < 2; i++) {
			octeon_rnm_iobdma(sc, sample, __arraycount(sample));
#ifdef OCTEON_RNM_DEBUG
			hexdump(printf, "rnm", sample, sizeof sample);
#endif
			rnd_add_data_sync(&sc->sc_rndsrc, sample,
			    sizeof sample, NBBY*sizeof(sample)/BPB);
			needed -= MIN(needed, MAX(1, NBBY*sizeof(sample)/BPB));
		}

		/* Yield if requested.  */
		if (__predict_false(curcpu()->ci_schedstate.spc_flags &
			SPCF_SHOULDYIELD)) {
			mutex_exit(&sc->sc_lock);
			preempt();
			mutex_enter(&sc->sc_lock);
		}
	}
	mutex_exit(&sc->sc_lock);

	/* Zero the sample.  */
	explicit_memset(sample, 0, sizeof sample);
}

/*
 * octeon_rnm_reset(sc)
 *
 *	Reset the RNM unit, disabling it and clearing the FIFO.
 */
static void
octeon_rnm_reset(struct octeon_rnm_softc *sc)
{

	bus_space_write_8(sc->sc_bust, sc->sc_regh, RNM_CTL_STATUS_OFFSET,
	    RNM_CTL_STATUS_RNG_RST|RNM_CTL_STATUS_RNM_RST);
}

/*
 * octeon_rnm_conditioned_deterministic(sc)
 *
 *	Switch the RNM unit into the deterministic LFSR/SHA-1 mode with
 *	no entropy, for the next data loaded into the FIFO.
 */
static void
octeon_rnm_conditioned_deterministic(struct octeon_rnm_softc *sc)
{

	bus_space_write_8(sc->sc_bust, sc->sc_regh, RNM_CTL_STATUS_OFFSET,
	    RNM_CTL_STATUS_RNG_EN);
}

/*
 * octeon_rnm_conditioned_entropy(sc)
 *
 *	Switch the RNM unit to generate ring oscillator samples
 *	conditioned with an LFSR/SHA-1, for the next data loaded into
 *	the FIFO.
 */
static void __unused
octeon_rnm_conditioned_entropy(struct octeon_rnm_softc *sc)
{

	bus_space_write_8(sc->sc_bust, sc->sc_regh, RNM_CTL_STATUS_OFFSET,
	    RNM_CTL_STATUS_RNG_EN|RNM_CTL_STATUS_ENT_EN);
}

/*
 * octeon_rnm_raw_entropy(sc, rogroup)
 *
 *	Switch the RNM unit to generate raw ring oscillator samples
 *	from the specified group of eight ring oscillator.
 */
static void
octeon_rnm_raw_entropy(struct octeon_rnm_softc *sc, unsigned rogroup)
{
	uint64_t ctl = 0;

	ctl |= RNM_CTL_STATUS_RNG_EN;	/* enable FIFO */
	ctl |= RNM_CTL_STATUS_ENT_EN;	/* enable entropy source */
	ctl |= RNM_CTL_STATUS_EXP_ENT;	/* expose entropy without LFSR/SHA-1 */
	ctl |= __SHIFTIN(rogroup, RNM_CTL_STATUS_ENT_SEL_MASK);

	bus_space_write_8(sc->sc_bust, sc->sc_regh, RNM_CTL_STATUS_OFFSET,
	    ctl);
}

/*
 * octeon_rnm_load(sc)
 *
 *	Load a single 64-bit word out of the FIFO.
 */
static uint64_t
octeon_rnm_load(struct octeon_rnm_softc *sc)
{
	uint64_t addr =
	    RNM_OPERATION_BASE_IO_BIT |
	    __BITS64_SET(RNM_OPERATION_BASE_MAJOR_DID, 0x08) |
	    __BITS64_SET(RNM_OPERATION_BASE_SUB_DID, 0x00);

	return octeon_xkphys_read_8(addr);
}

/*
 * octeon_rnm_iobdma(sc, buf, nwords)
 *
 *	Load nwords, at most 32, out of the FIFO into buf.
 */
static void
octeon_rnm_iobdma(struct octeon_rnm_softc *sc, uint64_t *buf, unsigned nwords)
{
	size_t scraddr = OCTEON_CVMSEG_OFFSET(csm_rnm);
	uint64_t iobdma =
	    __SHIFTIN(scraddr/sizeof(uint64_t), IOBDMA_SCRADDR) |
	    __SHIFTIN(nwords, IOBDMA_LEN) |
	    __SHIFTIN(RNM_IOBDMA_MAJORDID, IOBDMA_MAJORDID) |
	    __SHIFTIN(RNM_IOBDMA_SUBDID, IOBDMA_SUBDID);

	KASSERT(nwords < 256);	/* iobdma address restriction */
	KASSERT(nwords <= 32);	/* octeon_cvmseg_map limitation */

	octeon_iobdma_write_8(iobdma);
	OCTEON_SYNCIOBDMA;
	for (; nwords --> 0; scraddr += 8)
		*buf++ = octeon_cvmseg_read_8(scraddr);
}

/*
 * octeon_rnm_delay(ncycles)
 *
 *	Wait ncycles, at most UINT32_MAX/2 so we behave reasonably even
 *	if the cycle counter rolls over.
 */
static void
octeon_rnm_delay(uint32_t ncycles)
{
	uint32_t deadline = mips3_cp0_count_read() + ncycles;

	KASSERT(ncycles <= UINT32_MAX/2);

	while ((deadline - mips3_cp0_count_read()) < ncycles)
		continue;
}
