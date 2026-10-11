/*	$NetBSD: isp1763var.h,v 1.1 2026/10/11 16:09:06 rkujawa Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

#ifndef _DEV_IC_ISP1763VAR_H_
#define	_DEV_IC_ISP1763VAR_H_

#include <sys/types.h>

#ifdef _KERNEL
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <sys/pool.h>
#include <sys/queue.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#endif

#include <dev/ic/isp1763reg.h>

/* What a transfer descriptor is built from. */
struct isp1763hc_ptdreq {
	uint16_t	pr_mem;		/* payload, CPU address */
	uint16_t	pr_len;
	uint16_t	pr_mps;
	uint8_t		pr_mult;
	uint8_t		pr_addr;
	uint8_t		pr_ep;
	uint8_t		pr_token;
	uint8_t		pr_eptype;
	uint8_t		pr_hub;
	uint8_t		pr_port;
	uint8_t		pr_se;
	uint8_t		pr_uframe;
	uint8_t		pr_usa;
	uint8_t		pr_uscs;
	bool		pr_split;
	bool		pr_toggle;
	bool		pr_ping;
};

void	isp1763hc_ptd_build(const struct isp1763hc_ptdreq *, uint32_t *);
void	isp1763hc_ptd_encode(const uint32_t *, uint8_t *, int);
void	isp1763hc_ptd_decode(const uint8_t *, uint32_t *, int);
void	isp1763hc_intr_sched(bool, u_int, uint8_t *, uint8_t *);
uint16_t isp1763hc_last_map(uint16_t);

#ifdef _KERNEL

/* A transfer descriptor queue: ATL or INT. */
struct isp1763hc_queue {
	struct isp1763hc_xfer	*q_xfer[ISP1763_NSLOTS];
	TAILQ_HEAD(, isp1763hc_xfer) q_wait;
	bus_size_t		q_regdone;
	bus_size_t		q_regskip;
	bus_size_t		q_reglast;
	uint16_t		q_irq;
	uint16_t		q_ptdbase;
	uint16_t		q_membase;
	uint16_t		q_memsize;
	uint16_t		q_busy;
	uint16_t		q_skip;
};

struct isp1763hc_softc {
	device_t		sc_dev;
	struct usbd_bus		sc_bus;

	kmutex_t		sc_lock;
	kmutex_t		sc_rhlock;
	pool_cache_t		sc_xferpool;
	void			*sc_softint;
	device_t		sc_child;
	struct usbd_xfer	*sc_intrxfer;

	struct isp1763hc_queue	sc_atl;
	struct isp1763hc_queue	sc_int;

	uint16_t		sc_hwmode;	/* bus mode, front end */
	bool			sc_dying;
	bool			sc_isreset;

	/* Chip access, provided by the front end. */
	uint8_t		(*sc_rd1)(struct isp1763hc_softc *, bus_size_t);
	void		(*sc_wr1)(struct isp1763hc_softc *, bus_size_t,
			    uint8_t);
	uint16_t	(*sc_rd2)(struct isp1763hc_softc *, bus_size_t);
	void		(*sc_wr2)(struct isp1763hc_softc *, bus_size_t,
			    uint16_t);
	void		(*sc_memrd)(struct isp1763hc_softc *, uint16_t, void *,
			    size_t);
	void		(*sc_memwr)(struct isp1763hc_softc *, uint16_t,
			    const void *, size_t);
	void		(*sc_intr_enable)(struct isp1763hc_softc *, bool);
};

#define	ISP_READ1(sc, r)	((sc)->sc_rd1((sc), (r)))
#define	ISP_WRITE1(sc, r, v)	((sc)->sc_wr1((sc), (r), (v)))
#define	ISP_READ2(sc, r)	((sc)->sc_rd2((sc), (r)))
#define	ISP_WRITE2(sc, r, v)	((sc)->sc_wr2((sc), (r), (v)))
#define	ISP_MEMREAD(sc, a, p, n)					\
	((sc)->sc_memrd((sc), (a), (p), (n)))
#define	ISP_MEMWRITE(sc, a, p, n)					\
	((sc)->sc_memwr((sc), (a), (p), (n)))
#define	ISP_INTR_ENABLE(sc, on)	((sc)->sc_intr_enable((sc), (on)))

int	isp1763hc_attach(struct isp1763hc_softc *);
int	isp1763hc_detach(struct isp1763hc_softc *, int);
int	isp1763hc_activate(device_t, enum devact);
void	isp1763hc_childdet(device_t, device_t);
void	isp1763hc_shutdown(struct isp1763hc_softc *);
void	isp1763hc_intr(struct isp1763hc_softc *);

#endif /* _KERNEL */

#endif /* !_DEV_IC_ISP1763VAR_H_ */
