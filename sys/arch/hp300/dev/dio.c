/*	$NetBSD: dio.c,v 1.32 2005/12/24 20:07:03 perry Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
 * Autoconfiguration and mapping support for the DIO bus.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dio.c,v 1.32 2005/12/24 20:07:03 perry Exp $");

#define	_HP300_INTR_H_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/hp300spu.h>

#include <hp300/dev/dmavar.h>

#include <hp300/dev/dioreg.h>
#include <hp300/dev/diovar.h>

#include <hp300/dev/diodevs.h>
#include <hp300/dev/diodevs_data.h>

#include "locators.h"
#define        diocf_scode             cf_loc[DIOCF_SCODE]

struct dio_softc {
	struct device sc_dev;
	struct bus_space_tag sc_tag;
};

static int	dio_scodesize(struct dio_attach_args *);
static const char *dio_devinfo(struct dio_attach_args *, char *, size_t);

static int	diomatch(struct device *, struct cfdata *, void *);
static void	dioattach(struct device *, struct device *, void *);
static int	dioprint(void *, const char *);
static int	diosubmatch(struct device *, struct cfdata *, const int *, void *);

CFATTACH_DECL(dio, sizeof(struct dio_softc),
    diomatch, dioattach, NULL, NULL);

static int
diomatch(struct device *parent, struct cfdata *match, void *aux)
{
	static int dio_matched = 0;

	/* Allow only one instance. */
	if (dio_matched)
		return (0);

	dio_matched = 1;
	return (1);
}

static void
dioattach(struct device *parent, struct device *self, void *aux)
{
	struct dio_softc *sc = (struct dio_softc *)self;
	struct dio_attach_args da;
	caddr_t pa, va;
	bus_space_tag_t bst = &sc->sc_tag;
	int scode, scmax, scodesize;

	printf("\n");

	memset(bst, 0, sizeof(struct bus_space_tag));
	bst->bustype = HP300_BUS_SPACE_DIO;

	scmax = DIO_SCMAX(machineid);

	for (scode = 0; scode < scmax; ) {
		if (DIO_INHOLE(scode)) {
			scode++;
			continue;
		}

		/*
		 * Temporarily map the space corresponding to
		 * the current select code unless:
		 */
		pa = dio_scodetopa(scode);
		va = iomap(pa, PAGE_SIZE);
		if (va == NULL) {
			printf("%s: can't map scode %d\n",
			    self->dv_xname, scode);
			scode++;
			continue;
		}

		/* Check for hardware. */
		if (badaddr(va)) {
			iounmap(va, PAGE_SIZE);
			scode++;
			continue;
		}

		/* Fill out attach args. */
		memset(&da, 0, sizeof(da));
		da.da_bst = bst;
		da.da_scode = scode;

		da.da_id = DIO_ID(va);
		if (DIO_ISFRAMEBUFFER(da.da_id))
			da.da_secid = DIO_SECID(va);
		da.da_addr = (bus_addr_t)dio_scodetopa(scode);
		da.da_size = DIO_SIZE(scode, va);
		scodesize = dio_scodesize(&da);
		if (DIO_ISDIO(scode))
			da.da_size *= scodesize;
		da.da_ipl = DIO_IPL(va);

		/* No longer need the device to be mapped. */
		iounmap(va, PAGE_SIZE);

		/* Attach matching device. */
		config_found_sm_loc(self, "dio", NULL, &da, dioprint,
				    diosubmatch);
		scode += scodesize;
	}
}

static int
diosubmatch(struct device *parent, struct cfdata *cf,
	    const int *ldesc, void *aux)
{
	struct dio_attach_args *da = aux;

	if (cf->diocf_scode != DIOCF_SCODE_DEFAULT &&
	    cf->diocf_scode != da->da_scode)
		return (0);

	return (config_match(parent, cf, aux));
}

static int
dioprint(void *aux, const char *pnp)
{
	struct dio_attach_args *da = aux;
	char buf[128];

	if (pnp)
		aprint_normal("%s at %s",
		    dio_devinfo(da, buf, sizeof(buf)), pnp);
	aprint_normal(" scode %d ipl %d", da->da_scode, da->da_ipl);
	return (UNCONF);
}

