/* $NetBSD: pi1ppcvar.h,v 1.1 2005/12/28 08:31:09 kurahone Exp $ */

/*-
 * Copyright (c) 2001 Alcove - Nicolas Souchu
 * Copyright (c) 2005 Joe Britt <britt@danger.com> - SGI PI1 version
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
 *
 * FreeBSD: src/sys/isa/ppcreg.h,v 1.10.2.4 2001/10/02 05:21:45 nsouch Exp
 *
 */

#ifndef __PI1PPCVAR_H
#define __PI1PPCVAR_H

#include <machine/bus.h>
#include <machine/types.h>
#include <sys/device.h>
#include <sys/callout.h>

#include <dev/ppbus/ppbus_conf.h>


/* Maximum time to wait for device response */
#define MAXBUSYWAIT	(5 * (hz))

/* Poll interval when wating for device to become ready */
#define PI1PPC_POLL	((hz)/10)

/* Interrupt priority level for pi1ppc device */
#define IPL_PI1PPC	IPL_TTY
#define splpi1ppc	spltty


/* Diagnostic and verbose printing macros */

#ifdef PI1PPC_DEBUG
extern int pi1ppc_debug;
#define PI1PPC_DPRINTF(arg) if(pi1ppc_debug) printf arg
#else
#define PI1PPC_DPRINTF(arg)
#endif

#ifdef PI1PPC_VERBOSE
extern int pi1ppc_verbose;
#define PI1PPC_VPRINTF(arg) if(pi1ppc_verbose) printf arg
#else
#define PI1PPC_VPRINTF(arg)
#endif


/* Flag used in DMA transfer */
#define PI1PPC_DMA_MODE_READ 0x0
#define PI1PPC_DMA_MODE_WRITE 0x1


/* Flags passed via config */
#define PI1PPC_FLAG_DISABLE_INTR	0x01
#define PI1PPC_FLAG_DISABLE_DMA	0x02


/* Locking for pi1ppc device */
#if defined(MULTIPROCESSOR) || defined (LOCKDEBUG)
#include <sys/lock.h>
#define PI1PPC_SC_LOCK(sc) (&((sc)->sc_lock))
#define PI1PPC_LOCK_INIT(sc) simple_lock_init(PI1PPC_SC_LOCK((sc)))
#define PI1PPC_LOCK(sc) simple_lock(PI1PPC_SC_LOCK((sc)))
#define PI1PPC_UNLOCK(sc) simple_unlock(PI1PPC_SC_LOCK((sc)))
#else /* !(MULTIPROCESSOR) && !(LOCKDEBUG) */
#define PI1PPC_LOCK_INIT(sc)
#define PI1PPC_LOCK(sc)
#define PI1PPC_UNLOCK(sc)
#define PI1PPC_SC_LOCK(sc) NULL
#endif /* MULTIPROCESSOR || LOCKDEBUG */

/* Single softintr callback entry */
struct pi1ppc_handler_node {
	void (*func)(void *);
	void * arg;
	SLIST_ENTRY(pi1ppc_handler_node) entries;
};

/* Generic structure to hold parallel port chipset info. */
struct pi1ppc_softc {
	/* Generic device attributes */
	struct device sc_dev;

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	/* Simple lock */
	struct simplelock sc_lock;
#endif

	/* Machine independent bus infrastructure */
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_dma_tag_t sc_dmat;
	bus_dmamap_t sc_dmapt;
	bus_size_t sc_dma_maxsize;

	/* Child device */
	struct device * child;

        /* Opaque handle used for interrupt handler establishment */
	void * sc_ieh;

	/* List of soft interrupts to call */
	SLIST_HEAD(handler_list, pi1ppc_handler_node) sc_handler_listhead;

	 /* Input buffer: working pointers, and size in bytes. */
	char * sc_inb;
	char * sc_inbstart;
	u_int32_t sc_inb_nbytes;
	int sc_inerr;

	/* Output buffer pointer, working pointer, and size in bytes. */
	char * sc_outb;
	char * sc_outbstart;
	u_int32_t sc_outb_nbytes;
	int sc_outerr;

