/*	$NetBSD: octeon_powvar.h,v 1.1.2.3 2016/10/05 20:55:31 skrll Exp $	*/

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

#ifndef _OCTEON_POWVAR_H_
#define _OCTEON_POWVAR_H_

#include <sys/cpu.h>

#define POW_TAG_TYPE_ORDERED	0
#define POW_TAG_TYPE_ATOMIC	1
#define POW_TAG_TYPE_NULL	2
#define POW_TAG_TYPE_NULL_NULL	3

#define POW_TAG_OP_SWTAG		0
#define POW_TAG_OP_SWTAG_FULL		1
#define POW_TAG_OP_SWTAG_DESCHED	2
#define POW_TAG_OP_DESCHED		3
#define POW_TAG_OP_ADDWQ		4
#define POW_TAG_OP_UPD_WQP_GRP		5
#define POW_TAG_OP_CLR_NSCHED		7
#define POW_TAG_OP_NOP			15

#define POW_WAIT	1
#define POW_NO_WAIT	0

/* XXX */
struct octeon_pow_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;
	int			sc_port;
	int			sc_int_pc_base;
#ifdef OCTEON_ETH_DEBUG
	struct evcnt		sc_ev_powecciopcsrpend;
	struct evcnt		sc_ev_powecciopdbgpend;
	struct evcnt		sc_ev_powecciopaddwork;
	struct evcnt		sc_ev_powecciopillop;
	struct evcnt		sc_ev_poweccioppend24;
	struct evcnt		sc_ev_poweccioppend23;
	struct evcnt		sc_ev_poweccioppend22;
	struct evcnt		sc_ev_poweccioppend21;
	struct evcnt		sc_ev_poweccioptagnull;
	struct evcnt		sc_ev_poweccioptagnullnull;
	struct evcnt		sc_ev_powecciopordatom;
	struct evcnt		sc_ev_powecciopnull;
	struct evcnt		sc_ev_powecciopnullnull;
	struct evcnt		sc_ev_poweccrpe;
	struct evcnt		sc_ev_poweccsyn;
	struct evcnt		sc_ev_poweccdbe;
	struct evcnt		sc_ev_poweccsbe;
#endif
};

/* XXX */
struct octeon_pow_attach_args {
	int			aa_port;
	bus_space_tag_t		aa_regt;
};

void			octeon_pow_config(struct octeon_pow_softc *, int);
void			*octeon_pow_intr_establish(int, int,
			    void (*)(void *, uint64_t *),
			    void (*)(int *, int *, uint64_t, void *),
			    void *);
void			octeon_pow_error_int_enable(void *, int);
uint64_t		octeon_pow_error_int_summary(void *);
int			octeon_pow_ring_reduce(void *);
int			octeon_pow_ring_grow(void *);
int			octeon_pow_ring_size(void);
int			octeon_pow_ring_intr(void);

#define	_POW_RD8(sc, off) \
	bus_space_read_8((sc)->sc_regt, (sc)->sc_regh, (off))
#define	_POW_WR8(sc, off, v) \
	bus_space_write_8((sc)->sc_regt, (sc)->sc_regh, (off), (v))
#define	_POW_GROUP_RD8(sc, pi, off) \
	bus_space_read_8((sc)->sc_regt, (sc)->sc_regh, \
	    (off) + sizeof(uint64_t) * (pi)->pi_group)
#define	_POW_GROUP_WR8(sc, pi, off, v) \
	bus_space_write_8((sc)->sc_regt, (sc)->sc_regh, \
	    (off) + sizeof(uint64_t) * (pi)->pi_group, (v))

extern struct octeon_pow_softc	octeon_pow_softc;

/* -------------------------------------------------------------------------- */

/* Load Operations */

/* GET_WORK Loads */

static inline uint64_t
octeon_pow_ops_get_work_load(
	int wait)			/* 0-1 */
{
	uint64_t ptr =
	    POW_OPERATION_BASE_IO_BIT |
	    __BITS64_SET(POW_OPERATION_BASE_MAJOR_DID, 0x0c) |
	    __BITS64_SET(POW_OPERATION_BASE_SUB_DID, 0x00) |
	    __BITS64_SET(POW_GET_WORK_LOAD_WAIT, wait);

	return octeon_xkphys_read_8(ptr);
}

/* POW Status Loads */

/*
 * a) get_cur == 0, get_wqp == 0 (pend_tag)
 * b) get_cur == 0, get_wqp == 1 (pend_wqp)
 * c) get_cur == 1, get_wqp == 0, get_rev == 0 (cur_tag_next)
 * d) get_cur == 1, get_wqp == 0, get_rev == 1 (cur_tag_prev)
 * e) get_cur == 1, get_wqp == 1, get_rev == 0 (cur_wqp_next)
 * f) get_cur == 1, get_wqp == 1, get_rev == 1 (cur_wqp_prev)
 */

