/* $NetBSD: podulebus.c,v 1.21 1997/07/17 01:52:54 jtk Exp $ */

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
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <machine/io.h>
#include <machine/iomd.h>
#include <machine/katelib.h>
#include <machine/irqhandler.h>
#include <arm32/podulebus/podulebus.h>

#include <arm32/podulebus/podules.h>
#include <arm32/podulebus/podule_data.h>

#include "locators.h"

/* Array of podule structures, one per possible podule */

podule_t podules[MAX_PODULES + MAX_NETSLOTS];
irqhandler_t poduleirq;
extern u_int actual_mask;
extern irqhandler_t *irqhandlers[NIRQS];

extern struct bus_space podulebus_bs_tag;

/* Declare prototypes */

void map_section __P((vm_offset_t, vm_offset_t, vm_offset_t));
int poduleirqhandler __P((void *arg));
u_int poduleread __P((u_int address, int offset, int slottype));


/*
 * int podulebusmatch(struct device *parent, void *match, void *aux)
 *
 * Probe for the podule bus. Currently all this does is return 1 to
 * indicate that the podule bus was found.
 */
 
int
podulebusmatch(parent, match, aux)
	struct device *parent;
	void *match;
	void *aux;
{
	switch (IOMD_ID) {
	case RPC600_IOMD_ID:
	case ARM7500_IOC_ID:
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

	if (name) {
		printf("podule%d: ", pa->pa_podule_number);
		return(UNCONF);
	}

	if (pa->pa_podule->slottype == SLOT_POD)
		printf(" [ podule %d ]:", pa->pa_podule_number);
	else if (pa->pa_podule->slottype == SLOT_NET)
		printf(" [ netslot %d ]:", pa->pa_podule_number - MAX_PODULES);
	else
		panic("Invalid slot type\n");

	/* XXXX print flags */
	return (QUIET);
}


int
podulebussubmatch(parent, match, aux)
	struct device *parent;
	void *match;
	void *aux;
{
	struct cfdata *cf = match;
	struct podule_attach_args *pa = aux;

	/* Return priority 0 or 1 for wildcarded podule */

	if (cf->cf_loc[PODULEBUSCF_SLOT] == PODULEBUSCF_SLOT_DEFAULT)
		return((*cf->cf_attach->ca_match)(parent, match, aux));

	/* Return higher priority if we match the specific podule */

	else if (cf->cf_loc[PODULEBUSCF_SLOT] == pa->pa_podule_number)
		return((*cf->cf_attach->ca_match)(parent, match, aux) * 8);

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
		id = poduleread(podule->sync_base, address, podule->slottype);
		size = poduleread(podule->sync_base, address + 4, podule->slottype);
		size |= (poduleread(podule->sync_base, address + 8, podule->slottype) << 8);
		size |= (poduleread(podule->sync_base, address + 12, podule->slottype) << 16);
		if (id == 0xf5) {
			addr = poduleread(podule->sync_base, address + 16, podule->slottype);
			addr |= (poduleread(podule->sync_base, address + 20, podule->slottype) << 8);
			addr |= (poduleread(podule->sync_base, address + 24, podule->slottype) << 16);
			addr |= (poduleread(podule->sync_base, address + 28, podule->slottype) << 24);
			if (addr < 0x800 && done_f5 == 0) {
				done_f5 = 1;
				for (loop = 0; loop < size; ++loop) {
					if (loop < PODULE_DESCRIPTION_LENGTH) {
						podule->description[loop] =
						    poduleread(podule->sync_base, (addr + loop)*4, podule->slottype);
						podule->description[loop + 1] = 0;
					}
				}
			}
		}
#ifdef DEBUG_CHUNK_DIR
		if (id == 0xf5 || id == 0xf1 || id == 0xf2 || id == 0xf3 || id == 0xf4 || id == 0xf6) {
			addr = poduleread(podule->sync_base, address + 16, podule->slottype);
			addr |= (poduleread(podule->sync_base, address + 20, podule->slottype) << 8);
			addr |= (poduleread(podule->sync_base, address + 24, podule->slottype) << 16);
			addr |= (poduleread(podule->sync_base, address + 28, podule->slottype) << 24);
			printf("<%04x.%04x.%04x.%04x>", id, address, addr, size);
			if (addr < 0x800) {
				for (loop = 0; loop < size; ++loop) {
					printf("%c", poduleread(podule->sync_base, (addr + loop)*4, podule->slottype));
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
	struct podule_list *pod_list;
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
				pod_list = known_podules;
				while (pod_list->description) {
					if (pod_list->manufacturer_id == podule->manufacturer)
						break;
					++pod_list;
				}
				if (!pod_list->description)
					printf("man=%04x   : ", podule->manufacturer);
				else
					printf("%10s : ", pod_list->description);

				/* Do we know this product ? */

				pod_desc = pod_list->products;
				while (pod_desc->description) {
					if (pod_desc->product_id == podule->product)
						break;
					++pod_desc;
				}
				if (!pod_desc->description) {
					printf("prod=%04x : ", podule->product);
					printf("%s\n", podule->description);
				} else
					printf("%s : %s\n", pod_desc->description, podule->description);
			}
		}
	}
}


u_int
poduleread(address, offset, slottype)
	u_int address;
	int offset;
	int slottype;
{
	static int netslotoffset = -1;

	if (slottype == SLOT_NET) {
		if (netslotoffset == -1) {
			netslotoffset = 0;
			WriteByte(address, 0x00);
		}
		offset = offset >> 2;
		if (offset < netslotoffset) {
			WriteByte(address, 0);
			netslotoffset = 0;
		}
		while (netslotoffset < offset) {
			slottype = ReadByte(address);
			++netslotoffset;
		}
		++netslotoffset;
		return(ReadByte(address));
	}
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
		podule->dma_channel = -1;
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
				break;
			case 1:
				podule->dma_channel = 3;
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


void
netslotscan(dev)
	struct device *dev;
{
	podule_t *podule;
	volatile u_char *address;

	/* Only one netslot atm */

	/* Reset the address counter */

	WriteByte(NETSLOT_BASE, 0x00);

	address = (u_char *)NETSLOT_BASE;

	podule = &podules[MAX_PODULES];

	podule->fast_base = NETSLOT_BASE;
	podule->medium_base = NETSLOT_BASE;
	podule->slow_base = NETSLOT_BASE;
	podule->sync_base = NETSLOT_BASE;
	podule->mod_base = NETSLOT_BASE;
	podule->easi_base = 0;
	podule->attached = 0;
	podule->slottype = SLOT_NONE;
	podule->podulenum = MAX_PODULES;
	podule->dma_channel = -1;
	podule->description[0] = 0;

	/* XXX - Really needs to be linked to a DMA manager */
	if (IOMD_ID == RPC600_IOMD_ID)
		podule->dma_channel = 0;

	/* Get information from the podule header */

	podule->flags0 = *address;
	podule->flags1 = *address;
	podule->reserved = *address;
	podule->product = *address + (*address << 8);
	podule->manufacturer = *address + (*address << 8);
	podule->country = *address;
	if (podule->flags1 & PODULE_FLAGS_IS) {
		podule->irq_mask = *address;
		podule->irq_addr = *address + (*address << 8) + (*address << 16);
		podule->irq_addr += podule->slow_base;
		if (podule->irq_mask == 0)
			podule->irq_mask = 0x01;
		podule->fiq_mask = *address;
		podule->fiq_addr = *address + (*address << 8) + (*address << 16);
		podule->fiq_addr += podule->slow_base;
		if (podule->fiq_mask == 0)
			podule->fiq_mask = 0x04;
	} else {
		podule->irq_addr = podule->slow_base;
		podule->irq_mask = 0x01;
		podule->fiq_addr = podule->slow_base;
		podule->fiq_mask = 0x04;
	}

	poduleexamine(podule, dev, SLOT_NET);
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
	int easi_time;
	int bit;

	easi_time = ReadByte(IOMD_ECTCR);

	printf(": easi timings=");
	for (bit = 0x01; bit < 0x100; bit = bit << 1)
		if (easi_time & bit)
			printf("C");
		else
			printf("A");

	printf("\n");

	/* Ok we need to map in the podulebus */

	/* Map the FAST and SYNC simple podules */

	map_section(PAGE_DIRS_BASE, SYNC_PODULE_BASE & 0xfff00000,
	    SYNC_PODULE_HW_BASE & 0xfff00000);
	tlb_flush();

	/* Now map the EASI space */

	for (loop = 0; loop < MAX_PODULES; ++loop) {
		int loop1;
        
		for (loop1 = loop * EASI_SIZE; loop1 < ((loop + 1) * EASI_SIZE); loop1 += (1 << 20))
		map_section(PAGE_DIRS_BASE, EASI_BASE + loop1, EASI_HW_BASE + loop1);
	}
	tlb_flush();

	/*
	 * The MEDIUM and SLOW simple podules and the module space will have been
	 * mapped when the IOMD and COMBO we mapped in for the RPC
	 */

	/* Install an podule IRQ handler */

	poduleirq.ih_func = poduleirqhandler;
	poduleirq.ih_arg = NULL;
	poduleirq.ih_level = IPL_NONE;
	poduleirq.ih_name = "podule";

/*
	if (irq_claim(IRQ_PODULE, &poduleirq))
		panic("Cannot claim IRQ %d for podulebus%d\n", IRQ_PODULE, parent->dv_unit);
*/

	/* Find out what hardware is bolted on */

	podulescan(self); 
	netslotscan(self);

	/* Look for drivers to attach */

	for (loop = 0; loop < MAX_PODULES+MAX_NETSLOTS; ++loop)
		if (podules[loop].slottype != SLOT_NONE) {
			pa.pa_podule_number = loop;
			pa.pa_podule = &podules[loop];
			pa.pa_iot = &podulebus_bs_tag;
			config_found_sm(self, &pa, podulebusprint, podulebussubmatch);
		}
}


/*
 * int podule_irqhandler(void *arg)
 *
 * text irq handler to service expansion card IRQ's
 *
 * There is currently a problem here.
 * The spl_mask may mask out certain expansion card IRQ's e.g. SCSI
 * but allow others e.g. Ethernet.
 */

int
poduleirqhandler(arg)
	void *arg;
{
	int loop;
	irqhandler_t *handler;

	printf("eek ! Unknown podule IRQ received - Blocking all podule interrupts\n");
	disable_irq(IRQ_PODULE);
/*	return(1);*/

	/* Loop round the expansion card handlers */

	for (loop = IRQ_EXPCARD0; loop <= IRQ_EXPCARD7; ++loop) {

	/* Is the IRQ currently allowable */

		if (actual_mask & (1 << loop)) {
			handler = irqhandlers[loop];
        
			if (handler && handler->ih_maskaddr) {
				if (ReadByte(handler->ih_maskaddr) & handler->ih_maskbits)
					handler->ih_func(handler->ih_arg);
			}
		}
	}
	return(1);      
}

struct cfattach podulebus_ca = {
	sizeof(struct device), podulebusmatch, podulebusattach
};

struct cfdriver podulebus_cd = {
	NULL, "podulebus", DV_DULL, 0
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

	if (pa->pa_podule->manufacturer == manufacturer
	    && pa->pa_podule->product == product)
		return(1);

	return(0);
}

/* End of podulebus.c */
