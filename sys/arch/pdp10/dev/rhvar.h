/*	$NetBSD: rhvar.h,v 1.1 2003/08/19 10:51:58 ragge Exp $ */
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
#define	MB_RP		1
#define	MB_TU		2
#define	MB_MT		3

/*
 * Current state of the adapter.
 */
#define	SC_AUTOCONF	1
#define	SC_ACTIVE	2
#define	SC_IDLE		3

/*
 * Return value after a finished data transfer, from device driver.
 */
#define	XFER_RESTART	1
#define	XFER_FINISH	2

/*
 * Info passed do unit device driver during autoconfig.
 */
struct	rh_attach_args {
	int	ma_unit;
        int	ma_type;
	int	ma_devtyp;
	bus_space_tag_t ma_iot;
	bus_space_handle_t ma_ioh;
	char	*ma_name;
};

/*
 * Common struct used to communicate between the rh device driver
 * and the unit device driver.
 */
struct	rh_device {
	struct	rh_device *md_back;	/* linked list of runnable devices */
	    /* Start routine to be called by rhstart. */
	void	(*md_start)(struct rh_device *);
	    /* Routine to be called after attn intr */
	int	(*md_attn)(struct rh_device *);
	    /* Call after xfer finish */
	int	(*md_finish)(struct rh_device *, int, int *);
	void	*md_softc;	/* Backpointer to this units softc. */
	struct	rh_softc *md_rh;
	struct	bufq_state md_q;	/* queue of I/O requests */
	int	md_csr;	/* Drive command given to RH20 */
	int	md_da;	/* Disk address given to RH20 */
};

struct	rh_softc {
	struct  device sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	struct	evcnt sc_intrcnt;
	struct	rh_device *sc_first, *sc_last;
	int sc_state;
	struct	rh_device *sc_md[MAXMBADEV];
};

struct  rhunit {
	int     nr;
	char    *name;
	int	devtyp;
};

/* Common prototypes */
void	rhqueue(struct rh_device *);

