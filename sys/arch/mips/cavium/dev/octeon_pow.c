/*	$NetBSD: octeon_pow.c,v 1.9 2020/06/22 02:26:20 simonb Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon_pow.c,v 1.9 2020/06/22 02:26:20 simonb Exp $");

#include "opt_octeon.h"	/* CNMAC_DEBUG */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/kernel.h>				/* hz */
#include <sys/malloc.h>
#include <sys/device.h>				/* evcnt */
#include <sys/syslog.h>				/* evcnt */

#include <sys/bus.h>

#include <mips/include/locore.h>
#include <mips/cavium/octeonvar.h>
#include <mips/cavium/include/iobusvar.h>
#include <mips/cavium/dev/octeon_ciureg.h>	/* XXX */
#include <mips/cavium/dev/octeon_powreg.h>
#include <mips/cavium/dev/octeon_powvar.h>

/* XXX ensure assertion */
#if !defined(DIAGNOSTIC)
#define DIAGNOSTIC
#endif

extern int ipflow_fastforward_disable_flags;

struct octpow_intr_handle {
	void				*pi_ih;
	struct octpow_softc		*pi_sc;
	int				pi_group;
	void				(*pi_cb)(void *, uint64_t *);
	void				*pi_data;
};

void			octpow_bootstrap(struct octeon_config *);

static void		octpow_init(struct octpow_softc *);
static void		octpow_init_regs(struct octpow_softc *);
static inline int	octpow_tag_sw_poll(void) __unused;
static inline void	octpow_tag_sw_wait(void);
static inline void	octpow_config_int_pc(struct octpow_softc *, int);
static inline void      octpow_config_int(struct octpow_softc *, int, uint64_t,
			    uint64_t, uint64_t);
static inline void	octpow_intr_work(struct octpow_softc *,
			    struct octpow_intr_handle *, int);
static int		octpow_intr(void *);

/* XXX */
struct octpow_softc	octpow_softc;

/*
 * XXX: parameter tuning is needed: see files.octeon
 */
#ifndef CNMAC_RING_MAX
#define CNMAC_RING_MAX 512
#endif
#ifndef CNMAC_RING_MIN
#define CNMAC_RING_MIN 1
#endif

#ifdef CNMAC_INTR_FEEDBACK_RING
int max_recv_cnt = CNMAC_RING_MAX;
int min_recv_cnt = CNMAC_RING_MIN;
int recv_cnt = CNMAC_RING_MIN;
int int_rate = 1;
#endif

/* -------------------------------------------------------------------------- */

/* ---- utility functions */


/* ---- status by coreid */

static inline uint64_t
octpow_status_by_coreid_pend_tag(int coreid)
{
	return octpow_ops_pow_status(coreid, 0, 0, 0);
}

static inline uint64_t
octpow_status_by_coreid_pend_wqp(int coreid)
{
	return octpow_ops_pow_status(coreid, 0, 0, 1);
}

static inline uint64_t
octpow_status_by_coreid_cur_tag_next(int coreid)
{
	return octpow_ops_pow_status(coreid, 0, 1, 0);
}

static inline uint64_t
octpow_status_by_coreid_cur_tag_prev(int coreid)
{
	return octpow_ops_pow_status(coreid, 1, 1, 0);
}

static inline uint64_t
octpow_status_by_coreid_cur_wqp_next(int coreid)
{
	return octpow_ops_pow_status(coreid, 0, 1, 1);
}

static inline uint64_t
octpow_status_by_coreid_cur_wqp_prev(int coreid)
{
	return octpow_ops_pow_status(coreid, 1, 1, 1);
}

/* ---- status by index */

static inline uint64_t
octpow_status_by_index_tag(int index)
{
	return octpow_ops_pow_memory(index, 0, 0);
}

static inline uint64_t
octpow_status_by_index_wqp(int index)
{
	return octpow_ops_pow_memory(index, 0, 1);
}

static inline uint64_t
octpow_status_by_index_desched(int index)
{
	return octpow_ops_pow_memory(index, 1, 0);
}

/* ---- status by qos level */

