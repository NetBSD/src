/* 	$NetBSD: ioapic.c,v 1.38.10.2 2009/05/31 14:32:34 jym Exp $	*/

/*-
 * Copyright (c) 2000, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by RedBack Networks Inc, and by Andrew Doran.
 *
 * Author: Bill Sommerfeld
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
 * Copyright (c) 1999 Stefan Grefen
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
 *      This product includes software developed by the NetBSD 
 *      Foundation, Inc. and its contributors.  
 * 4. Neither the name of The NetBSD Foundation nor the names of its 
 *    contributors may be used to endorse or promote products derived  
 *    from this software without specific prior written permission.   
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR AND CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ioapic.c,v 1.38.10.2 2009/05/31 14:32:34 jym Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <machine/isa_machdep.h> /* XXX intrhand */
#include <machine/i82093reg.h>
#include <machine/i82093var.h>
#include <machine/i82489reg.h>
#include <machine/i82489var.h>
#include <machine/mpbiosvar.h>
#include <machine/pio.h>
#include <machine/pmap.h>
#include <machine/lock.h>

#include "acpi.h"
#include "opt_mpbios.h"
#include "opt_acpi.h"

#if !defined(MPBIOS) && NACPI == 0
#error "ioapic needs at least one of the MPBIOS or ACPI options"
#endif

/*
 * XXX locking
 */

int     ioapic_match(device_t, cfdata_t, void *);
void    ioapic_attach(device_t, device_t, void *);

extern int x86_mem_add_mapping(bus_addr_t, bus_size_t,
    int, bus_space_handle_t *); /* XXX XXX */

void ioapic_hwmask(struct pic *, int);
void ioapic_hwunmask(struct pic *, int);
bool ioapic_trymask(struct pic *, int);
static void ioapic_addroute(struct pic *, struct cpu_info *, int, int, int);
static void ioapic_delroute(struct pic *, struct cpu_info *, int, int, int);

int apic_verbose = 0;

struct ioapic_softc *ioapics;	 /* head of linked list */
int nioapics = 0;	   	 /* number attached */
static int ioapic_vecbase;

static inline u_long
ioapic_lock(struct ioapic_softc *sc)
{
	u_long flags;

	flags = x86_read_psl();
	x86_disable_intr();
	__cpu_simple_lock(&sc->sc_pic.pic_lock);
	return flags;
}

static inline void
ioapic_unlock(struct ioapic_softc *sc, u_long flags)
{
	__cpu_simple_unlock(&sc->sc_pic.pic_lock);
	x86_write_psl(flags);
}

#ifndef _IOAPIC_CUSTOM_RW
/*
 * Register read/write routines.
 */
static inline  uint32_t
ioapic_read_ul(struct ioapic_softc *sc,int regid)
{
	uint32_t val;
	
	*(sc->sc_reg) = regid;
	val = *sc->sc_data;

	return val;
	
}

static inline  void
ioapic_write_ul(struct ioapic_softc *sc,int regid, uint32_t val)
{
	*(sc->sc_reg) = regid;
	*(sc->sc_data) = val;
}
#endif /* !_IOAPIC_CUSTOM_RW */

static inline uint32_t
ioapic_read(struct ioapic_softc *sc, int regid)
{
	uint32_t val;
	u_long flags;

	flags = ioapic_lock(sc);
	val = ioapic_read_ul(sc, regid);
	ioapic_unlock(sc, flags);
	return val;
}

static inline  void
ioapic_write(struct ioapic_softc *sc,int regid, int val)
{
	u_long flags;

	flags = ioapic_lock(sc);
	ioapic_write_ul(sc, regid, val);
	ioapic_unlock(sc, flags);
}

struct ioapic_softc *
ioapic_find(int apicid)
{
	struct ioapic_softc *sc;

	if (apicid == MPS_ALL_APICS) {	/* XXX mpbios-specific */
		/*
		 * XXX kludge for all-ioapics interrupt support
		 * on single ioapic systems
		 */
		if (nioapics <= 1)
			return ioapics;
		panic("unsupported: all-ioapics interrupt with >1 ioapic");
	}

	for (sc = ioapics; sc != NULL; sc = sc->sc_next)
		if (sc->sc_pic.pic_apicid == apicid)
			return sc;

	return NULL;
}

/*
 * For the case the I/O APICs were configured using ACPI, there must
 * be an option to match global ACPI interrupts with APICs.
 */
struct ioapic_softc *
ioapic_find_bybase(int vec)
{
	struct ioapic_softc *sc;

	for (sc = ioapics; sc != NULL; sc = sc->sc_next) {
		if (vec >= sc->sc_pic.pic_vecbase &&
		    vec < (sc->sc_pic.pic_vecbase + sc->sc_apic_sz))
			return sc;
	}

	return NULL;
}

