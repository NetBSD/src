/*	$NetBSD: iopvar.h,v 1.2.2.2 2000/11/22 17:34:20 bouyer Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _I2O_IOPVAR_H_
#define	_I2O_IOPVAR_H_

/* Per-IOP statistics; not particularly useful at the moment. */
struct iop_stat {
	int	is_cur_swqueue;
	int	is_peak_swqueue;
	int	is_cur_hwqueue;
	int	is_peak_hwqueue;
	int64_t	is_requests;
	int64_t	is_bytes;
};

#ifdef _KERNEL

#include "locators.h"

/* XXX. Tunables. */
#define	IOP_MAX_INITIATORS	32
#define	IOP_MAX_PI_QUEUECNT	128
#define	IOP_MAX_HW_QUEUECNT	256
#define	IOP_MAX_HW_REPLYCNT	128

/*
 * XXX The following should be adjusted if and when:
 *
 * o MAXPHYS exceeds 64kB. 
 * o A new message or reply is defined in i2o.h. 
 *
 * As it stands, these make for a message frame of 200 bytes.
 */
#define	IOP_MAX_SGL_SIZE	132	/* Maximum S/G list size */
#define	IOP_MAX_BASE_MSG_SIZE	68	/* Maximum base message size */
#define	IOP_MAX_XFER		65536	/* Maximum transfer size */
#define	IOP_MAX_MSG_XFERS	3	/* Maximum transfer count per msg */

#define	IOP_MAX_SGL_ENTRIES	(IOP_MAX_SGL_SIZE / 8)
#define	IOP_MAX_MSG_SIZE	(IOP_MAX_BASE_MSG_SIZE + IOP_MAX_SGL_SIZE)

/*
 * Transfer descriptor.
 */
struct iop_xfer {
	bus_dmamap_t	ix_map;
	u_int		ix_size;
	u_int		ix_flags;
};
#define	IX_IN			0x0001	/* Data transfer from IOP */
#define	IX_OUT			0x0002	/* Data transfer to IOP */

/*
 * Message wrapper.
 */
struct iop_msg {
	SIMPLEQ_ENTRY(iop_msg)	im_queue;	/* Next queued message */
	TAILQ_ENTRY(iop_msg)	im_hash;	/* Hash chain */
	u_int			im_flags;	/* Control flags */
	u_int			im_tctx;	/* Transaction context */
	u_int			im_tid;		/* TID we're addressing */
	void			*im_dvcontext;	/* Un*x device context */
	u_int32_t		im_msg[IOP_MAX_MSG_SIZE / sizeof(u_int32_t)];
	struct iop_xfer		im_xfer[IOP_MAX_MSG_XFERS];
};

#define	IM_SYSMASK		0x00ff
#define	IM_REPLIED		0x0001	/* Message has been replied to */
#define	IM_ALLOCED		0x0002	/* This message wrapper is allocated */
#define	IM_WAITING		0x0004	/* Somebody is waiting for reply */
#define	IM_SGLOFFADJ		0x0008	/* S/G list offset adjusted */
#define	IM_DISCARD		0x0010	/* Discard message wrapper once sent */

#define	IM_USERMASK		0xff00
#define	IM_NOWAIT		0x0100	/* Don't sleep when processing */
#define	IM_NOICTX		0x0200	/* No initiator context field */
#define	IM_NOINTR		0x0400	/* Don't interrupt when complete */

struct iop_initiator {
	struct	device *ii_dv;
	void	(*ii_intr)(struct device *, struct iop_msg *, void *);
	int	ii_flags;
	int	ii_ictx;
};
#define	II_DISCARD	0x0001	/* Don't track state; discard msg wrappers */

#define	IOP_ICTX	0

/*
 * Per-IOP context.
 */
struct iop_softc {
	struct device	sc_dv;		/* generic device data */
	bus_space_handle_t sc_ioh;	/* bus space handle */
	bus_space_tag_t	sc_iot;		/* bus space tag */
	bus_dma_tag_t	sc_dmat;	/* bus DMA tag */
	void	 	*sc_ih;		/* interrupt handler cookie */

	struct i2o_hrt	*sc_hrt;	/* hardware resource table */
	struct i2o_lct	*sc_lct;	/* logical configuration table */
	int		sc_nlctent;	/* number of LCT entries */
	u_int8_t	*sc_lctmap;	/* lct map (per-entry flags) */
	struct i2o_status sc_status;	/* status */
	struct iop_stat	sc_stat;	/* counters */
	int		sc_flags;	/* IOP-wide flags */
	int		sc_maxreplycnt;	/* reply queue size */

	SIMPLEQ_HEAD(,iop_msg) sc_queue;/* software queue */
	int		sc_maxqueuecnt;	/* maximum # of msgs on h/w queue */

	/*
	 * Reply queue.
	 */
	bus_dmamap_t	sc_rep_dmamap;
	int		sc_rep_size;
	bus_addr_t	sc_rep_phys;
	caddr_t		sc_rep;

	/*
	 * Initiator table.
	 */
	struct iop_initiator *sc_itab[IOP_MAX_INITIATORS];

	/*
	 * The following are used only when posting the system table to
	 * the IOP.  They describe PCI memory and I/O port regions.
	 */
	bus_addr_t	sc_memaddr;
	int		sc_memsize;
	int		sc_ioaddr;
	int		sc_iosize;
};

#define	IOP_OPEN		0x01	/* Device interface open */
#define	IOP_LCTLKHELD		0x02	/* sc_lct lock held */
#define	IOP_LCTLKWANTED		0x04	/* sc_lct lock wanted */
#define	IOP_DRAIN		0x08	/* Draining IOP */

#define	IOP_LCTMAP_INUSE	0x01	/* target is in use */

struct iop_attach_args {
	int	ia_class;		/* device class */
	int	ia_tid;			/* target ID */
};
#define	iopcf_tid	cf_loc[IOPCF_TID]		/* TID */

int	iop_init(struct iop_softc *, const char *);
int	iop_intr(void *);
int	iop_params_get(struct iop_softc *, int, int, void *, int);
int	iop_simple_cmd(struct iop_softc *, int, int, int);
void	iop_strvis(const char *, int, char *, int);

int	iop_initiator_register(struct iop_softc *, struct iop_initiator *);
void	iop_initiator_unregister(struct iop_softc *, struct iop_initiator *);

int	iop_lct_get(struct iop_softc *);
int	iop_lct_lock(struct iop_softc *);
void	iop_lct_unlock(struct iop_softc *);

int	iop_msg_alloc(struct iop_softc *, struct iop_initiator *,
		      struct iop_msg **, int);
int	iop_msg_enqueue(struct iop_softc *, struct iop_msg *);
void	iop_msg_free(struct iop_softc *, struct iop_initiator *,
		     struct iop_msg *);
int	iop_msg_map(struct iop_softc *, struct iop_msg *, void *, int, int);
int	iop_msg_send(struct iop_softc *, struct iop_msg *, int);
void	iop_msg_unmap(struct iop_softc *, struct iop_msg *);
int	iop_msg_wait(struct iop_softc *, struct iop_msg *, int);

int	iop_tid_claim(struct iop_softc *, int, int, int);
int	iop_tid_inuse(struct iop_softc *, int);
int	iop_tid_lct_index(struct iop_softc *, int);
void	iop_tid_markallused(struct iop_softc *, int);

#endif	/* _KERNEL */

#endif	/* !_I2O_IOPVAR_H_ */
