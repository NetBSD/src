/*	$NetbBSD$	*/

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
__KERNEL_RCSID(0, "$NetBSD: octeon_pow.c,v 1.2.2.2 2015/06/06 14:40:01 skrll Exp $");

#include "opt_octeon.h"	/* OCTEON_ETH_DEBUG */

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

struct octeon_pow_intr_handle {
	void				*pi_ih;
	struct octeon_pow_softc		*pi_sc;
	int				pi_group;
	void				(*pi_cb)(void *, uint64_t *);
	void				*pi_data;

#ifdef OCTEON_ETH_DEBUG
#define	_EV_PER_N	32	/* XXX */
#define	_EV_IVAL_N	32	/* XXX */
	int				pi_first;
	struct timeval			pi_last;
	struct evcnt			pi_ev_per[_EV_PER_N];
	struct evcnt			pi_ev_ival[_EV_IVAL_N];
	struct evcnt			pi_ev_stray_tc;
	struct evcnt			pi_ev_stray_ds;
	struct evcnt			pi_ev_stray_iq;
#endif
};

void			octeon_pow_bootstrap(struct octeon_config *);

#ifdef OCTEON_ETH_DEBUG
void			octeon_pow_intr_evcnt_attach(struct octeon_pow_softc *);
void			octeon_pow_intr_rml(void *);

static void             octeon_pow_intr_debug_init(
			    struct octeon_pow_intr_handle *, int);
static inline void      octeon_pow_intr_work_debug_ival(struct octeon_pow_softc *,
			    struct octeon_pow_intr_handle *);
static inline void	octeon_pow_intr_work_debug_per(struct octeon_pow_softc *,
			    struct octeon_pow_intr_handle *, int);
#endif
static void		octeon_pow_init(struct octeon_pow_softc *);
static void		octeon_pow_init_regs(struct octeon_pow_softc *);
static inline int	octeon_pow_tag_sw_poll(void) __unused;
static inline void	octeon_pow_tag_sw_wait(void);
static inline void	octeon_pow_config_int_pc(struct octeon_pow_softc *, int);
static inline void      octeon_pow_config_int(struct octeon_pow_softc *, int,
			    uint64_t, uint64_t, uint64_t);
static inline void	octeon_pow_intr_work(struct octeon_pow_softc *,
			    struct octeon_pow_intr_handle *, int);
static int		octeon_pow_intr(void *);

#ifdef OCTEON_ETH_DEBUG
void			octeon_pow_dump(void);
#endif

/* XXX */
struct octeon_pow_softc	octeon_pow_softc;

#ifdef OCTEON_ETH_DEBUG
struct octeon_pow_softc *__octeon_pow_softc;
#endif

/*
 * XXX: parameter tuning is needed: see files.octeon
 */
#ifndef OCTEON_ETH_RING_MAX
#define OCTEON_ETH_RING_MAX 512
#endif
#ifndef OCTEON_ETH_RING_MIN
#define OCTEON_ETH_RING_MIN 1
#endif

#ifdef OCTEON_ETH_INTR_FEEDBACK_RING
int max_recv_cnt = OCTEON_ETH_RING_MAX;
int min_recv_cnt = OCTEON_ETH_RING_MIN;
int recv_cnt = OCTEON_ETH_RING_MIN;
int int_rate = 1;
#endif

/* -------------------------------------------------------------------------- */

/* ---- operation primitive functions */

/* Load Operations */

/* IOBDMA Operations */

/* Store Operations */

/* -------------------------------------------------------------------------- */

/* ---- utility functions */


/* ---- status by coreid */

static inline uint64_t
octeon_pow_status_by_coreid_pend_tag(int coreid)
{
	return octeon_pow_ops_pow_status(coreid, 0, 0, 0);
}

static inline uint64_t
octeon_pow_status_by_coreid_pend_wqp(int coreid)
{
	return octeon_pow_ops_pow_status(coreid, 0, 0, 1);
}

static inline uint64_t
octeon_pow_status_by_coreid_cur_tag_next(int coreid)
{
	return octeon_pow_ops_pow_status(coreid, 0, 1, 0);
}

static inline uint64_t
octeon_pow_status_by_coreid_cur_tag_prev(int coreid)
{
	return octeon_pow_ops_pow_status(coreid, 1, 1, 0);
}

static inline uint64_t
octeon_pow_status_by_coreid_cur_wqp_next(int coreid)
{
	return octeon_pow_ops_pow_status(coreid, 0, 1, 1);
}

static inline uint64_t
octeon_pow_status_by_coreid_cur_wqp_prev(int coreid)
{
	return octeon_pow_ops_pow_status(coreid, 1, 1, 1);
}

