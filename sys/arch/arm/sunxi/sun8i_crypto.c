/*	$NetBSD: sun8i_crypto.c,v 1.9.2.3 2020/02/29 20:18:20 ad Exp $	*/

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
__KERNEL_RCSID(1, "$NetBSD: sun8i_crypto.c,v 1.9.2.3 2020/02/29 20:18:20 ad Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/bus.h>
#include <sys/callout.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/rndpool.h>
#include <sys/rndsource.h>
#include <sys/sysctl.h>
#include <sys/workqueue.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sun8i_crypto.h>

#define	SUN8I_CRYPTO_TIMEOUT	hz
#define	SUN8I_CRYPTO_RNGENTROPY	100 /* estimated bits per bit of entropy */
#define	SUN8I_CRYPTO_RNGBYTES	PAGE_SIZE

struct sun8i_crypto_task;

struct sun8i_crypto_buf {
	bus_dma_segment_t	cb_seg[1];
	int			cb_nsegs;
	bus_dmamap_t		cb_map;
	void			*cb_kva;
};

struct sun8i_crypto_softc {
	device_t			sc_dev;
	bus_space_tag_t			sc_bst;
	bus_space_handle_t		sc_bsh;
	bus_dma_tag_t			sc_dmat;
	kmutex_t			sc_lock;
	struct sun8i_crypto_chan {
		struct sun8i_crypto_task	*cc_task;
		unsigned			cc_starttime;
	}				sc_chan[SUN8I_CRYPTO_NCHAN];
	struct callout			sc_timeout;
	struct workqueue		*sc_wq;
	struct work			sc_work;
	void				*sc_ih;
	uint32_t			sc_done;
	uint32_t			sc_esr;
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
	}				sc_selftest;
	struct sun8i_crypto_sysctl {
		struct sysctllog		*cy_log;
		const struct sysctlnode		*cy_root_node;
		const struct sysctlnode		*cy_trng_node;
	}				sc_sysctl;
};

struct sun8i_crypto_task {
	struct sun8i_crypto_buf	ct_buf;
	struct sun8i_crypto_taskdesc *ct_desc;
	void			(*ct_callback)(struct sun8i_crypto_softc *,
				    struct sun8i_crypto_task *, void *, int);
	void			*ct_cookie;
};

/*
 * Forward declarations
 */

static int	sun8i_crypto_match(device_t, cfdata_t, void *);
static void	sun8i_crypto_attach(device_t, device_t, void *);

static struct sun8i_crypto_task *
		sun8i_crypto_task_get(struct sun8i_crypto_softc *,
		    void (*)(struct sun8i_crypto_softc *,
			struct sun8i_crypto_task *, void *, int),
		    void *);
static void	sun8i_crypto_task_put(struct sun8i_crypto_softc *,
		    struct sun8i_crypto_task *);
static void	sun8i_crypto_task_reset(struct sun8i_crypto_task *);

static void	sun8i_crypto_task_set_key(struct sun8i_crypto_task *,
		    bus_dmamap_t);
static void	sun8i_crypto_task_set_iv(struct sun8i_crypto_task *,
		    bus_dmamap_t);
static void	sun8i_crypto_task_set_ctr(struct sun8i_crypto_task *,
		    bus_dmamap_t);
static void	sun8i_crypto_task_set_input(struct sun8i_crypto_task *,
		    bus_dmamap_t);
static void	sun8i_crypto_task_set_output(struct sun8i_crypto_task *,
		    bus_dmamap_t);

static void	sun8i_crypto_task_scatter(struct sun8i_crypto_adrlen *,
		    bus_dmamap_t);

static int	sun8i_crypto_submit_trng(struct sun8i_crypto_softc *,
		    struct sun8i_crypto_task *, uint32_t);
static int	sun8i_crypto_submit_aesecb(struct sun8i_crypto_softc *,
		    struct sun8i_crypto_task *, uint32_t, uint32_t, uint32_t);
static int	sun8i_crypto_submit(struct sun8i_crypto_softc *,
		    struct sun8i_crypto_task *);

static void	sun8i_crypto_timeout(void *);
static int	sun8i_crypto_intr(void *);
static void	sun8i_crypto_schedule_worker(struct sun8i_crypto_softc *);
static void	sun8i_crypto_worker(struct work *, void *);
static void	sun8i_crypto_chan_done(struct sun8i_crypto_softc *, unsigned,
		    int);

static int	sun8i_crypto_allocbuf(struct sun8i_crypto_softc *, size_t,
		    struct sun8i_crypto_buf *);
