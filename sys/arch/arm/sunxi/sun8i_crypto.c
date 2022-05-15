/*	$NetBSD: sun8i_crypto.c,v 1.31 2022/05/15 16:58:28 riastradh Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
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
 * sun8i_crypto -- Allwinner Crypto Engine driver
 *
 * The Crypto Engine is documented in Sec. 3.15 of the Allwinner A64
 * User Manual v1.1, on pp. 230--241.  We only use it for the TRNG at
 * the moment, but in principle it could be wired up with opencrypto(9)
 * to compute AES, DES, 3DES, MD5, SHA-1, SHA-224, SHA-256, HMAC-SHA1,
 * HMAC-HA256, RSA, and an undocumented PRNG.  It also seems to support
 * AES keys in SRAM (for some kind of HDMI HDCP stuff?).
 *
 * https://linux-sunxi.org/images/b/b4/Allwinner_A64_User_Manual_V1.1.pdf
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: sun8i_crypto.c,v 1.31 2022/05/15 16:58:28 riastradh Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/bus.h>
#include <sys/callout.h>
#include <sys/conf.h>
#include <sys/cprng.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/mbuf.h>
#include <sys/mutex.h>
#include <sys/rndsource.h>
#include <sys/sdt.h>
#include <sys/sysctl.h>
#include <sys/workqueue.h>

#include <dev/fdt/fdtvar.h>

#include <opencrypto/cryptodev.h>

#include <arm/sunxi/sun8i_crypto.h>

#define	SUN8I_CRYPTO_TIMEOUT	hz
#define	SUN8I_CRYPTO_RNGENTROPY	100 /* estimated bits per bit of entropy */
#define	SUN8I_CRYPTO_RNGBYTES	PAGE_SIZE

struct sun8i_crypto_config {
	u_int	mod_rate;	/* module clock rate */
};

/*
 * The module clock is set to 50 MHz on H3, 300 MHz otherwise.
 * From Linux drivers/crypto/allwinner/sun8i-ce/sun8i-ce-core.c:
 * Module clock is lower on H3 than other SoC due to some DMA
 * timeout occurring with high value.
 */
static const struct sun8i_crypto_config sun50i_a64_crypto_config = {
	.mod_rate = 300*1000*1000,
};

static const struct sun8i_crypto_config sun50i_h5_crypto_config = {
	.mod_rate = 300*1000*1000,
};

static const struct sun8i_crypto_config sun8i_h3_crypto_config = {
	.mod_rate = 50*1000*1000,
};

struct sun8i_crypto_task;

struct sun8i_crypto_buf {
	bus_dma_segment_t	cb_seg[1];
	int			cb_nsegs;
	void			*cb_kva;
};

struct sun8i_crypto_softc {
	device_t			sc_dev;
	bus_space_tag_t			sc_bst;
	bus_space_handle_t		sc_bsh;
	bus_dma_tag_t			sc_dmat;
	struct pool_cache		*sc_taskpool;

	const struct sun8i_crypto_config *sc_cfg;

	struct workqueue		*sc_wq;
	void				*sc_ih;
	bool				sc_polling;

	kmutex_t			sc_lock;
	struct sun8i_crypto_chan {
		struct sun8i_crypto_task	*cc_task;
		unsigned			cc_starttime;
	}				sc_chan[SUN8I_CRYPTO_NCHAN];
	struct callout			sc_timeout;

	kmutex_t			sc_intr_lock;
	uint32_t			sc_done;
	uint32_t			sc_esr;
	struct work			sc_work;
	bool				sc_work_pending;

	struct sun8i_crypto_rng {
		struct sun8i_crypto_buf		cr_buf;
		struct sun8i_crypto_task	*cr_task;
		struct krndsource		cr_rndsource;
		bool				cr_pending;
	}				sc_rng;
	struct sun8i_crypto_selftest {
		struct sun8i_crypto_buf		cs_in;
		struct sun8i_crypto_buf		cs_key;
		struct sun8i_crypto_buf		cs_out;
		struct sun8i_crypto_task	*cs_task;
		bool				cs_pending;
		bool				cs_passed;
	}				sc_selftest;
	struct sun8i_crypto_sysctl {
		struct sysctllog		*cy_log;
		const struct sysctlnode		*cy_root_node;
		const struct sysctlnode		*cy_trng_node;
	}				sc_sysctl;
	struct sun8i_crypto_opencrypto {
		uint32_t			co_driverid;
	}				sc_opencrypto;
};

struct sun8i_crypto_task {
	struct sun8i_crypto_buf	ct_descbuf;
	struct sun8i_crypto_taskdesc *ct_desc;
	struct sun8i_crypto_buf	ct_ivbuf;
	void			*ct_iv;
	struct sun8i_crypto_buf	ct_ctrbuf;
	void			*ct_ctr;
	bus_dmamap_t		ct_descmap;
	bus_dmamap_t		ct_keymap;
	bus_dmamap_t		ct_ivmap;	/* IV input */
	bus_dmamap_t		ct_ctrmap;	/* updated IV output */
	bus_dmamap_t		ct_srcmap;
	bus_dmamap_t		ct_dstmap;
	uint32_t		ct_nbytes;
	int			ct_flags;
#define	TASK_KEY		__BIT(0)
#define	TASK_IV			__BIT(1)
#define	TASK_CTR		__BIT(2)
#define	TASK_SRC		__BIT(3)
#define	TASK_BYTES		__BIT(4) /* datalen is in bytes, not words */
	void			(*ct_callback)(struct sun8i_crypto_softc *,
				    struct sun8i_crypto_task *, void *, int);
	void			*ct_cookie;
};

#define	SUN8I_CRYPTO_MAXDMASIZE		PAGE_SIZE
#define	SUN8I_CRYPTO_MAXDMASEGSIZE	PAGE_SIZE

CTASSERT(SUN8I_CRYPTO_MAXDMASIZE <= SUN8I_CRYPTO_MAXDATALEN);
CTASSERT(SUN8I_CRYPTO_MAXDMASEGSIZE <= SUN8I_CRYPTO_MAXSEGLEN);

/*
 * Forward declarations
 */

static int	sun8i_crypto_match(device_t, cfdata_t, void *);
static void	sun8i_crypto_attach(device_t, device_t, void *);

static int	sun8i_crypto_task_ctor(void *, void *, int);
static void	sun8i_crypto_task_dtor(void *, void *);
static struct sun8i_crypto_task *
		sun8i_crypto_task_get(struct sun8i_crypto_softc *,
		    void (*)(struct sun8i_crypto_softc *,
			struct sun8i_crypto_task *, void *, int),
		    void *, int);
static void	sun8i_crypto_task_put(struct sun8i_crypto_softc *,
		    struct sun8i_crypto_task *);

static int	sun8i_crypto_task_load(struct sun8i_crypto_softc *,
		    struct sun8i_crypto_task *, uint32_t,
		    uint32_t, uint32_t, uint32_t);
static int	sun8i_crypto_task_scatter(struct sun8i_crypto_task *,
		    struct sun8i_crypto_adrlen *, bus_dmamap_t, uint32_t);

static int	sun8i_crypto_task_load_trng(struct sun8i_crypto_softc *,
		    struct sun8i_crypto_task *, uint32_t);
static int	sun8i_crypto_task_load_aesecb(struct sun8i_crypto_softc *,
		    struct sun8i_crypto_task *, uint32_t, uint32_t, uint32_t);

static int	sun8i_crypto_submit(struct sun8i_crypto_softc *,
		    struct sun8i_crypto_task *);

static void	sun8i_crypto_timeout(void *);
static int	sun8i_crypto_intr(void *);
static void	sun8i_crypto_schedule_worker(struct sun8i_crypto_softc *);
static void	sun8i_crypto_worker(struct work *, void *);

static bool	sun8i_crypto_poll(struct sun8i_crypto_softc *, uint32_t *,
		    uint32_t *);
static bool	sun8i_crypto_done(struct sun8i_crypto_softc *, uint32_t,
		    uint32_t, unsigned);
static bool	sun8i_crypto_chan_done(struct sun8i_crypto_softc *, unsigned,
		    int);

static int	sun8i_crypto_allocbuf(struct sun8i_crypto_softc *, size_t,
		    struct sun8i_crypto_buf *, int);
static void	sun8i_crypto_freebuf(struct sun8i_crypto_softc *, size_t,
		    struct sun8i_crypto_buf *);

static void	sun8i_crypto_rng_attach(struct sun8i_crypto_softc *);
static void	sun8i_crypto_rng_get(size_t, void *);
static void	sun8i_crypto_rng_done(struct sun8i_crypto_softc *,
		    struct sun8i_crypto_task *, void *, int);

static bool	sun8i_crypto_selftest(struct sun8i_crypto_softc *);
static void	sun8i_crypto_selftest_done(struct sun8i_crypto_softc *,
		    struct sun8i_crypto_task *, void *, int);

static void	sun8i_crypto_sysctl_attach(struct sun8i_crypto_softc *);
static int	sun8i_crypto_sysctl_rng(SYSCTLFN_ARGS);
static void	sun8i_crypto_sysctl_rng_done(struct sun8i_crypto_softc *,
		    struct sun8i_crypto_task *, void *, int);

static void	sun8i_crypto_register(struct sun8i_crypto_softc *);
static void	sun8i_crypto_register1(struct sun8i_crypto_softc *, uint32_t);
static int	sun8i_crypto_newsession(void *, uint32_t *,
		    struct cryptoini *);
static int	sun8i_crypto_freesession(void *, uint64_t);
static u_int	sun8i_crypto_ivlen(const struct cryptodesc *);
static int	sun8i_crypto_process(void *, struct cryptop *, int);
static void	sun8i_crypto_callback(struct sun8i_crypto_softc *,
		    struct sun8i_crypto_task *, void *, int);

/*
 * Probes
 */

SDT_PROBE_DEFINE2(sdt, sun8i_crypto, register, read,
    "bus_size_t"/*reg*/,
    "uint32_t"/*value*/);
SDT_PROBE_DEFINE2(sdt, sun8i_crypto, register, write,
    "bus_size_t"/*reg*/,
    "uint32_t"/*write*/);

SDT_PROBE_DEFINE1(sdt, sun8i_crypto, task, ctor__success,
    "struct sun8i_crypto_task *"/*task*/);
SDT_PROBE_DEFINE1(sdt, sun8i_crypto, task, ctor__failure,
    "int"/*error*/);
SDT_PROBE_DEFINE1(sdt, sun8i_crypto, task, dtor,
    "struct sun8i_crypto_task *"/*task*/);
SDT_PROBE_DEFINE1(sdt, sun8i_crypto, task, get,
    "struct sun8i_crypto_task *"/*task*/);
SDT_PROBE_DEFINE1(sdt, sun8i_crypto, task, put,
    "struct sun8i_crypto_task *"/*task*/);

SDT_PROBE_DEFINE6(sdt, sun8i_crypto, task, load,
    "struct sun8i_crypto_task *"/*task*/,
    "uint32_t"/*tdqc*/,
    "uint32_t"/*tdqs*/,
    "uint32_t"/*tdqa*/,
    "struct sun8i_crypto_taskdesc *"/*desc*/,
    "int"/*error*/);
SDT_PROBE_DEFINE3(sdt, sun8i_crypto, task, misaligned,
    "struct sun8i_crypto_task *"/*task*/,
    "bus_addr_t"/*ds_addr*/,
    "bus_size_t"/*ds_len*/);
SDT_PROBE_DEFINE2(sdt, sun8i_crypto, task, done,
    "struct sun8i_crypto_task *"/*task*/,
    "int"/*error*/);

SDT_PROBE_DEFINE3(sdt, sun8i_crypto, engine, submit__failure,
    "struct sun8i_crypto_softc *"/*sc*/,
    "struct sun8i_crypto_task *"/*task*/,
    "int"/*error*/);
