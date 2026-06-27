/*	$NetBSD: bestcomm.c,v 1.1 2026/06/27 13:28:34 rkujawa Exp $	*/

/*-
 * Copyright (c) 2009, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa and Robert Swindells.
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
 * Driver for the MPC5200B BestComm SDMA engine.
 *
 * BestComm is a microcoded processor that runs DMA tasks from on-chip SRAM.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bestcomm.c,v 1.1 2026/06/27 13:28:34 rkujawa Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <dev/ofw/openfirm.h>

#include <machine/autoconf.h>

#include <powerpc/pic/picvar.h>

#include <powerpc/mpc5200/obiovar.h>
#include <powerpc/mpc5200/mpc5200reg.h>
#include <powerpc/mpc5200/bestcommreg.h>
#include <powerpc/mpc5200/sramvar.h>
#include <powerpc/mpc5200/bestcommvar.h>
#include <powerpc/mpc5200/bestcomm_image.h>

/*
 * The task table MUST be aligned so that the function-descriptor relocation
 * (which replaces a descriptor's high 24 bits with the TaskBar's) is exact.
 */
#define	BESTCOMM_TASKBAR_ALIGN	0x100

#define	BESTCOMM_CASCADE_IRQ	64

static int	bestcomm_match(device_t, cfdata_t, void *);
static void	bestcomm_attach(device_t, device_t, void *);
static void	bestcomm_load_image(device_t);

CFATTACH_DECL_NEW(bestcomm, sizeof(struct bestcomm_softc),
    bestcomm_match, bestcomm_attach, NULL, NULL);

static void	bestcomm_enable_irq(struct pic_ops *, int, int);
static void	bestcomm_disable_irq(struct pic_ops *, int);
static int	bestcomm_get_irq(struct pic_ops *, int);
static void	bestcomm_ack_irq(struct pic_ops *, int);
static void	bestcomm_establish_irq(struct pic_ops *, int, int, int);

static struct bestcomm_softc *bestcomm_sc;

static inline uint32_t
sdma_read(struct bestcomm_ops *b, bus_size_t off)
{
	return bus_space_read_4(b->bst, b->bsh, off);
}

static inline void
sdma_write(struct bestcomm_ops *b, bus_size_t off, uint32_t val)
{
	bus_space_write_4(b->bst, b->bsh, off, val);
}

static int
bestcomm_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args *oba = aux;
	char compat[40];
	int len;

	if (strcmp(oba->obio_name, "bestcomm") == 0 ||
	    strcmp(oba->obio_name, "sdma") == 0)
		return 1;

	len = OF_getprop(oba->obio_node, "compatible", compat, sizeof(compat));
	if (len > 0 &&
	    (strcmp(compat, "mpc5200-bestcomm") == 0 ||
	     strcmp(compat, "mpc5200b-bestcomm") == 0))
		return 1;

	return 0;
}

static void
bestcomm_attach(device_t parent, device_t self, void *aux)
{
	struct bestcomm_softc *sc = device_private(self);
	struct obio_attach_args *oba = aux;
	struct bestcomm_ops *b = &sc->sc_pic;
	struct pic_ops *pic = &b->pic;
	bus_size_t size;
	int i;

	sc->sc_dev = self;
	sc->sc_dmat = oba->obio_dmat;
	b->bst = oba->obio_bst;

	size = oba->obio_size != 0 ? oba->obio_size : SDMA_REG_SIZE;
	if (bus_space_map(b->bst, oba->obio_addr, size, 0, &b->bsh) != 0) {
		aprint_error(": can't map registers\n");
		return;
	}

	/*
	 * Bring the engine to an idle state
	 */
	for (i = 0; i < SDMA_NTASKS / 2; i++)
		sdma_write(b, SDMA_TCR + i * sizeof(uint32_t), 0);

	b->int_mask = 0xffffffff;
	sdma_write(b, SDMA_INT_MASK, b->int_mask);
	sdma_write(b, SDMA_INT_PEND, SDMA_INT_IMPL);

	pic->pic_cookie = b;
	pic->pic_numintrs = SDMA_NTASKS;
	pic->pic_enable_irq = bestcomm_enable_irq;
	pic->pic_reenable_irq = bestcomm_enable_irq;
	pic->pic_disable_irq = bestcomm_disable_irq;
	pic->pic_get_irq = bestcomm_get_irq;
	pic->pic_ack_irq = bestcomm_ack_irq;
	pic->pic_establish_irq = bestcomm_establish_irq;
	pic->pic_finish_setup = NULL;
	strlcpy(pic->pic_name, device_xname(self), sizeof(pic->pic_name));

	pic_add(pic);

	/*
	 * Cascade the secondary controller onto the SIU
	 */
	sc->sc_cascade = intr_establish(BESTCOMM_CASCADE_IRQ, IST_LEVEL,
	    IPL_NET, pic_handle_intr, pic);

	bestcomm_sc = sc;

	aprint_normal(": BestComm SDMA, %d tasks, %d interrupt sources\n",
	    SDMA_NTASKS, SDMA_NTASKS);

	config_interrupts(self, bestcomm_load_image);
}