static void	sun8i_crypto_freebuf(struct sun8i_crypto_softc *, size_t,
		    struct sun8i_crypto_buf *);

static void	sun8i_crypto_rng_attach(struct sun8i_crypto_softc *);
static void	sun8i_crypto_rng_get(size_t, void *);
static void	sun8i_crypto_rng_done(struct sun8i_crypto_softc *,
		    struct sun8i_crypto_task *, void *, int);

static void	sun8i_crypto_selftest(device_t);
static void	sun8i_crypto_selftest_done(struct sun8i_crypto_softc *,
		    struct sun8i_crypto_task *, void *, int);

static void	sun8i_crypto_sysctl_attach(struct sun8i_crypto_softc *);
static int	sun8i_crypto_sysctl_rng(SYSCTLFN_ARGS);
static void	sun8i_crypto_sysctl_rng_done(struct sun8i_crypto_softc *,
		    struct sun8i_crypto_task *, void *, int);

/*
 * Register access
 */

static uint32_t
sun8i_crypto_read(struct sun8i_crypto_softc *sc, bus_addr_t reg)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, reg);
}

static void
sun8i_crypto_write(struct sun8i_crypto_softc *sc, bus_addr_t reg, uint32_t v)
{
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, reg, v);
}

/*
 * Autoconf goo
 */

CFATTACH_DECL_NEW(sun8i_crypto, sizeof(struct sun8i_crypto_softc),
    sun8i_crypto_match, sun8i_crypto_attach, NULL, NULL);

static const struct of_compat_data compat_data[] = {
	{"allwinner,sun50i-a64-crypto", 0},
	{NULL}
};

