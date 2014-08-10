/*	$NetBSD: frodo.c,v 1.30.28.1 2014/08/10 06:53:57 tls Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 1999 The NetBSD Foundation, Inc.
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
 * Copyright (c) 1997 Michael Smith.  All rights reserved.
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

/*
 * Support for the "Frodo" (a.k.a. "Apollo Utility") chip found
 * in HP Apollo 9000/4xx workstations, as well as 9000/382 controllers.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: frodo.c,v 1.30.28.1 2014/08/10 06:53:57 tls Exp $");

#define	_HP300_INTR_H_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/hp300spu.h>

#include <hp300/dev/intiovar.h>

#include <hp300/dev/frodoreg.h>
#include <hp300/dev/frodovar.h>

/*
 * Description of a Frodo interrupt handler.
 */
struct frodo_interhand {
	int	(*ih_fn)(void *);
	void	*ih_arg;
	int	ih_priority;
};

struct frodo_softc {
	device_t	sc_dev;		/* generic device glue */
	volatile uint8_t *sc_regs;	/* register base */
	struct frodo_interhand sc_intr[FRODO_NINTR]; /* interrupt handlers */
	int		sc_ipl;
	void		*sc_ih;		/* out interrupt cookie */
	int		sc_refcnt;	/* number of interrupt refs */
	struct bus_space_tag sc_tag;	/* bus space tag */
};

static int	frodomatch(device_t, cfdata_t, void *);
static void	frodoattach(device_t, device_t, void *);

static int	frodoprint(void *, const char *);
static int	frodosubmatch(device_t, cfdata_t, const int *, void *);

static int	frodointr(void *);

static void	frodo_imask(struct frodo_softc *, uint16_t, uint16_t);

CFATTACH_DECL_NEW(frodo, sizeof(struct frodo_softc),
    frodomatch, frodoattach, NULL, NULL);

static const struct frodo_device frodo_subdevs[] = {
	{ "dnkbd",	FRODO_APCI_OFFSET(0),	FRODO_INTR_APCI0 },
	{ "com",	FRODO_APCI_OFFSET(1),	FRODO_INTR_APCI1 },
	{ "com",	FRODO_APCI_OFFSET(2),	FRODO_INTR_APCI2 },
	{ "com",	FRODO_APCI_OFFSET(3),	FRODO_INTR_APCI3 },
	{ "mcclock",	FRODO_CALENDAR,		FRODO_INTR_CALENDAR },
	{ NULL,		0,			0 }
};

static int
frodomatch(device_t parent, cfdata_t cf, void *aux)
{
	struct intio_attach_args *ia = aux;
	static int frodo_matched = 0;

	/* only allow one instance */
	if (frodo_matched)
		return 0;

	if (strcmp(ia->ia_modname, "frodo") != 0)
		return 0;

	if (badaddr((void *)ia->ia_addr))
		return 0;

	frodo_matched = 1;
	return 1;
}

