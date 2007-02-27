/* $NetBSD: vesabios.c,v 1.22.4.1 2007/02/27 16:51:31 yamt Exp $ */

/*
 * Copyright (c) 2002, 2004
 * 	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vesabios.c,v 1.22.4.1 2007/02/27 16:51:31 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <machine/frame.h>
#include <machine/kvm86.h>

#include <arch/i386/bios/vesabios.h>
#include <arch/i386/bios/vesabiosreg.h>

#include "opt_vesabios.h"

struct vbeinfoblock
{
	char VbeSignature[4];
	uint16_t VbeVersion;
	uint32_t OemStringPtr;
	uint32_t Capabilities;
	uint32_t VideoModePtr;
	uint16_t TotalMemory;
	uint16_t OemSoftwareRev;
	uint32_t OemVendorNamePtr, OemProductNamePtr, OemProductRevPtr;
	/* data area, in total max 512 bytes for VBE 2.0 */
} __attribute__ ((packed));

#define FAR2FLATPTR(p) ((p & 0xffff) + ((p >> 12) & 0xffff0))

static int vesabios_match(struct device *, struct cfdata *, void *);
static void vesabios_attach(struct device *, struct device *, void *);
static int vesabios_print(void *, const char *);

static int vbegetinfo(struct vbeinfoblock **);
static void vbefreeinfo(struct vbeinfoblock *);
#ifdef VESABIOSVERBOSE
static const char *mm2txt(unsigned int);
#endif

CFATTACH_DECL(vesabios, sizeof(struct device),
    vesabios_match, vesabios_attach, NULL, NULL);

static int
vesabios_match( struct device *parent, struct cfdata *match,
    void *aux)
{

	return 1;
}

static int
vbegetinfo(struct vbeinfoblock **vip)
{
	unsigned char *buf;
	struct trapframe tf;
	int res, error;

	buf = kvm86_bios_addpage(0x2000);
	if (!buf) {
		printf("vbegetinfo: kvm86_bios_addpage(0x2000) failed\n");
		return ENOMEM;
	}

	memcpy(buf, "VBE2", 4);

	memset(&tf, 0, sizeof(struct trapframe));
	tf.tf_eax = 0x4f00; /* function code */
	tf.tf_vm86_es = 0;
	tf.tf_edi = 0x2000; /* buf ptr */

	res = kvm86_bioscall(0x10, &tf);
	if (res || (tf.tf_eax & 0xff) != 0x4f) {
		printf("vbecall: res=%d, ax=%x\n", res, tf.tf_eax);
		error = ENXIO;
		goto out;
	}

	if (memcmp(((struct vbeinfoblock *)buf)->VbeSignature, "VESA", 4)) {
		error = EIO;
		goto out;
	}

	if (vip)
		*vip = (struct vbeinfoblock *)buf;
	return 0;

out:
	kvm86_bios_delpage(0x2000, buf);
	return error;
}

static void
vbefreeinfo(struct vbeinfoblock *vip)
{

	kvm86_bios_delpage(0x2000, vip);
}

int
vbeprobe(void)
{
	struct vbeinfoblock *vi;

	if (vbegetinfo(&vi))
		return 0;
	vbefreeinfo(vi);
	return 1;
}

#ifdef VESABIOSVERBOSE
static const char *
mm2txt(unsigned int mm)
{
	static char buf[30];
	static const char *names[] = {
		"Text mode",
		"CGA graphics",
		"Hercules graphics",
		"Planar",
		"Packed pixel",
		"Non-chain 4, 256 color",
		"Direct Color",
		"YUV"
	};

	if (mm < __arraycount(names))
		return names[mm];
	snprintf(buf, sizeof(buf), "unknown memory model %d", mm);
	return buf;
}
#endif

