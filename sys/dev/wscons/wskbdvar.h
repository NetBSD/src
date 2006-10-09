/* $NetBSD: wskbdvar.h,v 1.13 2006/10/09 11:03:43 peter Exp $ */

/*
 * Copyright (c) 1996, 1997 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
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

/*
 * WSKBD interfaces.
 */

/*
 * Keyboard access functions (must be provided by all keyboards).
 *
 * There is a "void *" cookie provided by the keyboard driver associated
 * with these functions, which is passed to them when they are invoked.
 */
struct wskbd_accessops {
	int	(*enable)(void *, int);
	void    (*set_leds)(void *, int);
	int     (*ioctl)(void *, u_long, caddr_t, int, struct lwp *);
};

/*
 * Keyboard console functions (must be provided by console input keyboards).
 *
 * There is a "void *" cookie provided by the keyboard driver associated
 * with these functions, which is passed to them when they are invoked.
 */
struct wskbd_consops {
	void    (*getc)(void *, u_int *, int *);
	void    (*pollc)(void *, int);
	void	(*bell)(void *, u_int, u_int, u_int);
};

/*
 * Attachment information provided by wskbddev devices when attaching
 * wskbd units.
 */
struct wskbddev_attach_args {
	int	console;				/* is it console? */
	const struct wskbd_mapdata *keymap;

	const struct wskbd_accessops *accessops;        /* access ops */
	void	*accesscookie;				/* access cookie */
};

#include "locators.h"

#define	wskbddevcf_console		cf_loc[WSKBDDEVCF_CONSOLE]	/* spec'd as console? */
#define	WSKBDDEVCF_CONSOLE_UNK		(WSKBDDEVCF_CONSOLE_DEFAULT)

#define	wskbddevcf_mux		cf_loc[WSKBDDEVCF_MUX]

/*
 * Autoconfiguration helper functions.
 */
void	wskbd_cnattach(const struct wskbd_consops *, void *,
			    const struct wskbd_mapdata *);
void	wskbd_cndetach(void);
int	wskbddevprint(void *, const char *);

/*
 * Callbacks from the keyboard driver to the wskbd interface driver.
 */
void	wskbd_input(struct device *, u_int, int);
/* for WSDISPLAY_COMPAT_RAWKBD */
void	wskbd_rawinput(struct device *, u_char *, int);

/*
 * Console interface.
 */
int	wskbd_cngetc(dev_t);
void	wskbd_cnpollc(dev_t, int);
void	wskbd_cnbell(dev_t, u_int, u_int, u_int);
