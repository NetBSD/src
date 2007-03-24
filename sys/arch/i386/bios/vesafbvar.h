/* $NetBSD: vesafbvar.h,v 1.3.18.1 2007/03/24 14:54:43 yamt Exp $ */

/*-
 * Copyright (c) 2006 Jared D. McNeill <jmcneill@invisible.ca>
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

#ifndef _ARCH_I386_BIOS_VESAFBVAR_H
#define _ARCH_I386_BIOS_VESAFBVAR_H

#include "opt_splash.h"

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>
#include <dev/wsfont/wsfont.h>
#include <dev/cons.h>

#include <arch/i386/bios/vesabiosreg.h>

#ifdef SPLASHSCREEN
#define	VESAFB_DISABLE_TEXT
#include <dev/splash/splash.h>
/* XXX */
extern const char _splash_header_data_cmap[64+32][3];
#endif

/* Safe defaults */
#ifndef VESAFB_WIDTH
#define VESAFB_WIDTH	640
#endif
#ifndef VESAFB_HEIGHT
#define	VESAFB_HEIGHT	480
#endif
#ifndef VESAFB_DEPTH
#define	VESAFB_DEPTH	8
#endif

#define	VESAFB_SHADOW_FB

struct vesafb_softc {
	struct device sc_dev;
	int sc_mode;
	int sc_isconsole;
	struct vcons_data sc_vd;
	struct modeinfoblock sc_mi;
#ifdef SPLASHSCREEN
	struct splash_info sc_si;
#endif
#ifdef SPLASHSCREEN_PROGRESS
	struct splash_progress sc_sp;
#endif
#ifdef VESAFB_DISABLE_TEXT
	struct vcons_data sc_vdnull;
#endif
	char *sc_buf;
	u_char *sc_bits;
	u_char *sc_shadowbits;
	u_char *sc_fbstart;
	u_char sc_cmap_red[256];
	u_char sc_cmap_green[256];
	u_char sc_cmap_blue[256];
	int sc_wsmode;
	int sc_nscreens;
	int sc_scrollscreens;
	uint32_t sc_screensize;
	uint32_t sc_fbsize;
	uint32_t sc_fblines;
	/* display wide buffer settings for VTs */
	uint8_t *sc_displ_bits;
	uint8_t *sc_displ_hwbits;
	/* delegate rasops function if hardware scrolling */
	void (*sc_orig_copyrows)(void *, int, int, int);

	int sc_pm;
	uint8_t sc_pmver;
	uint8_t sc_pmstates;
	void *sc_powerhook;
};

int	vesafb_cnattach(void);

#endif /* !_ARCH_I386_BIOS_VESAFBVAR_H */