static inline uint64_t
octpow_status_by_qos_free_loc(int qos)
{
	return octpow_ops_pow_idxptr(qos, 0, 0);
}

/* ---- status by desched group */

static inline uint64_t
octpow_status_by_grp_nosched_des(int grp)
{
	return octpow_ops_pow_idxptr(grp, 0, 1);
}

/* ---- status by memory input queue */

static inline uint64_t
octpow_status_by_queue_remote_head(int queue)
{
	return octpow_ops_pow_idxptr(queue, 1, 0);
}

static inline uint64_t
octpow_status_by_queue_remote_tail(int queue)
{
	return octpow_ops_pow_idxptr(queue, 1, 0);
}

/* ---- tag switch */

/*
 * "RDHWR rt, $30" returns:
 *	0 => pending bit is set
 *	1 => pending bit is clear
 */

/* return 1 if pending bit is clear (ready) */
static inline int
octpow_tag_sw_poll(void)
{
	uint64_t result;

	/* XXX O32 */
	__asm __volatile (
		"	.set	push		\n"
		"	.set	noreorder	\n"
		"	.set	arch=octeon	\n"
		"	rdhwr	%[result], $30	\n"
		"	 .set	pop		\n"
		: [result]"=r"(result)
	);
	/* XXX O32 */
	return (int)result;
}

/* -------------------------------------------------------------------------- */

/* ---- initialization and configuration */

void
octpow_bootstrap(struct octeon_config *mcp)
{
	struct octpow_softc *sc = &octpow_softc;

	sc->sc_regt = &mcp->mc_iobus_bust;
	/* XXX */

	octpow_init(sc);
}

static inline void
octpow_config_int(struct octpow_softc *sc, int group, uint64_t tc_thr,
    uint64_t ds_thr, uint64_t iq_thr)
{
	uint64_t wq_int_thr =
	    POW_WQ_INT_THRX_TC_EN |
	    __SHIFTIN(tc_thr, POW_WQ_INT_THRX_TC_THR) |
	    __SHIFTIN(ds_thr, POW_WQ_INT_THRX_DS_THR) |
	    __SHIFTIN(iq_thr, POW_WQ_INT_THRX_IQ_THR);

	_POW_WR8(sc, POW_WQ_INT_THR0_OFFSET + (group * 8), wq_int_thr);
}

/*
 * interrupt threshold configuration
 *
 * => DS / IQ
 *    => ...
 * => time counter threshold
 *    => unit is 1msec
 *    => each group can set timeout
 * => temporary disable bit
 *    => use CIU generic timer
 */

void
octpow_config(struct octpow_softc *sc, int group)
{

	octpow_config_int(sc, group,
	    0x0f,		/* TC */
	    0x00,		/* DS */
	    0x00);		/* IQ */
}

void *
octpow_intr_establish(int group, int level, void (*cb)(void *, uint64_t *),
    void (*fcb)(int*, int *, uint64_t, void *), void *data)
{
	struct octpow_intr_handle *pow_ih;

	KASSERT(group >= 0);
	KASSERT(group < 16);

	pow_ih = malloc(sizeof(*pow_ih), M_DEVBUF, M_WAITOK);
	pow_ih->pi_ih = octeon_intr_establish(
	    CIU_INT_WORKQ_0 + group,
	    level,
	    octpow_intr, pow_ih);
	KASSERT(pow_ih->pi_ih != NULL);

	pow_ih->pi_sc = &octpow_softc;	/* XXX */
	pow_ih->pi_group = group;
	pow_ih->pi_cb = cb;
	pow_ih->pi_data = data;

	return pow_ih;
}

void
octpow_init(struct octpow_softc *sc)
{
	octpow_init_regs(sc);

	sc->sc_int_pc_base = 10000;
	octpow_config_int_pc(sc, sc->sc_int_pc_base);
}

void
octpow_init_regs(struct octpow_softc *sc)
{
	int status;

	status = bus_space_map(sc->sc_regt, POW_BASE, POW_SIZE, 0,
	    &sc->sc_regh);
	if (status != 0)
		panic("can't map %s space", "pow register");
}

/* -------------------------------------------------------------------------- */

/* ---- interrupt handling */

