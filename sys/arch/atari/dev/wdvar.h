/*	$NetBSD: wdvar.h,v 1.1 1997/04/01 08:18:04 leo Exp $	*/

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *
 *	@(#)wdreg.h	7.1 (Berkeley) 5/9/91
 */

struct wdc_softc {
	struct device sc_dev;
	void *sc_ih;

	bus_space_tag_t sc_iot;		/* Bus space tag */
	bus_space_handle_t sc_ioh;	/* handle for drive registers */
	bus_space_handle_t sc_auxioh;	/* handle for aux register */
	bus_space_handle_t sc_dataioh;	/* handle for 16 bit data */
	bus_space_handle_t sc_data32ioh;/* handle for 32 bit data */

	void	(*sc_inten)(struct wdc_softc *, int);

	int sc_drq;			/* DMA channel */

	TAILQ_HEAD(drivehead, wd_softc) sc_drives;
	int sc_flags;
#define	WDCF_ACTIVE	0x01		/* controller is active */
#define	WDCF_SINGLE	0x02		/* sector at a time mode */
#define	WDCF_ERROR	0x04		/* processing a disk error */
#define	WDCF_WANTED	0x08		/* XXX locking for wd_get_parms() */
#define WDCF_QUIET	0x10		/* Be quiet about errors */
#define WDCF_32BIT	0x20		/* Use 32bit xfers */
	int sc_errors;			/* errors during current transfer */
	u_char sc_status;		/* copy of status register */
	u_char sc_error;		/* copy of error register */
};

int wdcprobe_internal __P((bus_space_tag_t iot, bus_space_handle_t ioh,
    bus_space_handle_t aux_ioh, bus_space_handle_t data_ioh,
    bus_space_handle_t data32_ioh, char *name));
void wdcattach_internal __P((struct wdc_softc *wdc, bus_space_tag_t iot, bus_space_handle_t ioh,
    bus_space_handle_t aux_ioh, bus_space_handle_t data_ioh,
    bus_space_handle_t data32_ioh,  int drq));
