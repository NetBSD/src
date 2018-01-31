/*-
 * Copyright (c) 2013 Phileas Fogg
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

#include <sys/cdefs.h>

#include "opt_openpic.h"
#include "opt_interrupt.h"

#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/kernel.h>

#include <machine/pio.h>
#include <powerpc/openpic.h>

#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_pci.h>
#include <dev/pci/pcireg.h>

#include <machine/autoconf.h>
#include <arch/powerpc/pic/picvar.h>

#ifdef U3_HT_PIC_DEPUG
#define DPRINTF aprint_error
#else
#define DPRINTF if (0) printf
#endif

#define HTAPIC_REQUEST_EOI	0x20
#define HTAPIC_TRIGGER_LEVEL	0x02
#define HTAPIC_MASK		0x01

struct u3_ht_irqmap {
	int im_index;
	int im_level;
	volatile uint8_t *im_base;
	volatile uint8_t *im_apple_base;
	uint32_t im_data;
};

struct u3_ht_ops {
	struct pic_ops pic;

	volatile uint8_t *ht_base;

	struct u3_ht_irqmap ht_irqmap[128];

	uint32_t (*ht_read)(struct u3_ht_ops *, u_int);
	void (*ht_write)(struct u3_ht_ops *, u_int, uint32_t);
};

#define u3_ht_read(ptr,reg)		(ptr)->ht_read(ptr, reg)
#define u3_ht_write(ptr,reg,val)	(ptr)->ht_write(ptr, reg, val)

static struct u3_ht_ops *setup_u3_ht(uint32_t, uint32_t, int);
static int setup_u3_ht_workarounds(struct u3_ht_ops *);

static void u3_ht_enable_irq(struct pic_ops *, int, int);
static void u3_ht_disable_irq(struct pic_ops *, int);
static int  u3_ht_get_irq(struct pic_ops *, int);
static void u3_ht_ack_irq(struct pic_ops *, int);
static void u3_ht_establish_irq(struct pic_ops *, int, int, int);
static void u3_ht_finish_setup(struct pic_ops *);

static int u3_ht_is_ht_irq(struct u3_ht_ops *, int);
static void u3_ht_establish_ht_irq(struct u3_ht_ops *, int, int);
static void u3_ht_enable_ht_irq(struct u3_ht_ops *, int);
static void u3_ht_disable_ht_irq(struct u3_ht_ops *, int);
static void u3_ht_ack_ht_irq(struct u3_ht_ops *, int);

static void u3_ht_set_priority(struct u3_ht_ops *, int, int);
static int u3_ht_read_irq(struct u3_ht_ops *, int);
static void u3_ht_eoi(struct u3_ht_ops *, int);

static uint32_t u3_ht_read_be(struct u3_ht_ops *, u_int);
static void u3_ht_write_be(struct u3_ht_ops *, u_int, uint32_t);
static uint32_t u3_ht_read_le(struct u3_ht_ops *, u_int);
static void u3_ht_write_le(struct u3_ht_ops *, u_int, uint32_t);

static const char *u3_compat[] = {
	"u3",
	NULL
};

const char *pic_compat[] = {
	"chrp,open-pic",
	"open-pic",
	"openpic",
	NULL
};

int init_u3_ht(void)
{
	int u4, pic;
	uint32_t reg[2];
	uint32_t base, len, tmp;
	int bigendian;
	volatile uint8_t *unin_reg;

	u4 = OF_finddevice("/u4");
	if (u4 == -1)
		return FALSE;

	if (of_compatible(u4, u3_compat) == -1)
		return FALSE;

	pic = OF_child(u4);
 	while ((pic != 0) && (of_compatible(pic, pic_compat) == -1))
 		pic = OF_peer(pic);

	if ((pic == -1) || (pic == 0))
		return FALSE;

	if (OF_getprop(u4, "reg", reg, sizeof(reg)) != 8) 
		return FALSE;

	base = reg[1];

	/* Enable and reset PIC */

	unin_reg = mapiodev(base, PAGE_SIZE, false);
	KASSERT(unin_reg != NULL);
	tmp = in32(unin_reg + 0xe0);
	tmp |= 0x06;
	out32(unin_reg + 0xe0, tmp);

	bigendian = 0;
	if (OF_getprop(pic, "big-endian", reg, 4) > -1)
		bigendian = 1;

	if (OF_getprop(pic, "reg", reg, 8) != 8) 
		return FALSE;

	base = reg[0];
	len = reg[1];

	aprint_normal("found U3/U4 HT PIC at %08x\n", base);

	setup_u3_ht(base, len, bigendian);

	return TRUE;
}

