/* $NetBSD: wskbdvar.h,v 1.3 1998/04/09 13:09:47 hannken Exp $ */

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
 * Attachment information provided by wskbddev devices when attaching
 * wskbd units or console input keyboards.
 *
 * There is a "void *" cookie provided by the keyboard driver associated
 * with these functions, which is passed to them when they are invoked.
 */
struct wskbddev_attach_args {
	int	console;				/* is it console? */
	int	layout;					/* initial layout */
	int	num_keydescs;				/* number of maps */
	const struct wscons_keydesc *keydesc;		/* array of maps */

							/* console only */
	void	(*getc) __P((void *, u_int *, int *));
	void	(*pollc) __P((void *, int));

							/* wskbd_attach only */
	void	(*set_leds) __P((void *, int));
	int	(*ioctl) __P((void *v, u_long cmd, caddr_t data, int flag,
		    struct proc *p));

	void	*accesscookie;				/* access cookie */
};

#define	wskbddevcf_console		cf_loc[0]	/* spec'd as console? */
#define	WSKBDDEVCF_CONSOLE_UNK		-1

/*
 * Autoconfiguration helper functions.
 */
void	wskbd_cnattach __P((const struct wskbddev_attach_args *));
int	wskbddevprint __P((void *, const char *));

/*
 * Callbacks from the keyboard driver to the wskbd interface driver.
 */
void	wskbd_input __P((struct device *kbddev, u_int type, int value));

/*
 * Console interface.
 */
int	wskbd_cngetc __P((dev_t dev));
void	wskbd_cnpollc __P((dev_t dev, int poll));