bool
bestcomm_available(void)
{
	return bestcomm_sc != NULL;
}

/*
 * Set the low 16 bits (the signed increment) of an increment word while
 * preserving the high half, matching the task layout.
 */
static void
bestcomm_set_incr(volatile uint32_t *incw, int value)
{
	*incw = (*incw & 0xffff0000) | ((uint16_t)value);
}

/*
 * Set up a single-pointer FIFO-to-memory BD task (the FEC receive shape).
 */
int
bestcomm_bd_setup(struct bestcomm_bdring *br, int task,
    const struct bestcomm_bd_layout *l, bus_addr_t fifo_pa, u_int nbd,
    uint32_t maxbuf, int datasize, int initiator, int prio)
{
	struct bestcomm_softc *sc = bestcomm_sc;
	struct bestcomm_ops *b;
	struct bestcomm_tdt *tdt;
	volatile uint32_t *var, *inc;
	bus_addr_t ring_pa, tcr_pa;
	uint8_t szbyte;

	if (sc == NULL || sc->sc_image_kva == NULL)
		return ENXIO;
	b = &sc->sc_pic;

	ring_pa = sram_alloc(nbd * sizeof(uint32_t) * 2, sizeof(uint32_t) * 2);
	if (ring_pa == 0)
		return ENOMEM;

	br->br_task = task;
	br->br_bd_pa = ring_pa;
	br->br_bd = sram_kva(ring_pa);
	br->br_nbd = nbd;
	br->br_bdflag = l->bdflag;
	memset(br->br_bd, 0, nbd * sizeof(uint32_t) * 2);

	tdt = (struct bestcomm_tdt *)((char *)sc->sc_image_kva +
	    task * sizeof(struct bestcomm_tdt));
	var = (volatile uint32_t *)sram_kva(tdt->tdt_var);
	inc = var + 24;

	tcr_pa = MPC5200_MBAR_DEFAULT + MPC5200_REG_SDMA + SDMA_TCR + task * 2;

	var[l->fifo_var] = fifo_pa;			/* peripheral FIFO	*/
	var[l->enable_var] = tcr_pa;			/* task's own TCR	*/
	var[l->base_var] = ring_pa;			/* BD ring base		*/
	var[l->base_var + 1] = ring_pa + (nbd - 1) * 8;	/* last		*/
	var[l->base_var + 2] = ring_pa;			/* current/start	*/
	var[l->bytes_var] = maxbuf;			/* per-buffer cap	*/
	if (l->drd_var >= 0)				/* flag-carrying DRD	*/
		var[l->drd_var] = tdt->tdt_start + l->drd_off;
	bestcomm_set_incr(&inc[0], -datasize);		/* IncrBytes		*/
	bestcomm_set_incr(&inc[1], datasize);		/* memory-side incr	*/
	bestcomm_set_incr(&inc[2], 1);			/* misalign adjust	*/

	((volatile uint8_t *)&tdt->tdt_fdt)[3] = 0x07;	/* task pragma	*/

	/* datasize transfer size on both sides (even/odd nibble). */
	szbyte = bus_space_read_1(b->bst, b->bsh, SDMA_SIZE_BYTE(task));
	if (task & 1)
		szbyte = (szbyte & 0xf0) | SDMA_SIZE_FIELD(datasize, datasize);
	else
		szbyte = (szbyte & 0x0f) |
		    (SDMA_SIZE_FIELD(datasize, datasize) << 4);
	bus_space_write_1(b->bst, b->bsh, SDMA_SIZE_BYTE(task), szbyte);

	/* Auto-restart the task after each buffer; leave it disabled for now. */
	bus_space_write_2(b->bst, b->bsh, SDMA_TCR_TASK(task),
	    SDMA_TCR_AUTOSTART | (task & SDMA_TCR_AUTOTASK_MASK));

	/* Give the (image-baked) hardware initiator a scheduling priority. */
	bus_space_write_1(b->bst, b->bsh, SDMA_IPR_INIT(initiator), prio & 0x07);

	__asm volatile ("sync" ::: "memory");
	return 0;
}