/*
 * Convert a select code to a system physical address.
 */
void *
dio_scodetopa(int scode)
{
	u_long rval;

	if (DIO_ISDIO(scode))
		rval = DIO_BASE + (scode * DIO_DEVSIZE);
	else if (DIO_ISDIOII(scode))
		rval = DIOII_BASE + ((scode - DIOII_SCBASE) * DIOII_DEVSIZE);
	else
		rval = 0;

	return ((void *)rval);
}

/*
 * Return the select code size for this device, defaulting to 1
 * if we don't know what kind of device we have.
 */
static int
dio_scodesize(struct dio_attach_args *da)
{
	int i;

	/*
	 * Find the dio_devdata matchind the primary id.
	 * If we're a framebuffer, we also check the secondary id.
	 */
	for (i = 0; i < DIO_NDEVICES; i++) {
		if (da->da_id == dio_devdatas[i].dd_id) {
			if (DIO_ISFRAMEBUFFER(da->da_id)) {
				if (da->da_secid == dio_devdatas[i].dd_secid) {
					goto foundit;
				}
			} else {
			foundit:
				return (dio_devdatas[i].dd_nscode);
			}
		}
	}

	/*
	 * Device is unknown.  Print a warning and assume a default.
	 */
	printf("WARNING: select code size unknown for id = 0x%x secid = 0x%x\n",
	    da->da_id, da->da_secid);
	return (1);
}

/*
 * Return a reasonable description of a DIO device.
 */
static const char *
dio_devinfo(struct dio_attach_args *da, char *buf, size_t buflen)
{
#ifdef DIOVERBOSE
	int i;
#endif

	memset(buf, 0, buflen);

#ifdef DIOVERBOSE
	/*
	 * Find the description matching our primary id.
	 * If we're a framebuffer, we also check the secondary id.
	 */
	for (i = 0; i < DIO_NDEVICES; i++) {
		if (da->da_id == dio_devdescs[i].dd_id) {
			if (DIO_ISFRAMEBUFFER(da->da_id)) {
				if (da->da_secid == dio_devdescs[i].dd_secid) {
					goto foundit;
				}
			} else {
			foundit:
				sprintf(buf, "%s", dio_devdescs[i].dd_desc);
				return (buf);
			}
		}
	}
#endif /* DIOVERBOSE */

	/*
	 * Device is unknown.  Construct something reasonable.
	 */
	sprintf(buf, "device id = 0x%x secid = 0x%x",
	    da->da_id, da->da_secid);
	return (buf);
}

/*
 * Establish an interrupt handler for a DIO device.
 */
void *
dio_intr_establish(int (*func)(void *), void *arg, int ipl, int priority)
{
	void *ih;

	ih = intr_establish(func, arg, ipl, priority);

	if (priority == IPL_BIO)
		dmacomputeipl();

	return (ih);
}

/*
 * Remove an interrupt handler for a DIO device.
 */
void
dio_intr_disestablish(void *arg)
{
	struct hp300_intrhand *ih = arg;
	int priority = ih->ih_priority;

	intr_disestablish(arg);

	if (priority == IPL_BIO)
		dmacomputeipl();
}

/*
 * DIO specific bus_space(9) support functions.
 */
static uint8_t dio_bus_space_read_oddbyte_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t);
static void dio_bus_space_write_oddbyte_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint8_t);

static void dio_bus_space_read_multi_oddbyte_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint8_t *, bus_size_t);
static void dio_bus_space_write_multi_oddbyte_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, const uint8_t *, bus_size_t);

static void dio_bus_space_read_region_oddbyte_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint8_t *, bus_size_t);
static void dio_bus_space_write_region_oddbyte_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, const uint8_t *, bus_size_t);

static void dio_bus_space_set_multi_oddbyte_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint8_t, bus_size_t);

static void dio_bus_space_set_region_oddbyte_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint8_t, bus_size_t);

/*
 * dio_set_bus_space_oddbyte():
 *	Override bus_space functions in bus_space_tag_t
 *	for devices which have odd byte address space.
 */
