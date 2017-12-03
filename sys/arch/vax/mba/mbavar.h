/*	$NetBSD: mbavar.h,v 1.13.38.1 2017/12/03 11:36:48 jdolecek Exp $ */
/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden
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

#include <sys/device.h>
#include <machine/scb.h>

#define MBCR_INIT	1
#define	MBCR_IE		(1<<2)
#define	MBDS_DPR	(1<<8)
#define	MBSR_NED	(1<<18)
#define	MBDT_MOH	(1<<13)
#define	MBDT_TYPE	511
#define MBDT_TAP	(1<<14)

#define	CLOSED		0
#define	WANTOPEN	1
#define	RDLABEL		2
#define	OPEN		3
#define	OPENRAW		4

#define	MAXMBADEV	8	/* Max units per MBA */

/*
 * Devices that have different device drivers.
 */
enum	mb_devices {
	MB_UK,	/* unknown */
	MB_RP,	/* RM/RP disk */
	MB_TU,	/* TM03 based tape, ex. TU45 or TU77 */
	MB_MT	/* TU78 tape */
};

/*
 * Current state of the adapter.
 */
enum    sc_state {
	SC_AUTOCONF,
	SC_ACTIVE,
	SC_IDLE
};

/*
 * Return value after a finished data transfer, from device driver.
 */
enum	xfer_action {
	XFER_RESTART,
	XFER_FINISH
};

/*
 * Info passed do unit device driver during autoconfig.
 */
struct	mba_attach_args {
	int	ma_unit;
        int	ma_type;
	const char	*ma_name;
	enum	mb_devices ma_devtyp;
	bus_space_tag_t ma_iot;
	bus_space_handle_t ma_ioh;
};

/*
 * Common struct used to communicate between the mba device driver
 * and the unit device driver.
 */
struct	mba_device {
	STAILQ_ENTRY(mba_device) md_link; /* linked list of runnable devices */
	void (*md_start)(struct mba_device *);
				/* Start routine to be called by mbastart. */
	int (*md_attn)(struct mba_device *);
				/* Routine to be called after attn intr */
	enum xfer_action (*md_finish)(struct mba_device *, int, int *);
				/* Call after xfer finish */
	void *md_softc;		/* Backpointer to this units softc. */
	struct mba_softc *md_mba;
	struct bufq_state *md_q;	/* queue of I/O requests */
};

struct	mba_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	STAILQ_HEAD(,mba_device) sc_xfers;
	struct evcnt sc_intrcnt;
	enum sc_state sc_state;
	struct mba_device *sc_md[MAXMBADEV];
};

struct  mbaunit {
	int     nr;
	const char    *name;
	enum	mb_devices devtyp;
};

/* Common prototypes */
void	mbaqueue(struct mba_device *);