/* ---- status by index */

static inline uint64_t
octeon_pow_status_by_index_tag(int index)
{
	return octeon_pow_ops_pow_memory(index, 0, 0);
}

static inline uint64_t
octeon_pow_status_by_index_wqp(int index)
{
	return octeon_pow_ops_pow_memory(index, 0, 1);
}

static inline uint64_t
octeon_pow_status_by_index_desched(int index)
{
	return octeon_pow_ops_pow_memory(index, 1, 0);
}

/* ---- status by qos level */

static inline uint64_t
octeon_pow_status_by_qos_free_loc(int qos)
{
	return octeon_pow_ops_pow_idxptr(qos, 0, 0);
}

/* ---- status by desched group */

static inline uint64_t
octeon_pow_status_by_grp_nosched_des(int grp)
{
	return octeon_pow_ops_pow_idxptr(grp, 0, 1);
}

/* ---- status by memory input queue */

static inline uint64_t
octeon_pow_status_by_queue_remote_head(int queue)
{
	return octeon_pow_ops_pow_idxptr(queue, 1, 0);
}

static inline uint64_t
octeon_pow_status_by_queue_remote_tail(int queue)
{
	return octeon_pow_ops_pow_idxptr(queue, 1, 0);
}

/* ---- tag switch */

/*
 * "RDHWR rt, $30" returns:
 *	0 => pending bit is set
 *	1 => pending bit is clear
 */

/* return 1 if pending bit is clear (ready) */
static inline int
octeon_pow_tag_sw_poll(void)
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
octeon_pow_bootstrap(struct octeon_config *mcp)
{
	struct octeon_pow_softc *sc = &octeon_pow_softc;

	sc->sc_regt = &mcp->mc_iobus_bust;
	/* XXX */

	octeon_pow_init(sc);

#ifdef OCTEON_ETH_DEBUG
	__octeon_pow_softc = sc;
#endif

}