static void
frodoattach(device_t parent, device_t self, void *aux)
{
	struct frodo_softc *sc = device_private(self);
	struct intio_attach_args *ia = aux;
	bus_space_tag_t bst = &sc->sc_tag;
	const struct frodo_device *fd;
	struct frodo_attach_args fa;

	sc->sc_dev = self;
	sc->sc_regs = (volatile uint8_t *)ia->ia_addr;
	sc->sc_ipl = ia->ia_ipl;

	if ((FRODO_READ(sc, FRODO_IISR) & FRODO_IISR_SERVICE) == 0)
		aprint_error(": service mode enabled");
	aprint_normal("\n");

	/* Initialize bus_space_tag_t for frodo */
	frodo_init_bus_space(bst);

	/* Clear all of the interrupt handlers. */
	memset(sc->sc_intr, 0, sizeof(sc->sc_intr));
	sc->sc_refcnt = 0;

	/*
	 * Disable all of the interrupt lines; we reenable them
	 * as subdevices attach.
	 */
	frodo_imask(sc, 0, 0xffff);

	/* Clear any pending interrupts. */
	FRODO_WRITE(sc, FRODO_PIC_PU, 0xff);
	FRODO_WRITE(sc, FRODO_PIC_PL, 0xff);

	/* Set interrupt polarities. */
	FRODO_WRITE(sc, FRODO_PIO_IPR, 0x10);

	/* ...and configure for edge triggering. */
	FRODO_WRITE(sc, FRODO_PIO_IELR, 0xcf);

	/*
	 * We defer hooking up our interrupt handler until
	 * a subdevice hooks up theirs.
	 */
	sc->sc_ih = NULL;

	/* ... and attach subdevices. */
	for (fd = frodo_subdevs; fd->fd_name != NULL; fd++) {
		/*
		 * Skip the first serial port if we're not a 425e;
		 * it's mapped to the DCA at select code 9 on all
		 * other models.
		 */
		if (fd->fd_offset == FRODO_APCI_OFFSET(1) &&
		    mmuid != MMUID_425_E)
			continue;
		/*
		 * The mcclock is available only on a 425e.
		 */
		if (fd->fd_offset == FRODO_CALENDAR && mmuid != MMUID_425_E)
			continue;
		fa.fa_name = fd->fd_name;
		fa.fa_bst = bst;
		fa.fa_base = ia->ia_iobase;
		fa.fa_offset = fd->fd_offset;
		fa.fa_line = fd->fd_line;
		config_found_sm_loc(self, "frodo", NULL, &fa, frodoprint,
		    frodosubmatch);
	}
}

static int
frodosubmatch(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct frodo_attach_args *fa = aux;

	if (cf->frodocf_offset != FRODO_UNKNOWN_OFFSET &&
	    cf->frodocf_offset != fa->fa_offset)
		return 0;

	return config_match(parent, cf, aux);
}

static int
frodoprint(void *aux, const char *pnp)
{
	struct frodo_attach_args *fa = aux;

	if (pnp)
		aprint_normal("%s at %s", fa->fa_name, pnp);
	aprint_normal(" offset 0x%x", fa->fa_offset);
	return UNCONF;
}

void
frodo_intr_establish(device_t frdev, int (*func)(void *), void *arg,
    int line, int priority)
{
	struct frodo_softc *sc = device_private(frdev);
	struct hp300_intrhand *ih = sc->sc_ih;

	if (line < 0 || line >= FRODO_NINTR) {
		aprint_error_dev(frdev, "bad interrupt line %d\n", line);
		goto lose;
	}
	if (sc->sc_intr[line].ih_fn != NULL) {
		aprint_error_dev(frdev, "interrupt line %d already used\n",
		    line);
		goto lose;
	}

	/* Install the handler. */
	sc->sc_intr[line].ih_fn = func;
	sc->sc_intr[line].ih_arg = arg;
	sc->sc_intr[line].ih_priority = priority;

	/*
	 * If this is the first one, establish the frodo
	 * interrupt handler.  If not, reestablish at a
	 * higher priority if necessary.
	 */
	if (ih == NULL || ih->ih_priority < priority) {
		if (ih != NULL)
			intr_disestablish(ih);
		sc->sc_ih = intr_establish(frodointr, sc, sc->sc_ipl, priority);
	}

	sc->sc_refcnt++;

	/* Enable the interrupt line. */
	frodo_imask(sc, (1 << line), 0);
	return;
 lose:
	panic("frodo_intr_establish");
}