void
dio_set_bus_space_oddbyte(bus_space_tag_t bst)
{

	/* XXX only 1-byte functions for now */
	bst->bsr1 = dio_bus_space_read_oddbyte_1;
	bst->bsw1 = dio_bus_space_write_oddbyte_1;

	bst->bsrm1 = dio_bus_space_read_multi_oddbyte_1;
	bst->bswm1 = dio_bus_space_write_multi_oddbyte_1;

	bst->bsrr1 = dio_bus_space_read_region_oddbyte_1;
	bst->bswr1 = dio_bus_space_write_region_oddbyte_1;

	bst->bssm1 = dio_bus_space_set_multi_oddbyte_1;

	bst->bssr1 = dio_bus_space_set_region_oddbyte_1;
}

static uint8_t
dio_bus_space_read_oddbyte_1(bus_space_tag_t bst, bus_space_handle_t bsh,
    bus_size_t offset)
{

	return *(volatile uint8_t *)(bsh + (offset << 1) + 1);
}

static void 
dio_bus_space_write_oddbyte_1(bus_space_tag_t bst, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t val)
{

	*(volatile uint8_t *)(bsh + (offset << 1) + 1) = val;
}

static void
dio_bus_space_read_multi_oddbyte_1(bus_space_tag_t bst, bus_space_handle_t bsh,
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
	    : "r" (bsh + (offset << 1) + 1), "g" (addr), "g" (len)
	    : "%a0","%a1","%d0");
}

static void
dio_bus_space_write_multi_oddbyte_1(bus_space_tag_t bst, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *addr, bus_size_t len)
{

	__asm volatile (
	"	movl	%0,%%a0		;\n"
	"	movl	%1,%%a1		;\n"
	"	movl	%2,%%d0		;\n"
	"1:	movb	%%a1@+,%%a0@	;\n"
	"	subql	#1,%%d0		;\n"
	"	jne	1b"
	    :
	    : "r" (bsh + (offset << 1) + 1), "g" (addr), "g" (len)
	    : "%a0","%a1","%d0");
}

static void
dio_bus_space_read_region_oddbyte_1(bus_space_tag_t bst, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t len)
{
	__asm volatile (
	"	movl	%0,%%a0		;\n"
	"	movl	%1,%%a1		;\n"
	"	movl	%2,%%d0		;\n"
	"1:	movb	%%a0@,%%a1@+	;\n"
	"	addql	#2,%%a0		;\n"
	"	subql	#1,%%d0		;\n"
	"	jne	1b"
	    :
	    : "r" (bsh + (offset << 1) + 1), "g" (addr), "g" (len)
	    : "%a0","%a1","%d0");
}

static void
dio_bus_space_write_region_oddbyte_1(bus_space_tag_t bst,
    bus_space_handle_t bsh, bus_size_t offset, const uint8_t *addr,
    bus_size_t len)
{

	__asm volatile (
	"	movl	%0,%%a0		;\n"
	"	movl	%1,%%a1		;\n"
	"	movl	%2,%%d0		;\n"
	"1:	movb	%%a1@+,%%a0@	;\n"
	"	addql	#2,%%a0		;\n"
	"	subql	#1,%%d0		;\n"
	"	jne	1b"
	    :
	    : "r" (bsh + (offset << 1) + 1), "g" (addr), "g" (len)
	    : "%a0","%a1","%d0");
}

static void
dio_bus_space_set_multi_oddbyte_1(bus_space_tag_t bst, bus_space_handle_t bsh,
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
	    : "r" (bsh + (offset << 1) + 1), "g" (val), "g" (count)
	    : "%a0","%d0","%d1");
}

static void
dio_bus_space_set_region_oddbyte_1(bus_space_tag_t bst, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t val, bus_size_t count)
{

	__asm volatile (
	"	movl	%0,%%a0		;\n"
	"	movl	%1,%%d1		;\n"
	"	movl	%2,%%d0		;\n"
	"1:	movb	%%d1,%%a0@	;\n"
	"	addql	#2,%%a0		;\n"
	"	subql	#1,%%d0		;\n"
	"	jne	1b"
	    :
	    : "r" (bsh + (offset << 1) + 1), "g" (val), "g" (count)
	    : "%a0","%d0","%d1");
}