static inline void
octeon_pow_config_int(struct octeon_pow_softc *sc, int group,
   uint64_t tc_thr, uint64_t ds_thr, uint64_t iq_thr)
{
	uint64_t wq_int_thr;

	wq_int_thr =
	    POW_WQ_INT_THRX_TC_EN |
	    (tc_thr << POW_WQ_INT_THRX_TC_THR_SHIFT) |
	    (ds_thr << POW_WQ_INT_THRX_DS_THR_SHIFT) |
	    (iq_thr << POW_WQ_INT_THRX_IQ_THR_SHIFT);
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
octeon_pow_config(struct octeon_pow_softc *sc, int group)
{

	octeon_pow_config_int(sc, group,
	    0x0f,		/* TC */
	    0x00,		/* DS */
	    0x00);		/* IQ */
}

void *
octeon_pow_intr_establish(int group, int level,
    void (*cb)(void *, uint64_t *), void (*fcb)(int*, int *, uint64_t, void *),
    void *data)
{
	struct octeon_pow_intr_handle *pow_ih;

	KASSERT(group >= 0);
	KASSERT(group < 16);

	pow_ih = malloc(sizeof(*pow_ih), M_DEVBUF, M_NOWAIT);
	KASSERT(pow_ih != NULL);

	pow_ih->pi_ih = octeon_intr_establish(
	    ffs64(CIU_INTX_SUM0_WORKQ_0) - 1 + group,
	    level,
	    octeon_pow_intr, pow_ih);
	KASSERT(pow_ih->pi_ih != NULL);

	pow_ih->pi_sc = &octeon_pow_softc;	/* XXX */
	pow_ih->pi_group = group;
	pow_ih->pi_cb = cb;
	pow_ih->pi_data = data;

#ifdef OCTEON_ETH_DEBUG
	octeon_pow_intr_debug_init(pow_ih, group);
#endif
	return pow_ih;
}

#ifdef OCTEON_ETH_DEBUG
#define	_NAMELEN	8
#define	_DESCRLEN	40

static void
octeon_pow_intr_debug_init(struct octeon_pow_intr_handle *pow_ih, int group)
{
	pow_ih->pi_first = 1;
	char *name, *descr;
	int i;

	name = malloc(_NAMELEN +
	    _DESCRLEN * __arraycount(pow_ih->pi_ev_per) +
	    _DESCRLEN * __arraycount(pow_ih->pi_ev_ival),
	    M_DEVBUF, M_NOWAIT);
	descr = name + _NAMELEN;
	snprintf(name, _NAMELEN, "pow%d", group);
	for (i = 0; i < (int)__arraycount(pow_ih->pi_ev_per); i++) {
		int n = 1 << (i - 1);

		(void)snprintf(descr, _DESCRLEN,
		    "# of works per intr (%d-%d)",
		    (i == 0) ? 0 : n,
		    (i == 0) ? 0 : ((n << 1) - 1));
		evcnt_attach_dynamic(&pow_ih->pi_ev_per[i],
		    EVCNT_TYPE_MISC, NULL, name, descr);
		descr += _DESCRLEN;
	}
	for (i = 0; i < (int)__arraycount(pow_ih->pi_ev_ival); i++) {
		int n = 1 << (i - 1);
		int p, q;
		char unit;

		p = n;
		q = (n << 1) - 1;
		unit = 'u';
		/*
		 * 0 is exceptional
		 */
		if (i == 0)
			p = q = 0;
		/*
		 * count 1024usec as 1msec
		 *
		 * XXX this is not exact
		 */
		if ((i - 1) >= 10) {
			p /= 1000;
			q /= 1000;
			unit = 'm';
		}
		(void)snprintf(descr, _DESCRLEN, "intr interval (%d-%d%csec)",
		    p, q, unit);
		evcnt_attach_dynamic(&pow_ih->pi_ev_ival[i],
		    EVCNT_TYPE_MISC, NULL, name, descr);
		descr += _DESCRLEN;
	}
	evcnt_attach_dynamic(&pow_ih->pi_ev_stray_tc,
	    EVCNT_TYPE_MISC, NULL, name, "stray intr (TC)");
	evcnt_attach_dynamic(&pow_ih->pi_ev_stray_ds,
	    EVCNT_TYPE_MISC, NULL, name, "stray intr (DS)");
	evcnt_attach_dynamic(&pow_ih->pi_ev_stray_iq,
	    EVCNT_TYPE_MISC, NULL, name, "stray intr (IQ)");
}
#endif

void
octeon_pow_init(struct octeon_pow_softc *sc)
{
	octeon_pow_init_regs(sc);

	sc->sc_int_pc_base = 10000;
	octeon_pow_config_int_pc(sc, sc->sc_int_pc_base);

#ifdef OCTEON_ETH_DEBUG
	octeon_pow_error_int_enable(sc, 1);
#endif
}

void
octeon_pow_init_regs(struct octeon_pow_softc *sc)
{
	int status;

	status = bus_space_map(sc->sc_regt, POW_BASE, POW_SIZE, 0,
	    &sc->sc_regh);
	if (status != 0)
		panic("can't map %s space", "pow register");

#ifdef OCTEON_ETH_DEBUG
	_POW_WR8(sc, POW_ECC_ERR_OFFSET,
	    POW_ECC_ERR_IOP_IE | POW_ECC_ERR_RPE_IE |
	    POW_ECC_ERR_DBE_IE | POW_ECC_ERR_SBE_IE);
#endif
}

/* -------------------------------------------------------------------------- */

/* ---- interrupt handling */

#ifdef OCTEON_ETH_DEBUG
static inline void
octeon_pow_intr_work_debug_ival(struct octeon_pow_softc *sc,
    struct octeon_pow_intr_handle *pow_ih)
{
	struct timeval now;
	struct timeval ival;
	int n;

	microtime(&now);
	if (__predict_false(pow_ih->pi_first == 1)) {
		pow_ih->pi_first = 0;
		goto stat_done;
	}
	timersub(&now, &pow_ih->pi_last, &ival);
	if (ival.tv_sec != 0)
		goto stat_done;	/* XXX */
	n = ffs64((uint64_t)ival.tv_usec);
	if (n > (int)__arraycount(pow_ih->pi_ev_ival) - 1)
		n = (int)__arraycount(pow_ih->pi_ev_ival) - 1;
	pow_ih->pi_ev_ival[n].ev_count++;

stat_done:
	pow_ih->pi_last = now;	/* struct copy */
}

static inline void
octeon_pow_intr_work_debug_per(struct octeon_pow_softc *sc,
    struct octeon_pow_intr_handle *pow_ih, int count)
{
	int n;

	n = ffs64(count);
	if (n > (int)__arraycount(pow_ih->pi_ev_per) - 1)
		n = (int)__arraycount(pow_ih->pi_ev_per) - 1;
	pow_ih->pi_ev_per[n].ev_count++;
#if 1
	if (count == 0) {
		uint64_t wq_int_cnt;

		wq_int_cnt = _POW_GROUP_RD8(sc, pow_ih, POW_WQ_INT_CNT0_OFFSET);
		if (wq_int_cnt & POW_WQ_INT_CNTX_TC_CNT)
			pow_ih->pi_ev_stray_tc.ev_count++;
		if (wq_int_cnt & POW_WQ_INT_CNTX_DS_CNT)
			pow_ih->pi_ev_stray_ds.ev_count++;
		if (wq_int_cnt & POW_WQ_INT_CNTX_IQ_CNT)
			pow_ih->pi_ev_stray_iq.ev_count++;
	}
#endif
}
#endif

#ifdef OCTEON_ETH_DEBUG
#define _POW_INTR_WORK_DEBUG_IVAL(sc, ih) \
	    octeon_pow_intr_work_debug_ival((sc), (ih))
#define _POW_INTR_WORK_DEBUG_PER(sc, ih, count) \
	    octeon_pow_intr_work_debug_per((sc), (ih), (count))
#else
#define _POW_INTR_WORK_DEBUG_IVAL(sc, ih) \
	    do {} while (0)
#define _POW_INTR_WORK_DEBUG_PER(sc, ih, count) \
	    do {} while (0)
#endif

/*
 * Interrupt handling by fixed count, following Cavium's SDK code.
 *
 * XXX the fixed count (MAX_RX_CNT) could be changed dynamically?
 *
 * XXX this does not utilize "tag switch" very well
 */
/*
 * usually all packet recieve
 */
#define MAX_RX_CNT 0x7fffffff 

static inline void
octeon_pow_intr_work(struct octeon_pow_softc *sc,
    struct octeon_pow_intr_handle *pow_ih, int recv_limit)
{
	uint64_t *work;
	uint64_t count = 0;

	_POW_WR8(sc, POW_PP_GRP_MSK0_OFFSET, UINT64_C(1) << pow_ih->pi_group);

	_POW_INTR_WORK_DEBUG_IVAL(sc, pow_ih);

	for (count = 0; count < recv_limit; count++) {
		octeon_pow_tag_sw_wait();
		octeon_pow_work_request_async(
		    OCTEON_CVMSEG_OFFSET(csm_pow_intr), POW_NO_WAIT);
		work = (uint64_t *)octeon_pow_work_response_async(
		    OCTEON_CVMSEG_OFFSET(csm_pow_intr));
		if (work == NULL)
			break;
		(*pow_ih->pi_cb)(pow_ih->pi_data, work);
	}

	_POW_INTR_WORK_DEBUG_PER(sc, pow_ih, count);
}

static int
octeon_pow_intr(void *data)
{
	struct octeon_pow_intr_handle *pow_ih = data;
	struct octeon_pow_softc *sc = pow_ih->pi_sc;
	uint64_t wq_int_mask = UINT64_C(0x1) << pow_ih->pi_group;

#ifdef OCTEON_ETH_INTR_FEEDBACK_RING
	octeon_pow_intr_work(sc, pow_ih, recv_cnt);
#else
	octeon_pow_intr_work(sc, pow_ih, INT_MAX);
#endif /* OCTEON_ETH_INTR_FEEDBACK_RING */

	_POW_WR8(sc, POW_WQ_INT_OFFSET, wq_int_mask << POW_WQ_INT_WQ_INT_SHIFT);
	return 1;
}

#ifdef OCTEON_ETH_INTR_FEEDBACK_RING
int
octeon_pow_ring_reduce(void *arg)
{
	struct octeon_pow_softc *sc = arg;
	int new, newi;
	int s;

#if 0
	if (ipflow_fastforward_disable_flags == 0) {
		newi = int_rate = 1;
		octeon_pow_config_int_pc_rate(sc, int_rate);
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
			octeon_pow_config_int_pc_rate(sc, int_rate);
			new = max_recv_cnt;
		}
	}

	s = splhigh(); /* XXX */
	recv_cnt = new;
	splx(s);

	return new;
}

