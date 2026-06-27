/*	$NetBSD: bestcommvar.h,v 1.1 2026/06/27 13:28:34 rkujawa Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
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

#ifndef _POWERPC_MPC5200_BESTCOMMVAR_H_
#define _POWERPC_MPC5200_BESTCOMMVAR_H_

#include <sys/device.h>
#include <sys/bus.h>

#include <powerpc/pic/picvar.h>

struct bestcomm_ops {
	struct pic_ops	pic;
	bus_space_tag_t	bst;
	bus_space_handle_t bsh;
	uint32_t	int_mask;		/* shadow of SDMA_INT_MASK */
};

struct bestcomm_softc {
	device_t		sc_dev;
	struct bestcomm_ops	sc_pic;
	bus_dma_tag_t		sc_dmat;
	bus_addr_t		sc_taskbar;	/* SRAM phys base of image */
	void			*sc_image_kva;	/* CPU pointer to the image */
	void			*sc_cascade;	/* SIU cascade interrupt */
};

/*
 * BestComm SDMA engine interface.
 */
struct bestcomm_tdt {
	uint32_t	tdt_start;	/* first descriptor word		*/
	uint32_t	tdt_stop;	/* last descriptor word			*/
	uint32_t	tdt_var;	/* variable table			*/
	uint32_t	tdt_fdt;	/* function descriptor table & flags	*/
	uint32_t	tdt_rsvd1;
	uint32_t	tdt_rsvd2;
	uint32_t	tdt_context;	/* context save area			*/
	uint32_t	tdt_litbase;	/* literal base				*/
};

bool	bestcomm_available(void);	/* did the SDMA engine attach? */

/*
 * Buffer-descriptor task support (FEC rx/tx and similar peripheral DMA).
 */
struct bestcomm_bdring {
	int		br_task;	/* task slot in the image	*/
	void		*br_bd;		/* CPU pointer to the BD array	*/
	bus_addr_t	br_bd_pa;	/* SRAM physical of the BD array */
	u_int		br_nbd;		/* number of descriptors	*/
	bool		br_bdflag;	/* descriptors carry frame flags */
};

#define	BESTCOMM_BD_READY	0x40000000	/* engine owns this BD	*/
#define	BESTCOMM_BD_LEN_MASK	0x0000ffff	/* transfer length	*/
#define	BESTCOMM_BD_FLAG_LAST	0x08000000	/* last buffer of frame	*/
#define	BESTCOMM_BD_FLAG_INT	0x04000000	/* interrupt after buffer */

/*
 * Variable-table layout of a single-pointer BD task
 */
struct bestcomm_bd_layout {
	uint8_t		fifo_var;
	uint8_t		enable_var;
	uint8_t		base_var;
	uint8_t		bytes_var;
	int8_t		drd_var;
	uint16_t	drd_off;
	bool		bdflag;
};

/* The FEC receive (FIFO->memory) and transmit (memory->FIFO) layouts. */
#define	BESTCOMM_LAYOUT_FEC_RX	{ 1, 0, 2, 5, -1, 0, false }
#define	BESTCOMM_LAYOUT_FEC_TX	{ 1, 2, 3, 6, 0, 0x7c, true }

/*
 * The general-purpose single-pointer BD tasks.
 */
#define	BESTCOMM_LAYOUT_GEN_TX	{ 0, 1, 2, 5, -1, 0, false }
#define	BESTCOMM_LAYOUT_GEN_RX	{ 1, 0, 2, 5, -1, 0, false }

/*
 * Set up a single-pointer BD task.
 */
int	bestcomm_bd_setup(struct bestcomm_bdring *, int task,
	    const struct bestcomm_bd_layout *, bus_addr_t fifo_pa, u_int nbd,
	    uint32_t maxbuf, int datasize, int initiator, int prio);
void	bestcomm_bd_teardown(struct bestcomm_bdring *);

/*
 * Stamp a hardware initiator into a runtime-configurable task.
 */
void	bestcomm_task_set_initiator(int task, int initiator,
	    const uint16_t *drd_offs, u_int ndrd);
void	bestcomm_bd_post(struct bestcomm_bdring *, u_int idx, bus_addr_t buf_pa,
	    uint32_t size, uint32_t flags);
uint32_t bestcomm_bd_status(struct bestcomm_bdring *, u_int idx);
void	bestcomm_task_start(int task);
void	bestcomm_task_stop(int task);
int	bestcomm_task_irq(int task);	/* flat IRQ of a task's completion */
uint32_t bestcomm_intpend(void);	/* raw SDMA interrupt-pending (debug) */
uint16_t bestcomm_task_tcr(int task);	/* raw task control register (debug) */

#endif /* _POWERPC_MPC5200_BESTCOMMVAR_H_ */