static int
sun8i_crypto_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct fdt_attach_args *const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
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

	sc->sc_dev = self;
	sc->sc_dmat = faa->faa_dmat;
	sc->sc_bst = faa->faa_bst;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);
	callout_init(&sc->sc_timeout, CALLOUT_MPSAFE);
	callout_setfunc(&sc->sc_timeout, &sun8i_crypto_timeout, sc);
	if (workqueue_create(&sc->sc_wq, device_xname(self),
		&sun8i_crypto_worker, sc, PRI_NONE, IPL_VM, WQ_MPSAFE) != 0) {
		aprint_error(": couldn't create workqueue\n");
		return;
	}

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

	/* Get the module clock and set it to 300 MHz.  */
	if ((clk = fdtbus_clock_get(phandle, "mod")) != NULL) {
		if (clk_enable(clk) != 0) {
			aprint_error(": couldn't enable CE clock\n");
			return;
		}
		if (clk_set_rate(clk, 300*1000*1000) != 0) {
			aprint_error(": couldn't set CE clock to 300MHz\n");
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

	/* Disable and clear interrupts.  */
	sun8i_crypto_write(sc, SUN8I_CRYPTO_ICR, 0);
	sun8i_crypto_write(sc, SUN8I_CRYPTO_ISR, 0);

	/* Establish an interrupt handler.  */
	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_VM, FDT_INTR_MPSAFE,
	    &sun8i_crypto_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	/* Set up the RNG.  */
	sun8i_crypto_rng_attach(sc);

	/* Attach the sysctl.  */
	sun8i_crypto_sysctl_attach(sc);

	/* Perform self-tests.  */
	config_interrupts(self, sun8i_crypto_selftest);
}

/*
 * Task allocation
 */

static struct sun8i_crypto_task *
sun8i_crypto_task_get(struct sun8i_crypto_softc *sc,
    void (*callback)(struct sun8i_crypto_softc *, struct sun8i_crypto_task *,
	void *, int),
    void *cookie)
{
	struct sun8i_crypto_task *task;
	int error;

	/* Allocate a task.  */
	task = kmem_zalloc(sizeof(*task), KM_SLEEP);

	/* Allocate a buffer for the descriptor.  */
	error = sun8i_crypto_allocbuf(sc, sizeof(*task->ct_desc),
	    &task->ct_buf);
	if (error)
		goto fail0;

	/* Initialize the task object and return it.  */
	task->ct_desc = task->ct_buf.cb_kva;
	task->ct_callback = callback;
	task->ct_cookie = cookie;
	return task;

fail1: __unused
	sun8i_crypto_freebuf(sc, sizeof(*task->ct_desc), &task->ct_buf);
fail0:	kmem_free(task, sizeof(*task));
	return NULL;
}

static void
sun8i_crypto_task_put(struct sun8i_crypto_softc *sc,
    struct sun8i_crypto_task *task)
{

	sun8i_crypto_freebuf(sc, sizeof(*task->ct_desc), &task->ct_buf);
	kmem_free(task, sizeof(*task));
}

/*
 * Task descriptor setup
 *
 * WARNING: Task descriptor fields are little-endian, not host-endian.
 */

static void
sun8i_crypto_task_reset(struct sun8i_crypto_task *task)
{

	memset(task->ct_desc, 0, sizeof(*task->ct_desc));
}

static void
sun8i_crypto_task_set_key(struct sun8i_crypto_task *task, bus_dmamap_t map)
{

	KASSERT(map->dm_nsegs == 1);
	task->ct_desc->td_keydesc = htole32(map->dm_segs[0].ds_addr);
}

static void __unused		/* XXX opencrypto(9) */
sun8i_crypto_task_set_iv(struct sun8i_crypto_task *task, bus_dmamap_t map)
{

	KASSERT(map->dm_nsegs == 1);
	task->ct_desc->td_ivdesc = htole32(map->dm_segs[0].ds_addr);
}

static void __unused		/* XXX opencrypto(9) */
sun8i_crypto_task_set_ctr(struct sun8i_crypto_task *task, bus_dmamap_t map)
{

	KASSERT(map->dm_nsegs == 1);
	task->ct_desc->td_ctrdesc = htole32(map->dm_segs[0].ds_addr);
}

static void
sun8i_crypto_task_set_input(struct sun8i_crypto_task *task, bus_dmamap_t map)
{

	sun8i_crypto_task_scatter(task->ct_desc->td_src, map);
}

static void
sun8i_crypto_task_set_output(struct sun8i_crypto_task *task, bus_dmamap_t map)
{

	sun8i_crypto_task_scatter(task->ct_desc->td_dst, map);
}

static void
sun8i_crypto_task_scatter(struct sun8i_crypto_adrlen *adrlen, bus_dmamap_t map)
{
	uint32_t total __diagused = 0;
	unsigned i;

	KASSERT(map->dm_nsegs <= SUN8I_CRYPTO_MAXSEGS);
	for (i = 0; i < map->dm_nsegs; i++) {
		KASSERT((map->dm_segs[i].ds_addr % 4) == 0);
		KASSERT(map->dm_segs[i].ds_addr <= UINT32_MAX);
		KASSERT(map->dm_segs[i].ds_len <= UINT32_MAX - total);
		adrlen[i].adr = htole32(map->dm_segs[i].ds_addr);
		adrlen[i].len = htole32(map->dm_segs[i].ds_len/4);
		total += map->dm_segs[i].ds_len;
	}

	/* Verify the remainder are zero.  */
	for (; i < SUN8I_CRYPTO_MAXSEGS; i++) {
		KASSERT(adrlen[i].adr == 0);
		KASSERT(adrlen[i].len == 0);
	}

	/* Verify the total size matches the DMA map.  */
	KASSERT(total == map->dm_mapsize);
}

/*
 * Task submission
 *
 * WARNING: Task descriptor fields are little-endian, not host-endian.
 */

static int
sun8i_crypto_submit_trng(struct sun8i_crypto_softc *sc,
    struct sun8i_crypto_task *task, uint32_t datalen)
{
	struct sun8i_crypto_taskdesc *desc = task->ct_desc;
	uint32_t tdqc = 0;
	uint32_t total __diagused;
	unsigned i __diagused;

	/* Data length must be a multiple of 4 because...reasons.  */
	KASSERT((datalen % 4) == 0);

	/* All of the sources should be empty.  */
	for (total = 0, i = 0; i < SUN8I_CRYPTO_MAXSEGS; i++)
		KASSERT(le32toh(task->ct_desc->td_src[i].len) == 0);

	/* Verify the total output length -- should be datalen/4.  */
	for (total = 0, i = 0; i < SUN8I_CRYPTO_MAXSEGS; i++) {
		uint32_t len = le32toh(task->ct_desc->td_dst[i].len);
		KASSERT(len <= UINT32_MAX - total);
		total += len;
	}
	KASSERT(total == datalen/4);

	/* Verify the key, IV, and CTR are unset.  */
	KASSERT(desc->td_keydesc == 0);
	KASSERT(desc->td_ivdesc == 0);
	KASSERT(desc->td_ctrdesc == 0);

	/* Set up the task descriptor queue control words.  */
	tdqc |= SUN8I_CRYPTO_TDQC_INTR_EN;
	tdqc |= __SHIFTIN(SUN8I_CRYPTO_TDQC_METHOD_TRNG,
	    SUN8I_CRYPTO_TDQC_METHOD);
	desc->td_tdqc = htole32(tdqc);
	desc->td_tdqs = 0;	/* no symmetric crypto */
	desc->td_tdqa = 0;	/* no asymmetric crypto */

	/* Set the data length for the output.  */
	desc->td_datalen = htole32(datalen/4);

	/* Submit!  */
	return sun8i_crypto_submit(sc, task);
}

static int
sun8i_crypto_submit_aesecb(struct sun8i_crypto_softc *sc,
    struct sun8i_crypto_task *task,
    uint32_t datalen, uint32_t keysize, uint32_t dir)
{
	struct sun8i_crypto_taskdesc *desc = task->ct_desc;
	uint32_t tdqc = 0, tdqs = 0;
	uint32_t total __diagused;
	unsigned i __diagused;

	/*
	 * Data length must be a multiple of 4 because...reasons.
	 *
	 * WARNING: For `AES-CTS' (maybe that means AES-XTS?), datalen
	 * is in units of bytes, not units of words -- but everything
	 * _else_ is in units of words.  This routine applies only to
	 * AES-ECB for the self-test.
	 */
	KASSERT((datalen % 4) == 0);

	/* Verify the total input length -- should be datalen/4.  */
	for (total = 0, i = 0; i < SUN8I_CRYPTO_MAXSEGS; i++) {
		uint32_t len = le32toh(task->ct_desc->td_src[i].len);
		KASSERT(len <= UINT32_MAX - total);
		total += len;
	}
	KASSERT(total == datalen/4);

	/* Verify the total output length -- should be datalen/4.  */
	for (total = 0, i = 0; i < SUN8I_CRYPTO_MAXSEGS; i++) {
		uint32_t len = le32toh(task->ct_desc->td_dst[i].len);
		KASSERT(len <= UINT32_MAX - total);
		total += len;
	}
	KASSERT(total == datalen/4);

	/* Set up the task descriptor queue control word.  */
	tdqc |= SUN8I_CRYPTO_TDQC_INTR_EN;
	tdqc |= __SHIFTIN(SUN8I_CRYPTO_TDQC_METHOD_AES,
	    SUN8I_CRYPTO_TDQC_METHOD);
	desc->td_tdqc = htole32(tdqc);

	/* Set up the symmetric control word.  */
	tdqs |= __SHIFTIN(SUN8I_CRYPTO_TDQS_SKEY_SELECT_SS_KEYx,
	    SUN8I_CRYPTO_TDQS_SKEY_SELECT);
	tdqs |= __SHIFTIN(SUN8I_CRYPTO_TDQS_OP_MODE_ECB,
	    SUN8I_CRYPTO_TDQS_OP_MODE);
	tdqs |= __SHIFTIN(SUN8I_CRYPTO_TDQS_AES_KEYSIZE_128,
	    SUN8I_CRYPTO_TDQS_AES_KEYSIZE);
	desc->td_tdqs = htole32(tdqs);

	desc->td_tdqa = 0;	/* no asymmetric crypto */

	/* Set the data length for the output.  */
	desc->td_datalen = htole32(datalen/4);

	/* Submit!  */
	return sun8i_crypto_submit(sc, task);
}

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

	/* Prepare to send the descriptor to the device by DMA.  */
	bus_dmamap_sync(sc->sc_dmat, task->ct_buf.cb_map, 0,
	    sizeof(*task->ct_desc), BUS_DMASYNC_PREWRITE);

	/* Confirm we're ready to go.  */
	if (sun8i_crypto_read(sc, SUN8I_CRYPTO_TLR) & SUN8I_CRYPTO_TLR_LOAD) {
		device_printf(sc->sc_dev, "TLR not clear\n");
		error = EIO;
		goto out;
	}

	/* Enable interrupts for this channel.  */
	icr = sun8i_crypto_read(sc, SUN8I_CRYPTO_ICR);
	icr |= __SHIFTIN(SUN8I_CRYPTO_ICR_INTR_EN_CHAN(i),
	    SUN8I_CRYPTO_ICR_INTR_EN);
	sun8i_crypto_write(sc, SUN8I_CRYPTO_ICR, icr);

	/* Set the task descriptor queue address.  */
	sun8i_crypto_write(sc, SUN8I_CRYPTO_TDQ,
	    task->ct_buf.cb_map->dm_segs[0].ds_addr);

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

	/* Loaded up and ready to go.  Start a timer ticking.  */
	sc->sc_chan[i].cc_task = task;
	sc->sc_chan[i].cc_starttime = atomic_load_relaxed(&hardclock_ticks);
	callout_schedule(&sc->sc_timeout, SUN8I_CRYPTO_TIMEOUT);

	/* XXX Consider polling if cold to get entropy earlier.  */

out:	/* Done!  */
	mutex_exit(&sc->sc_lock);
	return error;
}