int
octeon_pow_ring_grow(void *arg)
{
	struct octeon_pow_softc *sc = arg;
	int new, newi;
	int s;

#if 0
	if (ipflow_fastforward_disable_flags == 0) {
		newi = int_rate = 1;
		octeon_pow_config_int_pc_rate(sc, int_rate);
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
			octeon_pow_config_int_pc_rate(sc, int_rate);
			new = min_recv_cnt;
		}
	}

	s = splhigh(); /* XXX */
	recv_cnt = new;
	splx(s);

	return new;
}

int
octeon_pow_ring_size(void)
{
	return recv_cnt;
}

int
octeon_pow_ring_intr(void)
{
	return int_rate;
}
#endif /* OCTEON_ETH_INTR_FEEDBACK_RING */

/* -------------------------------------------------------------------------- */

/* ---- debug configuration */

#ifdef OCTEON_ETH_DEBUG

void
octeon_pow_error_int_enable(void *data, int enable)
{
	struct octeon_pow_softc *sc = data;
	uint64_t pow_error_int_xxx;

	pow_error_int_xxx =
	    POW_ECC_ERR_IOP | POW_ECC_ERR_RPE |
	    POW_ECC_ERR_DBE | POW_ECC_ERR_SBE;
	_POW_WR8(sc, POW_ECC_ERR_OFFSET, pow_error_int_xxx);
	_POW_WR8(sc, POW_ECC_ERR_OFFSET, enable ? pow_error_int_xxx : 0);
}