SDT_PROBE_DEFINE3(sdt, sun8i_crypto, engine, submit__success,
    "struct sun8i_crypto_softc *"/*sc*/,
    "struct sun8i_crypto_task *"/*task*/,
    "unsigned"/*chan*/);
SDT_PROBE_DEFINE3(sdt, sun8i_crypto, engine, intr,
    "struct sun8i_crypto_softc *"/*sc*/,
    "uint32_t"/*isr*/,
    "uint32_t"/*esr*/);
SDT_PROBE_DEFINE3(sdt, sun8i_crypto, engine, done,
    "struct sun8i_crypto_softc *"/*sc*/,
    "unsigned"/*chan*/,
    "int"/*error*/);

SDT_PROBE_DEFINE3(sdt, sun8i_crypto, process, entry,
    "struct sun8i_crypto_softc *"/*sc*/,
    "struct cryptop *"/*crp*/,
    "int"/*hint*/);
SDT_PROBE_DEFINE3(sdt, sun8i_crypto, process, busy,
    "struct sun8i_crypto_softc *"/*sc*/,
    "struct cryptop *"/*crp*/,
    "int"/*hint*/);
SDT_PROBE_DEFINE4(sdt, sun8i_crypto, process, queued,
    "struct sun8i_crypto_softc *"/*sc*/,
    "struct cryptop *"/*crp*/,
    "int"/*hint*/,
    "struct sun8i_crypto_task *"/*task*/);
SDT_PROBE_DEFINE3(sdt, sun8i_crypto, process, done,
    "struct sun8i_crypto_softc *"/*sc*/,
    "struct cryptop *"/*crp*/,
    "int"/*error*/);

/*
 * Register access
 */

static uint32_t
sun8i_crypto_read(struct sun8i_crypto_softc *sc, bus_size_t reg)
{
	uint32_t v = bus_space_read_4(sc->sc_bst, sc->sc_bsh, reg);

	SDT_PROBE2(sdt, sun8i_crypto, register, read,  reg, v);
	return v;
}

static void
sun8i_crypto_write(struct sun8i_crypto_softc *sc, bus_size_t reg, uint32_t v)
{

	SDT_PROBE2(sdt, sun8i_crypto, register, write,  reg, v);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, reg, v);
}

/*
 * Autoconf goo
 */

CFATTACH_DECL_NEW(sun8i_crypto, sizeof(struct sun8i_crypto_softc),
    sun8i_crypto_match, sun8i_crypto_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "allwinner,sun50i-a64-crypto",
	  .data = &sun50i_a64_crypto_config },
	{ .compat = "allwinner,sun50i-h5-crypto",
	  .data = &sun50i_h5_crypto_config },
	{ .compat = "allwinner,sun8i-h3-crypto",
	  .data = &sun8i_h3_crypto_config },
	DEVICE_COMPAT_EOL
};