static struct u3_ht_ops *
setup_u3_ht(uint32_t addr, uint32_t len, int bigendian)
{
	struct u3_ht_ops *u3_ht;
	struct pic_ops *pic;
	int irq;
	u_int x;

	u3_ht = kmem_alloc(sizeof(struct u3_ht_ops), KM_SLEEP);
	bzero(u3_ht, sizeof(struct u3_ht_ops));
	pic = &u3_ht->pic;

	u3_ht->ht_base = mapiodev(addr, len, false);
	KASSERT(u3_ht->ht_base != NULL);

	if (bigendian) {
		u3_ht->ht_read = u3_ht_read_be;
		u3_ht->ht_write = u3_ht_write_be;
	} else {
		u3_ht->ht_read = u3_ht_read_le;
		u3_ht->ht_write = u3_ht_write_le;
	}

	setup_u3_ht_workarounds(u3_ht);

	/* Reset PIC */

	x = u3_ht_read(u3_ht, OPENPIC_CONFIG);
	u3_ht_write(u3_ht, OPENPIC_CONFIG, x | OPENPIC_CONFIG_RESET);
	do {
		x = u3_ht_read(u3_ht, OPENPIC_CONFIG);
	} while (x & OPENPIC_CONFIG_RESET);

	x = u3_ht_read(u3_ht, OPENPIC_FEATURE);
	
	aprint_normal("OpenPIC Version 1.%d: "
	    "Supports %d CPUs and %d interrupt sources.\n",
	    x & 0xff, ((x & 0x1f00) >> 8) + 1, ((x & 0x07ff0000) >> 16) + 1);

	pic->pic_numintrs = ((x & 0x07ff0000) >> 16) + 1;
	pic->pic_cookie = (void *) addr;
	pic->pic_enable_irq = u3_ht_enable_irq;
	pic->pic_reenable_irq = u3_ht_enable_irq;
	pic->pic_disable_irq = u3_ht_disable_irq;
	pic->pic_get_irq = u3_ht_get_irq;
	pic->pic_ack_irq = u3_ht_ack_irq;
	pic->pic_establish_irq = u3_ht_establish_irq;
	pic->pic_finish_setup = u3_ht_finish_setup;
	strcpy(pic->pic_name, "openpic");
	pic_add(pic);

	u3_ht_set_priority(u3_ht, 0, 15);

	for (irq = 0; irq < 4; irq++) {
		x = irq;
		x |= OPENPIC_IMASK;
		x |= OPENPIC_SENSE_LEVEL;
		x |= 8 << OPENPIC_PRIORITY_SHIFT;
		u3_ht_write(u3_ht, OPENPIC_SRC_VECTOR(irq), x);
		u3_ht_write(u3_ht, OPENPIC_IDEST(irq), 1 << 0);
	}
	for (irq = 4; irq < pic->pic_numintrs; irq++) {
		x = irq;
		x |= OPENPIC_IMASK;
		x |= OPENPIC_SENSE_EDGE;
		x |= 8 << OPENPIC_PRIORITY_SHIFT;
		u3_ht_write(u3_ht, OPENPIC_SRC_VECTOR(irq), x);
		u3_ht_write(u3_ht, OPENPIC_IDEST(irq), 1 << 0);
	}

	x = u3_ht_read(u3_ht, OPENPIC_CONFIG);
	x |= OPENPIC_CONFIG_8259_PASSTHRU_DISABLE;
	u3_ht_write(u3_ht, OPENPIC_CONFIG, x);

	u3_ht_write(u3_ht, OPENPIC_SPURIOUS_VECTOR, 0xff);

	u3_ht_set_priority(u3_ht, 0, 0);

	for (irq = 0; irq < pic->pic_numintrs; irq++) {
		u3_ht_read_irq(u3_ht, 0);
		u3_ht_eoi(u3_ht, 0);
	}

	return u3_ht;
}

