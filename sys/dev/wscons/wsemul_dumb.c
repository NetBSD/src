/* $NetBSD: wsemul_dumb.c,v 1.1 1998/03/22 14:24:03 drochner Exp $ */

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

static const char _copyright[] __attribute__ ((unused)) =
    "Copyright (c) 1996, 1997 Christopher G. Demetriou.  All rights reserved.";
static const char _rcsid[] __attribute__ ((unused)) =
    "$NetBSD: wsemul_dumb.c,v 1.1 1998/03/22 14:24:03 drochner Exp $";

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/malloc.h>
#include <sys/fcntl.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsemulvar.h>
#include <dev/wscons/ascii.h>

void	*wsemul_dumb_cnattach __P((const struct wsscreen_descr *, void *,
				   int, int));
void	*wsemul_dumb_attach __P((int console, const struct wsscreen_descr *,
				 void *, int, int, void *));
void	wsemul_dumb_output __P((void *cookie, const u_char *data, u_int count));
void	wsemul_dumb_detach __P((void *cookie, u_int *crowp, u_int *ccolp));

const struct wsemul_ops wsemul_dumb_ops = {
	"dumb",
	wsemul_dumb_cnattach,
	wsemul_dumb_attach,
	wsemul_dumb_output,
	wsemul_dumb_detach,
};

struct wsemul_dumb_emuldata {
	const struct wsdisplay_emulops *emulops;
	void *emulcookie;
	void *cbcookie;
	u_int nrows, ncols, crow, ccol;
};

struct wsemul_dumb_emuldata wsemul_dumb_console_emuldata;

void *
wsemul_dumb_cnattach(type, cookie, ccol, crow)
	const struct wsscreen_descr *type;
	void *cookie;
	int ccol, crow;
{
	struct wsemul_dumb_emuldata *edp;

	edp = &wsemul_dumb_console_emuldata;

	edp->emulops = type->textops;
	edp->emulcookie = cookie;
	edp->nrows = type->nrows;
	edp->ncols = type->ncols;
	edp->crow = crow;
	edp->ccol = ccol;
	edp->cbcookie = NULL;

	return (edp);
}

void *
wsemul_dumb_attach(console, type, cookie, ccol, crow, cbcookie)
	int console;
	const struct wsscreen_descr *type;
	void *cookie;
	int ccol, crow;
	void *cbcookie;
{
	struct wsemul_dumb_emuldata *edp;

	if (console)
		edp = &wsemul_dumb_console_emuldata;
	else {
		edp = malloc(sizeof *edp, M_DEVBUF, M_WAITOK);

		edp->emulops = type->textops;
		edp->emulcookie = cookie;
		edp->nrows = type->nrows;
		edp->ncols = type->ncols;
		edp->crow = crow;
		edp->ccol = ccol;
	}

	edp->cbcookie = cbcookie;

	return (edp);
}

void
wsemul_dumb_output(cookie, data, count)
	void *cookie;
	const u_char *data;
	u_int count;
{
	struct wsemul_dumb_emuldata *edp = cookie;
	u_char c;
	int n;

	/* XXX */
	(*edp->emulops->cursor)(edp->emulcookie, 0, edp->crow, edp->ccol);
	while (count-- > 0) {
		c = *data++;
		switch (c) {
		case ASCII_BEL:
			wsdisplay_emulbell(edp->cbcookie);
			break;

		case ASCII_BS:
			if (edp->ccol > 0)
				edp->ccol--;
			break;

		case ASCII_CR:
			edp->ccol = 0;
			break;

		case ASCII_HT:
			n = min(8 - (edp->ccol & 7),
			    edp->ncols - edp->ccol - 1);
			(*edp->emulops->erasecols)(edp->emulcookie,
			    edp->crow, edp->ccol, n);
			edp->ccol += n;
			break;

		case ASCII_NP:
			(*edp->emulops->eraserows)(edp->emulcookie, 0,
			    edp->nrows);
			edp->ccol = 0;
			edp->crow = 0;
			break;

		case ASCII_VT:
			if (edp->crow > 0)
				edp->crow--;
			break;

		default:
			(*edp->emulops->putstr)(edp->emulcookie, edp->crow,
			    edp->ccol, &c, 1);
			edp->ccol++;

			/* if cur col is still on cur line, done. */
			if (edp->ccol < edp->ncols)
				break;

			/* wrap the column around. */
			edp->ccol = 0;

                	/* FALLTHRU */

		case ASCII_LF:
	                /* if the cur line isn't the last, incr and leave. */
			if (edp->crow < edp->nrows - 1) {
				edp->crow++;
				break;
			}
			n = 1;		/* number of lines to scroll */
			(*edp->emulops->copyrows)(edp->emulcookie, n, 0,
			    edp->nrows - n);
			(*edp->emulops->eraserows)(edp->emulcookie,
			    edp->nrows - n, n);
			edp->crow -= n - 1;
			break;
		}	
	}
	/* XXX */
	(*edp->emulops->cursor)(edp->emulcookie, 1, edp->crow, edp->ccol);
}

void
wsemul_dumb_detach(cookie, crowp, ccolp)
	void *cookie;
	u_int *crowp, *ccolp;
{
	struct wsemul_dumb_emuldata *edp = cookie;

	*crowp = edp->crow;
	*ccolp = edp->ccol;
	if (edp != &wsemul_dumb_console_emuldata)
		free(edp, M_DEVBUF);
}