static int
sun8i_crypto_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct fdt_attach_args *const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sun8i_crypto_attach(device_t parent, device_t self, void *aux)
{
	struct sun8i_crypto_softc *const sc = device_private(self);
	const struct fdt_attach_args *const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	struct clk *clk;
	struct fdtbus_reset *rst;
	u_int mod_rate;

	sc->sc_dev = self;
	sc->sc_dmat = faa->faa_dmat;
	sc->sc_bst = faa->faa_bst;
	sc->sc_taskpool = pool_cache_init(sizeof(struct sun8i_crypto_task),
	    0, 0, 0, "sun8icry", NULL, IPL_SOFTSERIAL,
	    &sun8i_crypto_task_ctor, &sun8i_crypto_task_dtor, sc);
	sc->sc_cfg = of_compatible_lookup(phandle, compat_data)->data;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SOFTSERIAL);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_VM);
	callout_init(&sc->sc_timeout, CALLOUT_MPSAFE);
	callout_setfunc(&sc->sc_timeout, &sun8i_crypto_timeout, sc);
	if (workqueue_create(&sc->sc_wq, device_xname(self),
		&sun8i_crypto_worker, sc, PRI_NONE, IPL_VM, WQ_MPSAFE) != 0) {
		aprint_error(": couldn't create workqueue\n");
		return;
	}

	/*
	 * Prime the pool with enough tasks that each channel can be
	 * busy with a task as we prepare another task for when it's
	 * done.
	 */
	pool_cache_prime(sc->sc_taskpool, 2*SUN8I_CRYPTO_NCHAN);

	/* Get and map device registers.  */
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	/* Get an interrupt handle.  */
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	/* Enable the bus clock.  */
	if (fdtbus_clock_enable(phandle, "bus", true) != 0) {
		aprint_error(": couldn't enable bus clock\n");
		return;
	}

	/* Get the module clock and set it. */
	mod_rate = sc->sc_cfg->mod_rate;
	if ((clk = fdtbus_clock_get(phandle, "mod")) != NULL) {
		if (clk_enable(clk) != 0) {
			aprint_error(": couldn't enable CE clock\n");
			return;
		}
		if (clk_set_rate(clk, mod_rate) != 0) {
			aprint_error(": couldn't set CE clock to %d MHz\n",
			    mod_rate / (1000 * 1000));
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
	aprint_normal(": Crypto Engine\n");
	aprint_debug_dev(self, ": clock freq %d\n", clk_get_rate(clk));

	/*
	 * Disable and clear interrupts.  Start in polling mode for
	 * synchronous self-tests and the first RNG draw.
	 */
	sun8i_crypto_write(sc, SUN8I_CRYPTO_ICR, 0);
	sun8i_crypto_write(sc, SUN8I_CRYPTO_ISR, 0);
	sc->sc_polling = true;

	/* Establish an interrupt handler.  */
	sc->sc_ih = fdtbus_intr_establish_xname(phandle, 0, IPL_VM,
	    FDT_INTR_MPSAFE, &sun8i_crypto_intr, sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	/* Perform self-tests.  If they fail, stop here.  */
	if (!sun8i_crypto_selftest(sc))
		return;

	/*
	 * Set up the RNG.  This will try to synchronously draw the
	 * first sample by polling, so do this before we establish
	 * the interrupt handler.
	 */
	sun8i_crypto_rng_attach(sc);

	/*
	 * Self-test has passed and first RNG draw has finished.  Use
	 * interrupts, not polling, for all subsequent tasks.  Set this
	 * atomically in case the interrupt handler has fired -- can't
	 * be from us because we've kept ICR set to 0 to mask all
	 * interrupts, but in case the interrupt vector is shared.
	 */
	atomic_store_relaxed(&sc->sc_polling, false);

	/* Attach the sysctl.  */
	sun8i_crypto_sysctl_attach(sc);

	/* Register opencrypto handlers.  */
	sun8i_crypto_register(sc);
}

static int
sun8i_crypto_task_ctor(void *cookie, void *vtask, int pflags)
{
	struct sun8i_crypto_softc *sc = cookie;
	struct sun8i_crypto_task *task = vtask;
	int dmaflags = (pflags & PR_WAITOK) ? BUS_DMA_WAITOK : BUS_DMA_NOWAIT;
	int error;

	/* Create a DMA buffer for the task descriptor.  */
	error = sun8i_crypto_allocbuf(sc, sizeof(*task->ct_desc),
	    &task->ct_descbuf, dmaflags);
	if (error)
		goto fail0;
	task->ct_desc = task->ct_descbuf.cb_kva;

	/* Create DMA buffers for the IV and CTR.  */
	error = sun8i_crypto_allocbuf(sc, SUN8I_CRYPTO_MAXIVBYTES,
	    &task->ct_ivbuf, dmaflags);
	if (error)
		goto fail1;
	task->ct_iv = task->ct_ivbuf.cb_kva;
	error = sun8i_crypto_allocbuf(sc, SUN8I_CRYPTO_MAXCTRBYTES,
	    &task->ct_ctrbuf, dmaflags);
	if (error)
		goto fail2;
	task->ct_ctr = task->ct_ctrbuf.cb_kva;

	/* Create a DMA map for the task descriptor and preload it.  */
	error = bus_dmamap_create(sc->sc_dmat, sizeof(*task->ct_desc), 1,
	    sizeof(*task->ct_desc), 0, dmaflags, &task->ct_descmap);
	if (error)
		goto fail3;
	error = bus_dmamap_load(sc->sc_dmat, task->ct_descmap, task->ct_desc,
	    sizeof(*task->ct_desc), NULL, BUS_DMA_WAITOK);
	if (error)
		goto fail4;

	/* Create DMA maps for the key, IV, and CTR.  */
	error = bus_dmamap_create(sc->sc_dmat, SUN8I_CRYPTO_MAXKEYBYTES, 1,
	    SUN8I_CRYPTO_MAXKEYBYTES, 0, dmaflags, &task->ct_keymap);
	if (error)
		goto fail5;
	error = bus_dmamap_create(sc->sc_dmat, SUN8I_CRYPTO_MAXIVBYTES, 1,
	    SUN8I_CRYPTO_MAXIVBYTES, 0, dmaflags, &task->ct_ivmap);
	if (error)
		goto fail6;
	error = bus_dmamap_create(sc->sc_dmat, SUN8I_CRYPTO_MAXCTRBYTES, 1,
	    SUN8I_CRYPTO_MAXCTRBYTES, 0, dmaflags, &task->ct_ctrmap);
	if (error)
		goto fail7;

	/* Create DMA maps for the src and dst scatter/gather vectors.  */
	error = bus_dmamap_create(sc->sc_dmat, SUN8I_CRYPTO_MAXDMASIZE,
	    SUN8I_CRYPTO_MAXSEGS, SUN8I_CRYPTO_MAXDMASEGSIZE, 0, dmaflags,
	    &task->ct_srcmap);
	if (error)
		goto fail8;
	error = bus_dmamap_create(sc->sc_dmat, SUN8I_CRYPTO_MAXDMASIZE,
	    SUN8I_CRYPTO_MAXSEGS, SUN8I_CRYPTO_MAXDMASEGSIZE, 0, dmaflags,
	    &task->ct_dstmap);
	if (error)
		goto fail9;

	/* Success!  */
	SDT_PROBE1(sdt, sun8i_crypto, task, ctor__success,  task);
	return 0;

fail10: __unused
	bus_dmamap_destroy(sc->sc_dmat, task->ct_dstmap);
fail9:	bus_dmamap_destroy(sc->sc_dmat, task->ct_srcmap);
fail8:	bus_dmamap_destroy(sc->sc_dmat, task->ct_ctrmap);
fail7:	bus_dmamap_destroy(sc->sc_dmat, task->ct_ivmap);
fail6:	bus_dmamap_destroy(sc->sc_dmat, task->ct_keymap);
fail5:	bus_dmamap_unload(sc->sc_dmat, task->ct_descmap);
fail4:	bus_dmamap_destroy(sc->sc_dmat, task->ct_descmap);
fail3:	sun8i_crypto_freebuf(sc, SUN8I_CRYPTO_MAXIVBYTES, &task->ct_ivbuf);
fail2:	sun8i_crypto_freebuf(sc, SUN8I_CRYPTO_MAXCTRBYTES, &task->ct_ctrbuf);
fail1:	sun8i_crypto_freebuf(sc, sizeof(*task->ct_desc), &task->ct_descbuf);
fail0:	SDT_PROBE1(sdt, sun8i_crypto, task, ctor__failure,  error);
	return error;
}

static void
sun8i_crypto_task_dtor(void *cookie, void *vtask)
{
	struct sun8i_crypto_softc *sc = cookie;
	struct sun8i_crypto_task *task = vtask;

	SDT_PROBE1(sdt, sun8i_crypto, task, dtor,  task);

	/* XXX Zero the bounce buffers if there are any.  */

	bus_dmamap_destroy(sc->sc_dmat, task->ct_dstmap);
	bus_dmamap_destroy(sc->sc_dmat, task->ct_srcmap);
	bus_dmamap_destroy(sc->sc_dmat, task->ct_ctrmap);
	bus_dmamap_destroy(sc->sc_dmat, task->ct_ivmap);
	bus_dmamap_destroy(sc->sc_dmat, task->ct_keymap);
	bus_dmamap_unload(sc->sc_dmat, task->ct_descmap);
	bus_dmamap_destroy(sc->sc_dmat, task->ct_descmap);
	sun8i_crypto_freebuf(sc, SUN8I_CRYPTO_MAXIVBYTES, &task->ct_ivbuf);
	sun8i_crypto_freebuf(sc, SUN8I_CRYPTO_MAXCTRBYTES, &task->ct_ctrbuf);
	sun8i_crypto_freebuf(sc, sizeof(*task->ct_desc), &task->ct_descbuf);
}

/*
 * sun8i_crypto_task_get(sc, callback, cookie, pflags)
 *
 *	Allocate a task that will call callback(sc, task, cookie,
 *	error) when done.  pflags is PR_WAITOK or PR_NOWAIT; if
 *	PR_NOWAIT, may fail and return NULL.  No further allocation is
 *	needed to submit the task if this succeeds (although task
 *	submission may still fail if all channels are busy).
 */
static struct sun8i_crypto_task *
sun8i_crypto_task_get(struct sun8i_crypto_softc *sc,
    void (*callback)(struct sun8i_crypto_softc *, struct sun8i_crypto_task *,
	void *, int),
    void *cookie, int pflags)
{
	struct sun8i_crypto_task *task;

	/* Allocate a task, or fail if we can't.  */
	task = pool_cache_get(sc->sc_taskpool, pflags);
	if (task == NULL)
		goto out;

	/* Set up flags and the callback.  */
	task->ct_flags = 0;
	task->ct_callback = callback;
	task->ct_cookie = cookie;

out:	SDT_PROBE1(sdt, sun8i_crypto, task, get,  task);
	return task;
}

/*
 * sun8i_crypto_task_invalid(sc, task, cookie, error)
 *
 *	Callback for a task not currently in use, to detect errors.
 */
static void
sun8i_crypto_task_invalid(struct sun8i_crypto_softc *sc,
    struct sun8i_crypto_task *task, void *cookie, int error)
{
	void (*callback)(struct sun8i_crypto_softc *,
	    struct sun8i_crypto_task *, void *, int) = cookie;

	panic("task for callback %p used after free", callback);
}

/*
 * sun8i_crypto_task_put(sc, task)
 *
 *	Free a task obtained with sun8i_crypto_task_get.
 */
static void
sun8i_crypto_task_put(struct sun8i_crypto_softc *sc,
    struct sun8i_crypto_task *task)
{

	SDT_PROBE1(sdt, sun8i_crypto, task, put,  task);

	task->ct_cookie = task->ct_callback;
	task->ct_callback = &sun8i_crypto_task_invalid;
	pool_cache_put(sc->sc_taskpool, task);
}

/*
 * sun8i_crypto_task_load(sc, task, nbytes, tdqc, tdqs, tdqa)
 *
 *	Set up the task descriptor after the relevant DMA maps have
 *	been loaded for a transfer of nbytes.  bus_dmamap_sync matches
 *	sun8i_crypto_chan_done.  May fail if input is inadequately
 *	aligned.
 *
 *	XXX Teach this to support task chains.
 */
static int
sun8i_crypto_task_load(struct sun8i_crypto_softc *sc,
    struct sun8i_crypto_task *task, uint32_t nbytes,
    uint32_t tdqc, uint32_t tdqs, uint32_t tdqa)
{
	struct sun8i_crypto_taskdesc *desc = task->ct_desc;
	int error;

	KASSERT(tdqs == 0 || tdqa == 0);
	KASSERT(nbytes % 4 == 0);

	memset(desc, 0, sizeof(*desc));

	/* Always enable interrupt for the task.  */
	tdqc |= SUN8I_CRYPTO_TDQC_INTR_EN;

	desc->td_tdqc = htole32(tdqc);
	desc->td_tdqs = htole32(tdqs);
	desc->td_tdqa = htole32(tdqa);

	if (task->ct_flags & TASK_KEY) {
		bus_dmamap_t keymap = task->ct_keymap;
		KASSERT(keymap->dm_nsegs == 1);
		desc->td_keydesc = htole32(keymap->dm_segs[0].ds_addr);
		bus_dmamap_sync(sc->sc_dmat, keymap, 0,
		    keymap->dm_segs[0].ds_len, BUS_DMASYNC_PREWRITE);
	}
	if (task->ct_flags & TASK_IV) {
		bus_dmamap_t ivmap = task->ct_ivmap;
		KASSERT(ivmap->dm_nsegs == 1);
		desc->td_ivdesc = htole32(ivmap->dm_segs[0].ds_addr);
		bus_dmamap_sync(sc->sc_dmat, ivmap, 0,
		    ivmap->dm_segs[0].ds_len, BUS_DMASYNC_PREWRITE);
	}
	if (task->ct_flags & TASK_CTR) {
		bus_dmamap_t ctrmap = task->ct_ctrmap;
		KASSERT(ctrmap->dm_nsegs == 1);
		desc->td_ctrdesc = htole32(ctrmap->dm_segs[0].ds_addr);
		bus_dmamap_sync(sc->sc_dmat, ctrmap, 0,
		    ctrmap->dm_segs[0].ds_len, BUS_DMASYNC_PREREAD);
	}

	if (task->ct_flags & TASK_BYTES)
		desc->td_datalen = htole32(nbytes);
	else
		desc->td_datalen = htole32(nbytes/4);

	if (task->ct_flags & TASK_SRC) {
		bus_dmamap_t srcmap = task->ct_srcmap;
		KASSERT(srcmap->dm_mapsize == task->ct_dstmap->dm_mapsize);
		error = sun8i_crypto_task_scatter(task, desc->td_src, srcmap,
		    nbytes);
		if (error)
			return error;
		bus_dmamap_sync(sc->sc_dmat, srcmap, 0, nbytes,
		    BUS_DMASYNC_PREWRITE);
	}

	error = sun8i_crypto_task_scatter(task, desc->td_dst, task->ct_dstmap,
	    nbytes);
	if (error)
		goto out;
	bus_dmamap_sync(sc->sc_dmat, task->ct_dstmap, 0, nbytes,
	    BUS_DMASYNC_PREREAD);

	task->ct_nbytes = nbytes;

	/* Success!  */
	error = 0;

out:	SDT_PROBE6(sdt, sun8i_crypto, task, load,
	    task, tdqc, tdqs, tdqa, desc, error);
	return error;
}

/*
 * sun8i_crypto_task_scatter(task, adrlen, map, nbytes)
 *
 *	Set up a task's scatter/gather vector -- src or dst -- with the
 *	given DMA map for a transfer of nbytes.  May fail if input is
 *	inadequately aligned.
 */
static int
sun8i_crypto_task_scatter(struct sun8i_crypto_task *task,
    struct sun8i_crypto_adrlen *adrlen, bus_dmamap_t map,
    uint32_t nbytes __diagused)
{
	uint32_t total __diagused = 0;
	unsigned i;

	/*
	 * Verify that the alignment is correct and initialize the
	 * scatter/gather vector.
	 */
	KASSERT(map->dm_nsegs <= SUN8I_CRYPTO_MAXSEGS);
	for (i = 0; i < map->dm_nsegs; i++) {
		if ((map->dm_segs[i].ds_addr % 4) |
		    (map->dm_segs[i].ds_len % 4)) {
			SDT_PROBE3(sdt, sun8i_crypto, task, misaligned,
			    task,
			    map->dm_segs[i].ds_addr,
			    map->dm_segs[i].ds_len);
			return EINVAL;
		}
		KASSERT(map->dm_segs[i].ds_addr <= UINT32_MAX);
		KASSERT(map->dm_segs[i].ds_len <= UINT32_MAX - total);
		adrlen[i].adr = htole32(map->dm_segs[i].ds_addr);
		adrlen[i].len = htole32(map->dm_segs[i].ds_len/4);
		total += map->dm_segs[i].ds_len;
	}

	/* Set the remainder to zero.  */
	for (; i < SUN8I_CRYPTO_MAXSEGS; i++) {
		adrlen[i].adr = 0;
		adrlen[i].len = 0;
	}

	/* Verify the total size matches the transfer length.  */
	KASSERT(total == nbytes);

	/* Success!  */
	return 0;
}

/*
 * sun8i_crypto_task_load_trng(task, nbytes)
 *
 *	Set up the task descriptor for a transfer of nbytes from the
 *	TRNG.
 */
static int
sun8i_crypto_task_load_trng(struct sun8i_crypto_softc *sc,
    struct sun8i_crypto_task *task, uint32_t nbytes)
{
	uint32_t tdqc = 0;

	/* Caller must provide dst only.  */
	KASSERT((task->ct_flags & TASK_KEY) == 0);
	KASSERT((task->ct_flags & TASK_IV) == 0);
	KASSERT((task->ct_flags & TASK_CTR) == 0);
	KASSERT((task->ct_flags & TASK_SRC) == 0);

	/* Set up the task descriptor queue control words.  */
	tdqc |= __SHIFTIN(SUN8I_CRYPTO_TDQC_METHOD_TRNG,
	    SUN8I_CRYPTO_TDQC_METHOD);

	/* Fill in the descriptor.  */
	return sun8i_crypto_task_load(sc, task, nbytes, tdqc, 0, 0);
}

static int
sun8i_crypto_task_load_aesecb(struct sun8i_crypto_softc *sc,
    struct sun8i_crypto_task *task,
    uint32_t nbytes, uint32_t keysize, uint32_t dir)
{
	uint32_t tdqc = 0, tdqs = 0;

	/* Caller must provide key, src, and dst only.  */
	KASSERT(task->ct_flags & TASK_KEY);
	KASSERT((task->ct_flags & TASK_IV) == 0);
	KASSERT((task->ct_flags & TASK_CTR) == 0);
	KASSERT(task->ct_flags & TASK_SRC);

	/* Set up the task descriptor queue control word.  */
	tdqc |= __SHIFTIN(SUN8I_CRYPTO_TDQC_METHOD_AES,
	    SUN8I_CRYPTO_TDQC_METHOD);
	tdqc |= __SHIFTIN(dir, SUN8I_CRYPTO_TDQC_OP_DIR);

#ifdef DIAGNOSTIC
	switch (keysize) {
	case SUN8I_CRYPTO_TDQS_AES_KEYSIZE_128:
		KASSERT(task->ct_keymap->dm_segs[0].ds_len == 16);
		break;
	case SUN8I_CRYPTO_TDQS_AES_KEYSIZE_192:
		KASSERT(task->ct_keymap->dm_segs[0].ds_len == 24);
		break;
	case SUN8I_CRYPTO_TDQS_AES_KEYSIZE_256:
		KASSERT(task->ct_keymap->dm_segs[0].ds_len == 32);
		break;
	}
#endif

	/* Set up the symmetric control word.  */
	tdqs |= __SHIFTIN(SUN8I_CRYPTO_TDQS_SKEY_SELECT_SS_KEYx,
	    SUN8I_CRYPTO_TDQS_SKEY_SELECT);
	tdqs |= __SHIFTIN(SUN8I_CRYPTO_TDQS_OP_MODE_ECB,
	    SUN8I_CRYPTO_TDQS_OP_MODE);
	tdqs |= __SHIFTIN(keysize, SUN8I_CRYPTO_TDQS_AES_KEYSIZE);

	/* Fill in the descriptor.  */
	return sun8i_crypto_task_load(sc, task, nbytes, tdqc, tdqs, 0);
}

/*
 * sun8i_crypto_submit(sc, task)
 *
 *	Submit a task to the crypto engine after it has been loaded
 *	with sun8i_crypto_task_load.  On success, guarantees to
 *	eventually call the task's callback.
 */
static int
sun8i_crypto_submit(struct sun8i_crypto_softc *sc,
    struct sun8i_crypto_task *task)
{
	unsigned i, retries = 0;
	uint32_t icr;
	int error = 0;

	/* One at a time at the device registers, please.  */
	mutex_enter(&sc->sc_lock);

	/* Find a channel.  */
	for (i = 0; i < SUN8I_CRYPTO_NCHAN; i++) {
		if (sc->sc_chan[i].cc_task == NULL)
			break;
	}
	if (i == SUN8I_CRYPTO_NCHAN) {
		device_printf(sc->sc_dev, "no free channels\n");
		error = ERESTART;
		goto out;
	}

	/*
	 * Set the channel id.  Caller is responsible for setting up
	 * all other parts of the descriptor.
	 */
	task->ct_desc->td_cid = htole32(i);

	/*
	 * Prepare to send the descriptor to the device by DMA.
	 * Matches POSTWRITE in sun8i_crypto_chan_done.
	 */
	bus_dmamap_sync(sc->sc_dmat, task->ct_descmap, 0,
	    sizeof(*task->ct_desc), BUS_DMASYNC_PREWRITE);

	/* Confirm we're ready to go.  */
	if (sun8i_crypto_read(sc, SUN8I_CRYPTO_TLR) & SUN8I_CRYPTO_TLR_LOAD) {
		device_printf(sc->sc_dev, "TLR not clear\n");
		error = EIO;
		goto out;
	}

	/*
	 * Enable interrupts for this channel, unless we're still
	 * polling.
	 */
	if (!sc->sc_polling) {
		icr = sun8i_crypto_read(sc, SUN8I_CRYPTO_ICR);
		icr |= __SHIFTIN(SUN8I_CRYPTO_ICR_INTR_EN_CHAN(i),
		    SUN8I_CRYPTO_ICR_INTR_EN);
		sun8i_crypto_write(sc, SUN8I_CRYPTO_ICR, icr);
	}

	/* Set the task descriptor queue address.  */
	sun8i_crypto_write(sc, SUN8I_CRYPTO_TDQ,
	    task->ct_descmap->dm_segs[0].ds_addr);

	/* Notify the engine to load it, and wait for acknowledgement.  */
	sun8i_crypto_write(sc, SUN8I_CRYPTO_TLR, SUN8I_CRYPTO_TLR_LOAD);
	while (sun8i_crypto_read(sc, SUN8I_CRYPTO_TLR) & SUN8I_CRYPTO_TLR_LOAD)
	{
		/*
		 * XXX Timeout pulled from arse.  Is it even important
		 * to wait here?
		 */
		if (++retries == 1000) {
			device_printf(sc->sc_dev, "TLR didn't clear: %08x\n",
			    sun8i_crypto_read(sc, SUN8I_CRYPTO_TLR));
			/*
			 * Hope it clears eventually; if not, we'll
			 * time out.
			 */
			break;
		}
		DELAY(1);
	}

	/*
	 * Loaded up and ready to go.  Start a timer ticking if it's
	 * not already and we're not polling.
	 */
	sc->sc_chan[i].cc_task = task;
	sc->sc_chan[i].cc_starttime = getticks();
	if (!sc->sc_polling && !callout_pending(&sc->sc_timeout))
		callout_schedule(&sc->sc_timeout, SUN8I_CRYPTO_TIMEOUT);

out:	/* Done!  */
	if (error)
		SDT_PROBE3(sdt, sun8i_crypto, engine, submit__failure,
		    sc, task, error);
	else
		SDT_PROBE3(sdt, sun8i_crypto, engine, submit__success,
		    sc, task, i);
	mutex_exit(&sc->sc_lock);
	return error;
}

/*
 * sun8i_crypto_timeout(cookie)
 *
 *	Timeout handler.  Schedules work in a thread to cancel all
 *	pending tasks that were started long enough ago we're bored of
 *	waiting for them.
 */
static void
sun8i_crypto_timeout(void *cookie)
{
	struct sun8i_crypto_softc *sc = cookie;

	mutex_enter(&sc->sc_intr_lock);
	sun8i_crypto_schedule_worker(sc);
	mutex_exit(&sc->sc_intr_lock);
}

/*
 * sun8i_crypto_intr(cookie)
 *
 *	Device interrupt handler.  Find what channels have completed,
 *	whether with success or with failure, and schedule work in
 *	thread context to invoke the appropriate callbacks.
 */
static int
sun8i_crypto_intr(void *cookie)
{
	struct sun8i_crypto_softc *sc = cookie;
	uint32_t done, esr;

	if (atomic_load_relaxed(&sc->sc_polling) ||
	    !sun8i_crypto_poll(sc, &done, &esr))
		return 0;	/* not ours */

	mutex_enter(&sc->sc_intr_lock);
	sun8i_crypto_schedule_worker(sc);
	sc->sc_done |= done;
	sc->sc_esr |= esr;
	mutex_exit(&sc->sc_intr_lock);

	return 1;
}

/*
 * sun8i_crypto_schedule_worker(sc)
 *
 *	Ensure that crypto engine thread context work to invoke task
 *	callbacks will run promptly.  Idempotent.
 */
static void
sun8i_crypto_schedule_worker(struct sun8i_crypto_softc *sc)
{

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	/* Start the worker if necessary.  */
	if (!sc->sc_work_pending) {
		workqueue_enqueue(sc->sc_wq, &sc->sc_work, NULL);
		sc->sc_work_pending = true;
	}
}

/*
 * sun8i_crypto_worker(wk, cookie)
 *
 *	Thread-context worker: Invoke all task callbacks for which the
 *	device has notified us of completion or for which we gave up
 *	waiting.
 */
static void
sun8i_crypto_worker(struct work *wk, void *cookie)
{
	struct sun8i_crypto_softc *sc = cookie;
	uint32_t done, esr;

	/*
	 * Under the interrupt lock, acknowledge our work and claim the
	 * done mask and error status.
	 */
	mutex_enter(&sc->sc_intr_lock);
	KASSERT(sc->sc_work_pending);
	sc->sc_work_pending = false;
	done = sc->sc_done;
	esr = sc->sc_esr;
	sc->sc_done = 0;
	sc->sc_esr = 0;
	mutex_exit(&sc->sc_intr_lock);

	/*
	 * If we cleared any channels, it is time to allow opencrypto
	 * to issue new operations.  Asymmetric operations (which we
	 * don't support, at the moment, but we could) and symmetric
	 * operations (which we do) use the same task channels, so we
	 * unblock both kinds.
	 */
	if (sun8i_crypto_done(sc, done, esr, getticks())) {
		crypto_unblock(sc->sc_opencrypto.co_driverid,
		    CRYPTO_SYMQ|CRYPTO_ASYMQ);
	}
}

/*
 * sun8i_crypto_poll(sc, &done, &esr)
 *
 *	Poll for completion.  Sets done and esr to the mask of done
 *	channels and error channels.  Returns true if anything was
 *	done or failed.
 */
static bool
sun8i_crypto_poll(struct sun8i_crypto_softc *sc,
    uint32_t *donep, uint32_t *esrp)
{
	uint32_t isr, esr;

	/*
	 * Get and acknowledge the interrupts and error status.
	 *
	 * XXX Data sheet says the error status register is read-only,
	 * but then advises writing 1 to bit x1xx (keysram access error
	 * for AES, SUN8I_CRYPTO_ESR_KEYSRAMERR) to clear it.  What do?
	 */
	isr = sun8i_crypto_read(sc, SUN8I_CRYPTO_ISR);
	esr = sun8i_crypto_read(sc, SUN8I_CRYPTO_ESR);
	sun8i_crypto_write(sc, SUN8I_CRYPTO_ISR, isr);
	sun8i_crypto_write(sc, SUN8I_CRYPTO_ESR, esr);

	SDT_PROBE3(sdt, sun8i_crypto, engine, intr,  sc, isr, esr);

	*donep = __SHIFTOUT(isr, SUN8I_CRYPTO_ISR_DONE);
	*esrp = esr;

	return *donep || *esrp;
}

/*
 * sun8i_crypto_done(sc, done, esr, now)
 *
 *	Invoke all task callbacks for the channels in done or esr, or
 *	for which we gave up waiting, according to the time `now'.
 *	Returns true if any channels completed or timed out.
 */
static bool
sun8i_crypto_done(struct sun8i_crypto_softc *sc, uint32_t done, uint32_t esr,
    unsigned now)
{
	uint32_t esr_chan;
	unsigned i;
	bool anydone = false;
	bool schedtimeout = false;
	int error;

	/* Under the lock, process the channels.  */
	mutex_enter(&sc->sc_lock);
	for (i = 0; i < SUN8I_CRYPTO_NCHAN; i++) {
		/* Check whether the channel is done.  */
		if (!ISSET(done, SUN8I_CRYPTO_ISR_DONE_CHAN(i))) {
			/* Nope.  Do we have a task to time out?  */
			if (sc->sc_chan[i].cc_task != NULL) {
				if (now - sc->sc_chan[i].cc_starttime >=
				    SUN8I_CRYPTO_TIMEOUT) {
					anydone |= sun8i_crypto_chan_done(sc,
					    i, ETIMEDOUT);
				} else {
					schedtimeout = true;
				}
			}
			continue;
		}

		/* Channel is done.  Interpret the error if any.  */
		esr_chan = __SHIFTOUT(esr, SUN8I_CRYPTO_ESR_CHAN(i));
		if (esr_chan & SUN8I_CRYPTO_ESR_CHAN_ALGNOTSUP) {
			device_printf(sc->sc_dev, "channel %u:"
			    " alg not supported\n", i);
			error = ENODEV;
		} else if (esr_chan & SUN8I_CRYPTO_ESR_CHAN_DATALENERR) {
			device_printf(sc->sc_dev, "channel %u:"
			    " data length error\n", i);
			error = EIO;	/* XXX */
		} else if (esr_chan & SUN8I_CRYPTO_ESR_CHAN_KEYSRAMERR) {
			device_printf(sc->sc_dev, "channel %u:"
			    " key sram error\n", i);
			error = EIO;	/* XXX */
		} else if (esr_chan != 0) {
			error = EIO;	/* generic I/O error */
		} else {
			error = 0;
		}

		/*
		 * Notify the task of completion.  May release the lock
		 * to invoke a callback.
		 */
		anydone |= sun8i_crypto_chan_done(sc, i, error);
	}
	mutex_exit(&sc->sc_lock);

	/*
	 * If there are tasks still pending, make sure there's a
	 * timeout scheduled for them.  If the callout is already
	 * pending, it will take another pass through here to time some
	 * things out and schedule a new timeout.
	 */
	if (schedtimeout && !callout_pending(&sc->sc_timeout))
		callout_schedule(&sc->sc_timeout, SUN8I_CRYPTO_TIMEOUT);

	return anydone;
}

/*
 * sun8i_crypto_chan_done(sc, i, error)
 *
 *	Notify the callback for the task on channel i, if there is one,
 *	of the specified error, or 0 for success.
 */
static bool
sun8i_crypto_chan_done(struct sun8i_crypto_softc *sc, unsigned i, int error)
{
	struct sun8i_crypto_task *task;
	uint32_t nbytes;
	uint32_t icr;

	KASSERT(mutex_owned(&sc->sc_lock));

	SDT_PROBE3(sdt, sun8i_crypto, engine, done,  sc, i, error);

	/* Claim the task if there is one; bail if not.  */
	if ((task = sc->sc_chan[i].cc_task) == NULL) {
		device_printf(sc->sc_dev, "channel %u: no task but error=%d\n",
		    i, error);
		/* We did not clear a channel.  */
		return false;
	}
	sc->sc_chan[i].cc_task = NULL;

	/* Disable interrupts on this channel.  */
	icr = sun8i_crypto_read(sc, SUN8I_CRYPTO_ICR);
	icr &= ~__SHIFTIN(SUN8I_CRYPTO_ICR_INTR_EN_CHAN(i),
	    SUN8I_CRYPTO_ICR_INTR_EN);
	sun8i_crypto_write(sc, SUN8I_CRYPTO_ICR, icr);

	/*
	 * Finished sending the descriptor to the device by DMA.
	 * Matches PREWRITE in sun8i_crypto_task_submit.
	 */
	bus_dmamap_sync(sc->sc_dmat, task->ct_descmap, 0,
	    sizeof(*task->ct_desc), BUS_DMASYNC_POSTWRITE);

	/*
	 * Finished with all the other bits of DMA too.  Matches
	 * sun8i_crypto_task_load.
	 */
	nbytes = task->ct_nbytes;
	bus_dmamap_sync(sc->sc_dmat, task->ct_dstmap, 0, nbytes,
	    BUS_DMASYNC_POSTREAD);
	if (task->ct_flags & TASK_SRC)
		bus_dmamap_sync(sc->sc_dmat, task->ct_srcmap, 0, nbytes,
		    BUS_DMASYNC_POSTWRITE);
	if (task->ct_flags & TASK_CTR)
		bus_dmamap_sync(sc->sc_dmat, task->ct_ctrmap, 0,
		    task->ct_ctrmap->dm_segs[0].ds_len, BUS_DMASYNC_POSTREAD);
	if (task->ct_flags & TASK_IV)
		bus_dmamap_sync(sc->sc_dmat, task->ct_ivmap, 0,
		    task->ct_ivmap->dm_segs[0].ds_len, BUS_DMASYNC_POSTWRITE);
	if (task->ct_flags & TASK_KEY)
		/* XXX Can we zero the bounce buffer if there is one?  */
		bus_dmamap_sync(sc->sc_dmat, task->ct_keymap, 0,
		    task->ct_keymap->dm_segs[0].ds_len, BUS_DMASYNC_POSTWRITE);

	/* Temporarily release the lock to invoke the callback.  */
	mutex_exit(&sc->sc_lock);
	SDT_PROBE2(sdt, sun8i_crypto, task, done,  task, error);
	(*task->ct_callback)(sc, task, task->ct_cookie, error);
	mutex_enter(&sc->sc_lock);

	/* We cleared a channel.  */
	return true;
}

/*
 * sun8i_crypto_allocbuf(sc, size, buf, dmaflags)
 *
 *	Allocate a single-segment DMA-safe buffer and map it into KVA.
 *	May fail if dmaflags is BUS_DMA_NOWAIT.
 */
static int
sun8i_crypto_allocbuf(struct sun8i_crypto_softc *sc, size_t size,
    struct sun8i_crypto_buf *buf, int dmaflags)
{
	int error;

	/* Allocate a DMA-safe buffer.  */
	error = bus_dmamem_alloc(sc->sc_dmat, size, sizeof(uint32_t), 0,
	    buf->cb_seg, __arraycount(buf->cb_seg), &buf->cb_nsegs, dmaflags);
	if (error)
		goto fail0;

	/* Map the buffer into kernel virtual address space.  */
	error = bus_dmamem_map(sc->sc_dmat, buf->cb_seg, buf->cb_nsegs,
	    size, &buf->cb_kva, dmaflags);
	if (error)
		goto fail1;

	/* Success!  */
	return 0;

fail2: __unused
	bus_dmamem_unmap(sc->sc_dmat, buf->cb_kva, size);
fail1:	bus_dmamem_free(sc->sc_dmat, buf->cb_seg, buf->cb_nsegs);
fail0:	return error;
}

/*
 * sun8i_crypto_freebuf(sc, buf)
 *
 *	Unmap buf and free it.
 */
static void
sun8i_crypto_freebuf(struct sun8i_crypto_softc *sc, size_t size,
    struct sun8i_crypto_buf *buf)
{

	bus_dmamem_unmap(sc->sc_dmat, buf->cb_kva, size);
	bus_dmamem_free(sc->sc_dmat, buf->cb_seg, buf->cb_nsegs);
}

/*
 * sun8i_crypto_rng_attach(sc)
 *
 *	Attach an rndsource for the crypto engine's TRNG.
 */
static void
sun8i_crypto_rng_attach(struct sun8i_crypto_softc *sc)
{
	device_t self = sc->sc_dev;
	struct sun8i_crypto_rng *rng = &sc->sc_rng;
	struct sun8i_crypto_task *task;
	unsigned timo = hztoms(SUN8I_CRYPTO_TIMEOUT);
	uint32_t done, esr;
	int error;

	/* Preallocate a buffer to reuse.  */
	error = sun8i_crypto_allocbuf(sc, SUN8I_CRYPTO_RNGBYTES, &rng->cr_buf,
	    BUS_DMA_WAITOK);
	if (error) {
		aprint_error_dev(self, "failed to allocate RNG buffer: %d\n",
		    error);
		goto fail0;
	}

	/* Create a task to reuse.  */
	task = rng->cr_task = sun8i_crypto_task_get(sc, sun8i_crypto_rng_done,
	    rng, PR_WAITOK);
	if (rng->cr_task == NULL) {
		aprint_error_dev(self, "failed to allocate RNG task\n");
		error = ENOMEM;
		goto fail1;
	}

	/* Preload the destination map.  */
	error = bus_dmamap_load(sc->sc_dmat, task->ct_dstmap,
	    rng->cr_buf.cb_kva, SUN8I_CRYPTO_RNGBYTES, NULL, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(self, "failed to load RNG buffer: %d\n",
		    error);
		goto fail2;
	}

	/*
	 * Attach the rndsource.  This will trigger an initial call to
	 * it since we have RND_FLAG_HASCB.
	 */
	rndsource_setcb(&rng->cr_rndsource, sun8i_crypto_rng_get, sc);
	rnd_attach_source(&rng->cr_rndsource, device_xname(self),
	    RND_TYPE_RNG,
	    RND_FLAG_COLLECT_VALUE|RND_FLAG_ESTIMATE_VALUE|RND_FLAG_HASCB);

	/*
	 * Poll for the first call to the RNG to complete.  If not done
	 * after the timeout, force a timeout.
	 */
	for (; rng->cr_pending && timo --> 0; DELAY(1000)) {
		if (sun8i_crypto_poll(sc, &done, &esr))
			(void)sun8i_crypto_done(sc, done, esr, getticks());
	}
	if (rng->cr_pending) {
		(void)sun8i_crypto_done(sc, 0, 0,
		    (unsigned)getticks() + SUN8I_CRYPTO_TIMEOUT);
		KASSERT(!rng->cr_pending);
	}

	return;

fail3: __unused
	bus_dmamap_unload(sc->sc_dmat, task->ct_dstmap);
fail2:	sun8i_crypto_task_put(sc, task);
fail1:	sun8i_crypto_freebuf(sc, SUN8I_CRYPTO_RNGBYTES, &rng->cr_buf);
fail0:	return;
}

/*
 * sun8i_crypto_rng_get(nbytes, cookie)
 *
 *	On-demand rndsource callback: try to gather nbytes of entropy
 *	and enter them into the pool ASAP.
 */
static void
sun8i_crypto_rng_get(size_t nbytes, void *cookie)
{
	struct sun8i_crypto_softc *sc = cookie;
	struct sun8i_crypto_rng *rng = &sc->sc_rng;
	struct sun8i_crypto_task *task = rng->cr_task;
	bool pending;
	int error;

	/*
	 * Test and set the RNG-pending flag.  If it's already in
	 * progress, nothing to do here.
	 */
	mutex_enter(&sc->sc_lock);
	pending = rng->cr_pending;
	rng->cr_pending = true;
	mutex_exit(&sc->sc_lock);
	if (pending)
		return;

	/* Load the task descriptor.  */
	error = sun8i_crypto_task_load_trng(sc, task, SUN8I_CRYPTO_RNGBYTES);
	if (error)
		goto fail;

	/* Submit!  */
	error = sun8i_crypto_submit(sc, task);
	if (error)
		goto fail;

	/* All done!  */
	return;

fail:	mutex_enter(&sc->sc_lock);
	rng->cr_pending = false;
	mutex_exit(&sc->sc_lock);
}

static void
sun8i_crypto_rng_done(struct sun8i_crypto_softc *sc,
    struct sun8i_crypto_task *task, void *cookie, int error)
{
	struct sun8i_crypto_rng *rng = cookie;
	uint8_t *buf = rng->cr_buf.cb_kva;
	uint32_t entropybits;

	KASSERT(rng == &sc->sc_rng);

	/* If anything went wrong, forget about it.  */
	if (error)
		goto out;

	/*
	 * This TRNG has quite low entropy at best.  But if it fails a
	 * repeated output test, then assume it's busted.
	 */
	CTASSERT(SUN8I_CRYPTO_RNGBYTES <= UINT32_MAX/NBBY);
	entropybits = (NBBY*SUN8I_CRYPTO_RNGBYTES)/SUN8I_CRYPTO_RNGENTROPY;
	if (consttime_memequal(buf, buf + SUN8I_CRYPTO_RNGBYTES/2,
		SUN8I_CRYPTO_RNGBYTES/2)) {
		device_printf(sc->sc_dev, "failed repeated output test\n");
		entropybits = 0;
	}

	/*
	 * Actually we don't believe in any of the entropy until this
	 * device has had more scrutiny.
	 */
	entropybits = 0;

	/* Success!  Enter and erase the data.  */
	rnd_add_data(&rng->cr_rndsource, buf, SUN8I_CRYPTO_RNGBYTES,
	    entropybits);
	explicit_memset(buf, 0, SUN8I_CRYPTO_RNGBYTES);

out:	/* Done -- clear the RNG-pending flag.  */
	mutex_enter(&sc->sc_lock);
	rng->cr_pending = false;
	mutex_exit(&sc->sc_lock);
}

/*
 * Self-test
 */

static const uint8_t selftest_input[16];
static const uint8_t selftest_key[16];
static const uint8_t selftest_output[16] = {
	0x66,0xe9,0x4b,0xd4,0xef,0x8a,0x2c,0x3b,
	0x88,0x4c,0xfa,0x59,0xca,0x34,0x2b,0x2e,
};

static bool
sun8i_crypto_selftest(struct sun8i_crypto_softc *sc)
{
	const size_t keybytes = sizeof selftest_key;
	const size_t nbytes = sizeof selftest_input;
	struct sun8i_crypto_selftest *selftest = &sc->sc_selftest;
	struct sun8i_crypto_task *task;
	unsigned timo = hztoms(SUN8I_CRYPTO_TIMEOUT);
	uint32_t done, esr;
	int error;

	CTASSERT(sizeof selftest_input == sizeof selftest_output);

	selftest->cs_pending = true;

	/* Allocate an input buffer.  */
	error = sun8i_crypto_allocbuf(sc, nbytes, &selftest->cs_in,
	    BUS_DMA_WAITOK);
	if (error)
		goto fail0;

	/* Allocate a key buffer.  */
	error = sun8i_crypto_allocbuf(sc, keybytes, &selftest->cs_key,
	    BUS_DMA_WAITOK);
	if (error)
		goto fail1;

	/* Allocate an output buffer.  */
	error = sun8i_crypto_allocbuf(sc, nbytes, &selftest->cs_out,
	    BUS_DMA_WAITOK);
	if (error)
		goto fail2;

	/* Allocate a task descriptor.  */
	task = selftest->cs_task = sun8i_crypto_task_get(sc,
	    sun8i_crypto_selftest_done, selftest, PR_WAITOK);
	if (selftest->cs_task == NULL) {
		error = ENOMEM;
		goto fail3;
	}

	/* Copy the input and key into their buffers.  */
	memcpy(selftest->cs_in.cb_kva, selftest_input, nbytes);
	memcpy(selftest->cs_key.cb_kva, selftest_key, keybytes);

	/* Load the key, src, and dst for DMA transfers.  */
	error = bus_dmamap_load(sc->sc_dmat, task->ct_keymap,
	    selftest->cs_key.cb_kva, keybytes, NULL, BUS_DMA_WAITOK);
	if (error)
		goto fail4;
	task->ct_flags |= TASK_KEY;

	error = bus_dmamap_load(sc->sc_dmat, task->ct_srcmap,
	    selftest->cs_in.cb_kva, nbytes, NULL, BUS_DMA_WAITOK);
	if (error)
		goto fail5;
	task->ct_flags |= TASK_SRC;

	error = bus_dmamap_load(sc->sc_dmat, task->ct_dstmap,
	    selftest->cs_out.cb_kva, nbytes, NULL, BUS_DMA_WAITOK);
	if (error)
		goto fail6;

	/* Set up the task descriptor.  */
	error = sun8i_crypto_task_load_aesecb(sc, task, nbytes,
	    SUN8I_CRYPTO_TDQS_AES_KEYSIZE_128, SUN8I_CRYPTO_TDQC_OP_DIR_ENC);
	if (error)
		goto fail7;

	/* Submit!  */
	error = sun8i_crypto_submit(sc, task);
	if (error)
		goto fail7;
	device_printf(sc->sc_dev, "AES-128 self-test initiated\n");

	/*
	 * Poll for completion.  If not done after the timeout, force a
	 * timeout.
	 */
	for (; selftest->cs_pending && timo --> 0; DELAY(1000)) {
		if (sun8i_crypto_poll(sc, &done, &esr))
			(void)sun8i_crypto_done(sc, done, esr, getticks());
	}
	if (selftest->cs_pending) {
		(void)sun8i_crypto_done(sc, 0, 0,
		    (unsigned)getticks() + SUN8I_CRYPTO_TIMEOUT);
		KASSERT(!selftest->cs_pending);
	}

	return selftest->cs_passed;

fail7:	bus_dmamap_unload(sc->sc_dmat, task->ct_dstmap);
fail6:	bus_dmamap_unload(sc->sc_dmat, task->ct_srcmap);
fail5:	bus_dmamap_unload(sc->sc_dmat, task->ct_keymap);
fail4:	sun8i_crypto_task_put(sc, task);
fail3:	sun8i_crypto_freebuf(sc, nbytes, &selftest->cs_out);
fail2:	sun8i_crypto_freebuf(sc, keybytes, &selftest->cs_key);
fail1:	sun8i_crypto_freebuf(sc, nbytes, &selftest->cs_in);
fail0:	aprint_error_dev(sc->sc_dev, "failed to run self-test, error=%d\n",
	    error);
	selftest->cs_pending = false;
	return false;
}

static bool
sun8i_crypto_selftest_check(struct sun8i_crypto_softc *sc, const char *title,
    size_t n, const void *expected, const void *actual)
{
	const uint8_t *e = expected;
	const uint8_t *a = actual;
	size_t i;

	if (memcmp(e, a, n) == 0)
		return true;

	device_printf(sc->sc_dev, "self-test: %s\n", title);
	printf("expected: ");
	for (i = 0; i < n; i++)
		printf("%02hhx", e[i]);
	printf("\n");
	printf("actual:   ");
	for (i = 0; i < n; i++)
		printf("%02hhx", a[i]);
	printf("\n");
	return false;
}

static void
sun8i_crypto_selftest_done(struct sun8i_crypto_softc *sc,
    struct sun8i_crypto_task *task, void *cookie, int error)
{
	const size_t keybytes = sizeof selftest_key;
	const size_t nbytes = sizeof selftest_input;
	struct sun8i_crypto_selftest *selftest = cookie;
	bool ok = true;

	KASSERT(selftest == &sc->sc_selftest);

	/* If anything went wrong, fail now.  */
	if (error) {
		device_printf(sc->sc_dev, "self-test error=%d\n", error);
		goto out;
	}

	/*
	 * Verify the input and key weren't clobbered, and verify the
	 * output matches what we expect.
	 */
	ok &= sun8i_crypto_selftest_check(sc, "input clobbered", nbytes,
	    selftest_input, selftest->cs_in.cb_kva);
	ok &= sun8i_crypto_selftest_check(sc, "key clobbered", keybytes,
	    selftest_key, selftest->cs_key.cb_kva);
	ok &= sun8i_crypto_selftest_check(sc, "output mismatch", nbytes,
	    selftest_output, selftest->cs_out.cb_kva);

	selftest->cs_passed = ok;
	if (ok)
		device_printf(sc->sc_dev, "AES-128 self-test passed\n");

out:	bus_dmamap_unload(sc->sc_dmat, task->ct_dstmap);
	bus_dmamap_unload(sc->sc_dmat, task->ct_srcmap);
	bus_dmamap_unload(sc->sc_dmat, task->ct_keymap);
	sun8i_crypto_task_put(sc, task);
	sun8i_crypto_freebuf(sc, nbytes, &selftest->cs_out);
	sun8i_crypto_freebuf(sc, keybytes, &selftest->cs_key);
	sun8i_crypto_freebuf(sc, nbytes, &selftest->cs_in);
	selftest->cs_pending = false;
}

/*
 * Sysctl for testing
 */

struct sun8i_crypto_userreq {
	kmutex_t			cu_lock;
	kcondvar_t			cu_cv;
	size_t				cu_size;
	struct sun8i_crypto_buf		cu_buf;
	struct sun8i_crypto_task	*cu_task;
	int				cu_error;
	bool				cu_done;
	bool				cu_cancel;
};

static void
sun8i_crypto_sysctl_attach(struct sun8i_crypto_softc *sc)
{
	struct sun8i_crypto_sysctl *cy = &sc->sc_sysctl;
	int error;

	/* hw.sun8icryptoN (node) */
	error = sysctl_createv(&cy->cy_log, 0, NULL, &cy->cy_root_node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, device_xname(sc->sc_dev),
	    SYSCTL_DESCR("sun8i crypto engine knobs"),
	    NULL, 0, NULL, 0,
	    CTL_HW, CTL_CREATE, CTL_EOL);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "failed to set up sysctl hw.%s: %d\n",
		    device_xname(sc->sc_dev), error);
		return;
	}

	/* hw.sun8icryptoN.rng (`struct', 4096-byte array) */
	sysctl_createv(&cy->cy_log, 0, &cy->cy_root_node, &cy->cy_trng_node,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY|CTLFLAG_PRIVATE, CTLTYPE_STRUCT,
	    "rng", SYSCTL_DESCR("Read up to 4096 bytes out of the TRNG"),
	    &sun8i_crypto_sysctl_rng, 0, sc, 0, CTL_CREATE, CTL_EOL);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "failed to set up sysctl hw.%s.rng: %d\n",
		    device_xname(sc->sc_dev), error);
		return;
	}
}

