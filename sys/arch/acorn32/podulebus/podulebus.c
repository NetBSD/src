/* $NetBSD: podulebus.c,v 1.1.4.6 2002/06/20 03:37:20 nathanw Exp $ */

/*
 * Copyright (c) 1994-1996 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * podulebus.c
 *
 * Podule probe and configuration routines
 *
 * Created      : 07/11/94
 */

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: podulebus.c,v 1.1.4.6 2002/06/20 03:37:20 nathanw Exp $");

#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <uvm/uvm_extern.h>
#include <machine/io.h>
#include <arm/arm32/katelib.h>
#include <machine/intr.h>
#include <machine/bootconfig.h>
#include <machine/pmap.h>
#include <arm/iomd/iomdreg.h>
#include <arm/iomd/iomdvar.h>
#include <acorn32/podulebus/podulebus.h>
#include <dev/podulebus/podules.h>
#include <dev/podulebus/podule_data.h>

#include "locators.h"

/* Array of podule structures, one per possible podule */

podule_t podules[MAX_PODULES + MAX_NETSLOTS];

extern struct bus_space podulebus_bs_tag;

/* Declare prototypes */

u_int poduleread __P((u_int, int));
int podulebusmatch(struct device *, struct cfdata *, void *);
void podulebusattach(struct device *, struct device *, void *);
int podulebusprint(void *, const char *);
int podulebussubmatch(struct device *, struct cfdata *, void *);
void podulechunkdirectory(podule_t *);
void podulescan(struct device *);

/*
 * int podulebusmatch(struct device *parent, void *match, void *aux)
 *
 * Probe for the podule bus. Currently all this does is return 1 to
 * indicate that the podule bus was found.
 */
 
int
podulebusmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	switch (IOMD_ID) {
	case RPC600_IOMD_ID:
	case ARM7500_IOC_ID:
	case ARM7500FE_IOC_ID:
		return(1);
	}
	return (0);
}


int
podulebusprint(aux, name)
	void *aux;
	const char *name;
{
	struct podule_attach_args *pa = aux;

	if (name)
		printf("podule at %s", name);
	if (pa->pa_podule->slottype == SLOT_POD)
		printf(" slot %d", pa->pa_podule_number);
	else if (pa->pa_podule->slottype == SLOT_NET)
		printf(" [ netslot %d ]", pa->pa_podule_number - MAX_PODULES);
#ifdef DIAGNOSTIC
	else
		panic("Invalid slot type\n");
#endif

	return (UNCONF);
}


int
podulebussubmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct podule_attach_args *pa = aux;

	/* Return priority 0 or 1 for wildcarded podule */

	if (cf->cf_loc[PODULEBUSCF_SLOT] == PODULEBUSCF_SLOT_DEFAULT)
		return((*cf->cf_attach->ca_match)(parent, cf, aux));

	/* Return higher priority if we match the specific podule */

	else if (cf->cf_loc[PODULEBUSCF_SLOT] == pa->pa_podule_number)
		return((*cf->cf_attach->ca_match)(parent, cf, aux) * 8);

	/* Fail */
	return(0);
}


#if 0
void
dump_podule(podule)
	podule_t *podule;
{
	printf("podule%d: ", podule->podulenum);
	printf("flags0=%02x ", podule->flags0);
	printf("flags1=%02x ", podule->flags1);
	printf("reserved=%02x ", podule->reserved);
	printf("product=%02x ", podule->product);
	printf("manufacturer=%02x ", podule->manufacturer);
	printf("country=%02x ", podule->country);
	printf("irq_addr=%08x ", podule->irq_addr);
	printf("irq_mask=%02x ", podule->irq_mask);
	printf("fiq_addr=%08x ", podule->fiq_addr);
	printf("fiq_mask=%02x ", podule->fiq_mask);
	printf("fast_base=%08x ", podule->fast_base);
	printf("medium_base=%08x ", podule->medium_base);
	printf("slow_base=%08x ", podule->slow_base);
	printf("sync_base=%08x ", podule->sync_base);
	printf("mod_base=%08x ", podule->mod_base);
	printf("easi_base=%08x ", podule->easi_base);
	printf("attached=%d ", podule->attached);
	printf("slottype=%d ", podule->slottype);
	printf("podulenum=%d ", podule->podulenum);
	printf("description=%s ", podule->description);
	printf("\n");
}
#endif

