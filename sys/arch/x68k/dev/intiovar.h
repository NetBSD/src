/*	$NetBSD: intiovar.h,v 1.8 2007/05/12 06:31:19 isaki Exp $	*/

/*
 *
 * Copyright (c) 1998 NetBSD Foundation, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * NetBSD/x68k internal I/O virtual bus.
 */

#ifndef _INTIOVAR_H_
#define _INTIOVAR_H_

#include <machine/frame.h>
#include <sys/malloc.h>
#include <sys/extent.h>
#include "locators.h"

#define cf_addr		cf_loc[INTIOCF_ADDR]
#define cf_intr		cf_loc[INTIOCF_INTR]
#define cf_dma		cf_loc[INTIOCF_DMA]
#define cf_dmaintr	cf_loc[INTIOCF_DMAINTR]


struct intio_attach_args {
	bus_space_tag_t	ia_bst;	/* bus_space tag */
	bus_dma_tag_t	ia_dmat; /* bus_dma tag */

	const char	*ia_name; /* device name */
	int		ia_addr; /* addr */
	int		ia_size;
	int		ia_intr; /* interrupt vector */
	int		ia_dma;	/* dma channel */
	int		ia_dmaintr; /* interrupt vector for dmac */
};

struct intio_softc {
	struct device	sc_dev;
	bus_space_tag_t	sc_bst;
	bus_dma_tag_t	sc_dmat;
	struct extent	*sc_map;
	struct device	*sc_dmac;
};

enum intio_map_flag {
	INTIO_MAP_ALLOCATE = 0,
	INTIO_MAP_TESTONLY = 1
};
int intio_map_allocate_region(struct device *, struct intio_attach_args *,
	enum intio_map_flag);
int intio_map_free_region(struct device *, struct intio_attach_args *);

typedef int (*intio_intr_handler_t)(void *);

int intio_intr_establish(int, const char *, intio_intr_handler_t, void *);
int intio_intr_disestablish(int, void *);
int intio_intr(struct frame *);


#define PHYS_INTIODEV 0x00c00000

extern u_int8_t *intiobase;

#define INTIO_ADDR(a)	((volatile u_int8_t *) (((u_int32_t) (a)) - (PHYS_INTIODEV) + intiobase))

#define INTIO_SYSPORT		(0x00e8e000)
#define intio_sysport		INTIO_ADDR(INTIO_SYSPORT)
#define sysport_contrast	1
#define sysport_tvctrl		3
#define sysport_imageunit	5
#define sysport_keyctrl		7
#define sysport_waitctrl	9
#define sysport_mpustat		11
#define sysport_sramwp		13
#define sysport_powoff		15

#define intio_set_sysport_contrast(a) \
	intio_sysport[sysport_contrast] = (a) /* 0-15 */
#define intio_set_sysport_tvctrl(a) \
	intio_sysport[sysport_tvctrl] = (a)
#define INTIO_SYSPORT_TVCTRL	0x08
#define intio_set_sysport_imageunit(a) \
	intio_sysport[sysport_imageunit] = (a)
#define intio_set_sysport_keyctrl(a) \
	intio_sysport[sysport_keyctrl] = (a)
#define INTIO_SYSPORT_KBENABLE	0x08
#define intio_set_sysport_waitctrl(a) \
	intio_sysport[sysport_waitctrl] = (a) /* X68030 only */
#define intio_set_sysport_sramwp(a) \
	intio_sysport[sysport_sramwp] = (a)
#define INTIO_SYSPORT_SRAMWP	0x31
#define intio_set_sysport_powoff(a) \
	intio_sysport[sysport_powoff] = (a)

#define intio_get_sysport_contrast() \
	(intio_sysport[sysport_contrast])
#define intio_get_sysport_tvctrl() \
	(intio_sysport[sysport_tvctrl])
#define INTIO_SYSPORT_TVSTAT	0x08
#define intio_get_sysport_keyctrl() \
	(intio_sysport[sysport_keyctrl])
#define INTIO_SYSPORT_KBEXIST	0x08
#define intio_get_sysport_waitctrl() \
	(intio_sysport[sysport_waitctrl])
#define intio_get_sysport_mpustat() \
	(intio_sysport[sysport_mpustat])

/* I/O controller (sicilian/pluto) */
#define INTIO_SICILIAN		(0x00e9c000)
#define intio_sicilian		INTIO_ADDR(INTIO_SICILIAN)
#define sicilian_intr		1
#define sicilian_ivec		3

#define intio_get_sicilian_intr() \
	(intio_sicilian[sicilian_intr])
#define intio_set_sicilian_intr(a) \
	intio_sicilian[sicilian_intr] = (a)
#define SICILIAN_INTR_PAR 0x01
#define SICILIAN_INTR_FDD 0x02
#define SICILIAN_INTR_FDC 0x04
#define SICILIAN_INTR_HDD 0x08
#define SICILIAN_STAT_HDD 0x10
#define SICILIAN_STAT_PAR 0x20
#define SICILIAN_STAT_FDD 0x40
#define SICILIAN_STAT_FDC 0x80

#define intio_enable_intr(a) \
	intio_sicilian[sicilian_intr] = ((a) | intio_sicilian[sicilian_intr])
#define intio_disable_intr(a) \
	intio_sicilian[sicilian_intr] = (~(a) & intio_sicilian[sicilian_intr])

#define intio_set_sicilian_ivec(a) \
	intio_sicilian[sicilian_ivec] = (a)
void intio_set_ivec(int);

struct intio_dma_cookie {
	int	id_flags;		/* flags; see below */

	/*
	 * Information about the original buffer used during
	 * DMA map syncs.  Note that origibuflen is only used
	 * for ID_BUFTYPE_LINEAR.
	 */
	void	*id_origbuf;		/* pointer to orig buffer if
					   bouncing */
	bus_size_t id_origbuflen;	/* ...and size */
	int	id_buftype;		/* type of buffer */

	void	*id_bouncebuf;		/* pointer to the bounce buffer */
	bus_size_t id_bouncebuflen;	/* ...and size */
	int	id_nbouncesegs;		/* number of valid bounce segs */
	bus_dma_segment_t id_bouncesegs[0]; /* array of bounce buffer
					       physical memory segments */
};

/* id_flags */
#define	ID_MIGHT_NEED_BOUNCE	0x01	/* map could need bounce buffers */
#define	ID_HAS_BOUNCE		0x02	/* map currently has bounce buffers */
#define	ID_IS_BOUNCING		0x04	/* map is bouncing current xfer */

/* id_buftype */
#define	ID_BUFTYPE_INVALID	0
#define	ID_BUFTYPE_LINEAR	1
#define	ID_BUFTYPE_MBUF		2
#define	ID_BUFTYPE_UIO		3
#define	ID_BUFTYPE_RAW		4

#endif