static int
sun8i_crypto_sysctl_rng(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct sun8i_crypto_softc *sc = node.sysctl_data;
	struct sun8i_crypto_userreq *req;
	struct sun8i_crypto_task *task;
	size_t size;
	int error;

	/* If oldp == NULL, the caller wants to learn the size.  */
	if (oldp == NULL) {
		*oldlenp = 4096;
		return 0;
	}

	/* Truncate to 4096 bytes.  */
	size = MIN(4096, *oldlenp);
	if (size == 0)
		return 0;	/* nothing to do */

	/* Allocate a request context.  */
	req = kmem_alloc(sizeof(*req), KM_NOSLEEP);
	if (req == NULL)
		return ENOMEM;

	/* Initialize the request context.  */
	mutex_init(&req->cu_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&req->cu_cv, "sun8isy");
	req->cu_size = size;
	req->cu_error = EIO;
	req->cu_done = false;
	req->cu_cancel = false;

	/* Allocate a buffer for the RNG output.  */
	error = sun8i_crypto_allocbuf(sc, size, &req->cu_buf, BUS_DMA_NOWAIT);
	if (error)
		goto out0;

	/* Allocate a task.  */
	task = req->cu_task = sun8i_crypto_task_get(sc,
	    sun8i_crypto_sysctl_rng_done, req, PR_NOWAIT);
	if (task == NULL) {
		error = ENOMEM;
		goto out1;
	}

	/* Set the task up for TRNG to our buffer.  */
	error = bus_dmamap_load(sc->sc_dmat, task->ct_dstmap,
	    req->cu_buf.cb_kva, SUN8I_CRYPTO_RNGBYTES, NULL, BUS_DMA_NOWAIT);
	if (error)
		goto out2;
	error = sun8i_crypto_task_load_trng(sc, task, SUN8I_CRYPTO_RNGBYTES);
	if (error)
		goto out3;

	/* Submit!  */
	error = sun8i_crypto_submit(sc, task);
	if (error) {
		/* Make sure we don't restart the syscall -- just fail.  */
		if (error == ERESTART)
			error = EBUSY;
		goto out3;
	}

	/* Wait for the request to complete.  */
	mutex_enter(&req->cu_lock);
	while (!req->cu_done) {
		error = cv_wait_sig(&req->cu_cv, &req->cu_lock);
		if (error) {
			/*
			 * If we finished while waiting to acquire the
			 * lock, ignore the error and just return now.
			 * Otherwise, notify the callback that it has
			 * to clean up after us.
			 */
			if (req->cu_done)
				error = 0;
			else
				req->cu_cancel = true;
			break;
		}
	}
	mutex_exit(&req->cu_lock);

	/*
	 * Return early on error from cv_wait_sig, which means
	 * interruption; the callback will clean up instead.
	 */
	if (error)
		return error;

	/* Check for error from the device.  */
	error = req->cu_error;
	if (error)
		goto out3;

	/* Copy out the data.  */
	node.sysctl_data = req->cu_buf.cb_kva;
	node.sysctl_size = size;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	/* Clear the buffer.  */
	explicit_memset(req->cu_buf.cb_kva, 0, size);

	/* Clean up.  */
out3:	bus_dmamap_unload(sc->sc_dmat, task->ct_dstmap);
out2:	sun8i_crypto_task_put(sc, task);
out1:	sun8i_crypto_freebuf(sc, req->cu_size, &req->cu_buf);
out0:	cv_destroy(&req->cu_cv);
	mutex_destroy(&req->cu_lock);
	kmem_free(req, sizeof(*req));
	return error;
}

