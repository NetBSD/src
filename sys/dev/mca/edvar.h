/*	$NetBSD: edvar.h,v 1.11.60.1 2008/01/08 22:11:07 bouyer Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jaromir Dolecek.
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

#include <sys/simplelock.h>

struct edc_mca_softc;

struct ed_softc {
	struct device sc_dev;

	/* General disk infos */
	struct disk sc_dk;
	struct bufq_state *sc_q;
	struct simplelock sc_q_lock;

	struct edc_mca_softc *edc_softc;   /* pointer to our controller */

	int sc_flags;
#define WDF_WLABEL	0x001 /* label is writable */
#define WDF_LABELLING   0x002 /* writing label */
#define WDF_LOADED	0x004 /* parameters loaded */
#define WDF_KLABEL	0x008 /* retain label after 'full' close */
#define EDF_INIT	0x100 /* disk initialized */

	/* actual drive parameters */
	int sc_capacity;
	int sc_devno;
	u_int16_t sense_data[4];	/* sensed drive parameters */
	u_int16_t cyl;
	u_int8_t heads;
	u_int8_t sectors;
	u_int8_t spares;	/* spares per cylinder */
	u_int32_t rba;		/* # of RBAs */

#if NRND > 0
	rndsource_element_t	rnd_source;
#endif
};