static inline void
ioapic_add(struct ioapic_softc *sc)
{
	struct ioapic_softc **scp;

	sc->sc_next = NULL;

	for (scp = &ioapics; *scp != NULL; scp = &(*scp)->sc_next)
		;
	*scp = sc;
	nioapics++;
}

void
ioapic_print_redir (struct ioapic_softc *sc, const char *why, int pin)
{
	uint32_t redirlo = ioapic_read(sc, IOAPIC_REDLO(pin));
	uint32_t redirhi = ioapic_read(sc, IOAPIC_REDHI(pin));

	apic_format_redir(device_xname(sc->sc_dev), why, pin, redirhi,
	    redirlo);
}

CFATTACH_DECL_NEW(ioapic, sizeof(struct ioapic_softc),
    ioapic_match, ioapic_attach, NULL, NULL);

int
ioapic_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

/*
 * can't use bus_space_xxx as we don't have a bus handle ...
 */
void 
ioapic_attach(device_t parent, device_t self, void *aux)
{
	struct ioapic_softc *sc = device_private(self);  
	struct apic_attach_args *aaa = (struct apic_attach_args *)aux;
	int apic_id;
	uint32_t ver_sz;
	int i;
	
	sc->sc_dev = self;
	sc->sc_flags = aaa->flags;
	sc->sc_pic.pic_apicid = aaa->apic_id;
	sc->sc_pic.pic_name = device_xname(self);
	sc->sc_pic.pic_ioapic = sc;

	aprint_naive("\n");

	if (ioapic_find(aaa->apic_id) != NULL) {
		aprint_error(": duplicate apic id (ignored)\n");
		return;
	}

	ioapic_add(sc);

	aprint_verbose(": pa 0x%jx", (uintmax_t)aaa->apic_address);
#ifndef _IOAPIC_CUSTOM_RW
	{
	bus_space_handle_t bh;

	if (x86_mem_add_mapping(aaa->apic_address, PAGE_SIZE, 0, &bh) != 0) {
		aprint_error(": map failed\n");
		return;
	}
	sc->sc_reg = (volatile uint32_t *)(bh + IOAPIC_REG);
	sc->sc_data = (volatile uint32_t *)(bh + IOAPIC_DATA);	
	}
#endif
	sc->sc_pa = aaa->apic_address;

	sc->sc_pic.pic_type = PIC_IOAPIC;
	__cpu_simple_lock_init(&sc->sc_pic.pic_lock);
	sc->sc_pic.pic_hwmask = ioapic_hwmask;
	sc->sc_pic.pic_hwunmask = ioapic_hwunmask;
	sc->sc_pic.pic_addroute = ioapic_addroute;
	sc->sc_pic.pic_delroute = ioapic_delroute;
	sc->sc_pic.pic_trymask = ioapic_trymask;
	sc->sc_pic.pic_edge_stubs = ioapic_edge_stubs;
	sc->sc_pic.pic_level_stubs = ioapic_level_stubs;

	apic_id = (ioapic_read(sc,IOAPIC_ID)&IOAPIC_ID_MASK)>>IOAPIC_ID_SHIFT;
	ver_sz = ioapic_read(sc, IOAPIC_VER);
	
	sc->sc_apic_vers = (ver_sz & IOAPIC_VER_MASK) >> IOAPIC_VER_SHIFT;
	sc->sc_apic_sz = (ver_sz & IOAPIC_MAX_MASK) >> IOAPIC_MAX_SHIFT;
	sc->sc_apic_sz++;

	if (aaa->apic_vecbase != -1)
		sc->sc_pic.pic_vecbase = aaa->apic_vecbase;
	else {
		/*
		 * XXX this assumes ordering of ioapics in the table.
		 * Only needed for broken BIOS workaround (see mpbios.c)
		 */
		sc->sc_pic.pic_vecbase = ioapic_vecbase;
		ioapic_vecbase += sc->sc_apic_sz;
	}

	if (mp_verbose) {
		printf(", %s mode",
		    aaa->flags & IOAPIC_PICMODE ? "PIC" : "virtual wire");
	}
	
	aprint_verbose(", version %x, %d pins", sc->sc_apic_vers,
	    sc->sc_apic_sz);
	aprint_normal("\n");

	sc->sc_pins = malloc(sizeof(struct ioapic_pin) * sc->sc_apic_sz,
	    M_DEVBUF, M_WAITOK);

	for (i=0; i<sc->sc_apic_sz; i++) {
		uint32_t redlo, redhi;

		sc->sc_pins[i].ip_next = NULL;
		sc->sc_pins[i].ip_map = NULL;
		sc->sc_pins[i].ip_vector = 0;
		sc->sc_pins[i].ip_type = IST_NONE;

		/* Mask all pins by default. */
		redlo = IOAPIC_REDLO_MASK;
		/*
		 * ISA interrupts are connect to pin 0-15 and
		 * edge triggered on high, which is the default.
		 *
		 * Expect all other interrupts to be PCI-like
		 * level triggered on low.
		 */  
		if (i >= 16)
			redlo |= IOAPIC_REDLO_LEVEL | IOAPIC_REDLO_ACTLO;
		redhi = (cpu_info_primary.ci_cpuid << IOAPIC_REDHI_DEST_SHIFT);
		ioapic_write(sc, IOAPIC_REDHI(i), redhi);
		ioapic_write(sc, IOAPIC_REDLO(i), redlo);
	}
	
	/*
	 * In case the APIC is not initialized to the correct ID
	 * do it now.
	 * Maybe we should record the original ID for interrupt
	 * mapping later ...
	 */
	if (apic_id != sc->sc_pic.pic_apicid) {
		aprint_debug_dev(sc->sc_dev, "misconfigured as apic %d\n",
				 apic_id);

		ioapic_write(sc,IOAPIC_ID,
		    (ioapic_read(sc,IOAPIC_ID)&~IOAPIC_ID_MASK)
		    |(sc->sc_pic.pic_apicid<<IOAPIC_ID_SHIFT));
		
		apic_id = (ioapic_read(sc,IOAPIC_ID)&IOAPIC_ID_MASK)>>IOAPIC_ID_SHIFT;
		
		if (apic_id != sc->sc_pic.pic_apicid) {
			aprint_error_dev(sc->sc_dev, "can't remap to apid %d\n",
			    sc->sc_pic.pic_apicid);
		} else {
			aprint_debug_dev(sc->sc_dev, "remapped to apic %d\n",
			    sc->sc_pic.pic_apicid);
		}
	}

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

#if 0
	/* output of this was boring. */
	if (mp_verbose)
		for (i=0; i<sc->sc_apic_sz; i++)
			ioapic_print_redir(sc, "boot", i);
#endif
}

