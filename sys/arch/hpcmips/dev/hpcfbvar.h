/*	$NetBSD: hpcfbvar.h,v 1.6 2000/12/25 10:09:31 sato Exp $	*/

/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
 * Copyright (c) 2000
 *         SATO Kazumi. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
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
 */

/*
 * video access functions (must be provided by all video).
 */
struct hpcfb_accessops {
	int	(*ioctl) __P((void *, u_long, caddr_t, int, struct proc *));
	paddr_t	(*mmap) __P((void *, off_t, int));
	void	(*cursor) __P((void *, int, int, int, int, int));
	void	(*bitblit) __P((void *, int, int, int, int, int, int));
	void	(*erase) __P((void *, int, int, int, int, int));
	void	(*putchar) __P((void *, int, int, struct wsdisplay_font *,
				int, int, u_int, int));
	void	(*setclut) __P((void *, struct rasops_info *));
	void	(*font) __P((void *, struct wsdisplay_font *));	/* load fonts */
	void	(*iodone) __P((void *));	/* wait i/o done */
};

/*
 * hpcfb attach arguments
 */
struct hpcfb_attach_args {
	int ha_console;
	const struct hpcfb_accessops *ha_accessops;	/* access ops */
	void	*ha_accessctx;	  	       		/* access cookie */

	int ha_curfbconf;
	int ha_nfbconf;
	struct hpcfb_fbconf *ha_fbconflist;
	int ha_curdspconf;
	int ha_ndspconf;
	struct hpcfb_dspconf *ha_dspconflist;
};

int	hpcfb_cnattach __P((struct hpcfb_fbconf *));
int	hpcfbprint __P((void *aux, const char *pnp));
