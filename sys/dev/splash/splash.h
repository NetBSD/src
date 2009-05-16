/* $NetBSD: splash.h,v 1.1.76.1 2009/05/16 10:41:44 yamt Exp $ */

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

#ifndef _DEV_SPLASH_SPLASH_H
#define _DEV_SPLASH_SPLASH_H

#include "opt_splash.h"

#ifdef SPLASHSCREEN

#define	SPLASH_CMAP_OFFSET	(16 * 3)
#define	SPLASH_CMAP_SIZE	(64 + 32)

struct splash_info {
	int	si_depth;
	u_char	*si_bits, *si_hwbits;
	int	si_width, si_height;
	int	si_stride;

	void	(*si_fillrect)(struct splash_info *, int, int, int, int, int);
};

#ifdef SPLASHSCREEN_PROGRESS
struct splash_progress {
	device_t sp_dev; /* XXX needed for config_finalize */
	int	sp_top;
	int	sp_left;
	int	sp_width;
	int	sp_height;
#define	SPLASH_PROGRESS_NSTATES 5
	int	sp_state;
	int	sp_running;
	int	sp_force;

	struct splash_info *	sp_si;
	struct callout 		sp_callout;
};
#endif /* !SPLASHSCREEN_PROGRESS */

#define	SPLASH_F_NONE	0x00	/* Nothing special */
#define	SPLASH_F_CENTER	0x01	/* Center splash image */
#define	SPLASH_F_FILL	0x02	/* Fill the rest of the screen with
				 * the colour of the top-left pixel
				 * in the splash image
				 */

void	splash_render(struct splash_info *, int);

#ifdef SPLASHSCREEN_PROGRESS
void	splash_progress_init(struct splash_progress *);
void	splash_progress_update(struct splash_progress *);
#endif /* !SPLASHSCREEN_PROGRESS */

#endif /* !SPLASHSCREEN */

#endif /* !_DEV_SPLASH_SPLASH_H */