static int
setup_u3_ht_workarounds(struct u3_ht_ops *u3_ht)
{
	struct u3_ht_irqmap *irqmap = u3_ht->ht_irqmap;
	int parent, child;
	uint32_t reg[5], tmp;
	uint8_t pos;
	uint16_t cr, ht_cr;
	int nirq, irq, i;
	volatile uint8_t *ht_reg, *dev_reg, *base;

	parent = OF_finddevice("/ht");
	if (parent == -1)
		return FALSE;

	if (OF_getprop(parent, "reg", reg, 12) != 12) 
		return FALSE;

	ht_reg = mapiodev(reg[1], reg[2], false);
	KASSERT(ht_reg != NULL);

	memset(irqmap, 0, sizeof(u3_ht->ht_irqmap));

	for (child = OF_child(parent); child != 0; child = OF_peer(child)) {
		if (OF_getprop(child, "reg", reg, 4) != 4) 
			continue;

		dev_reg = ht_reg + (reg[0] & (OFW_PCI_PHYS_HI_DEVICEMASK |
		    OFW_PCI_PHYS_HI_FUNCTIONMASK));

		tmp = in32rb(dev_reg + PCI_COMMAND_STATUS_REG);
		if ((tmp & PCI_STATUS_CAPLIST_SUPPORT) == 0)
			continue;

		for (pos = in8rb(dev_reg + PCI_CAPLISTPTR_REG);
		     pos != 0; pos = in8rb(dev_reg + pos + 0x01)) {
		     	cr = in16rb(dev_reg + pos);
			if (PCI_CAPLIST_CAP(cr) != 0x08)
				continue;

			ht_cr = in16rb(dev_reg + pos + 0x02);
			if ((ht_cr & 0xf800) == 0x8000)
				break;
		}

		if (pos == 0)
			continue;

		base = dev_reg + pos;

		out8rb(base + 0x02, 0x01);
		nirq = in32rb(base + 0x04);
		nirq = (nirq >> 16) & 0xff;

		DPRINTF("dev %08x nirq %d pos %08x\n", (uint32_t)base, nirq, (uint32_t)pos);
		DPRINTF("devreg %08x\n", in32rb(dev_reg + PCI_ID_REG));
		for (i = 0; i <= nirq; i++) {
			out8rb(base + 0x02, 0x10 + (i << 1));
			tmp = in32rb(base + 0x04);
			irq = (tmp >> 16) & 0xff;
			tmp |= HTAPIC_MASK;
			out32rb(base + 0x04, tmp);

			irqmap[irq].im_index = i;
			irqmap[irq].im_level = 0;
			irqmap[irq].im_base = base;

			tmp = in32rb(dev_reg + PCI_ID_REG);
			if (PCI_VENDOR(tmp) == 0x106b)
				irqmap[irq].im_apple_base = dev_reg + 0x60;
			else
				irqmap[irq].im_apple_base = NULL;

			out8rb(base + 0x02, 0x11 + (i << 1));
			irqmap[irq].im_data = in32rb(base + 0x04);
			irqmap[irq].im_data |= (1 << 31);
		}
	}

	return TRUE;
}

