/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: grfioctl.h 1.1 90/07/09$
 *
 *	@(#)grfioctl.h	7.2 (Berkeley) 11/4/90
 */


/* these are changeable values, encapsulated in their own structure, so
   no the whole thing has to be copied when setting parameters. */
struct grfdyninfo {
	int	gdi_fbx;			/* frame buffer x offset */
	int	gdi_fby;			/* frame buffer y offset */
	int	gdi_dwidth;		/* displayed part width */
	int	gdi_dheight;		/* displayed part height */
	int	gdi_dx;			/* displayed part x offset */
	int	gdi_dy;			/* displayed part y offset */ 
};

struct	grfinfo {
	caddr_t	gd_regaddr;		/* control registers physaddr */
	int	gd_regsize;		/* control registers size */
	caddr_t	gd_fbaddr;		/* frame buffer physaddr */
	int	gd_fbsize;		/* frame buffer size */
	short	gd_colors;		/* number of colors */
	short	gd_planes;		/* number of planes */

	int	gd_fbwidth;		/* frame buffer width */
	int	gd_fbheight;		/* frame buffer height */

	struct grfdyninfo gd_dyn;	/* everything changable by GRFIOCSINFO */
/* compatibility... */
#define gd_fbx		gd_dyn.gdi_fbx
#define gd_fby		gd_dyn.gdi_fby
#define gd_dwidth 	gd_dyn.gdi_dwidth
#define gd_dheight	gd_dyn.gdi_dheight
#define gd_dx		gd_dyn.gdi_dx
#define gd_dy		gd_dyn.gdi_dy
};

/*
 * BSD ioctls
 */
#define	GRFIOCGINFO	_IOR('G', 0, struct grfinfo) /* get info on device */
#define	GRFIOCON	_IO('G', 1)		/* turn graphics on */
#define	GRFIOCOFF	_IO('G', 2)		/* turn graphics off */
#define GRFIOCMAP	_IOWR('G', 5, int)	/* map in regs+framebuffer */
#define GRFIOCUNMAP	_IOW('G', 6, int)	/* unmap regs+framebuffer */
#define	GRFIOCSINFO	_IOW('G', 7, struct grfdyninfo) /* set info on device */