uint64_t
octeon_pow_error_int_summary(void *data)
{
	struct octeon_pow_softc *sc = data;
	uint64_t summary;

	summary = _POW_RD8(sc, POW_ECC_ERR_OFFSET);
	_POW_WR8(sc, POW_ECC_ERR_OFFSET, summary);
	return summary;
}

#endif

/* -------------------------------------------------------------------------- */

/* ---- debug counter */

#ifdef OCTEON_ETH_DEBUG
int			octeon_pow_intr_rml_verbose;
struct evcnt		octeon_pow_intr_evcnt;

static const struct octeon_evcnt_entry octeon_pow_intr_evcnt_entries[] = {
#define	_ENTRY(name, type, parent, descr) \
	OCTEON_EVCNT_ENTRY(struct octeon_pow_softc, name, type, parent, descr)
	_ENTRY(powecciopcsrpend,	MISC, NULL, "pow csr load"),
	_ENTRY(powecciopdbgpend,	MISC, NULL, "pow dbg load"),
	_ENTRY(powecciopaddwork,	MISC, NULL, "pow addwork"),
	_ENTRY(powecciopillop,		MISC, NULL, "pow ill op"),
	_ENTRY(poweccioppend24,		MISC, NULL, "pow pend24"),
	_ENTRY(poweccioppend23,		MISC, NULL, "pow pend23"),
	_ENTRY(poweccioppend22,		MISC, NULL, "pow pend22"),
	_ENTRY(poweccioppend21,		MISC, NULL, "pow pend21"),
	_ENTRY(poweccioptagnull,	MISC, NULL, "pow tag null"),
	_ENTRY(poweccioptagnullnull,	MISC, NULL, "pow tag nullnull"),
	_ENTRY(powecciopordatom,	MISC, NULL, "pow ordered atomic"),
	_ENTRY(powecciopnull,		MISC, NULL, "pow core null"),
	_ENTRY(powecciopnullnull,	MISC, NULL, "pow core nullnull"),
	_ENTRY(poweccrpe,		MISC, NULL, "pow remote-pointer error"),
	_ENTRY(poweccsyn,		MISC, NULL, "pow syndrome value"),
	_ENTRY(poweccdbe,		MISC, NULL, "pow double bit"),
	_ENTRY(poweccsbe,		MISC, NULL, "pow single bit"),
#undef	_ENTRY
};

void
octeon_pow_intr_evcnt_attach(struct octeon_pow_softc *sc)
{
	OCTEON_EVCNT_ATTACH_EVCNTS(sc, octeon_pow_intr_evcnt_entries, "pow0");
}

void
octeon_pow_intr_rml(void *arg)
{
	struct octeon_pow_softc *sc;
	uint64_t reg;

	octeon_pow_intr_evcnt.ev_count++;
	sc = __octeon_pow_softc;
	KASSERT(sc != NULL);
	reg = octeon_pow_error_int_summary(sc);
	if (octeon_pow_intr_rml_verbose)
		printf("%s: POW_ECC_ERR=0x%016" PRIx64 "\n", __func__, reg);
	switch (reg & POW_ECC_ERR_IOP) {
	case POW_ECC_ERR_IOP_CSRPEND:
		OCTEON_EVCNT_INC(sc, powecciopcsrpend);
		break;
	case POW_ECC_ERR_IOP_DBGPEND:
		OCTEON_EVCNT_INC(sc, powecciopdbgpend);
		break;
	case POW_ECC_ERR_IOP_ADDWORK:
		OCTEON_EVCNT_INC(sc, powecciopaddwork);
		break;
	case POW_ECC_ERR_IOP_ILLOP:
		OCTEON_EVCNT_INC(sc, powecciopillop);
		break;
	case POW_ECC_ERR_IOP_PEND24:
		OCTEON_EVCNT_INC(sc, poweccioppend24);
		break;
	case POW_ECC_ERR_IOP_PEND23:
		OCTEON_EVCNT_INC(sc, poweccioppend23);
		break;
	case POW_ECC_ERR_IOP_PEND22:
		OCTEON_EVCNT_INC(sc, poweccioppend22);
		break;
	case POW_ECC_ERR_IOP_PEND21:
		OCTEON_EVCNT_INC(sc, poweccioppend21);
		break;
	case POW_ECC_ERR_IOP_TAGNULL:
		OCTEON_EVCNT_INC(sc, poweccioptagnull);
		break;
	case POW_ECC_ERR_IOP_TAGNULLNULL:
		OCTEON_EVCNT_INC(sc, poweccioptagnullnull);
		break;
	case POW_ECC_ERR_IOP_ORDATOM:
		OCTEON_EVCNT_INC(sc, powecciopordatom);
		break;
	case POW_ECC_ERR_IOP_NULL:
		OCTEON_EVCNT_INC(sc, powecciopnull);
		break;
	case POW_ECC_ERR_IOP_NULLNULL:
		OCTEON_EVCNT_INC(sc, powecciopnullnull);
		break;
	default:
		break;
	}
	if (reg & POW_ECC_ERR_RPE)
		OCTEON_EVCNT_INC(sc, poweccrpe);
	if (reg & POW_ECC_ERR_SYN)
		OCTEON_EVCNT_INC(sc, poweccsyn);
	if (reg & POW_ECC_ERR_DBE)
		OCTEON_EVCNT_INC(sc, poweccdbe);
	if (reg & POW_ECC_ERR_SBE)
		OCTEON_EVCNT_INC(sc, poweccsbe);
}
#endif