static void
sun8i_crypto_sysctl_rng_done(struct sun8i_crypto_softc *sc,
    struct sun8i_crypto_task *task, void *cookie, int error)
{
	struct sun8i_crypto_userreq *req = cookie;
	bool cancel;

	/*
	 * Notify the waiting thread of the error, and find out whether
	 * that thread cancelled.
	 */
	mutex_enter(&req->cu_lock);
	cancel = req->cu_cancel;
	req->cu_error = error;
	req->cu_done = true;
	cv_broadcast(&req->cu_cv);
	mutex_exit(&req->cu_lock);

	/*
	 * If it wasn't cancelled, we're done -- the main thread will
	 * clean up after itself.
	 */
	if (!cancel)
		return;

	/* Clean up after the main thread cancelled.  */
	bus_dmamap_unload(sc->sc_dmat, task->ct_dstmap);
	sun8i_crypto_task_put(sc, task);
	sun8i_crypto_freebuf(sc, req->cu_size, &req->cu_buf);
	cv_destroy(&req->cu_cv);
	mutex_destroy(&req->cu_lock);
	kmem_free(req, sizeof(*req));
}

/*
 * sun8i_crypto_register(sc)
 *
 *	Register opencrypto algorithms supported by the crypto engine.
 */
static void
sun8i_crypto_register(struct sun8i_crypto_softc *sc)
{
	struct sun8i_crypto_opencrypto *co = &sc->sc_opencrypto;

	co->co_driverid = crypto_get_driverid(0);
	if (co->co_driverid == (uint32_t)-1) {
		aprint_error_dev(sc->sc_dev,
		    "failed to register crypto driver\n");
		return;
	}

	sun8i_crypto_register1(sc, CRYPTO_AES_CBC);
	sun8i_crypto_register1(sc, CRYPTO_AES_CTR);
#ifdef CRYPTO_AES_ECB
	sun8i_crypto_register1(sc, CRYPTO_AES_ECB);
#endif
#ifdef CRYPTO_AES_XTS
	sun8i_crypto_register1(sc, CRYPTO_AES_XTS);
#endif
#ifdef CRYPTO_DES_CBC
	sun8i_crypto_register1(sc, CRYPTO_DES_CBC);
#endif
#ifdef CRYPTO_DES_ECB
	sun8i_crypto_register1(sc, CRYPTO_DES_ECB);
#endif
	sun8i_crypto_register1(sc, CRYPTO_3DES_CBC);
#ifdef CRYPTO_3DES_ECB
	sun8i_crypto_register1(sc, CRYPTO_3DES_ECB);
#endif

	sun8i_crypto_register1(sc, CRYPTO_MD5);
	sun8i_crypto_register1(sc, CRYPTO_SHA1);
#ifdef CRYPTO_SHA224
	sun8i_crypto_register1(sc, CRYPTO_SHA224);
#endif
#ifdef CRYPTO_SHA256
	sun8i_crypto_register1(sc, CRYPTO_SHA256);
#endif

	sun8i_crypto_register1(sc, CRYPTO_SHA1_HMAC);
	sun8i_crypto_register1(sc, CRYPTO_SHA2_256_HMAC);

	//sun8i_crypto_kregister(sc, CRK_MOD_EXP);	/* XXX unclear */
}

