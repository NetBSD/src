/*	$NetBSD: fb.c,v 1.14.88.1 2014/08/20 00:03:26 tls Exp $ */

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
 *	@(#)fb.c	8.1 (Berkeley) 6/11/93
 */

/*
 * /dev/fb (indirect frame buffer driver).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fb.c,v 1.14.88.1 2014/08/20 00:03:26 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/proc.h>

#include <machine/eeprom.h>
#include <dev/sun/fbio.h>

#include <sun3/dev/fbvar.h>
#include <sun3/dev/p4reg.h>

dev_type_open(fbopen);
dev_type_close(fbclose);
dev_type_ioctl(fbioctl);
dev_type_mmap(fbmmap);

const struct cdevsw fb_cdevsw = {
	.d_open = fbopen,
	.d_close = fbclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = fbioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = fbmmap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

static struct fbdevice *devfb;
static int fbpriority;

/*
 * This is called by the real driver (i.e. bw2, cg3, ...)
 * to declare itself as a potential default frame buffer.
 */
void 
fb_attach(struct fbdevice *fb, int newpri)
{
	if (fbpriority < newpri) {
		fbpriority = newpri;
		devfb = fb;
	}
}

int 
fbopen(dev_t dev, int flags, int mode, struct lwp *l)
{

	if (devfb == NULL)
		return (ENXIO);
	return ((*devfb->fb_driver->fbd_open)(dev, flags, mode, l));
}

int 
fbclose(dev_t dev, int flags, int mode, struct lwp *l)
{

	return ((*devfb->fb_driver->fbd_close)(dev, flags, mode, l));
}

int 
fbioctl(dev_t dev, u_long cmd, void *data, int flags, struct lwp *l)
{
	return (fbioctlfb(devfb, cmd, data));
}

paddr_t 
fbmmap(dev_t dev, off_t off, int prot)
{
	return ((*devfb->fb_driver->fbd_mmap)(dev, off, prot));
}

/*
 * Common fb ioctl function
 */
int 
fbioctlfb(struct fbdevice *fb, u_long cmd, void *data)
{
	struct fbdriver *fbd = fb->fb_driver;
	void *vp = (void *)data;
	int error;

	switch (cmd) {

	case FBIOGTYPE:
		*(struct fbtype *)vp = fb->fb_fbtype;
		error = 0;
		break;

	case FBIOGATTR:
		error = (*fbd->fbd_gattr)(fb, vp);
		break;

	case FBIOGVIDEO:
		error = (*fbd->fbd_gvideo)(fb, vp);
		break;

	case FBIOSVIDEO:
		error = (*fbd->fbd_svideo)(fb, vp);
		break;

	case FBIOGETCMAP:
		error = (*fbd->fbd_getcmap)(fb, vp);
		break;

	case FBIOPUTCMAP:
		error = (*fbd->fbd_putcmap)(fb, vp);
		break;

	default:
		error = ENOTTY;
	}
	return (error);
}

void 
fb_unblank(void)
{
	int on = 1;

	if (devfb == NULL)
		return;

	(*devfb->fb_driver->fbd_svideo)(devfb, (void *)&on);
}

/*
 * Default ioctl function to put in struct fbdriver
 * for functions that are not supported.
 */
int 
fb_noioctl(struct fbdevice *fbd, void *vp)
{
	return ENOTTY;
}

/****************************************************************
 * Misc. helpers...
 */

/* Set FB size based on EEPROM screen shape code. */
void 
fb_eeprom_setsize(struct fbdevice *fb)
{
	int szcode;
	int w, h;

	/* Go get the EEPROM screen size byte. */
	szcode = eeprom_copy->eeScreenSize;

	w = h = 0;
	switch (szcode) {
	case EE_SCR_1152X900:
		w = 1152;
		h = 900;
		break;
	case EE_SCR_1024X1024:
		w = 1024;
		h = 1024;
		break;
	case EE_SCR_1600X1280:
		w = 1600;
		h = 1280;
		break;
	case EE_SCR_1440X1440:
		w = 1440;
		h = 1440;
		break;
	default:
		break;
	}

	if (w && h) {
		fb->fb_fbtype.fb_width  = w;
		fb->fb_fbtype.fb_height = h;
	} else {
		printf("%s: EE size code %d unknown\n",
			   fb->fb_name, szcode);
	}
}

/*
 * Probe for a P4 register at the passed virtual address.
 * Returns P4 ID value, or -1 if no P4 register.
 */
int 
fb_pfour_id(void *va)
{
	volatile uint32_t val, save, *pfour = va;

	/* Read the P4 register. */
	save = *pfour;

	/*
	 * Try to modify the type code.  If it changes, put the
	 * original value back, and tell the caller that the
	 * framebuffer does not have a P4 register.
	 */
	val = save & ~P4_REG_RESET;
	*pfour = (val ^ P4_FBTYPE_MASK);
	if ((*pfour ^ val) & P4_FBTYPE_MASK) {
		*pfour = save;
		return (-1);
	}

	return (P4_ID(val));
}

/*
 * Return the status of the video enable.
 */
int 
fb_pfour_get_video(struct fbdevice *fb)
{

	return ((*fb->fb_pfour & P4_REG_VIDEO) != 0);
}

/*
 * Turn video on or off using the P4 register.
 */
void 
fb_pfour_set_video(struct fbdevice *fb, int on)
{
	int pfour;

	pfour = *fb->fb_pfour & ~(P4_REG_INTCLR|P4_REG_VIDEO);
	*fb->fb_pfour = pfour | (on ? P4_REG_VIDEO : 0);
}

static const struct {
	int w, h;
} fb_p4sizedecode[P4_SIZE_MASK+1] = {
	{ 1600, 1280 },
	{ 1152,  900 },
	{ 1024, 1024 },
	{ 1280, 1024 },
	{ 1440, 1440 },
	{  640,  480 },
};

/*
 * Use the P4 register to determine the screen size.
 */
void 
fb_pfour_setsize(struct fbdevice *fb)
{
	int p4, p4type, p4size;
	int h, w;

	if (fb->fb_pfour == 0)
		return;

	p4 = *fb->fb_pfour;
	p4type = P4_FBTYPE(p4);

	/* These do not encode the screen size. */
	if (p4type == P4_ID_COLOR24)
		return;
	if ((p4type & P4_ID_MASK) == P4_ID_FASTCOLOR)
		return;

	p4size = p4type & P4_SIZE_MASK;
	w = fb_p4sizedecode[p4size].w;
	h = fb_p4sizedecode[p4size].h;
	if (w && h) {
		fb->fb_fbtype.fb_width  = w;
		fb->fb_fbtype.fb_height = h;
	} else {
		printf("%s: P4 size code %d unknown\n",
			   fb->fb_name, p4size);
	}
}