/*
 * Interrupt handling by fixed count, following Cavium's SDK code.
 *
 * XXX the fixed count (MAX_RX_CNT) could be changed dynamically?
 *
 * XXX this does not utilize "tag switch" very well
 */
/*
 * usually all packet receive
 */
#define MAX_RX_CNT 0x7fffffff 

static inline void
octpow_intr_work(struct octpow_softc *sc, struct octpow_intr_handle *pow_ih,
    int recv_limit)
{
	uint64_t *work;
	uint64_t count = 0;

	_POW_WR8(sc, POW_PP_GRP_MSK0_OFFSET, __BIT(pow_ih->pi_group));

	for (count = 0; count < recv_limit; count++) {
		octpow_tag_sw_wait();
		octpow_work_request_async(
		    OCTEON_CVMSEG_OFFSET(csm_pow_intr), POW_NO_WAIT);
		work = (uint64_t *)octpow_work_response_async(
		    OCTEON_CVMSEG_OFFSET(csm_pow_intr));
		if (work == NULL)
			break;
		(*pow_ih->pi_cb)(pow_ih->pi_data, work);
	}
}

static int
octpow_intr(void *data)
{
	struct octpow_intr_handle *pow_ih = data;
	struct octpow_softc *sc = pow_ih->pi_sc;
	uint64_t wq_int_mask = __BIT(pow_ih->pi_group);

#ifdef CNMAC_INTR_FEEDBACK_RING
	octpow_intr_work(sc, pow_ih, recv_cnt);
#else
	octpow_intr_work(sc, pow_ih, INT_MAX);
#endif /* CNMAC_INTR_FEEDBACK_RING */

	_POW_WR8(sc, POW_WQ_INT_OFFSET,
	    __SHIFTIN(wq_int_mask, POW_WQ_INT_WQ_INT));
	return 1;
}

#ifdef CNMAC_INTR_FEEDBACK_RING
int
octpow_ring_reduce(void *arg)
{
	struct octpow_softc *sc = arg;
	int new, newi;
	int s;

#if 0
	if (ipflow_fastforward_disable_flags == 0) {
		newi = int_rate = 1;
		octpow_config_int_pc_rate(sc, int_rate);
		return recv_cnt;
	}
#endif

	new = recv_cnt / 2;
	if (new < min_recv_cnt) {
		newi = int_rate << 1;
		if (newi > 128) {
			newi = 128;
#ifdef POW_DEBUG
			log(LOG_DEBUG,
				"Min intr rate.\n");
#endif
			new = min_recv_cnt;
		}
		else {
			log(LOG_DEBUG,
				"pow interrupt rate optimized %d->%d.\n",
				int_rate, newi);
			int_rate = newi;
			octpow_config_int_pc_rate(sc, int_rate);
			new = max_recv_cnt;
		}
	}

	s = splhigh(); /* XXX */
	recv_cnt = new;
	splx(s);

	return new;
}

int
octpow_ring_grow(void *arg)
{
	struct octpow_softc *sc = arg;
	int new, newi;
	int s;

#if 0
	if (ipflow_fastforward_disable_flags == 0) {
		newi = int_rate = 1;
		octpow_config_int_pc_rate(sc, int_rate);
		return recv_cnt;
	}
#endif

	new = recv_cnt + 1;
	if (new > max_recv_cnt) {
		newi = int_rate >> 1;
		if (newi <= 0) {
			newi = 1;
#ifdef POW_DEBUG
			log(LOG_DEBUG,
				"Max intr rate.\n");
#endif
			new = max_recv_cnt;
		}
		else {
			log(LOG_DEBUG,
				"pow interrupt rate optimized %d->%d.\n",
				int_rate, newi);
			int_rate = newi;
			octpow_config_int_pc_rate(sc, int_rate);
			new = min_recv_cnt;
		}
	}

	s = splhigh(); /* XXX */
	recv_cnt = new;
	splx(s);

	return new;
}

int
octpow_ring_size(void)
{
	return recv_cnt;
}

int
octpow_ring_intr(void)
{
	return int_rate;
}
#endif /* CNMAC_INTR_FEEDBACK_RING */