void
podulechunkdirectory(podule)
	podule_t *podule;
{
	u_int address;
	u_int id;
	u_int size;
	u_int addr;
	int loop;
	int done_f5;

	done_f5 = 0;	
	address = 0x40;

	do {
		id = podule->read_rom(podule->sync_base, address);
		size = podule->read_rom(podule->sync_base, address + 4);
		size |= (podule->read_rom(podule->sync_base, address + 8) << 8);
		size |= (podule->read_rom(podule->sync_base, address + 12) << 16);
		if (id == 0xf5) {
			addr = podule->read_rom(podule->sync_base, address + 16);
			addr |= (podule->read_rom(podule->sync_base, address + 20) << 8);
			addr |= (podule->read_rom(podule->sync_base, address + 24) << 16);
			addr |= (podule->read_rom(podule->sync_base, address + 28) << 24);
			if (addr < 0x800 && done_f5 == 0) {
				done_f5 = 1;
				for (loop = 0; loop < size; ++loop) {
					if (loop < PODULE_DESCRIPTION_LENGTH) {
						podule->description[loop] =
						    podule->read_rom(podule->sync_base, (addr + loop)*4);
						podule->description[loop + 1] = 0;
					}
				}
			}
		}
#ifdef DEBUG_CHUNK_DIR
		if (id == 0xf5 || id == 0xf1 || id == 0xf2 || id == 0xf3 || id == 0xf4 || id == 0xf6) {
			addr = podule->read_rom(podule->sync_base, address + 16);
			addr |= (podule->read_rom(podule->sync_base, address + 20) << 8);
			addr |= (podule->read_rom(podule->sync_base, address + 24) << 16);
			addr |= (podule->read_rom(podule->sync_base, address + 28) << 24);
			printf("<%04x.%04x.%04x.%04x>", id, address, addr, size);
			if (addr < 0x800) {
				for (loop = 0; loop < size; ++loop) {
					printf("%c", podule->read_rom(podule->sync_base, (addr + loop)*4));
				}
				printf("\\n\n");
			}
		}
#endif
		address += 32;
	} while (id != 0 && address < 0x800);
}


void
poduleexamine(podule, dev, slottype)
	podule_t *podule;
	struct device *dev;
	int slottype;
{
	struct manufacturer_description *man_desc;
	struct podule_description *pod_desc;

	/* Test to see if the podule is present */

	if ((podule->flags0 & 0x02) == 0x00) {
		podule->slottype = slottype;
		if (slottype == SLOT_NET)
			printf("netslot%d at %s : ", podule->podulenum - MAX_PODULES,
			    dev->dv_xname);
		else
			printf("podule%d  at %s : ", podule->podulenum,
			    dev->dv_xname);

		/* Is it Acorn conformant ? */

		if (podule->flags0 & 0x80)
			printf("Non-Acorn conformant expansion card\n");
		else {
			int id;

			/* Is it a simple podule ? */

			id = (podule->flags0 >> 3) & 0x0f;
			if (id != 0)
				printf("Simple expansion card <%x>\n", id);
			else {
				/* Scan the chunk directory if present for tags we use */
				if (podule->flags1 & PODULE_FLAGS_CD)
					podulechunkdirectory(podule);

				/* Do we know this manufacturer ? */
				man_desc = known_manufacturers;
				while (man_desc->description) {
					if (man_desc->manufacturer_id ==
					    podule->manufacturer)
						break;
					++man_desc;
				}
				if (!man_desc->description)
					printf("man=%04x   : ", podule->manufacturer);
				else
					printf("%s : ", man_desc->description);

				/* Do we know this product ? */

				pod_desc = known_podules;
				while (pod_desc->description) {
					if (pod_desc->product_id == podule->product)
						break;
					++pod_desc;
				}
				if (!pod_desc->description)
					printf("prod=%04x : ",
					    podule->product);
				else
					printf("%s : ", pod_desc->description);
				printf("%s\n", podule->description);
			}
		}
	}
}


u_int
poduleread(address, offset)
	u_int address;
	int offset;
{

	return(ReadByte(address + offset));
}

void
podulescan(dev)
	struct device *dev;
{
	int loop;
	podule_t *podule;
	u_char *address;
	u_int offset = 0;

	/* Loop round all the podules */