static inline uint64_t
octeon_pow_ops_pow_status(
	int coreid,			/* 0-15 */
	int get_rev,			/* 0-1 */
	int get_cur,			/* 0-1 */
	int get_wqp)			/* 0-1 */
{
	uint64_t ptr =
	    POW_OPERATION_BASE_IO_BIT |
	    __BITS64_SET(POW_OPERATION_BASE_MAJOR_DID, 0x0c) |
	    __BITS64_SET(POW_OPERATION_BASE_SUB_DID, 0x01) |
	    __BITS64_SET(POW_STATUS_LOAD_COREID, coreid) |
	    __BITS64_SET(POW_STATUS_LOAD_GET_REV, get_rev) |
	    __BITS64_SET(POW_STATUS_LOAD_GET_CUR, get_cur) |
	    __BITS64_SET(POW_STATUS_LOAD_GET_WQP, get_wqp);

	return octeon_xkphys_read_8(ptr);
}

/* POW Memory Loads */

/*
 * a) get_des == 0, get_wqp == 0 (tag)
 * b) get_des == 0, get_wqp == 1 (wqe)
 * c) get_des == 1 (desched)
 */

static inline uint64_t
octeon_pow_ops_pow_memory(
	int index,			/* 0-2047 */
	int get_des,			/* 0-1 */
	int get_wqp)			/* 0-1 */
{
	uint64_t ptr =
	    POW_OPERATION_BASE_IO_BIT |
	    __BITS64_SET(POW_OPERATION_BASE_MAJOR_DID, 0x0c) |
	    __BITS64_SET(POW_OPERATION_BASE_SUB_DID, 0x02) |
	    __BITS64_SET(POW_MEMORY_LOAD_INDEX, index) |
	    __BITS64_SET(POW_MEMORY_LOAD_GET_DES, get_des) |
	    __BITS64_SET(POW_MEMORY_LOAD_GET_WQP, get_wqp);

	return octeon_xkphys_read_8(ptr);
}

/* POW Index/Pointer Loads */

/*
 * a) get_rmt == 0, get_des_get_tail == 0
 * b) get_rmt == 0, get_des_get_tail == 1
 * c) get_rmt == 1, get_des_get_tail == 0
 * d) get_rmt == 1, get_des_get_tail == 1
 */

static inline uint64_t
octeon_pow_ops_pow_idxptr(
	int qosgrp,			/* 0-7 */
	int get_des_get_tail,		/* 0-1 */
	int get_rmt)			/* 0-1 */
{
	uint64_t ptr =
	    POW_OPERATION_BASE_IO_BIT |
	    __BITS64_SET(POW_OPERATION_BASE_MAJOR_DID, 0x0c) |
	    __BITS64_SET(POW_OPERATION_BASE_SUB_DID, 0x03) |
	    __BITS64_SET(POW_IDXPTR_LOAD_QOSGRP, qosgrp) |
	    __BITS64_SET(POW_IDXPTR_LOAD_GET_DES_GET_TAIL, get_des_get_tail) |
	    __BITS64_SET(POW_IDXPTR_LOAD_GET_RMT, get_rmt);

	return octeon_xkphys_read_8(ptr);
}

/* NULL_RD Loads */

static inline uint64_t
octeon_pow_ops_null_rd_load(void)
{
	uint64_t ptr =
	    POW_OPERATION_BASE_IO_BIT |
	    __BITS64_SET(POW_OPERATION_BASE_MAJOR_DID, 0x0c) |
	    __BITS64_SET(POW_OPERATION_BASE_SUB_DID, 0x04);

	return octeon_xkphys_read_8(ptr);
}

/* IOBDMA Operations */

/* ``subdid'' values are inverted between ``get_work_addr'' and ``null_read_id'' */

/* The ``scraddr'' part is index in 8 byte words, not address. */

/* GET_WORK IOBDMAs */

static inline void
octeon_pow_ops_get_work_iobdma(
	int scraddr,			/* 0-2047 */
	int wait)			/* 0-1 */
{
 	/* ``scraddr'' part is index in 64-bit words, not address */
	const int scrindex = scraddr / sizeof(uint64_t);

        uint64_t args =
             __BITS64_SET(POW_IOBDMA_GET_WORK_WAIT, wait);
        uint64_t value =
            __BITS64_SET(POW_IOBDMA_BASE_SCRADDR, scrindex) |
            __BITS64_SET(POW_IOBDMA_BASE_LEN, 0x01) |
            __BITS64_SET(POW_IOBDMA_BASE_MAJOR_DID, 0x0c) |
            __BITS64_SET(POW_IOBDMA_BASE_SUB_DID, 0x00) |
            __BITS64_SET(POW_IOBDMA_BASE_39_0, args);

        octeon_iobdma_write_8(value);
}