static void
u3_ht_enable_irq(struct pic_ops *pic, int irq, int type)
{
	struct u3_ht_ops *u3_ht = (struct u3_ht_ops *)pic;
	u_int x;

	x = u3_ht_read(u3_ht, OPENPIC_SRC_VECTOR(irq));
 	x &= ~OPENPIC_IMASK;
 	u3_ht_write(u3_ht, OPENPIC_SRC_VECTOR(irq), x);

	if (u3_ht_is_ht_irq(u3_ht, irq))
		u3_ht_enable_ht_irq(u3_ht, irq);
}

static void
u3_ht_disable_irq(struct pic_ops *pic, int irq)
{
	struct u3_ht_ops *u3_ht = (struct u3_ht_ops *)pic;
	u_int x;

 	x = u3_ht_read(u3_ht, OPENPIC_SRC_VECTOR(irq));
 	x |= OPENPIC_IMASK;
 	u3_ht_write(u3_ht, OPENPIC_SRC_VECTOR(irq), x);

	if (u3_ht_is_ht_irq(u3_ht, irq))
		u3_ht_disable_ht_irq(u3_ht, irq);
}

static int
u3_ht_get_irq(struct pic_ops *pic, int mode)
{
	struct u3_ht_ops *u3_ht = (struct u3_ht_ops *)pic;

	return u3_ht_read_irq(u3_ht, curcpu()->ci_index);
}

static void
u3_ht_ack_irq(struct pic_ops *pic, int irq)
{
	struct u3_ht_ops *u3_ht = (struct u3_ht_ops *)pic;

	if (u3_ht_is_ht_irq(u3_ht, irq))
		u3_ht_ack_ht_irq(u3_ht, irq);

	u3_ht_eoi(u3_ht, curcpu()->ci_index);
}

static void
u3_ht_establish_irq(struct pic_ops *pic, int irq, int type, int pri)
{
	struct u3_ht_ops *u3_ht = (struct u3_ht_ops *)pic;
	int realpri = max(1, min(15, pri));
	uint32_t x;

	x = irq;
	x |= OPENPIC_IMASK;

	if (u3_ht_is_ht_irq(u3_ht, irq)) {
		x |= OPENPIC_SENSE_EDGE;
	} else {
		if (type == IST_EDGE_FALLING || type == IST_EDGE_RISING)
			x |= OPENPIC_SENSE_EDGE;
		else
			x |= OPENPIC_SENSE_LEVEL;
	}

	x |= realpri << OPENPIC_PRIORITY_SHIFT;
	u3_ht_write(u3_ht, OPENPIC_SRC_VECTOR(irq), x);

	if (u3_ht_is_ht_irq(u3_ht, irq))
		u3_ht_establish_ht_irq(u3_ht, irq, type);

	aprint_error("%s: setting IRQ %d %d to priority %d %x\n", __func__, irq,
	    type, realpri, x);
}

static void
u3_ht_finish_setup(struct pic_ops *pic)
{
	struct u3_ht_ops *u3_ht = (struct u3_ht_ops *)pic;
	uint32_t cpumask = 0;
	int i;

#ifdef OPENPIC_DISTRIBUTE
	for (i = 0; i < ncpu; i++)
		cpumask |= (1 << cpu_info[i].ci_cpuid);
#else
	cpumask = 1;
#endif

	for (i = 0; i < pic->pic_numintrs; i++)
		u3_ht_write(u3_ht, OPENPIC_IDEST(i), cpumask);
}

static int
u3_ht_is_ht_irq(struct u3_ht_ops *u3_ht, int irq)
{
	return (irq < 128) && (u3_ht->ht_irqmap[irq].im_base != NULL);
}

static void
u3_ht_establish_ht_irq(struct u3_ht_ops *u3_ht, int irq, int type)
{
	struct u3_ht_irqmap *irqmap = &u3_ht->ht_irqmap[irq];
	u_int x;

	out8rb(irqmap->im_base + 0x02, 0x10 + (irqmap->im_index << 1));

	x = in32rb(irqmap->im_base + 0x04);
	/* mask interrupt */
	out32rb(irqmap->im_base + 0x04, x | HTAPIC_MASK);

	/* mask out EOI and LEVEL bits */
	x &= ~(HTAPIC_TRIGGER_LEVEL | HTAPIC_REQUEST_EOI);

	if (type == IST_LEVEL_HIGH || type == IST_LEVEL_LOW) {
		irqmap->im_level = 1;
		DPRINTF("level\n");
		x |= HTAPIC_TRIGGER_LEVEL | HTAPIC_REQUEST_EOI;
	} else {
		irqmap->im_level = 0;
	}

	out32rb(irqmap->im_base + 0x04, x);
}

