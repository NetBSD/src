/*	$NetBSD: grf_hy.c,v 1.16 2002/03/15 05:55:35 gmcgarry Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
 * Copyright (c) 1991 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Mark Davies of the Department of Computer
 * Science, Victoria University of Wellington, New Zealand.
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
 * from: Utah $Hdr: grf_hy.c 1.2 93/08/13$
 *
 *	@(#)grf_hy.c	8.4 (Berkeley) 1/12/94
 */

/*
 * Graphics routines for HYPERION frame buffer
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: grf_hy.c,v 1.16 2002/03/15 05:55:35 gmcgarry Exp $");                                                  

#include "opt_compat_hpux.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/uio.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <dev/cons.h>

#include <hp300/dev/diovar.h>
#include <hp300/dev/diodevs.h>
#include <hp300/dev/intiovar.h>

#include <hp300/dev/grfioctl.h>
#include <hp300/dev/grfvar.h>
#include <hp300/dev/grfreg.h>
#include <hp300/dev/grf_hyreg.h>

#include <hp300/dev/itevar.h>
#include <hp300/dev/itereg.h>

#include "ite.h"

caddr_t badhyaddr = (caddr_t) -1;

int	hy_init __P((struct grf_data *gp, int, caddr_t));
int	hy_mode __P((struct grf_data *gp, int, caddr_t));
void	hyper_ite_fontinit __P((struct ite_data *));

int	hyper_dio_match __P((struct device *, struct cfdata *, void *));
void	hyper_dio_attach __P((struct device *, struct device *, void *));

int	hypercnattach __P((bus_space_tag_t, bus_addr_t, int));

struct cfattach hyper_dio_ca = {
	sizeof(struct grfdev_softc), hyper_dio_match, hyper_dio_attach
};

/* Hyperion grf switch */
struct grfsw hyper_grfsw = {
	GID_HYPERION, GRFHYPERION, "hyperion", hy_init, hy_mode
};

static int hyperconscode;
static caddr_t hyperconaddr;

#if NITE > 0
void	hyper_init __P((struct ite_data *));
void	hyper_deinit __P((struct ite_data *));
void	hyper_int_fontinit __P((struct ite_data *));
void	hyper_putc __P((struct ite_data *, int, int, int, int));
void	hyper_cursor __P((struct ite_data *, int));
void	hyper_clear __P((struct ite_data *, int, int, int, int));
void	hyper_scroll __P((struct ite_data *, int, int, int, int));
void	hyper_windowmove __P((struct ite_data *, int, int, int, int,
		int, int, int));

/* Hyperion ite switch */
struct itesw hyper_itesw = {
	hyper_init, hyper_deinit, hyper_clear, hyper_putc,
	hyper_cursor, hyper_scroll, ite_readbyte, ite_writeglyph
};
#endif /* NITE > 0 */

int
hyper_dio_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct dio_attach_args *da = aux;

	if (da->da_id == DIO_DEVICE_ID_FRAMEBUFFER &&
	    da->da_secid == DIO_DEVICE_SECID_HYPERION)
		return (1);

	return (0);
}

void
hyper_dio_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct grfdev_softc *sc = (struct grfdev_softc *)self;
	struct dio_attach_args *da = aux;
	caddr_t grf;

	sc->sc_scode = da->da_scode;
	if (sc->sc_scode == hyperconscode)
		grf = hyperconaddr;
	else {
		grf = iomap(dio_scodetopa(sc->sc_scode), da->da_size);
		if (grf == 0) {
			printf("%s: can't map framebuffer\n",
			    sc->sc_dev.dv_xname);
			return;
		}
	}

	sc->sc_isconsole = (sc->sc_scode == hyperconscode);
	grfdev_attach(sc, hy_init, grf, &hyper_grfsw);
}

/*
 * Initialize hardware.
 * Must fill in the grfinfo structure in g_softc.
 * Returns 0 if hardware not present, non-zero ow.
 */
