/*-
 * Copyright (c) 2003, 2004 David Young
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _ATH_COMPAT_H
#define _ATH_COMPAT_H

#ifdef __FreeBSD__
#define ATH_TICKS() (ticks)

typedef struct task ath_task_t;

#define ATH_CALLOUT_INIT(chp) callout_init((chp), CALLOUT_MPSAFE)

#define ATH_TASK_INIT(task, func, context) TASK_INIT(task, 0, func, context)
#define ATH_TASK_RUN_OR_ENQUEUE(_task) taskqueue_enqueue(taskqueue_swi, _task)

#define ath_softc_critsect_decl(_x)		/* empty */
#define ath_softc_critsect_begin(sc, _x)	mtx_lock(&(sc)->sc_mtx)
#define ath_softc_critsect_end(sc, _x)		mtx_unlock(&(sc)->sc_mtx)
#define ath_txbuf_critsect_decl(_x)		/* empty */
#define ath_txbuf_critsect_begin(sc, _x)	mtx_lock(&(sc)->sc_txbuflock)
#define ath_txbuf_critsect_end(sc, _x)		mtx_unlock(&(sc)->sc_txbuflock)
#define ath_txq_critsect_decl(_x)		/* empty */
#define ath_txq_critsect_begin(sc, _x)		mtx_lock(&(sc)->sc_txqlock)
#define ath_txq_critsect_end(sc, _x)		mtx_unlock(&(sc)->sc_txqlock)

static __inline int
ath_buf_dmamap_load_mbuf(bus_dma_tag_t tag, struct ath_buf *bf,
    struct mbuf *m, int flags)
{
	int error;
	error = bus_dmamap_load_mbuf(tag, bf->bf_dmamap, m, ath_mbuf_load_cb,
	    bf, flags);
	if (error == 0 && bf->bf_nseg > ATH_TXDESC)
		return EFBIG;
	return error;
}

#define ath_buf_dmamap_sync(tag, bf, ops) \
	bus_dmamap_sync((tag), (bf)->bf_dmamap, (ops))

#else
#define ATH_TICKS() (hardclock_ticks)

#define BPF_MTAP(ifp, m)	bpf_mtap((ifp)->if_bpf, (m))
#undef KASSERT
#define KASSERT(cond, complaint) if (!(cond)) panic complaint

typedef struct ath_task {
	void	(*t_func)(void*, int);
	void	*t_context;
} ath_task_t;

#define ATH_CALLOUT_INIT(chp) callout_init((chp))

#define ATH_TASK_INIT(task, func, context)	\
	do {					\
		(task)->t_func = (func);	\
		(task)->t_context = (context);	\
	} while (0)

#define ATH_TASK_RUN_OR_ENQUEUE(task) ((*(task)->t_func)((task)->t_context, 1))

#define ath_critsect_decl(_x)		int _x
#define ath_critsect_begin(sc, _x)	do { _x = splnet(); } while (0)
#define ath_critsect_end(sc, _x)	splx(_x)
#define ath_softc_critsect_decl(_x)		ath_critsect_decl(_x)
#define ath_softc_critsect_begin(sc, _x)	ath_critsect_begin(sc, _x)
#define ath_softc_critsect_end(sc, _x)		ath_critsect_end(sc, _x)
#define ath_txbuf_critsect_decl(_x)		ath_critsect_decl(_x)
#define ath_txbuf_critsect_begin(sc, _x)	ath_critsect_begin(sc, _x)
#define ath_txbuf_critsect_end(sc, _x)		ath_critsect_end(sc, _x)
#define ath_txq_critsect_decl(_x)		ath_critsect_decl(_x)
#define ath_txq_critsect_begin(sc, _x)		ath_critsect_begin(sc, _x)
#define ath_txq_critsect_end(sc, _x)		ath_critsect_end(sc, _x)

#define ath_buf_dmamap_load_mbuf(tag, bf, m, flags) \
	bus_dmamap_load_mbuf((tag), (bf)->bf_dmamap, (m), (flags))

#define ath_buf_dmamap_sync(tag, bf, ops) \
	bus_dmamap_sync((tag), (bf)->bf_dmamap, 0, \
	    (bf)->bf_dmamap->dm_mapsize, (ops))

extern void device_printf(struct device, const char *fmt, ...);

#endif

#endif /* _ATH_COMPAT_H */