	for (loop = 0; loop < MAX_PODULES; ++loop, offset += SIMPLE_PODULE_SIZE) {
		podule = &podules[loop];
		podule->podulenum = loop;
		podule->attached = 0;
		podule->slottype = SLOT_NONE;
		podule->interrupt = IRQ_PODULE;
		podule->read_rom = poduleread;
		podule->dma_channel = -1;
		podule->dma_interrupt = -1;
		podule->description[0] = 0;

		if (loop == 4) offset += PODULE_GAP;
		address = ((u_char *)SYNC_PODULE_BASE) + offset;
        
		if ((address[0] & 0x02) == 0x00) {
			podule->fast_base = FAST_PODULE_BASE + offset;
			podule->medium_base = MEDIUM_PODULE_BASE + offset;
			podule->slow_base = SLOW_PODULE_BASE + offset;
			podule->sync_base = SYNC_PODULE_BASE + offset;
			podule->mod_base = MOD_PODULE_BASE + offset;
			podule->easi_base = EASI_BASE + loop * EASI_SIZE;
		} else {
			address = ((u_char *)EASI_BASE) + loop * EASI_SIZE;
			if ((address[0] & 0x02) != 0x00)
				continue;

			podule->fast_base = 0;
			podule->medium_base = 0;
			podule->slow_base = 0;
			podule->sync_base = 0;
			podule->mod_base = 0;
			podule->easi_base = EASI_BASE + loop * EASI_SIZE;
		}

		/* XXX - Really needs to be linked to a DMA manager */
		if (IOMD_ID == RPC600_IOMD_ID) {
			switch (loop) {
			case 0:
				podule->dma_channel = 2;
				podule->dma_interrupt = IRQ_DMACH2;
				break;
			case 1:
				podule->dma_channel = 3;
				podule->dma_interrupt = IRQ_DMACH3;
				break;
			}
		}

		/* Get information from the podule header */

		podule->flags0 = address[0];
		if ((podule->flags0 & 0x78) == 0) {
			podule->flags1 = address[4];
			podule->reserved = address[8];
			podule->product = address[12] + (address[16] << 8);
			podule->manufacturer = address[20] + (address[24] << 8);
			podule->country = address[28];
			if (podule->flags1 & PODULE_FLAGS_IS) {
				podule->irq_addr = address[52] + (address[56] << 8) + (address[60] << 16);
				podule->irq_addr += podule->slow_base;
				podule->irq_mask = address[48];
				if (podule->irq_mask == 0)
					podule->irq_mask = 0x01;
				podule->fiq_addr = address[36] + (address[40] << 8) + (address[44] << 16);
				podule->fiq_addr += podule->slow_base;
				podule->fiq_mask = address[32];
				if (podule->fiq_mask == 0)
					podule->fiq_mask = 0x04;
			} else {
				podule->irq_addr = podule->slow_base;
				podule->irq_mask = 0x01;
				podule->fiq_addr = podule->slow_base;
				podule->fiq_mask = 0x04;
			}
		}

		poduleexamine(podule, dev, SLOT_POD);
	}
}


/*
 * void podulebusattach(struct device *parent, struct device *dev, void *aux)
 *
 * Attach podulebus.
 * This probes all the podules and sets up the podules array with
 * information found in the podule headers.
 * After identifing all the podules, all the children of the podulebus
 * are probed and attached.
 */
  
void
podulebusattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	int loop;
	struct podule_attach_args pa;
#if 0
	int easi_time;
	int bit;
#endif
	unsigned int value;
	char argstring[20];

#if 0
	easi_time = IOMD_READ_BYTE(IOMD_ECTCR);
	printf(": easi timings=");
	for (bit = 0x01; bit < 0x100; bit = bit << 1)
		if (easi_time & bit)
			printf("C");
		else
			printf("A");