int
hy_init(gp, scode, addr)
	struct grf_data *gp;
	int scode;
	caddr_t addr;
{
	struct hyboxfb *hy = (struct hyboxfb *) addr;
	struct grfinfo *gi = &gp->g_display;
	int fboff;

	/*
	 * If the console has been initialized, and it was us, there's
	 * no need to repeat this.
	 */
	if (scode != hyperconscode) {
		if (ISIIOVA(addr))
			gi->gd_regaddr = (caddr_t) IIOP(addr);
		else
			gi->gd_regaddr = dio_scodetopa(scode);
		gi->gd_regsize = 0x20000;
		gi->gd_fbwidth = (hy->fbwmsb << 8) | hy->fbwlsb;
		gi->gd_fbheight = (hy->fbhmsb << 8) | hy->fbhlsb;
		gi->gd_fbsize = (gi->gd_fbwidth * gi->gd_fbheight) >> 3;
		fboff = (hy->fbomsb << 8) | hy->fbolsb;
		gi->gd_fbaddr = (caddr_t) (*((u_char *)addr + fboff) << 16);
		if (gi->gd_regaddr >= (caddr_t)DIOIIBASE) {
			/*
			 * For DIO II space the fbaddr just computed is
			 * the offset from the select code base (regaddr)
			 * of the framebuffer.  Hence it is also implicitly
			 * the size of the register set.
			 */
			gi->gd_regsize = (int) gi->gd_fbaddr;
			gi->gd_fbaddr += (int) gi->gd_regaddr;
			gp->g_regkva = addr;
			gp->g_fbkva = addr + gi->gd_regsize;
		} else {
			/*
			 * For DIO space we need to map the separate
			 * framebuffer.
			 */
			gp->g_regkva = addr;
			gp->g_fbkva = iomap(gi->gd_fbaddr, gi->gd_fbsize);
		}
		gi->gd_dwidth = (hy->dwmsb << 8) | hy->dwlsb;
		gi->gd_dheight = (hy->dhmsb << 8) | hy->dhlsb;
		gi->gd_planes = hy->num_planes;
		gi->gd_colors = 1 << gi->gd_planes;
	}
	return(1);
}

/*
 * Change the mode of the display.
 * Right now all we can do is grfon/grfoff.
 * Return a UNIX error number or 0 for success.
 * Function may not be needed anymore.
 */
int
hy_mode(gp, cmd, data)
	struct grf_data *gp;
	int cmd;
	caddr_t data;
{
	int error = 0;

	switch (cmd) {
	case GM_GRFON:
	case GM_GRFOFF:
		break;

	/*
	 * Remember UVA of mapping for GCDESCRIBE.
	 * XXX this should be per-process.
	 */
	case GM_MAP:
		gp->g_data = data;
		break;

	case GM_UNMAP:
		gp->g_data = 0;
		break;

#ifdef COMPAT_HPUX
	case GM_DESCRIBE:
	{
		struct grf_fbinfo *fi = (struct grf_fbinfo *)data;
		struct grfinfo *gi = &gp->g_display;
		int i;

		/* feed it what HP-UX expects */
		fi->id = gi->gd_id;
		fi->mapsize = gi->gd_fbsize;
		fi->dwidth = gi->gd_dwidth;
		fi->dlength = gi->gd_dheight;
		fi->width = gi->gd_fbwidth;
		fi->length = gi->gd_fbheight;
		fi->bpp = NBBY;
		fi->xlen = (fi->width * fi->bpp) / NBBY;
		fi->npl = gi->gd_planes;
		fi->bppu = fi->npl;
		fi->nplbytes = fi->xlen * ((fi->length * fi->bpp) / NBBY);
		memcpy(fi->name, "A1096A", 7);	/* ?? */
		fi->attr = 0;			/* ?? */
		/*
		 * If mapped, return the UVA where mapped.
		 */
		if (gp->g_data) {
			fi->regbase = gp->g_data;
			fi->fbbase = fi->regbase + gp->g_display.gd_regsize;
		} else {
			fi->fbbase = 0;
			fi->regbase = 0;
		}
		for (i = 0; i < 6; i++)
			fi->regions[i] = 0;
		break;
	}
#endif

	default:
		error = EINVAL;
		break;
	}
	return(error);
}

#if NITE > 0

/*
 * Hyperion ite routines
 */

#define REGBASE	    	((struct hyboxfb *)(ip->regbase))
#define WINDOWMOVER 	hyper_windowmove

#undef charX
#define	charX(ip,c)	\
	(((c) % (ip)->cpl) * ((((ip)->ftwidth + 7) / 8) * 8) + (ip)->fontx)

void
hyper_init(ip)
	struct ite_data *ip;
{
	int width;

