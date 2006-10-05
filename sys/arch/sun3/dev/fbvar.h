/*	$NetBSD: fbvar.h,v 1.11 2006/10/05 14:46:11 tsutsui Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)fbvar.h	8.1 (Berkeley) 6/11/93
 */

#include <sys/event.h>		/* for struct knote */

/*
 * Frame buffer variables.  All frame buffer drivers must provide the
 * following in order to participate.
 */

struct fbcmap;

struct fbdevice {
	struct	fbtype fb_fbtype;	/* see fbio.h */
	struct	fbdriver *fb_driver;	/* pointer to driver */
	void *fb_private;		/* for fb driver use */
	char *fb_name;			/* i.e. sc_dev.dx_name */

	caddr_t	fb_pixels;		/* display RAM */
	int	fb_linebytes;		/* bytes per display line */

	int	fb_flags;		/* copy of cf_flags */

	/* This points to the P4 register if the FB has one. */
	volatile uint32_t *fb_pfour;

	/*
	 * XXX - The "Raster console" stuff could be stored
	 * in the driver specific structure at fb_private
	 * if needed.
	 */
};

struct fbdriver {
	/* These avoid the need to know our major number. */
	int 	(*fbd_open)(dev_t, int, int, struct lwp *);
	int 	(*fbd_close)(dev_t, int, int, struct lwp *);
	paddr_t	(*fbd_mmap)(dev_t, off_t, int);
	int	(*fbd_kqfilter)(dev_t, struct knote *);
	/* These are the internal ioctl functions */
	int 	(*fbd_gattr)(struct fbdevice *,  void *);
	int 	(*fbd_gvideo)(struct fbdevice *, void *);
	int 	(*fbd_svideo)(struct fbdevice *, void *);
	int 	(*fbd_getcmap)(struct fbdevice *, void *);
	int 	(*fbd_putcmap)(struct fbdevice *, void *);
};

int 	fbioctlfb(struct fbdevice *, u_long, caddr_t);

void	fb_attach(struct fbdevice *, int);
int 	fb_noioctl(struct fbdevice *, void *);

void	fb_eeprom_setsize (struct fbdevice *);

int 	fb_pfour_id(void *);
int 	fb_pfour_get_video(struct fbdevice *);
void	fb_pfour_set_video(struct fbdevice *, int);

void	fb_pfour_setsize(struct fbdevice *);

/* This comes from enable.c */
void	enable_video(int);