static void
apic_set_redir(struct ioapic_softc *sc, int pin, int idt_vec,
	       struct cpu_info *ci)
{
	uint32_t redlo;
	uint32_t redhi;
	int delmode;
	struct ioapic_pin *pp;
	struct mp_intr_map *map;
	
	pp = &sc->sc_pins[pin];
	map = pp->ip_map;
	redlo = map == NULL ? IOAPIC_REDLO_MASK : map->redir;
	redhi = 0;
	delmode = (redlo & IOAPIC_REDLO_DEL_MASK) >> IOAPIC_REDLO_DEL_SHIFT;

	if (delmode == IOAPIC_REDLO_DEL_FIXED ||
	    delmode == IOAPIC_REDLO_DEL_LOPRI) {
	    	if (pp->ip_type == IST_NONE) {
			redlo |= IOAPIC_REDLO_MASK;
		} else {
			redhi = (ci->ci_cpuid << IOAPIC_REDHI_DEST_SHIFT);
			redlo |= (idt_vec & 0xff);
			redlo |= (IOAPIC_REDLO_DEL_FIXED<<IOAPIC_REDLO_DEL_SHIFT);
			redlo &= ~IOAPIC_REDLO_DSTMOD;

			/* XXX derive this bit from BIOS info */
			if (pp->ip_type == IST_LEVEL)
				redlo |= IOAPIC_REDLO_LEVEL;
			else
				redlo &= ~IOAPIC_REDLO_LEVEL;
			if (map != NULL && ((map->flags & 3) == MPS_INTPO_DEF)) {
				if (pp->ip_type == IST_LEVEL)
					redlo |= IOAPIC_REDLO_ACTLO;
				else
					redlo &= ~IOAPIC_REDLO_ACTLO;
			}
		}
	}
	ioapic_write(sc, IOAPIC_REDHI(pin), redhi);
	ioapic_write(sc, IOAPIC_REDLO(pin), redlo);
	if (mp_verbose)
		ioapic_print_redir(sc, "int", pin);
}

/*
 * Throw the switch and enable interrupts..
 */

void
ioapic_enable(void)
{
	if (ioapics == NULL)
		return;

	if (ioapics->sc_flags & IOAPIC_PICMODE) {
		aprint_debug_dev(ioapics->sc_dev,
				 "writing to IMCR to disable pics\n");
		outb(IMCR_ADDR, IMCR_REGISTER);
		outb(IMCR_DATA, IMCR_APIC);
	}
}