	/* XXX */
	if (ip->regbase == NULL) {
		struct grf_data *gp = ip->grf;

		ip->regbase = gp->g_regkva;
		ip->fbbase = gp->g_fbkva;
		ip->fbwidth = gp->g_display.gd_fbwidth;
		ip->fbheight = gp->g_display.gd_fbheight;
		ip->dwidth = gp->g_display.gd_dwidth;
		ip->dheight = gp->g_display.gd_dheight;
	}

	ite_fontinfo(ip);
	width = ((ip->ftwidth + 7) / 8) * 8;
	ip->cpl      = (ip->fbwidth - ip->dwidth) / width;
	ip->cblanky  = ip->fonty + ((128 / ip->cpl) +1) * ip->ftheight;

	/*
	 * Clear the framebuffer on all planes.
	 */
	hyper_windowmove(ip, 0, 0, 0, 0, ip->fbheight, ip->fbwidth, RR_CLEAR);

	hyper_ite_fontinit(ip);

	REGBASE->nblank = 0x05;

	/*
	 * Stash the inverted cursor.
	 */
	hyper_windowmove(ip, charY(ip, ' '), charX(ip, ' '),
			 ip->cblanky, ip->cblankx, ip->ftheight,
			 ip->ftwidth, RR_COPYINVERTED);
}

void
hyper_deinit(ip)
	struct ite_data *ip;
{
	hyper_windowmove(ip, 0, 0, 0, 0, ip->fbheight, ip->fbwidth, RR_CLEAR);

	REGBASE->nblank = 0x05;
   	ip->flags &= ~ITE_INITED;
}

void
hyper_ite_fontinit(ip)
	struct ite_data *ip;
{
	u_char *fbmem, *dp;
	int c, l, b;
	int stride, width;

	dp = (u_char *)(getword(ip, getword(ip, FONTROM) + FONTADDR) +
	    ip->regbase) + FONTDATA;
	stride = ip->fbwidth >> 3;
	width = (ip->ftwidth + 7) / 8;

	for (c = 0; c < 128; c++) {
		fbmem = (u_char *) FBBASE +
			(ip->fonty + (c / ip->cpl) * ip->ftheight) *
			stride;
		fbmem += (ip->fontx >> 3) + (c % ip->cpl) * width;
		for (l = 0; l < ip->ftheight; l++) {
			for (b = 0; b < width; b++) {
				*fbmem++ = *dp;
				dp += 2;
			}
			fbmem -= width;
			fbmem += stride;
		}
	}
}

void
hyper_putc(ip, c, dy, dx, mode)
	struct ite_data *ip;
	int c, dy, dx, mode;
{
        int wmrr = ((mode == ATTR_INV) ? RR_COPYINVERTED : RR_COPY);
	
	hyper_windowmove(ip, charY(ip, c), charX(ip, c),
			 dy * ip->ftheight, dx * ip->ftwidth,
			 ip->ftheight, ip->ftwidth, wmrr);
}

void
hyper_cursor(ip, flag)
	struct ite_data *ip;
	int flag;
{
	if (flag == DRAW_CURSOR)
		draw_cursor(ip)
	else if (flag == MOVE_CURSOR) {
		erase_cursor(ip)
		draw_cursor(ip)
	}
	else
		erase_cursor(ip)
}

void
hyper_clear(ip, sy, sx, h, w)
	struct ite_data *ip;
	int sy, sx, h, w;
{
	hyper_windowmove(ip, sy * ip->ftheight, sx * ip->ftwidth,
			 sy * ip->ftheight, sx * ip->ftwidth, 
			 h  * ip->ftheight, w  * ip->ftwidth,
			 RR_CLEAR);
}

void
hyper_scroll(ip, sy, sx, count, dir)
        struct ite_data *ip;
        int sy, count, dir, sx;
{
	int dy;
	int dx = sx;
	int height = 1;
	int width = ip->cols;

	if (dir == SCROLL_UP) {
		dy = sy - count;
		height = ip->rows - sy;
	}
	else if (dir == SCROLL_DOWN) {
		dy = sy + count;
		height = ip->rows - dy - 1;
	}
	else if (dir == SCROLL_RIGHT) {
		dy = sy;
		dx = sx + count;
		width = ip->cols - dx;
	}
	else {
		dy = sy;
		dx = sx - count;
		width = ip->cols - sx;
	}		

	hyper_windowmove(ip, sy * ip->ftheight, sx * ip->ftwidth,
			 dy * ip->ftheight, dx * ip->ftwidth,
			 height * ip->ftheight,
			 width  * ip->ftwidth, RR_COPY);
}

