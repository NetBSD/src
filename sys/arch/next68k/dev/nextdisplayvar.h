/* $NetBSD: nextdisplayvar.h,v 1.1 1999/01/28 11:46:23 dbj Exp $ */
/*
 * Copyright (c) 1998 Matt DeBergalis
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
 *      This product includes software developed by Matt DeBergalis
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

#include <dev/rcons/raster.h>
#include <dev/wscons/wscons_raster.h>

#include <machine/bus.h>

extern int nextdisplay_cnattach __P((paddr_t));

struct nextdisplay_config;
struct fbcmap;
struct fbcursor;
struct fbcurpos;

struct nextdisplay_config {
	int dc_type; /* WSCONS display type */

	vaddr_t dc_vaddr;		/* memory space virtual base address */
	paddr_t dc_paddr;		/* memory space physical base address */
	psize_t dc_size;		/* size of slot memory */

	vaddr_t dc_videobase;	/* base of flat frame buffer */

	int dc_wid; /* width of frame buffer */
	int dc_ht; /* height of frame buffer */
	int dc_depth; /* depth of frame buffer */
	int dc_rowbytes; /* bytes in fb scan line */

	struct raster dc_raster; /* raster description */
	struct rcons dc_rcons; /* raster blitter control info */

	int dc_blanked; /* currently has video disabled */

	int isconsole;
};

struct nextdisplay_softc {
	struct device sc_dev;

	struct nextdisplay_config *sc_dc;
				
	int nscreens;
};