void
frodo_intr_disestablish(device_t frdev, int line)
{
	struct frodo_softc *sc = device_private(frdev);
	struct hp300_intrhand *ih = sc->sc_ih;
	int newpri;

	if (sc->sc_intr[line].ih_fn == NULL) {
		printf("%s: no handler for line %d\n",
		    device_xname(sc->sc_dev), line);
		panic("frodo_intr_disestablish");
	}

	sc->sc_intr[line].ih_fn = NULL;
	frodo_imask(sc, 0, (1 << line));

	/* If this was the last, unhook ourselves. */
	if (sc->sc_refcnt-- == 1) {
		intr_disestablish(ih);
		return;
	}

	/* Lower our priority, if appropriate. */
	for (newpri = 0, line = 0; line < FRODO_NINTR; line++)
		if (sc->sc_intr[line].ih_fn != NULL &&
		    sc->sc_intr[line].ih_priority > newpri)
			newpri = sc->sc_intr[line].ih_priority;

	if (newpri != ih->ih_priority) {
		intr_disestablish(ih);
		sc->sc_ih = intr_establish(frodointr, sc, sc->sc_ipl, newpri);
	}
}

static int
frodointr(void *arg)
{
	struct frodo_softc *sc = arg;
	struct frodo_interhand *fih;
	int line, taken = 0;

	/* Any interrupts pending? */
	if (FRODO_GETPEND(sc) == 0)
		return 0;

	do {
		/*
		 * Get pending interrupt; this also clears it for us.
		 */
		line = FRODO_IPEND(sc);
		fih = &sc->sc_intr[line];
		if (fih->ih_fn == NULL ||
		    (*fih->ih_fn)(fih->ih_arg) == 0)
			printf("%s: spurious interrupt on line %d\n",
			    device_xname(sc->sc_dev), line);
		if (taken++ > 100)
			panic("frodointr: looping!");
	} while (FRODO_GETPEND(sc) != 0);

	return 1;
}

static void
frodo_imask(struct frodo_softc *sc, uint16_t set, uint16_t clear)
{
	uint16_t imask;

	imask = FRODO_GETMASK(sc);

	imask |= set;
	imask &= ~clear;

	FRODO_SETMASK(sc, imask);
}

/*
 * frodo chip specific bus_space(9) support functions.
 */
static uint8_t frodo_bus_space_read_sparse_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t);
static void frodo_bus_space_write_sparse_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint8_t);

static void frodo_bus_space_read_multi_sparse_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint8_t *, bus_size_t);
static void frodo_bus_space_write_multi_sparse_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, const uint8_t *, bus_size_t);

static void frodo_bus_space_read_region_sparse_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint8_t *, bus_size_t);
static void frodo_bus_space_write_region_sparse_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, const uint8_t *, bus_size_t);

static void frodo_bus_space_set_multi_sparse_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint8_t, bus_size_t);

static void frodo_bus_space_set_region_sparse_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint8_t, bus_size_t);

/*
 * frodo_init_bus_space():
 *	Initialize bus_space functions in bus_space_tag_t
 *	for frodo devices which have sparse address space.
 */
void
frodo_init_bus_space(bus_space_tag_t bst)
{

	memset(bst, 0, sizeof(struct bus_space_tag));
	bst->bustype = HP300_BUS_SPACE_INTIO;

	bst->bsr1 = frodo_bus_space_read_sparse_1;
	bst->bsw1 = frodo_bus_space_write_sparse_1;

	bst->bsrm1 = frodo_bus_space_read_multi_sparse_1;
	bst->bswm1 = frodo_bus_space_write_multi_sparse_1;

	bst->bsrr1 = frodo_bus_space_read_region_sparse_1;
	bst->bswr1 = frodo_bus_space_write_region_sparse_1;

	bst->bssm1 = frodo_bus_space_set_multi_sparse_1;

	bst->bssr1 = frodo_bus_space_set_region_sparse_1;
}

static uint8_t
frodo_bus_space_read_sparse_1(bus_space_tag_t bst, bus_space_handle_t bsh,
    bus_size_t offset)
{

	return *(volatile uint8_t *)(bsh + (offset << 2));
}