/* -------------------------------------------------------------------------- */

/* ---- debug dump */

#ifdef OCTEON_ETH_DEBUG

void			octeon_pow_dump_reg(void);
void			octeon_pow_dump_ops(void);

void
octeon_pow_dump(void)
{
	octeon_pow_dump_reg();
	octeon_pow_dump_ops();
}

/* ---- register dump */

struct octeon_pow_dump_reg_entry {
	const char *name;
	const char *format;
	size_t offset;
};

#define	_ENTRY(x)	{ #x, x##_BITS, x##_OFFSET }
#define	_ENTRY_0_7(x) \
	_ENTRY(x## 0), _ENTRY(x## 1), _ENTRY(x## 2), _ENTRY(x## 3), \
	_ENTRY(x## 4), _ENTRY(x## 5), _ENTRY(x## 6), _ENTRY(x## 7)
#define	_ENTRY_0_15(x) \
	_ENTRY(x## 0), _ENTRY(x## 1), _ENTRY(x## 2), _ENTRY(x## 3), \
	_ENTRY(x## 4), _ENTRY(x## 5), _ENTRY(x## 6), _ENTRY(x## 7), \
	_ENTRY(x## 8), _ENTRY(x## 9), _ENTRY(x##10), _ENTRY(x##11), \
	_ENTRY(x##12), _ENTRY(x##13), _ENTRY(x##14), _ENTRY(x##15)

static const struct octeon_pow_dump_reg_entry octeon_pow_dump_reg_entries[] = {
	_ENTRY		(POW_PP_GRP_MSK0),
	_ENTRY		(POW_PP_GRP_MSK1),
	_ENTRY_0_15	(POW_WQ_INT_THR),
	_ENTRY_0_15	(POW_WQ_INT_CNT),
	_ENTRY_0_7	(POW_QOS_THR),
	_ENTRY_0_7	(POW_QOS_RND),
	_ENTRY		(POW_WQ_INT),
	_ENTRY		(POW_WQ_INT_PC),
	_ENTRY		(POW_NW_TIM),
	_ENTRY		(POW_ECC_ERR),
	_ENTRY		(POW_NOS_CNT),
	_ENTRY_0_15	(POW_WS_PC),
	_ENTRY_0_7	(POW_WA_PC),
	_ENTRY_0_7	(POW_IQ_CNT),
	_ENTRY		(POW_WA_COM_PC),
	_ENTRY		(POW_IQ_COM_CNT),
	_ENTRY		(POW_TS_PC),
	_ENTRY		(POW_DS_PC),
	_ENTRY		(POW_BIST_STAT)
};

#undef _ENTRY

void
octeon_pow_dump_reg(void)
{
	struct octeon_pow_softc *sc = __octeon_pow_softc;
	const struct octeon_pow_dump_reg_entry *entry;
	uint64_t tmp;
	char buf[512];
	int i;

	for (i = 0; i < (int)__arraycount(octeon_pow_dump_reg_entries); i++) {
		entry = &octeon_pow_dump_reg_entries[i];
		tmp = _POW_RD8(sc, entry->offset);
		if (entry->format == NULL)
			snprintf(buf, sizeof(buf), "%16" PRIx64, tmp);
		else
			snprintb(buf, sizeof(buf), entry->format, tmp);
		printf("\t%-24s: %s\n", entry->name, buf);
	}
}

/* ---- operations dump */

struct octeon_pow_dump_ops_entry {
	const char *name;
	const char *format;
	uint64_t (*func)(int);
};

void			octeon_pow_dump_ops_coreid(int);
void			octeon_pow_dump_ops_index(int);
void			octeon_pow_dump_ops_qos(int);
void			octeon_pow_dump_ops_grp(int);
void			octeon_pow_dump_ops_queue(int);
void                    octeon_pow_dump_ops_common(const struct
			    octeon_pow_dump_ops_entry *, size_t, const char *,
			    int);

#define	_ENTRY_COMMON(name, prefix, x, y) \
	{ #name "_" #x, prefix##_##y##_BITS, octeon_pow_status_by_##name##_##x }

const struct octeon_pow_dump_ops_entry octeon_pow_dump_ops_coreid_entries[] = {
#define	_ENTRY(x, y)	_ENTRY_COMMON(coreid, POW_STATUS_LOAD_RESULT, x, y)
	_ENTRY(pend_tag, PEND_TAG),
	_ENTRY(pend_wqp, PEND_WQP),
	_ENTRY(cur_tag_next, CUR_TAG_NEXT),
	_ENTRY(cur_tag_prev, CUR_TAG_PREV),
	_ENTRY(cur_wqp_next, CUR_WQP_NEXT),
	_ENTRY(cur_wqp_prev, CUR_WQP_PREV)
#undef _ENTRY
};

const struct octeon_pow_dump_ops_entry octeon_pow_dump_ops_index_entries[] = {
#define	_ENTRY(x, y)	_ENTRY_COMMON(index, POW_MEMORY_LOAD_RESULT, x, y)
	_ENTRY(tag, TAG),
	_ENTRY(wqp, WQP),
	_ENTRY(desched, DESCHED)
#undef _ENTRY
};

const struct octeon_pow_dump_ops_entry octeon_pow_dump_ops_qos_entries[] = {
#define	_ENTRY(x, y)	_ENTRY_COMMON(qos, POW_IDXPTR_LOAD_RESULT_QOS, x, y)
	_ENTRY(free_loc, FREE_LOC)
#undef _ENTRY
};

const struct octeon_pow_dump_ops_entry octeon_pow_dump_ops_grp_entries[] = {
#define	_ENTRY(x, y)	_ENTRY_COMMON(grp, POW_IDXPTR_LOAD_RESULT_GRP, x, y)
	_ENTRY(nosched_des, NOSCHED_DES)
#undef _ENTRY
};

const struct octeon_pow_dump_ops_entry octeon_pow_dump_ops_queue_entries[] = {
#define	_ENTRY(x, y)	_ENTRY_COMMON(queue, POW_IDXPTR_LOAD_RESULT_QUEUE, x, y)
	_ENTRY(remote_head, REMOTE_HEAD),
	_ENTRY(remote_tail, REMOTE_TAIL)
#undef _ENTRY
};

void
octeon_pow_dump_ops(void)
{
	int i;

	/* XXX */
	for (i = 0; i < 2/* XXX */; i++)
		octeon_pow_dump_ops_coreid(i);

	/* XXX */
	octeon_pow_dump_ops_index(0);

	for (i = 0; i < 8; i++)
		octeon_pow_dump_ops_qos(i);

	for (i = 0; i < 16; i++)
		octeon_pow_dump_ops_grp(i);

	for (i = 0; i < 16; i++)
		octeon_pow_dump_ops_queue(i);
}

void
octeon_pow_dump_ops_coreid(int coreid)
{
	octeon_pow_dump_ops_common(octeon_pow_dump_ops_coreid_entries,
	    __arraycount(octeon_pow_dump_ops_coreid_entries), "coreid", coreid);
}

void
octeon_pow_dump_ops_index(int index)
{
	octeon_pow_dump_ops_common(octeon_pow_dump_ops_index_entries,
	    __arraycount(octeon_pow_dump_ops_index_entries), "index", index);
}

void
octeon_pow_dump_ops_qos(int qos)
{
	octeon_pow_dump_ops_common(octeon_pow_dump_ops_qos_entries,
	    __arraycount(octeon_pow_dump_ops_qos_entries), "qos", qos);
}

void
octeon_pow_dump_ops_grp(int grp)
{
	octeon_pow_dump_ops_common(octeon_pow_dump_ops_grp_entries,
	    __arraycount(octeon_pow_dump_ops_grp_entries), "grp", grp);
}

void
octeon_pow_dump_ops_queue(int queue)
{
	octeon_pow_dump_ops_common(octeon_pow_dump_ops_queue_entries,
	    __arraycount(octeon_pow_dump_ops_queue_entries), "queue", queue);
}

void
octeon_pow_dump_ops_common(const struct octeon_pow_dump_ops_entry *entries,
    size_t nentries, const char *by_what, int arg)
{
	const struct octeon_pow_dump_ops_entry *entry;
	uint64_t tmp;
	char buf[512];
	int i;

	printf("%s=%d\n", by_what, arg);
	for (i = 0; i < (int)nentries; i++) {
		entry = &entries[i];
		tmp = (*entry->func)(arg);
		if (entry->format == NULL)
			snprintf(buf, sizeof(buf), "%16" PRIx64, tmp);
		else
			snprintb(buf, sizeof(buf), entry->format, tmp);
		printf("\t%-24s: %s\n", entry->name, buf);
	}
}

#endif

/* -------------------------------------------------------------------------- */

/* ---- test */

#ifdef OCTEON_POW_TEST
/*
 * Standalone test entries; meant to be called from ddb.
 */

void			octeon_pow_test(void);
void			octeon_pow_test_dump_wqe(paddr_t);

static void		octeon_pow_test_1(void);

struct test_wqe {
	uint64_t word0;
	uint64_t word1;
	uint64_t word2;
	uint64_t word3;
} __packed;
struct test_wqe test_wqe;

void
octeon_pow_test(void)
{
	octeon_pow_test_1();
}

static void
octeon_pow_test_1(void)
{
	struct test_wqe *wqe = &test_wqe;
	int qos, grp, queue, tt;
	uint32_t tag;
	paddr_t ptr;

	qos = 7;			/* XXX */
	grp = queue = 15;		/* XXX */
	tt = POW_TAG_TYPE_ORDERED;	/* XXX */
	tag = UINT32_C(0x01234567);	/* XXX */

	/* => make sure that the queue is empty */

	octeon_pow_dump_ops_qos(qos);
	octeon_pow_dump_ops_grp(grp);
	printf("\n");

	/*
	 * Initialize WQE.
	 *
	 * word0:next is used by hardware.
	 *
	 * word1:qos, word1:grp, word1:tt, word1:tag must match with arguments
	 * of the following ADDWQ transaction.
	 */

	(void)memset(wqe, 0, sizeof(*wqe));
	wqe->word0 =
	    __BITS64_SET(POW_WQE_WORD0_NEXT, 0);
	wqe->word1 =
	    __BITS64_SET(POW_WQE_WORD1_QOS, qos) |
	    __BITS64_SET(POW_WQE_WORD1_GRP, grp) |
	    __BITS64_SET(POW_WQE_WORD1_TT, tt) |
	    __BITS64_SET(POW_WQE_WORD1_TAG, tag);

	printf("calling ADDWQ\n");
	octeon_pow_ops_addwq(MIPS_KSEG0_TO_PHYS(wqe), qos, grp, tt, tag);

	octeon_pow_dump_ops_qos(qos);
	octeon_pow_dump_ops_grp(grp);
	printf("\n");

	/* => make sure that a WQE is added to the queue */

	printf("calling GET_WORK_LOAD\n");
	ptr = octeon_pow_ops_get_work_load(0);

	octeon_pow_dump_ops_qos(qos);
	octeon_pow_dump_ops_grp(grp);
	printf("\n");

	octeon_pow_test_dump_wqe(ptr);

	/* => make sure that the WQE is in-flight (and scheduled) */

	printf("calling SWTAG(NULL)\n");
	octeon_pow_ops_swtag(POW_TAG_TYPE_NULL, tag);

	octeon_pow_dump_ops_qos(qos);
	octeon_pow_dump_ops_grp(grp);
	printf("\n");

	/* => make sure that the WQE is un-scheduled (completed) */
}

void
octeon_pow_test_dump_wqe(paddr_t ptr)
{
	uint64_t word0, word1;
	char buf[128];

	printf("wqe\n");

	word0 = *(uint64_t *)MIPS_PHYS_TO_XKPHYS_CACHED(ptr);
	snprintb(buf, sizeof(buf), POW_WQE_WORD0_BITS, word0);
	printf("\t%-24s: %s\n", "word0", buf);

	word1 = *(uint64_t *)MIPS_PHYS_TO_XKPHYS_CACHED(ptr + 8);
	snprintb(buf, sizeof(buf), POW_WQE_WORD1_BITS, word1);
	printf("\t%-24s: %s\n", "word1", buf);
}
#endif
