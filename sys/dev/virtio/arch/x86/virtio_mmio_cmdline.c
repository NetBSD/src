/* $NetBSD: virtio_mmio_cmdline.c,v 1.1 2025/01/15 13:16:23 imil Exp $ */

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emile 'iMil' Heitor.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

/*-
 * Copyright (c) 2022 Colin Percival
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
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

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/systm.h>

#define VIRTIO_PRIVATE
#include <dev/virtio/virtio_mmiovar.h>
#include <arch/x86/pv/pvvar.h>
#include <xen/hypervisor.h>

#include <machine/i82093var.h>
#include "ioapic.h"

#define VMMIOSTR "virtio_mmio.device="

struct mmio_args {
	uint64_t	sz;
	uint64_t	baseaddr;
	uint64_t	irq;
	uint64_t	id;
};

struct virtio_mmio_cmdline_softc {
	struct virtio_mmio_softc	sc_msc;
	struct mmio_args		margs;
};

static int	virtio_mmio_cmdline_match(device_t, cfdata_t, void *);
static void	virtio_mmio_cmdline_attach(device_t, device_t, void *);
static int	virtio_mmio_cmdline_do_attach(device_t,
		    struct pv_attach_args *, struct mmio_args *);
static int	virtio_mmio_cmdline_detach(device_t, int);
static int	virtio_mmio_cmdline_rescan(device_t, const char *, const int *);
static int	virtio_mmio_cmdline_alloc_interrupts(struct virtio_mmio_softc *);
static void	virtio_mmio_cmdline_free_interrupts(struct virtio_mmio_softc *);

CFATTACH_DECL3_NEW(mmio_cmdline,
    sizeof(struct virtio_mmio_cmdline_softc),
    virtio_mmio_cmdline_match, virtio_mmio_cmdline_attach,
    virtio_mmio_cmdline_detach, NULL,
    virtio_mmio_cmdline_rescan, NULL, 0);

static int
virtio_mmio_cmdline_match(device_t parent, cfdata_t match, void *aux)
{
	if (strstr(xen_start_info.cmd_line, VMMIOSTR) == NULL)
		return 0;

	return 1;
}

static void
parsearg(struct mmio_args *margs, const char *arg)
{
	char *p;

	/* <size> */
	margs->sz = strtoull(arg, (char **)&p, 0);
	if ((margs->sz == 0) || (margs->sz == UINT64_MAX))
		goto bad;
	switch (*p) {
	case 'E': case 'e':
		/* Check for overflow */
		if (margs->sz > (UINT64_MAX >> 60))
			goto bad;
		margs->sz <<= 10;
		/* FALLTHROUGH */
	case 'P': case 'p':
		if (margs->sz > (UINT64_MAX >> 50))
			goto bad;
		margs->sz <<= 10;
		/* FALLTHROUGH */
	case 'T': case 't':
		if (margs->sz > (UINT64_MAX >> 40))
			goto bad;
		margs->sz <<= 10;
		/* FALLTHROUGH */
	case 'G': case 'g':
		if (margs->sz > (UINT64_MAX >> 30))
			goto bad;
		margs->sz <<= 10;
		/* FALLTHROUGH */
	case 'M': case 'm':
		if (margs->sz > (UINT64_MAX >> 20))
			goto bad;
		margs->sz <<= 10;
		/* FALLTHROUGH */
	case 'K': case 'k':
		if (margs->sz > (UINT64_MAX >> 10))
			goto bad;
		margs->sz <<= 10;
		p++;
		break;
	}

	/* @<baseaddr> */
	if (*p++ != '@')
		goto bad;
	margs->baseaddr = strtoull(p, (char **)&p, 0);
	if ((margs->baseaddr == 0) || (margs->baseaddr == UINT64_MAX))
		goto bad;

	/* :<irq> */
	if (*p++ != ':')
		goto bad;
	margs->irq = strtoull(p, (char **)&p, 0);
	if ((margs->irq == 0) || (margs->irq == UINT64_MAX))
		goto bad;

	/* Optionally, :<id> */
	if (*p) {
		if (*p++ != ':')
			goto bad;
		margs->id = strtoull(p, (char **)&p, 0);
		if ((margs->id == 0) || (margs->id == UINT64_MAX))
			goto bad;
	} else {
		margs->id = 0;
	}

	/* Should have reached the end of the string. */
	if (*p)
		goto bad;

	return;

bad:
	aprint_error("Error parsing virtio_mmio parameter: %s\n", arg);
}