/*
 * Release a BD task's resources
 */
void
bestcomm_bd_teardown(struct bestcomm_bdring *br)
{
	if (br->br_bd_pa == 0)
		return;

	bestcomm_task_stop(br->br_task);
	sram_free(br->br_bd_pa, br->br_nbd * sizeof(uint32_t) * 2);
	br->br_bd = NULL;
	br->br_bd_pa = 0;
	br->br_nbd = 0;
}

void
bestcomm_bd_post(struct bestcomm_bdring *br, u_int idx, bus_addr_t buf_pa,
    uint32_t size, uint32_t flags)
{
	volatile uint32_t *bd = (volatile uint32_t *)br->br_bd + idx * 2;
	uint32_t status;

	if (br->br_bdflag)
		status = (flags & 0x0c000000) | (size & 0x03ffffff);
	else
		status = size & 0x7fffffff;

	bd[1] = buf_pa;				/* data pointer	*/
	__asm volatile ("sync" ::: "memory");
	bd[0] = status | BESTCOMM_BD_READY;	/* arm for the engine	*/
	__asm volatile ("sync" ::: "memory");
}

uint32_t
bestcomm_bd_status(struct bestcomm_bdring *br, u_int idx)
{
	volatile uint32_t *bd = (volatile uint32_t *)br->br_bd + idx * 2;

	return bd[0];
}

/*
 * Stamp a runtime initiator into a task
 */
void
bestcomm_task_set_initiator(int task, int initiator, const uint16_t *drd_offs,
    u_int ndrd)
{
	struct bestcomm_softc *sc = bestcomm_sc;
	struct bestcomm_ops *b;
	struct bestcomm_tdt *tdt;
	uint16_t tcr;
	u_int i;
	bool ext = false;

	if (sc == NULL || sc->sc_image_kva == NULL)
		return;
	b = &sc->sc_pic;
	tdt = (struct bestcomm_tdt *)((char *)sc->sc_image_kva +
	    task * sizeof(struct bestcomm_tdt));

	/* Task own-initiator field in the TCR (preserve the other bits). */
	tcr = bus_space_read_2(b->bst, b->bsh, SDMA_TCR_TASK(task));
	tcr = (tcr & ~SDMA_TCR_INIT_MASK) |
	    (((uint16_t)initiator << SDMA_TCR_INIT_SHIFT) & SDMA_TCR_INIT_MASK);
	bus_space_write_2(b->bst, b->bsh, SDMA_TCR_TASK(task), tcr);

	for (i = 0; i < ndrd; i++) {
		volatile uint32_t *drd =
		    (volatile uint32_t *)sram_kva(tdt->tdt_start + drd_offs[i]);

		if (!ext) {
			if (((*drd & SDMA_DRD_INIT_MASK) >> SDMA_DRD_INIT_SHIFT) !=
			    SDMA_INITIATOR_ALWAYS)
				*drd = (*drd & ~SDMA_DRD_INIT_MASK) |
				    ((uint32_t)initiator << SDMA_DRD_INIT_SHIFT);
			ext = (*drd & SDMA_DRD_EXT) != 0;
		} else {
			/* This word is a DRD extension operand; skip, then
			 * resume looking for descriptors once it ends. */
			ext = (*drd & SDMA_DRD_EXT) != 0;
		}
	}

	__asm volatile ("sync" ::: "memory");
}

void
bestcomm_task_start(int task)
{
	struct bestcomm_ops *b = &bestcomm_sc->sc_pic;
	uint16_t tcr;

	tcr = bus_space_read_2(b->bst, b->bsh, SDMA_TCR_TASK(task));
	bus_space_write_2(b->bst, b->bsh, SDMA_TCR_TASK(task),
	    tcr | SDMA_TCR_ENABLE);
}