#endif
	printf("\n");

	/* Ok we need to map in the podulebus */

	/* Map the FAST and SYNC simple podules */

	pmap_map_section((vm_offset_t)pmap_kernel()->pm_pdir,
	    SYNC_PODULE_BASE & 0xfff00000, SYNC_PODULE_HW_BASE & 0xfff00000,
	    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE);
	cpu_tlb_flushD();

	/* Now map the EASI space */

	for (loop = 0; loop < MAX_PODULES; ++loop) {
		int loop1;
        
		for (loop1 = loop * EASI_SIZE; loop1 < ((loop + 1) * EASI_SIZE);
		    loop1 += L1_S_SIZE)
		pmap_map_section((vm_offset_t)pmap_kernel()->pm_pdir,
		    EASI_BASE + loop1, EASI_HW_BASE + loop1,
		    VM_PROT_READ|VM_PROT_WRITE, PTE_NOCACHE);
	}
	cpu_tlb_flushD();

	/*
	 * The MEDIUM and SLOW simple podules and the module space will have been
	 * mapped when the IOMD and COMBO we mapped in for the RPC
	 */

	/* Find out what hardware is bolted on */

	podulescan(self); 
	netslotscan(self);

	/* Look for drivers to attach */

	for (loop = 0; loop < MAX_PODULES+MAX_NETSLOTS; ++loop) {
#if 1
		/* Provide backwards compat for a while */
		sprintf(argstring, "podule%d.disable", loop);
		if (get_bootconf_option(boot_args, argstring,
		    BOOTOPT_TYPE_BOOLEAN, &value)) {
			if (value) {
				if (podules[loop].slottype != SLOT_NONE)
					printf("podule%d: Disabled\n", loop);
				continue;
			}
 		}
#endif
 		sprintf(argstring, "podule%d=", loop);
 		if (get_bootconf_option(boot_args, argstring,
 		    BOOTOPT_TYPE_HEXINT, &value)) {
			/* Override the ID */
			podules[loop].manufacturer = value >> 16;
 			podules[loop].product = value & 0xffff;
			/* Any old description is now wrong */
			podules[loop].description[0] = 0;
			if (value != 0xffff) {
				printf("podule%d: ID overriden man=%04x prod=%04x\n",
				    loop, podules[loop].manufacturer,
				    podules[loop].product);
				podules[loop].slottype = SLOT_POD;
				pa.pa_podule_number = loop;
				pa.pa_ih = pa.pa_podule_number;
				pa.pa_podule = &podules[loop];
				pa.pa_iot = &podulebus_bs_tag;
				config_found_sm(self, &pa, podulebusprint,
				    podulebussubmatch);
				continue;
			}
			if (value == 0xffff) {
				printf("podule%d: Disabled\n", loop);
				continue;
			}
		}
		
		if (podules[loop].slottype != SLOT_NONE) {
			pa.pa_podule_number = loop;
			pa.pa_ih = pa.pa_podule_number;
			pa.pa_podule = &podules[loop];
			pa.pa_iot = &podulebus_bs_tag;
			config_found_sm(self, &pa, podulebusprint, podulebussubmatch);
		}
	}
}


struct cfattach podulebus_ca = {
	sizeof(struct device), podulebusmatch, podulebusattach
};

/* Useful functions that drivers may share */

/*
 * Match a podule structure with the specified parameters
 * Returns 0 if the match failed
 * The required_slot is not used at the moment.
 */

int
matchpodule(pa, manufacturer, product, required_slot)
	struct podule_attach_args *pa;
	int manufacturer;
	int product;
	int required_slot;
{
	if (pa->pa_podule->attached)
		panic("podulebus: Podule already attached\n");

	if (IS_PODULE(pa, manufacturer, product))
		return(1);

	return(0);
}

void *
podulebus_irq_establish(ih, ipl, func, arg, ev)
	podulebus_intr_handle_t ih;
	int ipl;
	int (*func) __P((void *));
	void *arg;
	struct evcnt *ev;
{

	/* XXX We don't actually use the evcnt supplied, just its name. */
	return intr_claim(podules[ih].interrupt, ipl, ev->ev_group, func,
	    arg);
}

/*
 * Generate a bus_space_tag_t with the specified address-bus shift.
 */
void
podulebus_shift_tag(tag, shift, tagp)
	bus_space_tag_t tag, *tagp;
	u_int shift;
{

	/*
	 * For the podulebus, the bus tag cookie is the shift to apply
	 * to registers, so duplicate the bus space tag and change the
	 * cookie.
	 */

	/* XXX never freed, but podules are never detached anyway. */
        *tagp = malloc(sizeof(struct bus_space), M_DEVBUF, M_WAITOK);
	**tagp = *tag;
	(*tagp)->bs_cookie = (void *)shift;
}

int
podulebus_initloader(struct podulebus_attach_args *pa)
{

	/* No loader support at present on arm32, so always fail. */
	return -1;
}

int
podloader_readbyte(struct podulebus_attach_args *pa, u_int addr)
{

	panic("podloader_readbyte");
}

void
podloader_writebyte(struct podulebus_attach_args *pa, u_int addr, int val)
{

	panic("podloader_writebyte");
}

void
podloader_reset(struct podulebus_attach_args *pa)
{

	panic("podloader_reset");
}

int
podloader_callloader(struct podulebus_attach_args *pa, u_int r0, u_int r1)
{

	panic("podloader_callloader");
}

/* End of podulebus.c */
