/*	$NetBSD: octeon_pow.c,v 1.7 2020/06/18 13:52:08 simonb Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: octeon_pow.c,v 1.7 2020/06/18 13:52:08 simonb Exp $");

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

#ifdef CNMAC_DEBUG
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

void			octpow_bootstrap(struct octeon_config *);

#ifdef CNMAC_DEBUG
void			octpow_intr_evcnt_attach(struct octpow_softc *);
void			octpow_intr_rml(void *);

static void             octpow_intr_debug_init(
			    struct octpow_intr_handle *, int);
static inline void      octpow_intr_work_debug_ival(struct octpow_softc *,
			    struct octpow_intr_handle *);
static inline void	octpow_intr_work_debug_per(struct octpow_softc *,
			    struct octpow_intr_handle *, int);
#endif
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

#ifdef CNMAC_DEBUG
void			octpow_dump(void);
#endif

/* XXX */
struct octpow_softc	octpow_softc;

#ifdef CNMAC_DEBUG
struct octpow_softc	*__octpow_softc;
#endif

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

#ifdef CNMAC_DEBUG
	__octpow_softc = sc;
#endif

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
	    ffs64(CIU_INTX_SUM0_WORKQ_0) - 1 + group,
	    level,
	    octpow_intr, pow_ih);
	KASSERT(pow_ih->pi_ih != NULL);

	pow_ih->pi_sc = &octpow_softc;	/* XXX */
	pow_ih->pi_group = group;
	pow_ih->pi_cb = cb;
	pow_ih->pi_data = data;

#ifdef CNMAC_DEBUG
	octpow_intr_debug_init(pow_ih, group);
#endif
	return pow_ih;
}

#ifdef CNMAC_DEBUG
#define	_NAMELEN	8
#define	_DESCRLEN	40