/* NULL_RD IOBDMAs */

static inline void
octeon_pow_ops_null_rd_iobdma(
	int scraddr)			/* 0-2047 */
{
 	/* ``scraddr'' part is index in 64-bit words, not address */
	const int scrindex = scraddr / sizeof(uint64_t);

        uint64_t value =
            __BITS64_SET(POW_IOBDMA_BASE_SCRADDR, scrindex) |
            __BITS64_SET(POW_IOBDMA_BASE_LEN, 0x01) |
            __BITS64_SET(POW_IOBDMA_BASE_MAJOR_DID, 0x0c) |
            __BITS64_SET(POW_IOBDMA_BASE_SUB_DID, 0x04) |
            __BITS64_SET(POW_IOBDMA_BASE_39_0, 0);

        octeon_iobdma_write_8(value);
}

/* Store Operations */

static inline void
octeon_pow_store(
	int subdid,			/* 0, 1, 3 */
	uint64_t addr,			/* 0-0x0000.000f.ffff.ffff */
	int no_sched,			/* 0, 1 */
	int index,			/* 0-8191 */
	int op,				/* 0-15 */
	int qos,			/* 0-7 */
	int grp,			/* 0-7 */
	int type,			/* 0-7 */
	uint32_t tag)			/* 0-0xffff.ffff */
{
	/* Physical Address to Store to POW */
	uint64_t ptr =
	    POW_OPERATION_BASE_IO_BIT |
	    __BITS64_SET(POW_OPERATION_BASE_MAJOR_DID, 0x0c) |
	    __BITS64_SET(POW_OPERATION_BASE_SUB_DID, subdid) |
	    __BITS64_SET(POW_PHY_ADDR_STORE_ADDR, addr);

	/* Store Data on Store to POW */
	uint64_t args =
	    __BITS64_SET(POW_STORE_DATA_NO_SCHED, no_sched) |
	    __BITS64_SET(POW_STORE_DATA_INDEX, index) |
	    __BITS64_SET(POW_STORE_DATA_OP, op) |
	    __BITS64_SET(POW_STORE_DATA_QOS, qos) |
	    __BITS64_SET(POW_STORE_DATA_GRP, grp) |
	    __BITS64_SET(POW_STORE_DATA_TYPE, type) |
	    __BITS64_SET(POW_STORE_DATA_TAG, tag);

	octeon_xkphys_write_8(ptr, args);
}

/* SWTAG */

static inline void
octeon_pow_ops_swtag(int type, uint32_t tag)
{
	octeon_pow_store(
		1,			/* subdid == 1 */
		0, 			/* addr (not used for SWTAG) */
		0,			/* no_sched (not used for SWTAG) */
		0,			/* index (not used for SWTAG) */
		POW_TAG_OP_SWTAG,	/* op == SWTAG */
		0,			/* qos (not used for SWTAG) */
		0,			/* grp (not used for SWTAG) */
		type,
		tag);
	/* switch to NULL completes immediately */
}

/* SWTAG_FULL */

static inline void
octeon_pow_ops_swtag_full(paddr_t addr, int grp, int type, uint32_t tag)
{
	octeon_pow_store(
		0,			/* subdid == 0 */
		addr,
		0,			/* no_sched (not used for SWTAG_FULL) */
		0,			/* index (not used for SWTAG_FULL) */
		POW_TAG_OP_SWTAG_FULL,	/* op == SWTAG_FULL */
		0,			/* qos (not used for SWTAG_FULL) */
		grp,
		type,
		tag);
}

/* SWTAG_DESCHED */

static inline void
octeon_pow_ops_swtag_desched(int no_sched, int grp, int type, uint32_t tag)
{
	octeon_pow_store(
		3,			/* subdid == 3 */
		0,			/* addr (not used for SWTAG_DESCHED) */
		no_sched,
		0,			/* index (not used for SWTAG_DESCHED) */
		POW_TAG_OP_SWTAG_DESCHED, /* op == SWTAG_DESCHED */
		0,			/* qos (not used for SWTAG_DESCHED) */
		grp,
		type,
		tag);
}

/* DESCHED */

static inline void
octeon_pow_ops_desched(int no_sched)
{
	octeon_pow_store(
		3,			/* subdid == 3 */
		0,			/* addr (not used for DESCHED) */
		no_sched,
		0,			/* index (not used for DESCHED) */
		POW_TAG_OP_DESCHED,	/* op == DESCHED */
		0,			/* qos (not used for DESCHED) */
		0,			/* grp (not used for DESCHED) */
		0,			/* type (not used for DESCHED) */
		0);			/* tag (not used for DESCHED) */
}

