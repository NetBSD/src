/*	$NetBSD: smg.c,v 1.1 1998/06/04 15:51:12 ragge Exp $ */
/*
 * Copyright (c) 1998 Ludd, University of Lule}, Sweden.
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
 *      This product includes software developed at Ludd, University of 
 *      Lule}, Sweden and its contributors.
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




#include <sys/types.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/malloc.h>
#include <sys/conf.h>

#include <dev/cons.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wscons_callbacks.h>

#include <machine/vsbus.h>
#include <machine/sid.h>

#include "lkc.h"

#define	SM_COLS		128	/* char width of screen */
#define	SM_ROWS		57	/* rows of char on screen */
#define	SM_CHEIGHT	15	/* lines a char consists of */
#define	SM_NEXTROW	(SM_COLS * SM_CHEIGHT)

static	int smg_match __P((struct device *, struct cfdata *, void *));
static	void smg_attach __P((struct device *, struct device *, void *));

struct	smg_softc {
	struct	device ss_dev;
};

struct cfattach smg_ca = {
	sizeof(struct smg_softc), smg_match, smg_attach,
};

static void	smg_cursor __P((void *, int, int, int));
static void	smg_putstr __P((void *, int, int, char *, int, long));
static void	smg_copycols __P((void *, int, int, int,int));
static void	smg_erasecols __P((void *, int, int, int, long));
static void	smg_copyrows __P((void *, int, int, int));
static void	smg_eraserows __P((void *, int, int, long));
static int	smg_alloc_attr __P((void *, int, int, int, long *));

const struct wsdisplay_emulops smg_emulops = {
	smg_cursor,
	smg_putstr,
	smg_copycols,
	smg_erasecols,
	smg_copyrows,
	smg_eraserows,
	smg_alloc_attr
};

const struct wsscreen_descr smg_stdscreen = {
        "128x57", 128, 57,
        &smg_emulops,
        8, 15,
        0, /* WSSCREEN_UNDERLINE??? */
};

const struct wsscreen_descr *_smg_scrlist[] = {
	&smg_stdscreen,
};

const struct wsscreen_list smg_screenlist = {
	sizeof(_smg_scrlist) / sizeof(struct wsscreen_descr *),
	_smg_scrlist,
};

static int	smg_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
static int	smg_mmap __P((void *, off_t, int));
static int	smg_alloc_screen __P((void *, const struct wsscreen_descr *,
                                      void **, int *, int *, long *));
static void	smg_free_screen __P((void *, void *));
static void	smg_show_screen __P((void *, void *));
static int	smg_load_font __P((void *, void *, int, int, int, void *));

const struct wsdisplay_accessops smg_accessops = {
	smg_ioctl,
	smg_mmap,
	smg_alloc_screen,
	smg_free_screen,
	smg_show_screen,
	smg_load_font
};

struct	smg_screen {
	int	ss_curx;
	int	ss_cury;
};

struct	smg_config {
	struct	smg_screen ss_devs[8]; /* XXX */
};

int
smg_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct  vsbus_attach_args *va = aux;

	if (va->va_type != INR_VF)
		return 0;
#ifdef DIAGNOSTIC
	if (sm_addr == 0)
		panic("smg: inconsistency in smg address mapping");
#endif
	return 1;
}

void
smg_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct wsemuldisplaydev_attach_args aa;
	struct smg_config *sc;

	printf("\n");
        sc = malloc(sizeof(struct smg_config), M_DEVBUF, M_WAITOK);
	aa.console = !(vax_confdata & 0x20);
	aa.scrdata = &smg_screenlist;
	aa.accessops = &smg_accessops;
        aa.accesscookie = sc;

	config_found(self, &aa, wsemuldisplaydevprint);
}

void
smg_cursor(id, on, row, col)
	void *id;
	int on, row, col;
{
	sm_addr[(row * 15 * 128) + col + (14 * 128)] = on ? 255 : 0;
}

static void
smg_putstr(id, row, col, cp, len, attr)
	void *id;
	int row, col;
	char *cp;
	int len;
	long attr;
{
	int i, j;
	extern char q_font[];

	for (i = 0; i < len; i++)
		for (j = 0; j < 15; j++)
			sm_addr[col + i + (row * 15 * 128) + j * 128] =
			    q_font[(cp[i] - 32) * 15 + j];
}

/*
 * copies columns inside a row.
 */
