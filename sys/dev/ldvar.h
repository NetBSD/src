/*	$NetBSD: ldvar.h,v 1.3 2001/01/03 21:01:28 ad Exp $	*/

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

#ifndef	_DEV_LDVAR_H_
#define	_DEV_LDVAR_H_

struct ld_softc {
	struct	device sc_dv;
	struct	disk sc_dk;
	struct	buf_queue sc_bufq;
#if NRND > 0
	rndsource_element_t	sc_rnd_source;
#endif
	int	sc_queuecnt;

	/*
	 * The following are filled by hardware specific attachment code.
	 */
	int	sc_flags;			/* control flags */
	int	sc_secperunit;			/* # sectors in total */
	int	sc_ncylinders;			/* # cylinders */
	int	sc_nheads;			/* # heads */
	int	sc_nsectors;			/* # sectors per track */
	int	sc_secsize;			/* sector size in bytes */
	int	sc_maxxfer;			/* max xfer size in bytes */
	int	sc_maxqueuecnt;			/* maximum h/w queue count */

	int	(*sc_dump)(struct ld_softc *, void *, int, int);
	int	(*sc_flush)(struct ld_softc *);
	int	(*sc_start)(struct ld_softc *, struct buf *);
};

#define	LDF_ENABLED	0x01		/* device enabled */
#define	LDF_LKHELD	0x02		/* lock held */
#define	LDF_LKWANTED	0x04		/* lock wanted */
#define	LDF_WLABEL	0x08		/* label is writable */
#define	LDF_LABELLING	0x10		/* writing label */
#define	LDF_DRAIN	0x20		/* detach pending */

void	ldattach(struct ld_softc *);
void	lddetach(struct ld_softc *);
void	lddone(struct ld_softc *, struct buf *);
int	lddrain(struct ld_softc *, int);

#endif	/* !_DEV_LDVAR_H_ */