static void
virtio_mmio_cmdline_attach(device_t parent, device_t self, void *aux)
{
	struct virtio_mmio_cmdline_softc *sc = device_private(self);
	struct pv_attach_args *pvaa = aux;
	struct mmio_args *margs = &sc->margs;
	int keylen = strlen(VMMIOSTR);
	char *next;
	static char cmdline[LINE_MAX], *parg = NULL;

	aprint_normal("\n");
	aprint_naive("\n");

	if (parg == NULL) { /* first pass */
		strlcpy(cmdline, xen_start_info.cmd_line, sizeof(cmdline));
		aprint_verbose_dev(self, "kernel parameters: %s\n",
		    cmdline);
		parg = strstr(cmdline, VMMIOSTR);
	}

	if (parg != NULL) {
		parg += keylen;
		if (!*parg)
			return;

		next = parg;
		while (*next && *next != ' ') /* find end of argument */
			next++;
		if (*next) { /* space */
			*next++ = '\0'; /* end the argument string */
			next = strstr(next, VMMIOSTR);
		}

		aprint_normal_dev(self, "viommio: %s\n", parg);
		parsearg(margs, parg);

		if (virtio_mmio_cmdline_do_attach(self, pvaa, margs))
			return;

		if (next) {
			parg = next;
			config_found(parent, pvaa, NULL, CFARGS_NONE);
		}
	}
}

static int
virtio_mmio_cmdline_do_attach(device_t self,
    struct pv_attach_args *pvaa,
    struct mmio_args *margs)
{
	struct virtio_mmio_cmdline_softc *sc = device_private(self);
	struct virtio_mmio_softc *const msc = &sc->sc_msc;
	struct virtio_softc *const vsc = &msc->sc_sc;
	int error;

	msc->sc_iot = pvaa->pvaa_memt;
	vsc->sc_dmat = pvaa->pvaa_dmat;
	msc->sc_iosize = margs->sz;
	vsc->sc_dev = self;

	error = bus_space_map(msc->sc_iot, margs->baseaddr, margs->sz, 0,
	    &msc->sc_ioh);
	if (error) {
		aprint_error_dev(self, "couldn't map %#" PRIx64 ": %d",
		    margs->baseaddr, error);
		return error;
	}

	msc->sc_alloc_interrupts = virtio_mmio_cmdline_alloc_interrupts;
	msc->sc_free_interrupts = virtio_mmio_cmdline_free_interrupts;

	virtio_mmio_common_attach(msc);
	virtio_mmio_cmdline_rescan(self, "virtio", NULL);

	return 0;
}

static int
virtio_mmio_cmdline_detach(device_t self, int flags)
{
	struct virtio_mmio_cmdline_softc * const sc = device_private(self);
	struct virtio_mmio_softc * const msc = &sc->sc_msc;

	return virtio_mmio_common_detach(msc, flags);
}

static int
virtio_mmio_cmdline_rescan(device_t self, const char *ifattr, const int *locs)
{
	struct virtio_mmio_cmdline_softc *const sc = device_private(self);
	struct virtio_mmio_softc *const msc = &sc->sc_msc;
	struct virtio_softc *const vsc = &msc->sc_sc;
	struct virtio_attach_args va;

	if (vsc->sc_child)
		return 0;

	memset(&va, 0, sizeof(va));
	va.sc_childdevid = vsc->sc_childdevid;

	config_found(self, &va, NULL, CFARGS_NONE);

	if (virtio_attach_failed(vsc))
		return 0;

	return 0;
}


static int
virtio_mmio_cmdline_alloc_interrupts(struct virtio_mmio_softc *msc)
{
	struct virtio_mmio_cmdline_softc *const sc =
	    (struct virtio_mmio_cmdline_softc *)msc;
	struct virtio_softc *const vsc = &msc->sc_sc;
	struct ioapic_softc *ioapic;
	struct pic *pic;
	int irq = sc->margs.irq;
	int pin = irq;
	bool mpsafe;

	ioapic = ioapic_find_bybase(irq);

	if (ioapic != NULL) {
		KASSERT(ioapic->sc_pic.pic_type == PIC_IOAPIC);
		pic = &ioapic->sc_pic;
		pin = irq - pic->pic_vecbase;
		irq = -1;
	} else
		pic = &i8259_pic;

	mpsafe = (0 != (vsc->sc_flags & VIRTIO_F_INTR_MPSAFE));

	msc->sc_ih = intr_establish_xname(irq, pic, pin, IST_LEVEL, vsc->sc_ipl,
	    virtio_mmio_intr, msc, mpsafe, device_xname(vsc->sc_dev));
	if (msc->sc_ih == NULL) {
		aprint_error_dev(vsc->sc_dev,
		    "failed to establish interrupt\n");
		return -1;
	}
	aprint_normal_dev(vsc->sc_dev, "interrupting on %d\n", irq);

	return 0;
}

static void
virtio_mmio_cmdline_free_interrupts(struct virtio_mmio_softc *msc)
{
	if (msc->sc_ih != NULL) {
		intr_disestablish(msc->sc_ih);
		msc->sc_ih = NULL;
	}
}

