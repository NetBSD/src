/*	$NetBSD: wsdisplay_vconsvar.h,v 1.3.6.1 2006/05/24 15:50:32 tron Exp $ */

/*-
 * Copyright (c) 2005, 2006 Michael Lorenz
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

#ifndef _WSDISPLAY_VCONS_H_
#define _WSDISPLAY_VCONS_H_

struct vcons_data;

struct vcons_screen {
	struct rasops_info scr_ri;
	LIST_ENTRY(vcons_screen) next;
	void *scr_cookie;
	struct vcons_data *scr_vd;
	struct vcons_data *scr_origvd;
	const struct wsscreen_descr *scr_type;
	uint16_t *scr_chars;
	long *scr_attrs;
	long scr_defattr;
	/* static flags set by the driver */
	uint32_t scr_flags;
#define VCONS_NO_REDRAW		1	/* don't readraw in switch_screen */
#define	VCONS_SCREEN_IS_STATIC	2	/* don't free() this vcons_screen */
#define VCONS_SWITCH_NEEDS_POLLING 4	/* rasops can overlap so we need to
					 * poll the busy flag when switching
					 * - for drivers that use software
					 * drawing */
#define VCONS_DONT_DRAW		8	/* don't draw on this screen at all */
	/* status flags used by vcons */
	uint32_t scr_status;
#define VCONS_IS_VISIBLE	1	/* this screen is currently visible */
	/* non zero when some rasops operation is in progress */
	int scr_busy;
};

#define SCREEN_IS_VISIBLE(scr) (((scr)->scr_status & VCONS_IS_VISIBLE) != 0)
#define SCREEN_IS_BUSY(scr) ((scr)->scr_busy != 0)
#define SCREEN_CAN_DRAW(scr) (((scr)->scr_flags & VCONS_DONT_DRAW) == 0)
#define SCREEN_BUSY(scr) ((scr)->scr_busy = 1)
#define SCREEN_IDLE(scr) ((scr)->scr_busy = 0)
#define SCREEN_VISIBLE(scr) ((scr)->scr_status |= VCONS_IS_VISIBLE)
#define SCREEN_INVISIBLE(scr) ((scr)->scr_status &= ~VCONS_IS_VISIBLE)
#define SCREEN_DISABLE_DRAWING(scr) ((scr)->scr_flags |= VCONS_DONT_DRAW)
#define SCREEN_ENABLE_DRAWING(scr) ((scr)->scr_flags &= ~VCONS_DONT_DRAW)
struct vcons_data {
	/* usually the drivers softc */
	void *cookie;
	
	/*
	 * setup the rasops part of the passed vcons_screen, like
	 * geometry, framebuffer address, font, characters, acceleration.
	 * we pass the cookie as 1st parameter
	 */
	void (*init_screen)(void *, struct vcons_screen *, int, 
	    long *);

	/* accessops */
	int (*ioctl)(void *, void *, u_long, caddr_t, int, struct lwp *);

	/* rasops */
	void (*copycols)(void *, int, int, int, int);
	void (*erasecols)(void *, int, int, int, long);
	void (*copyrows)(void *, int, int, int);
	void (*eraserows)(void *, int, int, long);
	void (*putchar)(void *, int, int, u_int, long);
	void (*cursor)(void *, int, int, int);
	/* called before cvons_redraw_screen */
	void (*show_screen_cb)(struct vcons_screen *);
	/* virtual screen management stuff */
	void (*switch_cb)(void *, int, int);
	void *switch_cb_arg;
	struct callout switch_callout;
	uint32_t switch_pending;
	LIST_HEAD(, vcons_screen) screens;
	struct vcons_screen *active, *wanted;
	const struct wsscreen_descr *currenttype;
#ifdef DIAGNOSTIC
	int switch_poll_count;
#endif
};

int	vcons_init(struct vcons_data *, void *cookie, struct wsscreen_descr *,
    struct wsdisplay_accessops *);

int	vcons_init_screen(struct vcons_data *, struct vcons_screen *, int,
    long *);

void	vcons_redraw_screen(struct vcons_screen *);



#endif /* _WSDISPLAY_VCONS_H_ */