#include <hp300/dev/maskbits.h>

/* NOTE:
 * the first element in starttab could be 0xffffffff.  making it 0
 * lets us deal with a full first word in the middle loop, rather
 * than having to do the multiple reads and masks that we'd
 * have to do if we thought it was partial.
 */
int starttab[32] =
    {
	0x00000000,
	0x7FFFFFFF,
	0x3FFFFFFF,
	0x1FFFFFFF,
	0x0FFFFFFF,
	0x07FFFFFF,
	0x03FFFFFF,
	0x01FFFFFF,
	0x00FFFFFF,
	0x007FFFFF,
	0x003FFFFF,
	0x001FFFFF,
	0x000FFFFF,
	0x0007FFFF,
	0x0003FFFF,
	0x0001FFFF,
	0x0000FFFF,
	0x00007FFF,
	0x00003FFF,
	0x00001FFF,
	0x00000FFF,
	0x000007FF,
	0x000003FF,
	0x000001FF,
	0x000000FF,
	0x0000007F,
	0x0000003F,
	0x0000001F,
	0x0000000F,
	0x00000007,
	0x00000003,
	0x00000001
    };

int endtab[32] =
    {
	0x00000000,
	0x80000000,
	0xC0000000,
	0xE0000000,
	0xF0000000,
	0xF8000000,
	0xFC000000,
	0xFE000000,
	0xFF000000,
	0xFF800000,
	0xFFC00000,
	0xFFE00000,
	0xFFF00000,
	0xFFF80000,
	0xFFFC0000,
	0xFFFE0000,
	0xFFFF0000,
	0xFFFF8000,
	0xFFFFC000,
	0xFFFFE000,
	0xFFFFF000,
	0xFFFFF800,
	0xFFFFFC00,
	0xFFFFFE00,
	0xFFFFFF00,
	0xFFFFFF80,
	0xFFFFFFC0,
	0xFFFFFFE0,
	0xFFFFFFF0,
	0xFFFFFFF8,
	0xFFFFFFFC,
	0xFFFFFFFE
    };