void
bestcomm_task_stop(int task)
{
	struct bestcomm_ops *b = &bestcomm_sc->sc_pic;
	uint16_t tcr;

	tcr = bus_space_read_2(b->bst, b->bsh, SDMA_TCR_TASK(task));
	bus_space_write_2(b->bst, b->bsh, SDMA_TCR_TASK(task),
	    tcr & ~SDMA_TCR_ENABLE);
}

int
bestcomm_task_irq(int task)
{
	return bestcomm_sc->sc_pic.pic.pic_intrbase + task;
}

uint32_t
bestcomm_intpend(void)
{
	return sdma_read(&bestcomm_sc->sc_pic, SDMA_INT_PEND);
}

uint16_t
bestcomm_task_tcr(int task)
{
	struct bestcomm_ops *b = &bestcomm_sc->sc_pic;

	return bus_space_read_2(b->bst, b->bsh, SDMA_TCR_TASK(task));
}

/*
 * Load the task microcode image into SRAM and point the engine at it.
 */
static void
bestcomm_load_image(device_t self)
{
	struct bestcomm_softc *sc = device_private(self);
	struct bestcomm_ops *b = &sc->sc_pic;
	struct bestcomm_tdt *tdt;
	bus_addr_t pa;
	void *kva;
	uint32_t i;

	if (!sram_available()) {
		aprint_error_dev(self, "no SRAM; SDMA image not loaded\n");
		return;
	}

	pa = sram_alloc(bestcomm_image_bytes, BESTCOMM_TASKBAR_ALIGN);
	if (pa == 0) {
		aprint_error_dev(self, "no SRAM space for %u-byte SDMA image\n",
		    bestcomm_image_bytes);
		return;
	}
	kva = sram_kva(pa);

	memcpy(kva, bestcomm_image, bestcomm_image_bytes);

	/* Relocate each task descriptor's pointer words by the TaskBar base. */
	tdt = (struct bestcomm_tdt *)((char *)kva + bestcomm_image_entry);
	for (i = 0; i < bestcomm_image_ntasks; i++) {
		tdt[i].tdt_start   += pa;
		tdt[i].tdt_stop    += pa;
		tdt[i].tdt_var     += pa;
		tdt[i].tdt_fdt      = (pa & 0xffffff00) + tdt[i].tdt_fdt;
		tdt[i].tdt_context += pa;
	}

	sc->sc_taskbar = pa;
	sc->sc_image_kva = kva;
	sdma_write(b, SDMA_TASKBAR, pa);

	aprint_normal_dev(self,
	    "loaded %u-task SDMA image (%u bytes) at SRAM 0x%08jx\n",
	    bestcomm_image_ntasks, bestcomm_image_bytes, (uintmax_t)pa);
}

/*
 * Secondary PIC for the 16 task-completion events.
 */
static void
bestcomm_enable_irq(struct pic_ops *pic, int irq, int type)
{
	struct bestcomm_ops *b = pic->pic_cookie;

	b->int_mask &= ~SDMA_INT_TASK(irq);
	sdma_write(b, SDMA_INT_MASK, b->int_mask);
}

static void
bestcomm_disable_irq(struct pic_ops *pic, int irq)
{
	struct bestcomm_ops *b = pic->pic_cookie;

	b->int_mask |= SDMA_INT_TASK(irq);
	sdma_write(b, SDMA_INT_MASK, b->int_mask);
}

static int
bestcomm_get_irq(struct pic_ops *pic, int mode)
{
	struct bestcomm_ops *b = pic->pic_cookie;
	uint32_t pending;

	pending = sdma_read(b, SDMA_INT_PEND) & ~b->int_mask &
	    SDMA_INT_TASK_MASK;
	if (pending == 0)
		return 255;

	return ffs(pending) - 1;	/* lowest-numbered pending task */
}

static void
bestcomm_ack_irq(struct pic_ops *pic, int irq)
{
	struct bestcomm_ops *b = pic->pic_cookie;

	/* IntPend is write-1-to-clear. */
	sdma_write(b, SDMA_INT_PEND, SDMA_INT_TASK(irq));
}

static void
bestcomm_establish_irq(struct pic_ops *pic, int irq, int type, int maxlevel)
{
	/* Per-task priority is left at its reset default for now. */
}