/*
 * sun8i_crypto_register1(sc, alg)
 *
 *	Register support for one algorithm alg using
 *	sun8i_crypto_newsession/freesession/process.
 */
static void
sun8i_crypto_register1(struct sun8i_crypto_softc *sc, uint32_t alg)
{

	crypto_register(sc->sc_opencrypto.co_driverid, alg, 0, 0,
	    sun8i_crypto_newsession,
	    sun8i_crypto_freesession,
	    sun8i_crypto_process,
	    sc);
}

/*
 * sun8i_crypto_newsession(cookie, sidp, cri)
 *
 *	Called by opencrypto to allocate a new session.  We don't keep
 *	track of sessions, since there are no persistent keys in the
 *	hardware that we take advantage of, so this only validates the
 *	crypto operations and returns a dummy session id of 1.
 */
static int
sun8i_crypto_newsession(void *cookie, uint32_t *sidp, struct cryptoini *cri)
{

	/* No composition of operations is supported here.  */
	if (cri->cri_next)
		return EINVAL;

	/*
	 * No variation of rounds is supported here.  (XXX Unused and
	 * unimplemented in opencrypto(9) altogether?)
	 */
	if (cri->cri_rnd)
		return EINVAL;

	/*
	 * Validate per-algorithm key length.
	 *
	 * XXX Does opencrypto(9) do this internally?
	 */
	switch (cri->cri_alg) {
	case CRYPTO_MD5:
	case CRYPTO_SHA1:
#ifdef CRYPTO_SHA224
	case CRYPTO_SHA224:
#endif
#ifdef CRYPTO_SHA256
	case CRYPTO_SHA256:
#endif
		if (cri->cri_klen)
			return EINVAL;
		break;
	case CRYPTO_AES_CBC:
#ifdef CRYPTO_AES_ECB
	case CRYPTO_AES_ECB:
#endif
		switch (cri->cri_klen) {
		case 128:
		case 192:
		case 256:
			break;
		default:
			return EINVAL;
		}
		break;
	case CRYPTO_AES_CTR:
		/*
		 * opencrypto `AES-CTR' takes four bytes of the input
		 * block as the last four bytes of the key, for reasons
		 * that are not entirely clear.
		 */
		switch (cri->cri_klen) {
		case 128 + 32:
		case 192 + 32:
		case 256 + 32:
			break;
		default:
			return EINVAL;
		}
		break;
#ifdef CRYPTO_AES_XTS
	case CRYPTO_AES_XTS:
		switch (cri->cri_klen) {
		case 256:
		case 384:
		case 512:
			break;
		default:
			return EINVAL;
		}
		break;
#endif
	case CRYPTO_DES_CBC:
#ifdef CRYPTO_DES_ECB
	case CRYPTO_DES_ECB:
#endif
		switch (cri->cri_klen) {
		case 64:
			break;
		default:
			return EINVAL;
		}
		break;
	case CRYPTO_3DES_CBC:
#ifdef CRYPTO_3DES_ECB
	case CRYPTO_3DES_ECB:
#endif
		switch (cri->cri_klen) {
		case 192:
			break;
		default:
			return EINVAL;
		}
		break;
	case CRYPTO_SHA1_HMAC:
		/*
		 * XXX Unclear what the length limit is, but since HMAC
		 * behaves qualitatively different for a key of at
		 * least the full block size -- and is generally best
		 * to use with half the block size -- let's limit it to
		 * one block.
		 */
		if (cri->cri_klen % 8)
			return EINVAL;
		if (cri->cri_klen > 512)
			return EINVAL;
		break;
	case CRYPTO_SHA2_256_HMAC:
		if (cri->cri_klen % 8)
			return EINVAL;
		if (cri->cri_klen > 512)
			return EINVAL;
		break;
	default:
		panic("unsupported algorithm %d", cri->cri_alg);
	}

	KASSERT(cri->cri_klen % 8 == 0);

	/* Success!  */
	*sidp = 1;
	return 0;
}