static void
u3_ht_enable_ht_irq(struct u3_ht_ops *u3_ht, int irq)
{
	struct u3_ht_irqmap *irqmap = &u3_ht->ht_irqmap[irq];
	u_int x;

	out8rb(irqmap->im_base + 0x02, 0x10 + (irqmap->im_index << 1));
	x = in32rb(irqmap->im_base + 0x04);
	x &= ~HTAPIC_MASK;
	out32rb(irqmap->im_base + 0x04, x);

	u3_ht_ack_ht_irq(u3_ht, irq);
}

static void
u3_ht_disable_ht_irq(struct u3_ht_ops *u3_ht, int irq)
{
	struct u3_ht_irqmap *irqmap = &u3_ht->ht_irqmap[irq];
	u_int x;

	out8rb(irqmap->im_base + 0x02, 0x10 + (irqmap->im_index << 1));
	x = in32rb(irqmap->im_base + 0x04);
	x |= HTAPIC_MASK;
	out32rb(irqmap->im_base + 0x04, x);
}

static void
u3_ht_ack_ht_irq(struct u3_ht_ops *u3_ht, int irq)
{
	struct u3_ht_irqmap *irqmap = &u3_ht->ht_irqmap[irq];

	if (irqmap->im_level != 0) {
		if (irqmap->im_apple_base != NULL) {
			out32rb(irqmap->im_apple_base + ((irqmap->im_index >> 3) & ~0x03),
			    1 << (irqmap->im_index & 0x1f));
		} else {
			out8rb(irqmap->im_base + 0x02, 0x11 + (irqmap->im_index << 1));
			out32rb(irqmap->im_base + 0x04, irqmap->im_data);
		}
	}
}

static void
u3_ht_set_priority(struct u3_ht_ops *u3_ht, int cpu, int pri)
{
	u_int x;

	x = u3_ht_read(u3_ht, OPENPIC_CPU_PRIORITY(cpu));
	x &= ~OPENPIC_CPU_PRIORITY_MASK;
	x |= pri;
	u3_ht_write(u3_ht, OPENPIC_CPU_PRIORITY(cpu), x);
}

static int
u3_ht_read_irq(struct u3_ht_ops *u3_ht, int cpu)
{
	return u3_ht_read(u3_ht, OPENPIC_IACK(cpu)) & OPENPIC_VECTOR_MASK;
}

static void
u3_ht_eoi(struct u3_ht_ops *u3_ht, int cpu)
{
	u3_ht_write(u3_ht, OPENPIC_EOI(cpu), 0);
	u3_ht_read(u3_ht, OPENPIC_EOI(cpu));
}

static uint32_t
u3_ht_read_be(struct u3_ht_ops *u3_ht, u_int reg)
{
	volatile uint8_t *addr = u3_ht->ht_base + reg;

	return in32(addr);
}

static void
u3_ht_write_be(struct u3_ht_ops *u3_ht, u_int reg, uint32_t val)
{
	volatile uint8_t *addr = u3_ht->ht_base + reg;

	out32(addr, val);
}

static uint32_t
u3_ht_read_le(struct u3_ht_ops *u3_ht, u_int reg)
{
	volatile uint8_t *addr = u3_ht->ht_base + reg;

	return in32rb(addr);
}

static void
u3_ht_write_le(struct u3_ht_ops *u3_ht, u_int reg, uint32_t val)
{
	volatile uint8_t *addr = u3_ht->ht_base + reg;

	out32rb(addr, val);
}