void
ioapic_reenable(void)
{
	int p, apic_id;
	struct ioapic_softc *sc;

	if (ioapics == NULL)
		return;

	aprint_normal("%s reenabling\n", device_xname(ioapics->sc_dev));

	for (sc = ioapics; sc != NULL; sc = sc->sc_next) {
		apic_id = (ioapic_read(sc,IOAPIC_ID)&IOAPIC_ID_MASK)>>IOAPIC_ID_SHIFT;
		if (apic_id != sc->sc_pic.pic_apicid) {
			ioapic_write(sc,IOAPIC_ID,
			    (ioapic_read(sc,IOAPIC_ID)&~IOAPIC_ID_MASK)
			    |(sc->sc_pic.pic_apicid<<IOAPIC_ID_SHIFT));
		}

		for (p = 0; p < sc->sc_apic_sz; p++) {
			apic_set_redir(sc, p, sc->sc_pins[p].ip_vector,
				    sc->sc_pins[p].ip_cpu);
		}
	}

	ioapic_enable();
}

void
ioapic_hwmask(struct pic *pic, int pin)
{
	uint32_t redlo;
	struct ioapic_softc *sc = pic->pic_ioapic;
	u_long flags;

	flags = ioapic_lock(sc);
	redlo = ioapic_read_ul(sc, IOAPIC_REDLO(pin));
	redlo |= IOAPIC_REDLO_MASK;
	ioapic_write_ul(sc, IOAPIC_REDLO(pin), redlo);
	ioapic_unlock(sc, flags);
}

bool
ioapic_trymask(struct pic *pic, int pin)
{
	uint32_t redlo;
	struct ioapic_softc *sc = pic->pic_ioapic;
	u_long flags;
	bool rv;

	/* Mask it. */
	flags = ioapic_lock(sc);
	redlo = ioapic_read_ul(sc, IOAPIC_REDLO(pin));
	redlo |= IOAPIC_REDLO_MASK;
	ioapic_write_ul(sc, IOAPIC_REDLO(pin), redlo);

	/* If pending, unmask and abort. */
	redlo = ioapic_read_ul(sc, IOAPIC_REDLO(pin));
	if ((redlo & (IOAPIC_REDLO_RIRR|IOAPIC_REDLO_DELSTS)) != 0) {
		redlo &= ~IOAPIC_REDLO_MASK;
		ioapic_write_ul(sc, IOAPIC_REDLO(pin), redlo);
		rv = false;
	} else {
		rv = true;
	}
	ioapic_unlock(sc, flags);
	return rv;
}

void
ioapic_hwunmask(struct pic *pic, int pin)
{
	uint32_t redlo;
	struct ioapic_softc *sc = pic->pic_ioapic;
	u_long flags;

	flags = ioapic_lock(sc);
	redlo = ioapic_read_ul(sc, IOAPIC_REDLO(pin));
	redlo &= ~IOAPIC_REDLO_MASK;
	ioapic_write_ul(sc, IOAPIC_REDLO(pin), redlo);
	ioapic_unlock(sc, flags);
}

static void
ioapic_addroute(struct pic *pic, struct cpu_info *ci, int pin,
		int idtvec, int type)

{
	struct ioapic_softc *sc = pic->pic_ioapic;
	struct ioapic_pin *pp;

	pp = &sc->sc_pins[pin];
	pp->ip_type = type;
	pp->ip_vector = idtvec;
	pp->ip_cpu = ci;
	apic_set_redir(sc, pin, idtvec, ci);
}

static void
ioapic_delroute(struct pic *pic, struct cpu_info *ci, int pin,
    int idtvec, int type)
{
	ioapic_hwmask(pic, pin);
}

#ifdef DDB
void ioapic_dump(void);
void ioapic_dump_raw(void);

void
ioapic_dump(void)
{
	struct ioapic_softc *sc;
	struct ioapic_pin *ip;
	int p;

	for (sc = ioapics; sc != NULL; sc = sc->sc_next) {
		for (p = 0; p < sc->sc_apic_sz; p++) {
			ip = &sc->sc_pins[p];
			if (ip->ip_type != IST_NONE)
				ioapic_print_redir(sc, "dump", p);
		}
	}
}

void
ioapic_dump_raw(void)
{
	struct ioapic_softc *sc;
	int i;
	uint32_t reg;

	for (sc = ioapics; sc != NULL; sc = sc->sc_next) {
		printf("Register dump of %s\n", device_xname(sc->sc_dev));
		i = 0;
		do {
			if (i % 0x08 == 0)
				printf("%02x", i);
			reg = ioapic_read(sc, i);
			printf(" %08x", (u_int)reg);
			if (++i % 0x08 == 0)
				printf("\n");
		} while (i < 0x40);
	}
}
#endif