	/* DMA functions: setup by bus specific attach code */
	int (*sc_dma_start)(struct pi1ppc_softc *, void *, u_int, u_int8_t);
	int (*sc_dma_finish)(struct pi1ppc_softc *);
	int (*sc_dma_abort)(struct pi1ppc_softc *);
	int (*sc_dma_malloc)(struct device *, caddr_t *, bus_addr_t *,
		bus_size_t);
	void (*sc_dma_free)(struct device *, caddr_t *, bus_addr_t *,
		bus_size_t);

	/* Microsequence related members */
	char * sc_ptr;		/* microseq current pointer */
	int sc_accum;		/* microseq accumulator */

	/* Device attachment state */
#define PI1PPC_ATTACHED 1
#define PI1PPC_NOATTACH 0
	u_int8_t sc_dev_ok;

	/*
	 * Hardware capabilities flags: standard mode and nibble mode are
	 * assumed to always be available since if they aren't you don't
	 * HAVE a parallel port.
	 */
#define PI1PPC_HAS_INTR	0x01	/* Interrupt available */
#define PI1PPC_HAS_DMA	0x02	/* DMA available */
#define PI1PPC_HAS_FIFO	0x04	/* FIFO available */
#define PI1PPC_HAS_PS2	0x08	/* PS2 mode capable */
	u_int8_t sc_has;	/* Chipset detected capabilities */

	/* Flags specifying mode of chipset operation . */
#define PI1PPC_MODE_STD	0x01	/* Use centronics-compatible mode */
#define PI1PPC_MODE_PS2	0x02	/* Use PS2 mode */
#define PI1PPC_MODE_NIBBLE 0x10	/* Use nibble mode */
	u_int8_t sc_mode;	/* Current operational mode */

	/* Flags which further define chipset operation */
#define PI1PPC_USE_INTR	0x01	/* Use interrupts */
#define PI1PPC_USE_DMA	0x02	/* Use DMA */
	u_int8_t sc_use;	/* Capabilities to use */

	/* Parallel Port Chipset model. */
#define GENERIC         6
	u_int8_t sc_model;	/* chipset model */

	/* EPP mode - UNUSED */
	u_int8_t sc_epp;

	/* Parallel Port Chipset Type.  Only Indy-style needed? */
#define PI1PPC_TYPE_INDY 0
	u_int8_t sc_type;

	/* Stored register values after an interrupt occurs */
	u_int8_t sc_ecr_intr;
	u_int8_t sc_ctr_intr;
	u_int8_t sc_str_intr;

#define PI1PPC_IRQ_NONE	0x0
#define PI1PPC_IRQ_nACK	0x1
#define PI1PPC_IRQ_DMA	0x2
#define PI1PPC_IRQ_FIFO	0x4
#define PI1PPC_IRQ_nFAULT	0x8
	u_int8_t sc_irqstat;	/* Record irq settings */

#define PI1PPC_DMA_INIT		0x01
#define PI1PPC_DMA_STARTED	0x02
#define PI1PPC_DMA_COMPLETE	0x03
#define PI1PPC_DMA_INTERRUPTED	0x04
#define PI1PPC_DMA_ERROR		0x05
	u_int8_t sc_dmastat;	/* Record dma state */

#define PI1PPC_PWORD_MASK	0x30
#define PI1PPC_PWORD_16	0x00
#define PI1PPC_PWORD_8	0x10
#define PI1PPC_PWORD_32	0x20
	u_int8_t sc_pword;	/* PWord size: used for FIFO DMA transfers */
	u_int8_t sc_fifo;	/* FIFO size */

	/* Indicates number of PWords in FIFO queues that generate interrupt */
	u_int8_t sc_wthr;	/* writeIntrThresold */
	u_int8_t sc_rthr;	/* readIntrThresold */
};



#ifdef _KERNEL

/* Function prototypes */

/* Soft config attach/detach routines */
void pi1ppc_sc_attach(struct pi1ppc_softc *);
int pi1ppc_sc_detach(struct pi1ppc_softc *, int);

/* Detection routines */
int pi1ppc_detect_port(bus_space_tag_t, bus_space_handle_t);

/* Interrupt handler for pi1ppc device */
int pi1ppcintr(void *);

#endif /* _KERNEL */

#endif /* __PI1PPCVAR_H */