static void
sun8i_crypto_timeout(void *cookie)
{
	struct sun8i_crypto_softc *sc = cookie;
	unsigned i;

	mutex_enter(&sc->sc_lock);

	/* Check whether there are any tasks pending.  */
	for (i = 0; i < SUN8I_CRYPTO_NCHAN; i++) {
		if (sc->sc_chan[i].cc_task)
			break;
	}
	if (i == SUN8I_CRYPTO_NCHAN)
		/* None pending, so nothing to do.  */
		goto out;

	/*
	 * Schedule the worker to check for timeouts, and schedule
	 * another timeout in case we need it.
	 */
	sun8i_crypto_schedule_worker(sc);
	callout_schedule(&sc->sc_timeout, SUN8I_CRYPTO_TIMEOUT);

out:	mutex_exit(&sc->sc_lock);
}

static int
sun8i_crypto_intr(void *cookie)
{
	struct sun8i_crypto_softc *sc = cookie;
	uint32_t isr, esr;

	mutex_enter(&sc->sc_lock);

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

	/* Start the worker if necessary.  */
	sun8i_crypto_schedule_worker(sc);

	/* Tell the worker what to do.  */
	sc->sc_done |= __SHIFTOUT(isr, SUN8I_CRYPTO_ISR_DONE);
	sc->sc_esr |= esr;

	mutex_exit(&sc->sc_lock);

	return __SHIFTOUT(isr, SUN8I_CRYPTO_ISR_DONE) != 0;
}