/* ADDWQ */

static inline void
octeon_pow_ops_addwq(paddr_t addr, int qos, int grp, int type, uint32_t tag)
{
	octeon_pow_store(
		1,			/* subdid == 1 */
		addr,
		0,			/* no_sched (not used for ADDWQ) */
		0,			/* index (not used for ADDWQ) */
		POW_TAG_OP_ADDWQ,	/* op == ADDWQ */
		qos,
		grp,
		type,
		tag);
}

/* UPD_WQP_GRP */

static inline void
octeon_pow_ops_upd_wqp_grp(paddr_t addr, int grp)
{
	octeon_pow_store(
		1,			/* subdid == 1 */
		addr,
		0,			/* no_sched (not used for UPD_WQP_GRP) */
		0,			/* index (not used for UPD_WQP_GRP) */
		POW_TAG_OP_UPD_WQP_GRP,	/* op == UPD_WQP_GRP */
		0,			/* qos (not used for UPD_WQP_GRP) */
		grp,
		0,			/* type (not used for UPD_WQP_GRP) */
		0);			/* tag (not used for UPD_WQP_GRP) */
}

/* CLR_NSCHED */

static inline void
octeon_pow_ops_clr_nsched(paddr_t addr, int index)
{
	octeon_pow_store(
		1,			/* subdid == 1 */
		addr,
		0,			/* no_sched (not used for CLR_NSCHED) */
		index,
		POW_TAG_OP_CLR_NSCHED,	/* op == CLR_NSCHED */
		0,			/* qos (not used for CLR_NSCHED) */
		0,			/* grp (not used for CLR_NSCHED) */
		0,			/* type (not used for CLR_NSCHED) */
		0);			/* tag (not used for CLR_NSCHED) */
}

/* NOP */

static inline void
octeon_pow_ops_nop(void)
{
	octeon_pow_store(
		1,			/* subdid == 1 */
		0,			/* addr (not used for NOP) */
		0,			/* no_sched (not used for NOP) */
		0,			/* index (not used for NOP) */
		POW_TAG_OP_NOP,		/* op == NOP */
		0,			/* qos (not used for NOP) */
		0,			/* grp (not used for NOP) */
		0,			/* type (not used for NOP) */
		0);			/* tag (not used for NOP) */
}

/* -------------------------------------------------------------------------- */

/*
 * global functions
 */
static inline void
octeon_pow_work_request_async(uint64_t scraddr, uint64_t wait)
{
        octeon_pow_ops_get_work_iobdma(scraddr, wait);
}

static inline uint64_t *
octeon_pow_work_response_async(uint64_t scraddr)
{
	uint64_t result;

	OCTEON_SYNCIOBDMA;
	result = octeon_cvmseg_read_8(scraddr);

	paddr_t addr = result & POW_IOBDMA_GET_WORK_RESULT_ADDR;

	if (result & POW_IOBDMA_GET_WORK_RESULT_NO_WORK)
	    return NULL;
#ifdef __mips_n32
	KASSERT(addr < MIPS_PHYS_MASK);
	//if (addr < MIPS_PHYS_MASK)
		return (uint64_t *)MIPS_PHYS_TO_KSEG0(addr);
#else
	return (uint64_t *)MIPS_PHYS_TO_XKPHYS_CACHED(addr);
#endif
}

static inline void
octeon_pow_config_int_pc(struct octeon_pow_softc *sc, int unit)
{
	uint64_t wq_int_pc;
	uint64_t pc_thr;
	static uint64_t cpu_clock_hz;

	if (cpu_clock_hz == 0)
		cpu_clock_hz  = curcpu()->ci_cpu_freq;

	/* from SDK */
	pc_thr = (cpu_clock_hz) / (unit * 16 * 256);

	wq_int_pc = pc_thr << POW_WQ_INT_PC_PC_THR_SHIFT;
	_POW_WR8(sc, POW_WQ_INT_PC_OFFSET, wq_int_pc);
}

static inline void
octeon_pow_config_int_pc_rate(struct octeon_pow_softc *sc, int rate)
{
	octeon_pow_config_int_pc(sc, sc->sc_int_pc_base / rate);
}

/* wait until ready */
static inline void
octeon_pow_tag_sw_wait(void)
{
	__asm __volatile (
		"	.set	push		\n"
		"	.set	noreorder	\n"
		"	.set	arch=octeon	\n"
		"1:	rdhwr	$2, $30		\n"
		"	beqz	$2, 1b		\n"
		"	 nop			\n"
		"	.set	pop		\n"
	);
}

#endif /* _OCTEON_POWVAR_H_ */