/*
 * sun8i_crypto_freesession(cookie, dsid)
 *
 *	Called by opencrypto to free a session.  We don't keep track of
 *	sessions, since there are no persistent keys in the hardware
 *	that we take advantage of, so this is a no-op.
 *
 *	Note: dsid is actually a 64-bit quantity containing both the
 *	driver id in the high half and the session id in the low half.
 */
static int
sun8i_crypto_freesession(void *cookie, uint64_t dsid)
{

	KASSERT((dsid & 0xffffffff) == 1);

	/* Success! */
	return 0;
}

/*
 * sun8i_crypto_ivlen(crd)
 *
 *	Return the crypto engine's notion of `IV length', in bytes, for
 *	an opencrypto operation.
 */
static u_int
sun8i_crypto_ivlen(const struct cryptodesc *crd)
{

	switch (crd->crd_alg) {
	case CRYPTO_AES_CBC:
		return 16;
#ifdef CRYPTO_AES_XTS
	case CRYPTO_AES_XTS:
		return 16;
#endif
	case CRYPTO_AES_CTR:	/* XXX opencrypto quirk */
		return 8;
#ifdef CRYPTO_DES_CBC
	case CRYPTO_DES_CBC:
		return 8;
#endif
	case CRYPTO_3DES_CBC:
		return 8;
	case CRYPTO_MD5:
		return 16;
#ifdef CRYPTO_SHA224
	case CRYPTO_SHA224:
		return 32;
#endif
#ifdef CRYPTO_SHA256
	case CRYPTO_SHA256:
		return 32;
#endif
	case CRYPTO_SHA1_HMAC:
		return 20;
	case CRYPTO_SHA2_256_HMAC:
		return 32;
	default:
		return 0;
	}
}

/*
 * sun8i_crypto_process(cookie, crp, hint)
 *
 *	Main opencrypto processing dispatch.
 */