static void
octpow_intr_debug_init(struct octpow_intr_handle *pow_ih, int group)
{
	pow_ih->pi_first = 1;
	char *name, *descr;
	int i;

	name = malloc(_NAMELEN +
	    _DESCRLEN * __arraycount(pow_ih->pi_ev_per) +
	    _DESCRLEN * __arraycount(pow_ih->pi_ev_ival),
	    M_DEVBUF, M_WAITOK);
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
octpow_init(struct octpow_softc *sc)
{
	octpow_init_regs(sc);

	sc->sc_int_pc_base = 10000;
	octpow_config_int_pc(sc, sc->sc_int_pc_base);

#ifdef CNMAC_DEBUG
	octpow_error_int_enable(sc, 1);
#endif
}

void
octpow_init_regs(struct octpow_softc *sc)
{
	int status;

	status = bus_space_map(sc->sc_regt, POW_BASE, POW_SIZE, 0,
	    &sc->sc_regh);
	if (status != 0)
		panic("can't map %s space", "pow register");

#ifdef CNMAC_DEBUG
	_POW_WR8(sc, POW_ECC_ERR_OFFSET,
	    POW_ECC_ERR_IOP_IE | POW_ECC_ERR_RPE_IE |
	    POW_ECC_ERR_DBE_IE | POW_ECC_ERR_SBE_IE);
#endif
}

/* -------------------------------------------------------------------------- */

/* ---- interrupt handling */

#ifdef CNMAC_DEBUG
static inline void
octpow_intr_work_debug_ival(struct octpow_softc *sc,
    struct octpow_intr_handle *pow_ih)
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
octpow_intr_work_debug_per(struct octpow_softc *sc,
    struct octpow_intr_handle *pow_ih, int count)
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

#ifdef CNMAC_DEBUG
#define _POW_INTR_WORK_DEBUG_IVAL(sc, ih) \
	    octpow_intr_work_debug_ival((sc), (ih))
#define _POW_INTR_WORK_DEBUG_PER(sc, ih, count) \
	    octpow_intr_work_debug_per((sc), (ih), (count))
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

	_POW_INTR_WORK_DEBUG_IVAL(sc, pow_ih);

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

	_POW_INTR_WORK_DEBUG_PER(sc, pow_ih, count);
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

/* -------------------------------------------------------------------------- */

/* ---- debug configuration */

#ifdef CNMAC_DEBUG

void
octpow_error_int_enable(void *data, int enable)
{
	struct octpow_softc *sc = data;
	uint64_t pow_error_int_xxx;

	pow_error_int_xxx =
	    POW_ECC_ERR_IOP | POW_ECC_ERR_RPE |
	    POW_ECC_ERR_DBE | POW_ECC_ERR_SBE;
	_POW_WR8(sc, POW_ECC_ERR_OFFSET, pow_error_int_xxx);
	_POW_WR8(sc, POW_ECC_ERR_OFFSET, enable ? pow_error_int_xxx : 0);
}

uint64_t
octpow_error_int_summary(void *data)
{
	struct octpow_softc *sc = data;
	uint64_t summary;

	summary = _POW_RD8(sc, POW_ECC_ERR_OFFSET);
	_POW_WR8(sc, POW_ECC_ERR_OFFSET, summary);
	return summary;
}

#endif

/* -------------------------------------------------------------------------- */

/* ---- debug counter */

#ifdef CNMAC_DEBUG
int			octpow_intr_rml_verbose;
struct evcnt		octpow_intr_evcnt;

static const struct octeon_evcnt_entry octpow_intr_evcnt_entries[] = {
#define	_ENTRY(name, type, parent, descr) \
	OCTEON_EVCNT_ENTRY(struct octpow_softc, name, type, parent, descr)
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
octpow_intr_evcnt_attach(struct octpow_softc *sc)
{
	OCTEON_EVCNT_ATTACH_EVCNTS(sc, octpow_intr_evcnt_entries, "pow0");
}

void
octpow_intr_rml(void *arg)
{
	struct octpow_softc *sc;
	uint64_t reg;

	octpow_intr_evcnt.ev_count++;
	sc = __octpow_softc;
	KASSERT(sc != NULL);
	reg = octpow_error_int_summary(sc);
	if (octpow_intr_rml_verbose)
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

#ifdef CNMAC_DEBUG

void			octpow_dump_reg(void);
void			octpow_dump_ops(void);

void
octpow_dump(void)
{
	octpow_dump_reg();
	octpow_dump_ops();
}

/* ---- register dump */

struct octpow_dump_reg_entry {
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

static const struct octpow_dump_reg_entry octpow_dump_reg_entries[] = {
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
octpow_dump_reg(void)
{
	struct octpow_softc *sc = __octpow_softc;
	const struct octpow_dump_reg_entry *entry;
	uint64_t tmp;
	char buf[512];
	int i;

	for (i = 0; i < (int)__arraycount(octpow_dump_reg_entries); i++) {
		entry = &octpow_dump_reg_entries[i];
		tmp = _POW_RD8(sc, entry->offset);
		if (entry->format == NULL)
			snprintf(buf, sizeof(buf), "%16" PRIx64, tmp);
		else
			snprintb(buf, sizeof(buf), entry->format, tmp);
		printf("\t%-24s: %s\n", entry->name, buf);
	}
}

/* ---- operations dump */

struct octpow_dump_ops_entry {
	const char *name;
	const char *format;
	uint64_t (*func)(int);
};

void	octpow_dump_ops_coreid(int);
void	octpow_dump_ops_index(int);
void	octpow_dump_ops_qos(int);
void	octpow_dump_ops_grp(int);
void	octpow_dump_ops_queue(int);
void	octpow_dump_ops_common(const struct octpow_dump_ops_entry *, size_t,
	    const char *, int);

#define	_ENTRY_COMMON(name, prefix, x, y) \
	{ #name "_" #x, prefix##_##y##_BITS, octpow_status_by_##name##_##x }

const struct octpow_dump_ops_entry octpow_dump_ops_coreid_entries[] = {
#define	_ENTRY(x, y)	_ENTRY_COMMON(coreid, POW_STATUS_LOAD_RESULT, x, y)
	_ENTRY(pend_tag, PEND_TAG),
	_ENTRY(pend_wqp, PEND_WQP),
	_ENTRY(cur_tag_next, CUR_TAG_NEXT),
	_ENTRY(cur_tag_prev, CUR_TAG_PREV),
	_ENTRY(cur_wqp_next, CUR_WQP_NEXT),
	_ENTRY(cur_wqp_prev, CUR_WQP_PREV)
#undef _ENTRY
};

const struct octpow_dump_ops_entry octpow_dump_ops_index_entries[] = {
#define	_ENTRY(x, y)	_ENTRY_COMMON(index, POW_MEMORY_LOAD_RESULT, x, y)
	_ENTRY(tag, TAG),
	_ENTRY(wqp, WQP),
	_ENTRY(desched, DESCHED)
#undef _ENTRY
};

const struct octpow_dump_ops_entry octpow_dump_ops_qos_entries[] = {
#define	_ENTRY(x, y)	_ENTRY_COMMON(qos, POW_IDXPTR_LOAD_RESULT_QOS, x, y)
	_ENTRY(free_loc, FREE_LOC)
#undef _ENTRY
};

const struct octpow_dump_ops_entry octpow_dump_ops_grp_entries[] = {
#define	_ENTRY(x, y)	_ENTRY_COMMON(grp, POW_IDXPTR_LOAD_RESULT_GRP, x, y)
	_ENTRY(nosched_des, NOSCHED_DES)
#undef _ENTRY
};

const struct octpow_dump_ops_entry octpow_dump_ops_queue_entries[] = {
#define	_ENTRY(x, y)	_ENTRY_COMMON(queue, POW_IDXPTR_LOAD_RESULT_QUEUE, x, y)
	_ENTRY(remote_head, REMOTE_HEAD),
	_ENTRY(remote_tail, REMOTE_TAIL)
#undef _ENTRY
};

void
octpow_dump_ops(void)
{
	int i;

	/* XXX */
	for (i = 0; i < 2/* XXX */; i++)
		octpow_dump_ops_coreid(i);

	/* XXX */
	octpow_dump_ops_index(0);

	for (i = 0; i < 8; i++)
		octpow_dump_ops_qos(i);

	for (i = 0; i < 16; i++)
		octpow_dump_ops_grp(i);

	for (i = 0; i < 16; i++)
		octpow_dump_ops_queue(i);
}

void
octpow_dump_ops_coreid(int coreid)
{
	octpow_dump_ops_common(octpow_dump_ops_coreid_entries,
	    __arraycount(octpow_dump_ops_coreid_entries), "coreid", coreid);
}

void
octpow_dump_ops_index(int index)
{
	octpow_dump_ops_common(octpow_dump_ops_index_entries,
	    __arraycount(octpow_dump_ops_index_entries), "index", index);
}

void
octpow_dump_ops_qos(int qos)
{
	octpow_dump_ops_common(octpow_dump_ops_qos_entries,
	    __arraycount(octpow_dump_ops_qos_entries), "qos", qos);
}

void
octpow_dump_ops_grp(int grp)
{
	octpow_dump_ops_common(octpow_dump_ops_grp_entries,
	    __arraycount(octpow_dump_ops_grp_entries), "grp", grp);
}

void
octpow_dump_ops_queue(int queue)
{
	octpow_dump_ops_common(octpow_dump_ops_queue_entries,
	    __arraycount(octpow_dump_ops_queue_entries), "queue", queue);
}

void
octpow_dump_ops_common(const struct octpow_dump_ops_entry *entries,
    size_t nentries, const char *by_what, int arg)
{
	const struct octpow_dump_ops_entry *entry;
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

#ifdef octPOW_TEST
/*
 * Standalone test entries; meant to be called from ddb.
 */

void			octpow_test(void);
void			octpow_test_dump_wqe(paddr_t);

static void		octpow_test_1(void);

struct test_wqe {
	uint64_t word0;
	uint64_t word1;
	uint64_t word2;
	uint64_t word3;
} __packed;
struct test_wqe test_wqe;

void
octpow_test(void)
{
	octpow_test_1();
}

static void
octpow_test_1(void)
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

	octpow_dump_ops_qos(qos);
	octpow_dump_ops_grp(grp);
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
	octpow_ops_addwq(MIPS_KSEG0_TO_PHYS(wqe), qos, grp, tt, tag);

	octpow_dump_ops_qos(qos);
	octpow_dump_ops_grp(grp);
	printf("\n");

	/* => make sure that a WQE is added to the queue */

	printf("calling GET_WORK_LOAD\n");
	ptr = octpow_ops_get_work_load(0);

	octpow_dump_ops_qos(qos);
	octpow_dump_ops_grp(grp);
	printf("\n");

	octpow_test_dump_wqe(ptr);

	/* => make sure that the WQE is in-flight (and scheduled) */

	printf("calling SWTAG(NULL)\n");
	octpow_ops_swtag(POW_TAG_TYPE_NULL, tag);

	octpow_dump_ops_qos(qos);
	octpow_dump_ops_grp(grp);
	printf("\n");

	/* => make sure that the WQE is un-scheduled (completed) */
}

void
octpow_test_dump_wqe(paddr_t ptr)
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