static void
smg_copycols(id, row, srccol, dstcol, ncols)
	void *id;
	int row, srccol, dstcol, ncols;
{
	int i;

	for (i = 0; i < SM_CHEIGHT; i++)
		bcopy(&sm_addr[(row * SM_NEXTROW) + srccol + (i * 128)],
		    &sm_addr[(row * SM_NEXTROW) + dstcol + i*128], ncols);
	
}

/*
 * Erases a bunch of chars inside one row.
 */
static void
smg_erasecols(id, row, startcol, ncols, fillattr)
	void *id;
	int row, startcol, ncols;
	long fillattr;
{
	int i;

	for (i = 0; i < SM_CHEIGHT; i++)
		bzero(&sm_addr[(row * SM_NEXTROW) + startcol + i * 128], ncols);
}

static void
smg_copyrows(id, srcrow, dstrow, nrows)
	void *id;
	int srcrow, dstrow, nrows;
{
	int frows;

	if (nrows > 25) {
		frows = nrows >> 1;
		if (srcrow > dstrow) {
			bcopy(&sm_addr[(srcrow * SM_NEXTROW)],
			    &sm_addr[(dstrow * SM_NEXTROW)],
			    frows * SM_NEXTROW);
			bcopy(&sm_addr[((srcrow + frows) * SM_NEXTROW)],
			    &sm_addr[((dstrow + frows) * SM_NEXTROW)],
			    (nrows - frows) * SM_NEXTROW);
		} else {
			bcopy(&sm_addr[((srcrow + frows) * SM_NEXTROW)],
			    &sm_addr[((dstrow + frows) * SM_NEXTROW)],
			    (nrows - frows) * SM_NEXTROW);
			bcopy(&sm_addr[(srcrow * SM_NEXTROW)],
			    &sm_addr[(dstrow * SM_NEXTROW)],
			    frows * SM_NEXTROW);
		}
	} else
		bcopy(&sm_addr[(srcrow * SM_NEXTROW)],
		    &sm_addr[(dstrow * SM_NEXTROW)], nrows * SM_NEXTROW);
}

static void
smg_eraserows(id, startrow, nrows, fillattr)
	void *id;
	int startrow, nrows;
	long fillattr;
{
	int frows;

	if (nrows > 25) {
		frows = nrows >> 1;
		bzero(&sm_addr[(startrow * SM_NEXTROW)], frows * SM_NEXTROW);
		bzero(&sm_addr[((startrow + frows) * SM_NEXTROW)],
		    (nrows - frows) * SM_NEXTROW);
	} else
		bzero(&sm_addr[(startrow * SM_NEXTROW)], nrows * SM_NEXTROW);
}

static int
smg_alloc_attr(id, fg, bg, flags, attrp)
	void *id;
	int fg, bg;
	int flags;
	long *attrp;
{
	return 0;
}

int
smg_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	return -1;
}

static int
smg_mmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{
	return -1;
}

int
smg_alloc_screen(v, type, cookiep, curxp, curyp, defattrp)
	void *v;
	const struct wsscreen_descr *type;
	void **cookiep;
	int *curxp, *curyp;
	long *defattrp;
{
	return 0;
}

void
smg_free_screen(v, cookie)
	void *v;
	void *cookie;
{
}

void
smg_show_screen(v, cookie)
	void *v;
	void *cookie;
{
}

static int
smg_load_font(v, cookie, first, num, stride, data)
	void *v;
	void *cookie;
	int first, num, stride;
	void *data;
{
	return 0;
}

cons_decl(smg);

#define	WSCONSOLEMAJOR 68

void
smgcninit(cndev)
	struct  consdev *cndev;
{
	extern void lkccninit __P((struct consdev *));
	extern int lkccngetc __P((dev_t));
	/* Clear screen */
	blkclr(sm_addr, 128*864);

	wsdisplay_cnattach(&smg_stdscreen, 0, 0, 0, 0);
	cn_tab->cn_dev = makedev(WSCONSOLEMAJOR, 0);
#if NLKC
	lkccninit(cndev);
	wsdisplay_set_cons_kbd(lkccngetc, nullcnpollc);
#endif
}

int smgprobe(void);
int
smgprobe()
{
	switch (vax_boardtype) {
	case VAX_BTYP_410:
	case VAX_BTYP_420:
	case VAX_BTYP_43:
		if (vax_confdata & 0x20) /* doesn't use graphics console */
			break;
		if (sm_addr == 0) /* Haven't mapped graphic area */
			break;

		return 1;

	default:
		break;
	}
	return 0;
}