static int
sun8i_crypto_process(void *cookie, struct cryptop *crp, int hint)
{
	struct sun8i_crypto_softc *sc = cookie;
	struct sun8i_crypto_task *task;
	struct cryptodesc *crd = crp->crp_desc;
	unsigned klen, ivlen;
	uint32_t tdqc = 0, tdqs = 0;
	uint32_t dir, method, mode = 0, ctrwidth = 0, aeskeysize = 0;
	const uint32_t tdqa = 0;
	int error;

	SDT_PROBE3(sdt, sun8i_crypto, process, entry,  sc, crp, hint);

	/* Reject compositions -- we do not handle them.  */
	if (crd->crd_next != NULL) {
		error = EOPNOTSUPP;
		goto fail0;
	}

	/* Reject transfers with nonsense skip.  */
	if (crd->crd_skip < 0) {
		error = EINVAL;
		goto fail0;
	}

	/*
	 * Actually just reject any nonzero skip, because it requires
	 * DMA segment bookkeeping that we don't do yet.
	 */
	if (crd->crd_skip) {
		error = EOPNOTSUPP;
		goto fail0;
	}

	/* Reject large transfers.  */
	if (crd->crd_len > SUN8I_CRYPTO_MAXDMASIZE) {
		error = EFBIG;
		goto fail0;
	}

	/* Reject nonsense, unaligned, or mismatched lengths.  */
	if (crd->crd_len < 0 ||
	    crd->crd_len % 4 ||
	    crd->crd_len != crp->crp_ilen) {
		error = EINVAL;
		goto fail0;
	}

	/* Reject mismatched buffer lengths.  */
	/* XXX Handle crd_skip.  */
	if (crp->crp_flags & CRYPTO_F_IMBUF) {
		struct mbuf *m = crp->crp_buf;
		uint32_t nbytes = 0;
		while (m != NULL) {
			KASSERT(m->m_len >= 0);
			if (m->m_len > crd->crd_len ||
			    nbytes > crd->crd_len - m->m_len) {
				error = EINVAL;
				goto fail0;
			}
			nbytes += m->m_len;
			m = m->m_next;
		}
		if (nbytes != crd->crd_len) {
			error = EINVAL;
			goto fail0;
		}
	} else if (crp->crp_flags & CRYPTO_F_IOV) {
		struct uio *uio = crp->crp_buf;
		if (uio->uio_resid != crd->crd_len) {
			error = EINVAL;
			goto fail0;
		}
	}

	/* Get a task, or fail with ERESTART if we can't.  */
	task = sun8i_crypto_task_get(sc, &sun8i_crypto_callback, crp,
	    PR_NOWAIT);
	if (task == NULL) {
		/*
		 * Don't invoke crypto_done -- we are asking the
		 * opencrypto(9) machinery to queue the request and get
		 * back to us.
		 */
		SDT_PROBE3(sdt, sun8i_crypto, process, busy,  sc, crp, hint);
		return ERESTART;
	}

	/* Load key in, if relevant.  */
	klen = crd->crd_klen;
	if (klen) {
		if (crd->crd_alg == CRYPTO_AES_CTR)
			/* AES-CTR is special -- see IV processing below.  */
			klen -= 32;
		error = bus_dmamap_load(sc->sc_dmat, task->ct_keymap,
		    crd->crd_key, klen/8, NULL, BUS_DMA_NOWAIT);
		if (error)
			goto fail1;
		task->ct_flags |= TASK_KEY;
	}

	/* Handle the IV, if relevant.  */
	ivlen = sun8i_crypto_ivlen(crd);
	if (ivlen) {
		void *iv;

		/*
		 * If there's an explicit IV, use it; otherwise
		 * randomly generate one.
		 */
		if (crd->crd_flags & CRD_F_IV_EXPLICIT) {
			iv = crd->crd_iv;
		} else {
			cprng_fast(task->ct_iv, ivlen);
			iv = task->ct_iv;
		}

		/*
		 * If the IV is not already present in the user's
		 * buffer, copy it over.
		 */
		if ((crd->crd_flags & CRD_F_IV_PRESENT) == 0) {
			if (crp->crp_flags & CRYPTO_F_IMBUF) {
				m_copyback(crp->crp_buf, crd->crd_inject,
				    ivlen, iv);
			} else if (crp->crp_flags & CRYPTO_F_IOV) {
				cuio_copyback(crp->crp_buf, crd->crd_inject,
				    ivlen, iv);
			} else {
				panic("invalid buffer type %x",
				    crp->crp_flags);
			}
		}

		/*
		 * opencrypto's idea of `AES-CTR' is special.
		 *
		 * - The low 4 bytes of the input block are drawn from
		 *   an extra 4 bytes at the end of the key.
		 *
		 * - The next 8 bytes of the input block are drawn from
		 *   the opencrypto iv.
		 *
		 * - The high 4 bytes are the big-endian block counter,
		 *   which starts at 1 because why not.
		 */
		if (crd->crd_alg == CRYPTO_AES_CTR) {
			uint8_t block[16];
			uint32_t blkno = 1;

			/* Format the initial input block.  */
			memcpy(block, crd->crd_key + klen/8, 4);
			memcpy(block + 4, iv, 8);
			be32enc(block + 12, blkno);

			/* Copy it into the DMA buffer.  */
			memcpy(task->ct_iv, block, 16);
			iv = task->ct_iv;
			ivlen = 16;
		}

		/* Load the IV.  */
		error = bus_dmamap_load(sc->sc_dmat, task->ct_ivmap, iv, ivlen,
		    NULL, BUS_DMA_NOWAIT);
		if (error)
			goto fail1;
		task->ct_flags |= TASK_IV;
	}

	/* Load the src and dst.  */
	if (crp->crp_flags & CRYPTO_F_IMBUF) {
		struct mbuf *m = crp->crp_buf;

		/* XXX Handle crd_skip.  */
		KASSERT(crd->crd_skip == 0);
		error = bus_dmamap_load_mbuf(sc->sc_dmat, task->ct_srcmap, m,
		    BUS_DMA_NOWAIT);
		if (error)
			goto fail1;
		task->ct_flags |= TASK_SRC;

		/* XXX Handle crd_skip.  */
		KASSERT(crd->crd_skip == 0);
		error = bus_dmamap_load_mbuf(sc->sc_dmat, task->ct_dstmap, m,
		    BUS_DMA_NOWAIT);
		if (error)
			goto fail1;
	} else if (crp->crp_flags & CRYPTO_F_IOV) {
		struct uio *uio = crp->crp_buf;

		/* XXX Handle crd_skip.  */
		KASSERT(crd->crd_skip == 0);
		error = bus_dmamap_load_uio(sc->sc_dmat, task->ct_srcmap, uio,
		    BUS_DMA_NOWAIT);
		if (error)
			goto fail1;
		task->ct_flags |= TASK_SRC;

		/* XXX Handle crd_skip.  */
		KASSERT(crd->crd_skip == 0);
		error = bus_dmamap_load_uio(sc->sc_dmat, task->ct_dstmap, uio,
		    BUS_DMA_NOWAIT);
		if (error)
			goto fail1;
	} else {
		panic("invalid buffer type %x", crp->crp_flags);
	}

	/* Set the encryption direction.  */
	if (crd->crd_flags & CRD_F_ENCRYPT)
		dir = SUN8I_CRYPTO_TDQC_OP_DIR_ENC;
	else
		dir = SUN8I_CRYPTO_TDQC_OP_DIR_DEC;
	tdqc |= __SHIFTIN(dir, SUN8I_CRYPTO_TDQC_OP_DIR);

	/* Set the method.  */
	switch (crd->crd_alg) {
	case CRYPTO_AES_CBC:
	case CRYPTO_AES_CTR:
#ifdef CRYPTO_AES_ECB
	case CRYPTO_AES_ECB:
#endif
		method = SUN8I_CRYPTO_TDQC_METHOD_AES;
		break;
#ifdef CRYPTO_AES_XTS
	case CRYPTO_AES_XTS:
		method = SUN8I_CRYPTO_TDQC_METHOD_AES;
		break;
#endif
	case CRYPTO_DES_CBC:
#ifdef CRYPTO_DES_ECB
	case CRYPTO_DES_ECB:
#endif
		method = SUN8I_CRYPTO_TDQC_METHOD_DES;
		break;
	case CRYPTO_3DES_CBC:
#ifdef CRYPTO_3DES_ECB
	case CRYPTO_3DES_ECB:
#endif
		method = SUN8I_CRYPTO_TDQC_METHOD_3DES;
		break;
	case CRYPTO_MD5:
		method = SUN8I_CRYPTO_TDQC_METHOD_MD5;
		break;
	case CRYPTO_SHA1:
		method = SUN8I_CRYPTO_TDQC_METHOD_SHA1;
		break;
#ifdef CRYPTO_SHA224
	case CRYPTO_SHA224:
		method = SUN8I_CRYPTO_TDQC_METHOD_SHA224;
		break;
#endif
#ifdef CRYPTO_SHA256
	case CRYPTO_SHA256:
		method = SUN8I_CRYPTO_TDQC_METHOD_SHA256;
		break;
#endif
	case CRYPTO_SHA1_HMAC:
		method = SUN8I_CRYPTO_TDQC_METHOD_HMAC_SHA1;
		break;
	case CRYPTO_SHA2_256_HMAC:
		method = SUN8I_CRYPTO_TDQC_METHOD_HMAC_SHA256;
		break;
	default:
		panic("unknown algorithm %d", crd->crd_alg);
	}
	tdqc |= __SHIFTIN(method, SUN8I_CRYPTO_TDQC_METHOD);

	/* Set the key selector.  No idea how to use the internal keys.  */
	tdqs |= __SHIFTIN(SUN8I_CRYPTO_TDQS_SKEY_SELECT_SS_KEYx,
	    SUN8I_CRYPTO_TDQS_SKEY_SELECT);

	/* XXX Deal with AES_CTS_Last_Block_Flag.  */

	/* Set the mode.  */
	switch (crd->crd_alg) {
#ifdef CRYPTO_AES_ECB
	case CRYPTO_AES_ECB:
		mode = SUN8I_CRYPTO_TDQS_OP_MODE_ECB;
		break;
#endif
#ifdef CRYPTO_DES_ECB
	case CRYPTO_DES_ECB:
		mode = SUN8I_CRYPTO_TDQS_OP_MODE_ECB;
		break;
#endif
#ifdef CRYPTO_3DES_ECB
	case CRYPTO_3DES_ECB:
		mode = SUN8I_CRYPTO_TDQS_OP_MODE_ECB;
		break;
#endif
	case CRYPTO_AES_CBC:
	case CRYPTO_DES_CBC:
	case CRYPTO_3DES_CBC:
		mode = SUN8I_CRYPTO_TDQS_OP_MODE_CBC;
		break;
	case CRYPTO_AES_CTR:
		mode = SUN8I_CRYPTO_TDQS_OP_MODE_CTR;
		break;
#ifdef CRYPTO_AES_XTS
	case CRYPTO_AES_XTS:
		mode = SUN8I_CRYPTO_TDQS_OP_MODE_CTS;
		break;
#endif
	default:
		panic("unknown algorithm %d", crd->crd_alg);
	}
	tdqs |= __SHIFTIN(mode, SUN8I_CRYPTO_TDQS_OP_MODE);

	/* Set the CTR width.  */
	switch (crd->crd_alg) {
	case CRYPTO_AES_CTR:
		ctrwidth = SUN8I_CRYPTO_TDQS_CTR_WIDTH_32;
		break;
	}
	tdqs |= __SHIFTIN(ctrwidth, SUN8I_CRYPTO_TDQS_CTR_WIDTH);

	/* Set the AES key size.  */
	switch (crd->crd_alg) {
	case CRYPTO_AES_CBC:
#ifdef CRYPTO_AES_ECB
	case CRYPTO_AES_ECB:
#endif
		switch (crd->crd_klen) {
		case 128:
			aeskeysize = SUN8I_CRYPTO_TDQS_AES_KEYSIZE_128;
			break;
		case 192:
			aeskeysize = SUN8I_CRYPTO_TDQS_AES_KEYSIZE_192;
			break;
		case 256:
			aeskeysize = SUN8I_CRYPTO_TDQS_AES_KEYSIZE_256;
			break;
		default:
			panic("invalid AES key size in bits: %u",
			    crd->crd_klen);
		}
		break;
	case CRYPTO_AES_CTR:
		switch (crd->crd_klen) {
		case 128 + 32:
			aeskeysize = SUN8I_CRYPTO_TDQS_AES_KEYSIZE_128;
			break;
		case 192 + 32:
			aeskeysize = SUN8I_CRYPTO_TDQS_AES_KEYSIZE_192;
			break;
		case 256 + 32:
			aeskeysize = SUN8I_CRYPTO_TDQS_AES_KEYSIZE_256;
			break;
		default:
			panic("invalid `AES-CTR' ` ``key'' size' in bits: %u",
			    crd->crd_klen);
		}
		break;
#ifdef CRYPTO_AES_XTS
	case CRYPTO_AES_XTS:
		switch (crd->crd_klen) {
		case 256:
			aeskeysize = SUN8I_CRYPTO_TDQS_AES_KEYSIZE_128;
			break;
		case 384:
			aeskeysize = SUN8I_CRYPTO_TDQS_AES_KEYSIZE_192;
			break;
		case 512:
			aeskeysize = SUN8I_CRYPTO_TDQS_AES_KEYSIZE_256;
			break;
		default:
			panic("invalid AES-XTS key size in bits: %u",
			    crd->crd_klen);
		}
		break;
#endif
	}
	tdqs |= __SHIFTIN(aeskeysize, SUN8I_CRYPTO_TDQS_AES_KEYSIZE);

	/* Set up the task descriptor.  */
	error = sun8i_crypto_task_load(sc, task, crd->crd_len,
	    tdqc, tdqs, tdqa);
	if (error)
		goto fail2;

	/* Submit!  */
	error = sun8i_crypto_submit(sc, task);
	if (error)
		goto fail2;

	/* Success!  */
	SDT_PROBE4(sdt, sun8i_crypto, process, queued,  sc, crp, hint, task);
	return 0;

fail2:	bus_dmamap_unload(sc->sc_dmat, task->ct_dstmap);
fail1:	if (task->ct_flags & TASK_SRC)
		bus_dmamap_unload(sc->sc_dmat, task->ct_srcmap);
	if (task->ct_flags & TASK_CTR)
		bus_dmamap_unload(sc->sc_dmat, task->ct_ctrmap);
	if (task->ct_flags & TASK_IV)
		bus_dmamap_unload(sc->sc_dmat, task->ct_ivmap);
	if (task->ct_flags & TASK_KEY)
		bus_dmamap_unload(sc->sc_dmat, task->ct_keymap);
	sun8i_crypto_task_put(sc, task);
fail0:	KASSERT(error);
	KASSERT(error != ERESTART);
	crp->crp_etype = error;
	SDT_PROBE3(sdt, sun8i_crypto, process, done,  sc, crp, error);
	crypto_done(crp);
	return 0;
}

/*
 * sun8i_crypto_callback(sc, task, cookie, error)
 *
 *	Completion callback for a task submitted via opencrypto.
 *	Release the task and pass the error on to opencrypto with
 *	crypto_done.
 */
static void
sun8i_crypto_callback(struct sun8i_crypto_softc *sc,
    struct sun8i_crypto_task *task, void *cookie, int error)
{
	struct cryptop *crp = cookie;
	struct cryptodesc *crd __diagused = crp->crp_desc;

	KASSERT(error != ERESTART);
	KASSERT(crd != NULL);
	KASSERT(crd->crd_next == NULL);

	/* Return the number of bytes processed.  */
	crp->crp_olen = error ? 0 : crp->crp_ilen;

	bus_dmamap_unload(sc->sc_dmat, task->ct_dstmap);
	bus_dmamap_unload(sc->sc_dmat, task->ct_srcmap);
	if (task->ct_flags & TASK_CTR)
		bus_dmamap_unload(sc->sc_dmat, task->ct_ctrmap);
	if (task->ct_flags & TASK_IV)
		bus_dmamap_unload(sc->sc_dmat, task->ct_ivmap);
	if (task->ct_flags & TASK_KEY)
		bus_dmamap_unload(sc->sc_dmat, task->ct_keymap);
	sun8i_crypto_task_put(sc, task);
	KASSERT(error != ERESTART);
	crp->crp_etype = error;
	SDT_PROBE3(sdt, sun8i_crypto, process, done,  sc, crp, error);
	crypto_done(crp);
}
