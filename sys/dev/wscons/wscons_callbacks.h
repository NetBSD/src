/* $NetBSD: wscons_callbacks.h,v 1.8 1999/01/18 20:03:59 drochner Exp $ */

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
 * Call to the glue, to get the whole process started.
 */
void	wscons_glue_set_callback __P((void));

/*
 * Calls to the display interface from the glue code.
 */
int	wsdisplay_is_console __P((struct device *));
struct device *wsdisplay_kbd __P((struct device *));
void	wsdisplay_set_kbd __P((struct device *, struct device *));

/*
 * Calls to the display interface from the keyboard interface.
 */
void	wsdisplay_kbdinput __P((struct device *v, keysym_t));
int	wsdisplay_switch __P((struct device *, int, int));
enum wsdisplay_resetops {
	WSDISPLAY_RESETEMUL,
	WSDISPLAY_RESETCLOSE
};
void	wsdisplay_reset __P((struct device *, enum wsdisplay_resetops));
void	wsdisplay_kbdholdscreen __P((struct device *v, int));

void	wsdisplay_set_cons_kbd __P((int (*get)(dev_t),
				    void (*poll)(dev_t, int)));

/*
 * Calls to the keyboard interface from the glue code.
 */
int	wskbd_is_console __P((struct device *));
struct device *wskbd_display __P((struct device *));
void	wskbd_set_display __P((struct device *, struct device *));

/*
 * Calls to the keyboard interface from the display interface.
 */
int	wskbd_displayioctl __P((struct device *dev, u_long cmd,
	    caddr_t data, int flag, struct proc *p));
int	wskbd_enable __P((struct device *, int));