static void
vesabios_attach(struct device *parent, struct device *dev,
    void *aux)
{
	struct vbeinfoblock *vi;
	unsigned char *buf;
	struct trapframe tf;
	int res;
	char name[256];
#define MAXMODES 60
	uint16_t modes[MAXMODES];
	int rastermodes[MAXMODES];
	int textmodes[MAXMODES];
	int nmodes, nrastermodes, ntextmodes, i;
	uint32_t modeptr;
	struct modeinfoblock *mi;
	struct vesabiosdev_attach_args vbaa;

	if (vbegetinfo(&vi)) {
		printf("\n");
		panic("vesabios_attach: disappeared");
	}

	aprint_naive("\n");
	aprint_normal(": version %d.%d",
	    vi->VbeVersion >> 8, vi->VbeVersion & 0xff);

	res = kvm86_bios_read(FAR2FLATPTR(vi->OemVendorNamePtr),
			      name, sizeof(name));
	if (res > 0) {
		name[res - 1] = 0;
		aprint_normal(", %s", name);
		res = kvm86_bios_read(FAR2FLATPTR(vi->OemProductNamePtr),
				      name, sizeof(name));
		if (res > 0) {
			name[res - 1] = 0;
			aprint_normal(" %s", name);
		}
	}
	aprint_normal("\n");

	nmodes = 0;
	modeptr = FAR2FLATPTR(vi->VideoModePtr);
	while (nmodes < MAXMODES) {
		res = kvm86_bios_read(modeptr, (char *)&modes[nmodes], 2);
		if (res != 2 || modes[nmodes] == 0xffff)
			break;
		nmodes++;
		modeptr += 2;
	}

	vbefreeinfo(vi);
	if (nmodes == 0)
		return;

	nrastermodes = ntextmodes = 0;

	buf = kvm86_bios_addpage(0x2000);
	if (!buf) {
		aprint_error("%s: kvm86_bios_addpage(0x2000) failed\n",
		    dev->dv_xname);
		return;
	}
	for (i = 0; i < nmodes; i++) {

		memset(&tf, 0, sizeof(struct trapframe));
		tf.tf_eax = 0x4f01; /* function code */
		tf.tf_ecx = modes[i];
		tf.tf_vm86_es = 0;
		tf.tf_edi = 0x2000; /* buf ptr */

		res = kvm86_bioscall(0x10, &tf);
		if (res || (tf.tf_eax & 0xff) != 0x4f) {
			aprint_error("%s: vbecall: res=%d, ax=%x\n",
			    dev->dv_xname, res, tf.tf_eax);
			aprint_error("%s: error getting info for mode %04x\n",
			    dev->dv_xname, modes[i]);
			continue;
		}
		mi = (struct modeinfoblock *)buf;
#ifdef VESABIOSVERBOSE
		aprint_verbose("%s: VESA mode %04x: attributes %04x",
		       dev->dv_xname, modes[i], mi->ModeAttributes);
#endif
		if (!(mi->ModeAttributes & 1)) {
#ifdef VESABIOSVERBOSE
			aprint_verbose("\n");
#endif
			continue;
		}
		if (mi->ModeAttributes & 0x10) {
			/* graphics */
#ifdef VESABIOSVERBOSE
			aprint_verbose(", %dx%d %dbbp %s\n",
			       mi->XResolution, mi->YResolution,
			       mi->BitsPerPixel, mm2txt(mi->MemoryModel));
#endif
			if (mi->ModeAttributes & 0x80) {
				/* flat buffer */
				rastermodes[nrastermodes++] = modes[i];
			}
		} else {
			/* text */
#ifdef VESABIOSVERBOSE
			aprint_verbose(", text %dx%d\n",
			       mi->XResolution, mi->YResolution);
#endif
			if (!(mi->ModeAttributes & 0x20)) /* VGA compatible */
				textmodes[ntextmodes++] = modes[i];
		}
	}
	kvm86_bios_delpage(0x2000, buf);

	if (nrastermodes) {
		vbaa.vbaa_type = "raster";
		vbaa.vbaa_modes = rastermodes;
		vbaa.vbaa_nmodes = nrastermodes;

		config_found(dev, &vbaa, vesabios_print);
	}
	if (ntextmodes) {
		vbaa.vbaa_type = "text";
		vbaa.vbaa_modes = textmodes;
		vbaa.vbaa_nmodes = ntextmodes;

		config_found(dev, &vbaa, vesabios_print);
	}
}

static int
vesabios_print(void *aux, const char *pnp)
{
	struct vesabiosdev_attach_args *vbaa = aux;

	if (pnp)
		aprint_normal("%s at %s", vbaa->vbaa_type, pnp);
	return UNCONF;
}