void
hyper_windowmove(ip, sy, sx, dy, dx, h, w, func)
	struct ite_data *ip;
	int sy, sx, dy, dx, h, w, func;
{
	int width;		/* add to get to same position in next line */

	unsigned int *psrcLine, *pdstLine;
                                /* pointers to line with current src and dst */
	unsigned int *psrc;  /* pointer to current src longword */
	unsigned int *pdst;  /* pointer to current dst longword */

                                /* following used for looping through a line */
	unsigned int startmask, endmask;  /* masks for writing ends of dst */
	int nlMiddle;		/* whole longwords in dst */
	int nl;	/* temp copy of nlMiddle */
	unsigned int tmpSrc;
                                /* place to store full source word */
	int xoffSrc;	/* offset (>= 0, < 32) from which to
                                   fetch whole longwords fetched
                                   in src */
	int nstart;		/* number of ragged bits at start of dst */
	int nend;		/* number of ragged bits at end of dst */
	int srcStartOver;	/* pulling nstart bits from src
                                   overflows into the next word? */

	if (h == 0 || w == 0)
		return;

	width = ip->fbwidth >> 5;

	if (sy < dy) /* start at last scanline of rectangle */
	{
	    psrcLine = ((unsigned int *) ip->fbbase) + ((sy+h-1) * width);
	    pdstLine = ((unsigned int *) ip->fbbase) + ((dy+h-1) * width);
	    width = -width;
	}
	else /* start at first scanline */
	{
	    psrcLine = ((unsigned int *) ip->fbbase) + (sy * width);
	    pdstLine = ((unsigned int *) ip->fbbase) + (dy * width);
	}

	/* x direction doesn't matter for < 1 longword */
	if (w <= 32)
	{
	    int srcBit, dstBit;     /* bit offset of src and dst */

	    pdstLine += (dx >> 5);
	    psrcLine += (sx >> 5);
	    psrc = psrcLine;
	    pdst = pdstLine;

	    srcBit = sx & 0x1f;
	    dstBit = dx & 0x1f;

	    while(h--)
	    {
                getandputrop(psrc, srcBit, dstBit, w, pdst, func)
	        pdst += width;
		psrc += width;
	    }
	}
	else
        {
	    maskbits(dx, w, startmask, endmask, nlMiddle)
	    if (startmask)
	      nstart = 32 - (dx & 0x1f);
	    else
	      nstart = 0;
	    if (endmask)
	      nend = (dx + w) & 0x1f;
	    else
	      nend = 0;

	    xoffSrc = ((sx & 0x1f) + nstart) & 0x1f;
	    srcStartOver = ((sx & 0x1f) + nstart) > 31;

	    if (sx >= dx) /* move left to right */
	    {
	        pdstLine += (dx >> 5);
		psrcLine += (sx >> 5);

		while (h--)
		{
		    psrc = psrcLine;
		    pdst = pdstLine;

		    if (startmask)
		    {
			getandputrop(psrc, (sx & 0x1f),
				     (dx & 0x1f), nstart, pdst, func)
			    pdst++;
			if (srcStartOver)
			    psrc++;
		    }

		    /* special case for aligned operations */
		    if (xoffSrc == 0)
		    {
			nl = nlMiddle;
			while (nl--)
			{
			    DoRop (*pdst, func, *psrc++, *pdst);
			    pdst++;
			}
		    }
		    else
		    {
			nl = nlMiddle + 1;
			while (--nl)
			{
			    getunalignedword (psrc, xoffSrc, tmpSrc)
				DoRop (*pdst, func, tmpSrc, *pdst);
			    pdst++;
			    psrc++;
			}
		    }

		    if (endmask)
		    {
			getandputrop0(psrc, xoffSrc, nend, pdst, func);
		    }

		    pdstLine += width;
		    psrcLine += width;
		}
	    }
	    else /* move right to left */
	    {
		pdstLine += ((dx + w) >> 5);
		psrcLine += ((sx + w) >> 5);
		/* if fetch of last partial bits from source crosses
		   a longword boundary, start at the previous longword
		   */
		if (xoffSrc + nend >= 32)
		    --psrcLine;

		while (h--)
		{
		    psrc = psrcLine;
		    pdst = pdstLine;

		    if (endmask)
		    {
			getandputrop0(psrc, xoffSrc, nend, pdst, func);
		    }

		    nl = nlMiddle + 1;
		    while (--nl)
		    {
			--psrc;
			--pdst;
			getunalignedword(psrc, xoffSrc, tmpSrc)
                        DoRop(*pdst, func, tmpSrc, *pdst);
		    }

		    if (startmask)
		    {
			if (srcStartOver)
			    --psrc;
			--pdst;
			getandputrop(psrc, (sx & 0x1f),
				     (dx & 0x1f), nstart, pdst, func)
                    }

		    pdstLine += width;
		    psrcLine += width;
		}
	    } /* move right to left */
	}
}

/*
 * Hyperion console support
 */
int
hypercnattach(bus_space_tag_t bst, bus_addr_t addr, int scode)
{
	bus_space_handle_t bsh;
	caddr_t va;
	struct grfreg *grf;
	struct grf_data *gp = &grf_cn;
	u_int8_t *dioiidev;
	int size;

	if (bus_space_map(bst, addr, NBPG, 0, &bsh))
		return (1);
	va = bus_space_vaddr(bst, bsh);
	grf = (struct grfreg *)va;

	if ((grf->gr_id != GRFHWID) || (grf->gr_id2 != GID_HYPERION)) {
		bus_space_unmap(bst, bsh, NBPG);
		return (1);
	}

	if (scode > 132) {
		dioiidev = (u_int8_t *)va;
		size =  ((dioiidev[0x101] + 1) * 0x100000);
	} else
		size = DIOCSIZE;

	bus_space_unmap(bst, bsh, NBPG);
	if (bus_space_map(bst, addr, size, 0, &bsh))
		return (1);
	va = bus_space_vaddr(bst, bsh);

	/*
	 * Initialize the framebuffer hardware.
	 */
	(void)hy_init(gp, scode, va);
	hyperconscode = scode;
	hyperconaddr = va;

	/*
	 * Set up required grf data.
	*/
	gp->g_sw = &hyper_grfsw;
	gp->g_display.gd_id = gp->g_sw->gd_swid;
	gp->g_flags = GF_ALIVE;

	/*
	 * Initialize the terminal emulator.
	*/
	itedisplaycnattach(gp, &hyper_itesw);
	return (0);
}

#endif /* NITE > 0 */
