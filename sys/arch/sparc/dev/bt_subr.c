/*	$NetBSD: bt_subr.c,v 1.10 2000/04/04 21:47:17 pk Exp $ */

/*
 * Copyright (c) 1993
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
 *	@(#)bt_subr.c	8.2 (Berkeley) 1/21/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/malloc.h>

#include <vm/vm.h>

#include <uvm/uvm_extern.h>

#include <machine/fbio.h>

#include <sparc/dev/btreg.h>
#include <sparc/dev/btvar.h>

/*
 * Common code for dealing with Brooktree video DACs.
 * (Contains some software-only code as well, since the colormap
 * ioctls are shared between the cgthree and cgsix drivers.)
 */

/*
 * Implement an FBIOGETCMAP-like ioctl.
 */
int
bt_getcmap(p, cm, cmsize, uspace)
	struct fbcmap *p;
	union bt_cmap *cm;
	int cmsize;
	int uspace;
{
	u_int i, start, count;
	int error = 0;
	u_char *cp, *r, *g, *b;
	u_char *cbuf = NULL;

	start = p->index;
	count = p->count;
	if (start >= cmsize || start + count > cmsize)
		return (EINVAL);

	if (uspace) {
		/* Check user buffers for appropriate access */
		if (!uvm_useracc(p->red, count, B_WRITE) ||
		    !uvm_useracc(p->green, count, B_WRITE) ||
		    !uvm_useracc(p->blue, count, B_WRITE))
			return (EFAULT);

		/* Allocate temporary buffer for color values */
		cbuf = malloc(3*count*sizeof(char), M_TEMP, M_WAITOK);
		r = cbuf;
		g = r + count;
		b = g + count;
	} else {
		/* Direct access in kernel space */
		r = p->red;
		g = p->green;
		b = p->blue;
	}

	/* Copy colors from BT map to fbcmap */
	for (cp = &cm->cm_map[start][0], i = 0; i < count; cp += 3, i++) {
		r[i] = cp[0];
		g[i] = cp[1];
		b[i] = cp[2];
	}

	if (uspace) {
		error = copyout(r, p->red, count);
		if (error)
			goto out;
		error = copyout(g, p->green, count);
		if (error)
			goto out;
		error = copyout(b, p->blue, count);
		if (error)
			goto out;
	}

out:
	if (cbuf != NULL)
		free(cbuf, M_TEMP);

	return (error);
}

/*
 * Implement the software portion of an FBIOPUTCMAP-like ioctl.
 */
int
bt_putcmap(p, cm, cmsize, uspace)
	struct fbcmap *p;
	union bt_cmap *cm;
	int cmsize;
	int uspace;
{
	u_int i, start, count;
	int error = 0;
	u_char *cp, *r, *g, *b;
	u_char *cbuf = NULL;

	start = p->index;
	count = p->count;
	if (start >= cmsize || start + count > cmsize)
		return (EINVAL);

	if (uspace) {
		/* Check user buffers for appropriate access */
		if (!uvm_useracc(p->red, count, B_READ) ||
		    !uvm_useracc(p->green, count, B_READ) ||
		    !uvm_useracc(p->blue, count, B_READ))
			return (EFAULT);

		/* Allocate temporary buffer for color values */
		cbuf = malloc(3*count*sizeof(char), M_TEMP, M_WAITOK);
		r = cbuf;
		g = r + count;
		b = g + count;
		error = copyin(p->red, r, count);
		if (error)
			goto out;
		error = copyin(p->green, g, count);
		if (error)
			goto out;
		error = copyin(p->blue, b, count);
		if (error)
			goto out;
	} else {
		/* Direct access in kernel space */
		r = p->red;
		g = p->green;
		b = p->blue;
	}

	/* Copy colors from fbcmap to BT map */
	for (cp = &cm->cm_map[start][0], i = 0; i < count; cp += 3, i++) {
		cp[0] = r[i];
		cp[1] = g[i];
		cp[2] = b[i];
	}

out:
	if (cbuf != NULL)
		free(cbuf, M_TEMP);

	return (error);
}

/*
 * Initialize the color map to the default state:
 *
 *	- 0 is black
 *	- all other entries are full white
 */
void
bt_initcmap(cm, cmsize)
	union bt_cmap *cm;
	int cmsize;
{
	int i;
	u_char *cp;

	cp = &cm->cm_map[0][0];
	cp[0] = cp[1] = cp[2] = 0;

	for (i = 1, cp = &cm->cm_map[i][0]; i < cmsize; cp += 3, i++)
		cp[0] = cp[1] = cp[2] = 0xff;
}