static void
frodo_bus_space_write_sparse_1(bus_space_tag_t bst, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t val)
{

	*(volatile uint8_t *)(bsh + (offset << 2)) = val;
}

static void
frodo_bus_space_read_multi_sparse_1(bus_space_tag_t bst, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t len)
{

	__asm volatile (
	"	movl	%0,%%a0		;\n"
	"	movl	%1,%%a1		;\n"
	"	movl	%2,%%d0		;\n"
	"1:	movb	%%a0@,%%a1@+	;\n"
	"	subql	#1,%%d0		;\n"
	"	jne	1b"
	    :
	    : "r" (bsh + (offset << 2)), "g" (addr), "g" (len)
	    : "%a0","%a1","%d0");
}

static void
frodo_bus_space_write_multi_sparse_1(bus_space_tag_t bst,
    bus_space_handle_t bsh, bus_size_t offset, const uint8_t *addr,
    bus_size_t len)
{

	__asm volatile (
	"	movl	%0,%%a0		;\n"
	"	movl	%1,%%a1		;\n"
	"	movl	%2,%%d0		;\n"
	"1:	movb	%%a1@+,%%a0@	;\n"
	"	subql	#1,%%d0		;\n"
	"	jne	1b"
	    :
	    : "r" (bsh + (offset << 2)), "g" (addr), "g" (len)
	    : "%a0","%a1","%d0");
}

static void
frodo_bus_space_read_region_sparse_1(bus_space_tag_t bst,
    bus_space_handle_t bsh, bus_size_t offset, uint8_t *addr, bus_size_t len)
{
	__asm volatile (
	"	movl	%0,%%a0		;\n"
	"	movl	%1,%%a1		;\n"
	"	movl	%2,%%d0		;\n"
	"1:	movb	%%a0@,%%a1@+	;\n"
	"	addql	#4,%%a0		;\n"
	"	subql	#1,%%d0		;\n"
	"	jne	1b"
	    :
	    : "r" (bsh + (offset << 2)), "g" (addr), "g" (len)
	    : "%a0","%a1","%d0");
}

static void
frodo_bus_space_write_region_sparse_1(bus_space_tag_t bst,
    bus_space_handle_t bsh, bus_size_t offset, const uint8_t *addr,
    bus_size_t len)
{

	__asm volatile (
	"	movl	%0,%%a0		;\n"
	"	movl	%1,%%a1		;\n"
	"	movl	%2,%%d0		;\n"
	"1:	movb	%%a1@+,%%a0@	;\n"
	"	addql	#4,%%a0		;\n"
	"	subql	#1,%%d0		;\n"
	"	jne	1b"
	    :
	    : "r" (bsh + (offset << 2)), "g" (addr), "g" (len)
	    : "%a0","%a1","%d0");
}

static void
frodo_bus_space_set_multi_sparse_1(bus_space_tag_t bst, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t val, bus_size_t count)
{
	__asm volatile (
	"	movl	%0,%%a0		;\n"
	"	movl	%1,%%d1		;\n"
	"	movl	%2,%%d0		;\n"
	"1:	movb	%%d1,%%a0@	;\n"
	"	subql	#1,%%d0		;\n"
	"	jne	1b"
	    :
	    : "r" (bsh + (offset << 2)), "g" (val), "g" (count)
	    : "%a0","%d0","%d1");
}

static void
frodo_bus_space_set_region_sparse_1(bus_space_tag_t bst, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t val, bus_size_t count)
{

	__asm volatile (
	"	movl	%0,%%a0		;\n"
	"	movl	%1,%%d1		;\n"
	"	movl	%2,%%d0		;\n"
	"1:	movb	%%d1,%%a0@	;\n"
	"	addql	#4,%%a0		;\n"
	"	subql	#1,%%d0		;\n"
	"	jne	1b"
	    :
	    : "r" (bsh + (offset << 2)), "g" (val), "g" (count)
	    : "%a0","%d0","%d1");
}