static void
sun8i_crypto_schedule_worker(struct sun8i_crypto_softc *sc)
{

	KASSERT(mutex_owned(&sc->sc_lock));

	/* Start the worker if necessary.  */
	if (!sc->sc_work_pending) {
		workqueue_enqueue(sc->sc_wq, &sc->sc_work, NULL);
		sc->sc_work_pending = true;
	}
}

static void
sun8i_crypto_worker(struct work *wk, void *cookie)
{
	struct sun8i_crypto_softc *sc = cookie;
	uint32_t done, esr, esr_chan;
	unsigned i, now;
	int error;

	/*
	 * Acquire the lock.  Note: We will be releasing and
	 * reacquiring it throughout the loop.
	 */
	mutex_enter(&sc->sc_lock);

	/* Acknowledge the work.  */
	KASSERT(sc->sc_work_pending);
	sc->sc_work_pending = false;

	/*
	 * Claim the done mask and error status once; we will be
	 * releasing and reacquiring the lock for the callbacks, so
	 * they may change.
	 */
	done = sc->sc_done;
	esr = sc->sc_esr;
	sc->sc_done = 0;
	sc->sc_esr = 0;

	/* Check the time to determine what's timed out.  */
	now = atomic_load_relaxed(&hardclock_ticks);

	/* Process the channels.  */
	for (i = 0; i < SUN8I_CRYPTO_NCHAN; i++) {
		/* Check whether the channel is done.  */
		if (!ISSET(done, SUN8I_CRYPTO_ISR_DONE_CHAN(i))) {
			/* Nope.  Do we have a task to time out?  */
			if ((sc->sc_chan[i].cc_task != NULL) &&
			    ((now - sc->sc_chan[i].cc_starttime) >=
				SUN8I_CRYPTO_TIMEOUT))
				sun8i_crypto_chan_done(sc, i, ETIMEDOUT);
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
		sun8i_crypto_chan_done(sc, i, error);
	}

	/* All one; release the lock one last time.  */
	mutex_exit(&sc->sc_lock);
}

static void
sun8i_crypto_chan_done(struct sun8i_crypto_softc *sc, unsigned i, int error)
{
	struct sun8i_crypto_task *task;
	uint32_t icr;

	KASSERT(mutex_owned(&sc->sc_lock));

	/* Claim the task if there is one; bail if not.  */
	if ((task = sc->sc_chan[i].cc_task) == NULL) {
		device_printf(sc->sc_dev, "channel %u: no task but error=%d\n",
		    i, error);
		return;
	}
	sc->sc_chan[i].cc_task = NULL;

	/* Disable interrupts on this channel.  */
	icr = sun8i_crypto_read(sc, SUN8I_CRYPTO_ICR);
	icr &= ~__SHIFTIN(SUN8I_CRYPTO_ICR_INTR_EN_CHAN(i),
	    SUN8I_CRYPTO_ICR_INTR_EN);
	sun8i_crypto_write(sc, SUN8I_CRYPTO_ICR, icr);

	/* Finished sending the descriptor to the device by DMA.  */
	bus_dmamap_sync(sc->sc_dmat, task->ct_buf.cb_map, 0,
	    sizeof(*task->ct_desc), BUS_DMASYNC_POSTWRITE);

	/* Temporarily release the lock to invoke the callback.  */
	mutex_exit(&sc->sc_lock);
	(*task->ct_callback)(sc, task, task->ct_cookie, error);
	mutex_enter(&sc->sc_lock);
}

/*
 * DMA buffers
 */

static int
sun8i_crypto_allocbuf(struct sun8i_crypto_softc *sc, size_t size,
    struct sun8i_crypto_buf *buf)
{
	int error;

	/* Allocate a DMA-safe buffer.  */
	error = bus_dmamem_alloc(sc->sc_dmat, size, 0, 0, buf->cb_seg,
	    __arraycount(buf->cb_seg), &buf->cb_nsegs, BUS_DMA_WAITOK);
	if (error)
		goto fail0;

	/* Map the buffer into kernel virtual address space.  */
	error = bus_dmamem_map(sc->sc_dmat, buf->cb_seg, buf->cb_nsegs,
	    size, &buf->cb_kva, BUS_DMA_WAITOK);
	if (error)
		goto fail1;

	/* Create a DMA map for the buffer.   */
	error = bus_dmamap_create(sc->sc_dmat, size, 1, size, 0,
	    BUS_DMA_WAITOK, &buf->cb_map);
	if (error)
		goto fail2;

	/* Load the buffer into the DMA map.  */
	error = bus_dmamap_load(sc->sc_dmat, buf->cb_map, buf->cb_kva, size,
	    NULL, BUS_DMA_WAITOK);
	if (error)
		goto fail3;

	/* Success!  */
	return 0;

fail4: __unused
	bus_dmamap_unload(sc->sc_dmat, buf->cb_map);
fail3:	bus_dmamap_destroy(sc->sc_dmat, buf->cb_map);
fail2:	bus_dmamem_unmap(sc->sc_dmat, buf->cb_kva, size);
fail1:	bus_dmamem_free(sc->sc_dmat, buf->cb_seg, buf->cb_nsegs);
fail0:	return error;
}

static void
sun8i_crypto_freebuf(struct sun8i_crypto_softc *sc, size_t size,
    struct sun8i_crypto_buf *buf)
{

	bus_dmamap_unload(sc->sc_dmat, buf->cb_map);
	bus_dmamap_destroy(sc->sc_dmat, buf->cb_map);
	bus_dmamem_unmap(sc->sc_dmat, buf->cb_kva, size);
	bus_dmamem_free(sc->sc_dmat, buf->cb_seg, buf->cb_nsegs);
}

/*
 * Crypto Engine - TRNG
 */

static void
sun8i_crypto_rng_attach(struct sun8i_crypto_softc *sc)
{
	device_t self = sc->sc_dev;
	struct sun8i_crypto_rng *rng = &sc->sc_rng;
	int error;

	/* Preallocate a buffer to reuse.  */
	error = sun8i_crypto_allocbuf(sc, SUN8I_CRYPTO_RNGBYTES, &rng->cr_buf);
	if (error)
		goto fail0;

	/* Create a task to reuse.  */
	rng->cr_task = sun8i_crypto_task_get(sc, sun8i_crypto_rng_done, rng);
	if (rng->cr_task == NULL)
		goto fail1;

	/*
	 * Attach the rndsource.  This is _not_ marked as RND_TYPE_RNG
	 * because the output is not uniformly distributed.  The bits
	 * are heavily weighted toward 0 or 1, at different times, and
	 * I haven't scienced a satisfactory story out of it yet.
	 */
	rndsource_setcb(&rng->cr_rndsource, sun8i_crypto_rng_get, sc);
	rnd_attach_source(&rng->cr_rndsource, device_xname(self),
	    RND_TYPE_UNKNOWN,
	    RND_FLAG_COLLECT_VALUE|RND_FLAG_ESTIMATE_VALUE|RND_FLAG_HASCB);

	/* Success!  */
	return;

fail2: __unused
	sun8i_crypto_task_put(sc, rng->cr_task);
fail1:	sun8i_crypto_freebuf(sc, SUN8I_CRYPTO_RNGBYTES, &rng->cr_buf);
fail0:	aprint_error_dev(self, "failed to set up RNG, error=%d\n", error);
}

static void
sun8i_crypto_rng_get(size_t nbytes, void *cookie)
{
	struct sun8i_crypto_softc *sc = cookie;
	struct sun8i_crypto_rng *rng = &sc->sc_rng;
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

	/* Prepare for a DMA read into the buffer.  */
	bus_dmamap_sync(sc->sc_dmat, rng->cr_buf.cb_map,
	    0, SUN8I_CRYPTO_RNGBYTES, BUS_DMASYNC_PREREAD);

	/* Set the task up for TRNG to our buffer.  */
	sun8i_crypto_task_reset(rng->cr_task);
	sun8i_crypto_task_set_output(rng->cr_task, rng->cr_buf.cb_map);

	/* Submit the TRNG task.  */
	error = sun8i_crypto_submit_trng(sc, rng->cr_task,
	    SUN8I_CRYPTO_RNGBYTES);
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

	/* Finished the DMA read into the buffer.  */
	bus_dmamap_sync(sc->sc_dmat, rng->cr_buf.cb_map,
	    0, SUN8I_CRYPTO_RNGBYTES, BUS_DMASYNC_POSTREAD);

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

static void
sun8i_crypto_selftest(device_t self)
{
	const size_t datalen = sizeof selftest_input;
	struct sun8i_crypto_softc *sc = device_private(self);
	struct sun8i_crypto_selftest *selftest = &sc->sc_selftest;
	int error;

	CTASSERT(sizeof selftest_input == sizeof selftest_output);

	/* Allocate an input buffer.  */
	error = sun8i_crypto_allocbuf(sc, sizeof selftest_input,
	    &selftest->cs_in);
	if (error)
		goto fail0;

	/* Allocate a key buffer.  */
	error = sun8i_crypto_allocbuf(sc, sizeof selftest_key,
	    &selftest->cs_key);
	if (error)
		goto fail1;

	/* Allocate an output buffer.  */
	error = sun8i_crypto_allocbuf(sc, sizeof selftest_output,
	    &selftest->cs_out);
	if (error)
		goto fail2;

	/* Allocate a task descriptor.  */
	selftest->cs_task = sun8i_crypto_task_get(sc,
	    sun8i_crypto_selftest_done, selftest);
	if (selftest->cs_task == NULL)
		goto fail3;

	/* Copy the input and key into their buffers.  */
	memcpy(selftest->cs_in.cb_kva, selftest_input, sizeof selftest_input);
	memcpy(selftest->cs_key.cb_kva, selftest_key, sizeof selftest_key);

	/* Prepare for a DMA write from the input and key buffers.  */
	bus_dmamap_sync(sc->sc_dmat, selftest->cs_in.cb_map, 0,
	    sizeof selftest_input, BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(sc->sc_dmat, selftest->cs_key.cb_map, 0,
	    sizeof selftest_key, BUS_DMASYNC_PREWRITE);

	/* Prepare for a DMA read into the output buffer.  */
	bus_dmamap_sync(sc->sc_dmat, selftest->cs_out.cb_map, 0,
	    sizeof selftest_output, BUS_DMASYNC_PREREAD);

	/* Set up the task descriptor.  */
	sun8i_crypto_task_reset(selftest->cs_task);
	sun8i_crypto_task_set_key(selftest->cs_task, selftest->cs_key.cb_map);
	sun8i_crypto_task_set_input(selftest->cs_task, selftest->cs_in.cb_map);
	sun8i_crypto_task_set_output(selftest->cs_task,
	    selftest->cs_out.cb_map);

	/* Submit the AES-128 ECB task.  */
	error = sun8i_crypto_submit_aesecb(sc, selftest->cs_task, datalen,
	    SUN8I_CRYPTO_TDQS_AES_KEYSIZE_128, SUN8I_CRYPTO_TDQC_OP_DIR_ENC);
	if (error)
		goto fail4;

	device_printf(sc->sc_dev, "AES-128 self-test initiated\n");

	/* Success!  */
	return;

fail4:	sun8i_crypto_task_put(sc, selftest->cs_task);
fail3:	sun8i_crypto_freebuf(sc, sizeof selftest_output, &selftest->cs_out);
fail2:	sun8i_crypto_freebuf(sc, sizeof selftest_key, &selftest->cs_key);
fail1:	sun8i_crypto_freebuf(sc, sizeof selftest_input, &selftest->cs_in);
fail0:	aprint_error_dev(self, "failed to run self-test, error=%d\n", error);
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
	struct sun8i_crypto_selftest *selftest = cookie;
	bool ok = true;

	KASSERT(selftest == &sc->sc_selftest);

	/*
	 * Finished the DMA read into the output buffer, and finished
	 * the DMA writes from the key buffer and input buffer.
	 */
	bus_dmamap_sync(sc->sc_dmat, selftest->cs_out.cb_map, 0,
	    sizeof selftest_output, BUS_DMASYNC_POSTREAD);
	bus_dmamap_sync(sc->sc_dmat, selftest->cs_key.cb_map, 0,
	    sizeof selftest_key, BUS_DMASYNC_POSTWRITE);
	bus_dmamap_sync(sc->sc_dmat, selftest->cs_in.cb_map, 0,
	    sizeof selftest_input, BUS_DMASYNC_POSTWRITE);

	/* If anything went wrong, fail now.  */
	if (error) {
		device_printf(sc->sc_dev, "self-test error=%d\n", error);
		goto out;
	}

	/*
	 * Verify the input and key weren't clobbered, and verify the
	 * output matches what we expect.
	 */
	ok &= sun8i_crypto_selftest_check(sc, "input clobbered",
	    sizeof selftest_input, selftest_input, selftest->cs_in.cb_kva);
	ok &= sun8i_crypto_selftest_check(sc, "key clobbered",
	    sizeof selftest_key, selftest_key, selftest->cs_key.cb_kva);
	ok &= sun8i_crypto_selftest_check(sc, "output mismatch",
	    sizeof selftest_output, selftest_output, selftest->cs_out.cb_kva);

	/* XXX Disable the RNG and other stuff if this fails...  */
	if (ok)
		device_printf(sc->sc_dev, "AES-128 self-test passed\n");

out:	sun8i_crypto_task_put(sc, task);
	sun8i_crypto_freebuf(sc, sizeof selftest_output, &selftest->cs_out);
	sun8i_crypto_freebuf(sc, sizeof selftest_key, &selftest->cs_key);
	sun8i_crypto_freebuf(sc, sizeof selftest_input, &selftest->cs_in);
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
	size_t size;
	int error;

	/* If oldp == NULL, the caller wants to learn the size.  */
	if (oldp == NULL) {
		*oldlenp = 4096;
		return 0;
	}

	/* Verify the output buffer size is reasonable.  */
	size = *oldlenp;
	if (size > 4096)	/* size_t, so never negative */
		return E2BIG;
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
	error = sun8i_crypto_allocbuf(sc, size, &req->cu_buf);
	if (error)
		goto out0;

	/* Allocate a task.  */
	req->cu_task = sun8i_crypto_task_get(sc, sun8i_crypto_sysctl_rng_done,
	    req);
	if (req->cu_task == NULL)
		goto out1;

	/* Prepare for a DMA read into the buffer.  */
	bus_dmamap_sync(sc->sc_dmat, req->cu_buf.cb_map, 0, size,
	    BUS_DMASYNC_PREREAD);

	/* Set the task up for TRNG to our buffer.  */
	sun8i_crypto_task_reset(req->cu_task);
	sun8i_crypto_task_set_output(req->cu_task, req->cu_buf.cb_map);

	/* Submit the TRNG task.  */
	error = sun8i_crypto_submit_trng(sc, req->cu_task, size);
	if (error) {
		if (error == ERESTART)
			error = EBUSY;
		goto out2;
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
		goto out2;

	/* Finished the DMA read into the buffer.  */
	bus_dmamap_sync(sc->sc_dmat, req->cu_buf.cb_map, 0, req->cu_size,
	    BUS_DMASYNC_POSTREAD);

	/* Copy out the data.  */
	node.sysctl_data = req->cu_buf.cb_kva;
	node.sysctl_size = size;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	/* Clear the buffer.  */
	explicit_memset(req->cu_buf.cb_kva, 0, size);

	/* Clean up.  */
out2:	sun8i_crypto_task_put(sc, req->cu_task);
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
	sun8i_crypto_task_put(sc, req->cu_task);
	sun8i_crypto_freebuf(sc, req->cu_size, &req->cu_buf);
	cv_destroy(&req->cu_cv);
	mutex_destroy(&req->cu_lock);
	kmem_free(req, sizeof(*req));
}
