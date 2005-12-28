/* $NetBSD: pi1ppc.c,v 1.1 2005/12/28 08:31:09 kurahone Exp $ */

/*
 * Copyright (c) 2001 Alcove - Nicolas Souchu
 * Copyright (c) 2003, 2004 Gary Thorpe <gathorpe@users.sourceforge.net>
 * Copyright (c) 2005 Joe Britt <britt@danger.com> - SGI PI1 version
 * All rights reserved.
 *
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
 * FreeBSD: src/sys/isa/ppc.c,v 1.26.2.5 2001/10/02 05:21:45 nsouch Exp
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pi1ppc.c,v 1.1 2005/12/28 08:31:09 kurahone Exp $");

#include "opt_pi1ppc.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/syslog.h>

#include <machine/bus.h>
/*#include <machine/intr.h>*/

#include <dev/ppbus/ppbus_conf.h>
#include <dev/ppbus/ppbus_msq.h>
#include <dev/ppbus/ppbus_io.h>
#include <dev/ppbus/ppbus_var.h>

#include <machine/autoconf.h>
#include <machine/machtype.h>

#include <sgimips/ioc/iocreg.h>

#include <sgimips/hpc/hpcvar.h>
#include <sgimips/hpc/hpcreg.h>

#include <sgimips/hpc/pi1ppcreg.h>
#include <sgimips/hpc/pi1ppcvar.h>

#ifdef PI1PPC_DEBUG
int pi1ppc_debug = 1;
#endif

#ifdef PI1PPC_VERBOSE
int pi1ppc_verbose = 1;
#endif


/* Prototypes for functions. */

/* PC-style register emulation */
static u_int8_t r_reg(int reg, struct pi1ppc_softc *pi1ppc);
static void w_reg(int reg, struct pi1ppc_softc *pi1ppc, u_int8_t byte);

#define	AT_DATA_REG	0
#define	AT_STAT_REG	1
#define	AT_CTL_REG	2

#define pi1ppc_r_str(_x)	r_reg(AT_STAT_REG,_x)
#define	pi1ppc_r_ctr(_x)	r_reg(AT_CTL_REG,_x)
#define	pi1ppc_r_dtr(_x)	r_reg(AT_DATA_REG,_x)

#define	pi1ppc_w_str(_x,_y)
#define	pi1ppc_w_ctr(_x,_y)	w_reg(AT_CTL_REG,_x,_y)
#define	pi1ppc_w_dtr(_x,_y)	w_reg(AT_DATA_REG,_x,_y)

/* do we need to do these? */
#define	pi1ppc_barrier_r(_x) bus_space_barrier(_x->sc_iot,_x->sc_ioh, \
					0,4,BUS_SPACE_BARRIER_READ)
#define	pi1ppc_barrier_w(_x) bus_space_barrier(_x->sc_iot,_x->sc_ioh, \
					0,4,BUS_SPACE_BARRIER_WRITE)
#define	pi1ppc_barrier(_x)  pi1ppc_barrier_r(_x)


/* Print function for config_found() */
static int pi1ppc_print(void *, const char *);

/* Routines for ppbus interface (bus + device) */
static int pi1ppc_read(struct device *, char *, int, int, size_t *);
static int pi1ppc_write(struct device *, char *, int, int, size_t *);
static int pi1ppc_setmode(struct device *, int);
static int pi1ppc_getmode(struct device *);
static int pi1ppc_exec_microseq(struct device *, struct ppbus_microseq * *);
static u_int8_t pi1ppc_io(struct device *, int, u_char *, int, u_char);
static int pi1ppc_read_ivar(struct device *, int, unsigned int *);
static int pi1ppc_write_ivar(struct device *, int, unsigned int *);
static int pi1ppc_add_handler(struct device *, void (*)(void *), void *);
static int pi1ppc_remove_handler(struct device *, void (*)(void *));

/* no-ops, do any IOC machines have ECP/EPP-capable ports? */
static void pi1ppc_reset_epp_timeout(struct device *);
static void pi1ppc_ecp_sync(struct device *);

/* Utility functions */

/* Functions to read bytes into device's input buffer */
static void pi1ppc_nibble_read(struct pi1ppc_softc * const);
static void pi1ppc_byte_read(struct pi1ppc_softc * const);

/* Functions to write bytes to device's output buffer */
static void pi1ppc_std_write(struct pi1ppc_softc * const);

/* Miscellaneous */
static void pi1ppc_set_intr_mask(struct pi1ppc_softc * const, u_int8_t);
static u_int8_t pi1ppc_get_intr_stat(struct pi1ppc_softc * const);

#ifdef USE_INDY_ACK_HACK
static u_int8_t pi1ppc_get_intr_mask(struct pi1ppc_softc * const);
#endif

static int pi1ppc_poll_str(struct pi1ppc_softc * const, const u_int8_t,
	const u_int8_t);
static int pi1ppc_wait_interrupt(struct pi1ppc_softc * const, const caddr_t,
	const u_int8_t);

static int pi1ppc_poll_interrupt_stat(struct pi1ppc_softc * const, 
	const u_int8_t);

static int pi1ppc_match(struct device * parent, struct cfdata * match, void *aux);
static void pi1ppc_attach(struct device * parent, struct device *self, void *aux);

CFATTACH_DECL(pi1ppc, sizeof(struct pi1ppc_softc), 
				pi1ppc_match, 
				pi1ppc_attach, 
				NULL, 
				NULL);

/* Currently only matching on Indy, though I think the Indigo1 also
   uses PI1.  If it does, then the driver should work (if it is attached
   at the appropriate base addr).
 */

static int
pi1ppc_match(struct device * parent, struct cfdata * match, void *aux)
{
	if (mach_type == MACH_SGI_IP22)
		return 1;

	return 0;
}

static void
pi1ppc_attach(struct device * parent, struct device *self, void *aux)
{
	struct pi1ppc_softc *sc;
	struct hpc_attach_args *haa;

	sc = (struct pi1ppc_softc *)self;
	haa = aux;
	sc->sc_iot = haa->ha_st;

	if (bus_space_subregion(haa->ha_st, haa->ha_sh, haa->ha_devoff,
			0x28, 		/* # bytes in par port regs */
			&sc->sc_ioh)) {
		aprint_error(": unable to map control registers\n");
		return;
	}

	pi1ppc_sc_attach(sc);
}

/*
 * Generic attach and detach functions for pi1ppc device. 
 *
 * If sc_dev_ok in soft configuration data is not ATPPC_ATTACHED, these should
 * be skipped altogether.
 */

/* Soft configuration attach for pi1ppc */
void
pi1ppc_sc_attach(struct pi1ppc_softc *lsc)
{
	/* Adapter used to configure ppbus device */
	struct parport_adapter sc_parport_adapter;
	char buf[64];

	PI1PPC_LOCK_INIT(lsc);

	/* For a PC, this is where the installed chipset is probed.
	 * We *know* what we have, no need to probe.
	 */
	lsc->sc_type = PI1PPC_TYPE_INDY;
	lsc->sc_model = GENERIC;

	/* XXX Once we support Interrupts & DMA, update this */
	lsc->sc_has = PI1PPC_HAS_PS2;
	   
        /* Print out chipset capabilities */
	bitmask_snprintf(lsc->sc_has, "\20\1INTR\2DMA\3FIFO\4PS2\5ECP\6EPP",
		buf, sizeof(buf));
	printf("\n%s: capabilities=%s\n", lsc->sc_dev.dv_xname, buf);

	/* Initialize device's buffer pointers */
	lsc->sc_outb = lsc->sc_outbstart = lsc->sc_inb = lsc->sc_inbstart
		= NULL;
	lsc->sc_inb_nbytes = lsc->sc_outb_nbytes = 0;

	/* Last configuration step: set mode to standard mode */
	if (pi1ppc_setmode(&(lsc->sc_dev), PPBUS_COMPATIBLE) != 0) {
		PI1PPC_DPRINTF(("%s: unable to initialize mode.\n",
			lsc->sc_dev.dv_xname));
	}

#if defined (MULTIPROCESSOR) || defined (LOCKDEBUG)
	/* Initialize lock structure */
	simple_lock_init(&(lsc->sc_lock));
#endif

	/* Set up parport_adapter structure */

	/* Set capabilites */
	sc_parport_adapter.capabilities = 0;
	if (lsc->sc_has & PI1PPC_HAS_INTR) {
		sc_parport_adapter.capabilities |= PPBUS_HAS_INTR;
	}
	if (lsc->sc_has & PI1PPC_HAS_DMA) {
		sc_parport_adapter.capabilities |= PPBUS_HAS_DMA;
	}
	if (lsc->sc_has & PI1PPC_HAS_FIFO) {
		sc_parport_adapter.capabilities |= PPBUS_HAS_FIFO;
	}
	if (lsc->sc_has & PI1PPC_HAS_PS2) {
		sc_parport_adapter.capabilities |= PPBUS_HAS_PS2;
	}

	/* Set function pointers */
	sc_parport_adapter.parport_io = pi1ppc_io;
	sc_parport_adapter.parport_exec_microseq = pi1ppc_exec_microseq;
	sc_parport_adapter.parport_setmode = pi1ppc_setmode;
	sc_parport_adapter.parport_getmode = pi1ppc_getmode;
	sc_parport_adapter.parport_read = pi1ppc_read;
	sc_parport_adapter.parport_write = pi1ppc_write;
	sc_parport_adapter.parport_read_ivar = pi1ppc_read_ivar;
	sc_parport_adapter.parport_write_ivar = pi1ppc_write_ivar;
	sc_parport_adapter.parport_dma_malloc = lsc->sc_dma_malloc;
	sc_parport_adapter.parport_dma_free = lsc->sc_dma_free;
	sc_parport_adapter.parport_add_handler = pi1ppc_add_handler;
	sc_parport_adapter.parport_remove_handler = pi1ppc_remove_handler;

	/* these are no-ops (does later machines have ECP/EPP support?) */
	sc_parport_adapter.parport_ecp_sync = pi1ppc_ecp_sync;
	sc_parport_adapter.parport_reset_epp_timeout =
		pi1ppc_reset_epp_timeout;

	/* Initialize handler list, may be added to by grandchildren */
	SLIST_INIT(&(lsc->sc_handler_listhead));

	/* Initialize interrupt state */
	lsc->sc_irqstat = PI1PPC_IRQ_NONE;
	lsc->sc_ecr_intr = lsc->sc_ctr_intr = lsc->sc_str_intr = 0;

	/* Disable DMA/interrupts (each ppbus driver selects usage itself) */
	lsc->sc_use = 0;

	/* Configure child of the device. */
	lsc->child = config_found(&(lsc->sc_dev), &(sc_parport_adapter),
		pi1ppc_print);

	return;
}

/* Soft configuration detach */
int
pi1ppc_sc_detach(struct pi1ppc_softc *lsc, int flag)
{
	struct device *dev = (struct device *)lsc;

	/* Detach children devices */
	if (config_detach(lsc->child, flag) && !(flag & DETACH_QUIET)) {
		printf("%s not able to detach child device, ", dev->dv_xname);

		if (!(flag & DETACH_FORCE)) {
			printf("cannot detach\n");
			return 1;
		} else {
			printf("continuing (DETACH_FORCE)\n");
		}
	}

	if (!(flag & DETACH_QUIET))
		printf("%s detached", dev->dv_xname);

	return 0;
}

/* Used by config_found() to print out device information */
static int
pi1ppc_print(void *aux, const char *name)
{
	/* Print out something on failure. */
	if (name != NULL) {
		printf("%s: child devices", name);
		return UNCONF;
	}

	return QUIET;
}

/* Interrupt handler for pi1ppc device: wakes up read/write functions */
int
pi1ppcintr(void *arg)
{
/* NO INTERRUPTS YET */
#if 0
	struct pi1ppc_softc *pi1ppc = (struct pi1ppc_softc *)arg;
	struct device *dev = &pi1ppc->sc_dev;
	int claim = 1;
	enum { NONE, READER, WRITER } wake_up = NONE;
	int s;

	s = splpi1ppc();
	PI1PPC_LOCK(pi1ppc);

	/* Record registers' status */
	pi1ppc->sc_str_intr = pi1ppc_r_str(pi1ppc);
	pi1ppc->sc_ctr_intr = pi1ppc_r_ctr(pi1ppc);
	pi1ppc_barrier_r(pi1ppc);

	/* Determine cause of interrupt and wake up top half */
	switch (atppc->sc_mode) {
	case ATPPC_MODE_STD:
		/* nAck pulsed for 5 usec, too fast to check reliably, assume */
		atppc->sc_irqstat = ATPPC_IRQ_nACK;
		if (atppc->sc_outb)
			wake_up = WRITER;
		else
			claim = 0;
		break;

	case ATPPC_MODE_NIBBLE:
	case ATPPC_MODE_PS2:
		/* nAck is set low by device and then high on ack */
		if (!(atppc->sc_str_intr & nACK)) {
			claim = 0;
			break;
		}
		atppc->sc_irqstat = ATPPC_IRQ_nACK;
		if (atppc->sc_inb)
			wake_up = READER;
		break;

	case ATPPC_MODE_ECP:
	case ATPPC_MODE_FAST:
		/* Confirm interrupt cause: these are not pulsed as in nAck. */
		if (atppc->sc_ecr_intr & ATPPC_SERVICE_INTR) {
			if (atppc->sc_ecr_intr & ATPPC_ENABLE_DMA)
				atppc->sc_irqstat |= ATPPC_IRQ_DMA;
			else
				atppc->sc_irqstat |= ATPPC_IRQ_FIFO;

			/* Decide where top half will be waiting */
			if (atppc->sc_mode & ATPPC_MODE_ECP) {
				if (atppc->sc_ctr_intr & PCD) {
					if (atppc->sc_inb)
						wake_up = READER;
					else
						claim = 0;
				} else {
					if (atppc->sc_outb)
						wake_up = WRITER;
					else
						claim = 0;
				}
			} else {
				if (atppc->sc_outb)
					wake_up = WRITER;
				else
					claim = 0;
			}
		}
		/* Determine if nFault has occurred */
		if ((atppc->sc_mode & ATPPC_MODE_ECP) &&
			(atppc->sc_ecr_intr & ATPPC_nFAULT_INTR) &&
			!(atppc->sc_str_intr & nFAULT)) {

			/* Device is requesting the channel */
			atppc->sc_irqstat |= ATPPC_IRQ_nFAULT;
			claim = 1;
		}
		break;

	case ATPPC_MODE_EPP:
		/* nAck pulsed for 5 usec, too fast to check reliably */
		atppc->sc_irqstat = ATPPC_IRQ_nACK;
		if (atppc->sc_inb)
			wake_up = WRITER;
		else if (atppc->sc_outb)
			wake_up = READER;
		else
			claim = 0;
		break;

	default:
		panic("%s: chipset is in invalid mode.", dev->dv_xname);
	}

	if (claim) {
		switch (wake_up) {
		case NONE:
			break;

		case READER:
			wakeup(atppc->sc_inb);
			break;

		case WRITER:
			wakeup(atppc->sc_outb);
			break;
		}
	}

	PI1PPC_UNLOCK(atppc);

	/* Call all of the installed handlers */
	if (claim) {
		struct atppc_handler_node * callback;
		SLIST_FOREACH(callback, &(atppc->sc_handler_listhead),
			entries) {
				(*callback->func)(callback->arg);
		}
	}

	splx(s);

	return claim;
#else
	return 0;		/* NO INTERRUPTS YET */
#endif
}

/* Functions which support ppbus interface */

static void
pi1ppc_reset_epp_timeout(struct device *dev)
{
	return;
}

/* Read from pi1ppc device: returns 0 on success. */
static int
pi1ppc_read(struct device *dev, char *buf, int len, int ioflag,
	size_t *cnt)
{
	struct pi1ppc_softc *pi1ppc = (struct pi1ppc_softc *)dev;
	int error = 0;
	int s;

	s = splpi1ppc();
	PI1PPC_LOCK(pi1ppc);

	*cnt = 0;

	/* Initialize buffer */
	pi1ppc->sc_inb = pi1ppc->sc_inbstart = buf;
	pi1ppc->sc_inb_nbytes = len;

	/* Initialize device input error state for new operation */
	pi1ppc->sc_inerr = 0;

	/* Call appropriate function to read bytes */
	switch(pi1ppc->sc_mode) {
	case PI1PPC_MODE_STD:
		error = ENODEV;
		break;

	case PI1PPC_MODE_NIBBLE:
		pi1ppc_nibble_read(pi1ppc);
		break;

	case PI1PPC_MODE_PS2:
		pi1ppc_byte_read(pi1ppc);
		break;

	default:
		panic("%s(%s): chipset in invalid mode.\n", __func__,
			dev->dv_xname);
	}

	/* Update counter*/
	*cnt = (pi1ppc->sc_inbstart - pi1ppc->sc_inb);

	/* Reset buffer */
	pi1ppc->sc_inb = pi1ppc->sc_inbstart = NULL;
	pi1ppc->sc_inb_nbytes = 0;

	if (!(error))
		error = pi1ppc->sc_inerr;

	PI1PPC_UNLOCK(pi1ppc);
	splx(s);

	return (error);
}

/* Write to pi1ppc device: returns 0 on success. */
static int
pi1ppc_write(struct device *dev, char *buf, int len, int ioflag, size_t *cnt)
{
	struct pi1ppc_softc * const pi1ppc = (struct pi1ppc_softc *)dev;
	int error = 0;
	int s;

	*cnt = 0;

	s = splpi1ppc();
	PI1PPC_LOCK(pi1ppc);

	/* Set up line buffer */
	pi1ppc->sc_outb = pi1ppc->sc_outbstart = buf;
	pi1ppc->sc_outb_nbytes = len;

	/* Initialize device output error state for new operation */
	pi1ppc->sc_outerr = 0;

	/* Call appropriate function to write bytes */
	switch (pi1ppc->sc_mode) {
	case PI1PPC_MODE_STD:
		pi1ppc_std_write(pi1ppc);
		break;

	case PI1PPC_MODE_NIBBLE:
	case PI1PPC_MODE_PS2:
		error = ENODEV;
		break;

	default:
		panic("%s(%s): chipset in invalid mode.\n", __func__,
			dev->dv_xname);
	}

	/* Update counter*/
	*cnt = (pi1ppc->sc_outbstart - pi1ppc->sc_outb);

	/* Reset output buffer */
	pi1ppc->sc_outb = pi1ppc->sc_outbstart = NULL;
	pi1ppc->sc_outb_nbytes = 0;

	if (!(error))
		error = pi1ppc->sc_outerr;

	PI1PPC_UNLOCK(pi1ppc);
	splx(s);

	return (error);
}

/*
 * Set mode of chipset to mode argument. Modes not supported are ignored. If
 * multiple modes are flagged, the mode is not changed. Modes are those
 * defined for ppbus_softc.sc_mode in ppbus_conf.h. Only ECP-capable chipsets
 * can change their mode of operation. However, ALL operation modes support
 * centronics mode and nibble mode. Modes determine both hardware AND software
 * behaviour.
 * NOTE: the mode for ECP should only be changed when the channel is in
 * forward idle mode. This function does not make sure FIFO's have flushed or
 * any consistency checks.
 */
static int
pi1ppc_setmode(struct device *dev, int mode)
{
	struct pi1ppc_softc *pi1ppc = (struct pi1ppc_softc *)dev;
	u_int8_t ecr;
	u_int8_t chipset_mode;
	int s;
	int rval = 0;

	s = splpi1ppc();
	PI1PPC_LOCK(pi1ppc);

	switch (mode) {
	case PPBUS_PS2:
		/* Indy has this, other PI1 machines do too? */
		chipset_mode = PI1PPC_MODE_PS2;
		break;

	case PPBUS_NIBBLE:
		/* Set nibble mode (virtual) */
		chipset_mode = PI1PPC_MODE_NIBBLE;
		break;

	case PPBUS_COMPATIBLE:
		chipset_mode = PI1PPC_MODE_STD;
		break;

	case PPBUS_ECP:
	case PPBUS_EPP:
		rval = ENODEV;
		goto end;

	default:
		PI1PPC_DPRINTF(("%s(%s): invalid mode passed as "
			"argument.\n", __func__, dev->dv_xname));
		rval = ENODEV;
		goto end;
	}

	pi1ppc->sc_mode = chipset_mode;
	if (chipset_mode == PI1PPC_MODE_PS2) {
		/* Set direction bit to reverse */
		ecr = pi1ppc_r_ctr(pi1ppc);
		pi1ppc_barrier_r(pi1ppc);
		ecr |= PCD;			/* data is INPUT */
		pi1ppc_w_ctr(pi1ppc, ecr);
		pi1ppc_barrier_w(pi1ppc);
	}

end:
	PI1PPC_UNLOCK(pi1ppc);
	splx(s);

	return rval;
}

/* Get the current mode of chipset */
static int
pi1ppc_getmode(struct device *dev)
{
	struct pi1ppc_softc *pi1ppc = (struct pi1ppc_softc *)dev;
	int mode;
	int s;

	s = splpi1ppc();
	PI1PPC_LOCK(pi1ppc);

	/* The chipset can only be in one mode at a time logically */
	switch (pi1ppc->sc_mode) {
	case PI1PPC_MODE_PS2:
		mode = PPBUS_PS2;
		break;

	case PI1PPC_MODE_STD:
		mode = PPBUS_COMPATIBLE;
		break;

	case PI1PPC_MODE_NIBBLE:
		mode = PPBUS_NIBBLE;
		break;

	default:
		panic("%s(%s): device is in invalid mode!", __func__,
			dev->dv_xname);
		break;
	}

	PI1PPC_UNLOCK(pi1ppc);
	splx(s);

	return mode;
}


/* Wait for FIFO buffer to empty for ECP-capable chipset */
static void
pi1ppc_ecp_sync(struct device *dev)
{
	return;
}

/* Execute a microsequence to handle fast I/O operations. */

/* microsequence registers are equivalent to PC-like port registers */
/* therefore, translate bit positions & polarities */

/* Bit 4 of ctl_reg_int_en is used to emulate the PC's int enable
   bit.  Without it, lpt doesn't like the port.
 */
static u_int8_t ctl_reg_int_en = 0;

static u_int8_t
r_reg(int reg, struct pi1ppc_softc *pi1ppc)
{
	int val = 0;

	/* if we read the status reg, make it look like the PC */
	if(reg == AT_STAT_REG) {
		val = bus_space_read_4((pi1ppc)->sc_iot, 
				(pi1ppc)->sc_ioh, IOC_PLP_STAT);
		val &= 0xff;

		/* invert /BUSY */
		val ^= 0x80;

		/* bit 2 reads as '1' on Indy (why?) */
		val &= 0xf8;

		return val;
	}

	/* if we read the ctl reg, make it look like the PC */
	if(reg == AT_CTL_REG) {
		val = bus_space_read_4((pi1ppc)->sc_iot, 
				(pi1ppc)->sc_ioh, IOC_PLP_CTL);
		val &= 0xff;

		/* get the dir bit in the right place */
		val = ((val >> 1) & 0x20) | (val & 0x0f);

		/* invert /SEL, /AUTOFD, and /STB */
		val ^= 0x0b;
	
		/* emulate the PC's int enable ctl bit */
		val |= (ctl_reg_int_en & 0x10);

		return val;
	}

	if(reg == AT_DATA_REG) {
		val = bus_space_read_4((pi1ppc)->sc_iot, 
				(pi1ppc)->sc_ioh, IOC_PLP_DATA);
		val &= 0xff;

		return val;
	}
		
	return 0;
}

static void
w_reg(int reg, struct pi1ppc_softc *pi1ppc, u_int8_t byte)
{
	/* don't try to write to the status reg */

	/* if we are writing the ctl reg, adjust PC style -> IOC style */
	if(reg == AT_CTL_REG) {
		/* preserve pc-style int enable bit */
		ctl_reg_int_en = (byte & 0x10);

		/* get the dir bit in the right place */
		byte = ((byte << 1) & 0x40) | (byte & 0x0f);

		/* invert /SEL, /AUTOFD, and /STB */
		byte ^= 0x0b;

		bus_space_write_4((pi1ppc)->sc_iot, 
				(pi1ppc)->sc_ioh, IOC_PLP_CTL, byte);
	}

	if(reg == AT_DATA_REG) {
		bus_space_write_4((pi1ppc)->sc_iot, 
				(pi1ppc)->sc_ioh, IOC_PLP_DATA, byte);
	}
}

static int
pi1ppc_exec_microseq(struct device *dev, struct ppbus_microseq **p_msq)
{
	struct pi1ppc_softc *pi1ppc = (struct pi1ppc_softc *)dev;
	struct ppbus_microseq *mi = *p_msq;
	char cc, *p;
	int i, iter, len;
	int error;
	int s;
	register int reg;
	register unsigned char mask;
	register int accum = 0;
	register char *ptr = NULL;
	struct ppbus_microseq *stack = NULL;

	s = splpi1ppc();
	PI1PPC_LOCK(pi1ppc);

	/* Loop until microsequence execution finishes (ending op code) */
	for (;;) {
		switch (mi->opcode) {
		case MS_OP_RSET:
			cc = r_reg(mi->arg[0].i, pi1ppc);
			pi1ppc_barrier_r(pi1ppc);
			cc &= (char)mi->arg[2].i;	/* clear mask */
			cc |= (char)mi->arg[1].i;	/* assert mask */
			w_reg(mi->arg[0].i, pi1ppc, cc);
			pi1ppc_barrier_w(pi1ppc);
			mi++;
                       	break;

		case MS_OP_RASSERT_P:
			reg = mi->arg[1].i;
			ptr = pi1ppc->sc_ptr;

			if ((len = mi->arg[0].i) == MS_ACCUM) {
				accum = pi1ppc->sc_accum;
				for (; accum; accum--) {
					w_reg(reg, pi1ppc, *ptr++);
					pi1ppc_barrier_w(pi1ppc);
				}
				pi1ppc->sc_accum = accum;
			} else {
				for (i = 0; i < len; i++) {
					w_reg(reg, pi1ppc, *ptr++);
					pi1ppc_barrier_w(pi1ppc);
				}
			}

			pi1ppc->sc_ptr = ptr;
			mi++;
			break;

       	        case MS_OP_RFETCH_P:
			reg = mi->arg[1].i;
			mask = (char)mi->arg[2].i;
			ptr = pi1ppc->sc_ptr;

			if ((len = mi->arg[0].i) == MS_ACCUM) {
				accum = pi1ppc->sc_accum;
				for (; accum; accum--) {
					*ptr++ = r_reg(reg, pi1ppc) & mask;
					pi1ppc_barrier_r(pi1ppc);
				}
				pi1ppc->sc_accum = accum;
			} else {
				for (i = 0; i < len; i++) {
					*ptr++ = r_reg(reg, pi1ppc) & mask;
					pi1ppc_barrier_r(pi1ppc);
				}
			}

			pi1ppc->sc_ptr = ptr;
			mi++;
			break;

                case MS_OP_RFETCH:
			*((char *)mi->arg[2].p) = r_reg(mi->arg[0].i, pi1ppc) &
				(char)mi->arg[1].i;
			pi1ppc_barrier_r(pi1ppc);
			mi++;
       	                break;

		case MS_OP_RASSERT:
                case MS_OP_DELAY:
			/* let's suppose the next instr. is the same */
			do {
				for (;mi->opcode == MS_OP_RASSERT; mi++) {
					w_reg(mi->arg[0].i, pi1ppc,
						(char)mi->arg[1].i);
					pi1ppc_barrier_w(pi1ppc);
				}

				for (;mi->opcode == MS_OP_DELAY; mi++) {
					delay(mi->arg[0].i);
				}
			} while (mi->opcode == MS_OP_RASSERT);
			break;

		case MS_OP_ADELAY:
			if (mi->arg[0].i) {
				tsleep(pi1ppc, PPBUSPRI, "pi1ppcdelay",
					mi->arg[0].i * (hz/1000));
			}
			mi++;
			break;

		case MS_OP_TRIG:
			reg = mi->arg[0].i;
			iter = mi->arg[1].i;
			p = (char *)mi->arg[2].p;

			/* XXX delay limited to 255 us */
			for (i = 0; i < iter; i++) {
				w_reg(reg, pi1ppc, *p++);
				pi1ppc_barrier_w(pi1ppc);
				delay((unsigned char)*p++);
			}

			mi++;
			break;

		case MS_OP_SET:
                        pi1ppc->sc_accum = mi->arg[0].i;
			mi++;
                       	break;

		case MS_OP_DBRA:
                       	if (--pi1ppc->sc_accum > 0) {
                               	mi += mi->arg[0].i;
			}

			mi++;
			break;

		case MS_OP_BRSET:
			cc = pi1ppc_r_str(pi1ppc);
			pi1ppc_barrier_r(pi1ppc);
			if ((cc & (char)mi->arg[0].i) == (char)mi->arg[0].i) {
				mi += mi->arg[1].i;
			}
			mi++;
			break;

		case MS_OP_BRCLEAR:
			cc = pi1ppc_r_str(pi1ppc);
			pi1ppc_barrier_r(pi1ppc);
			if ((cc & (char)mi->arg[0].i) == 0) {
				mi += mi->arg[1].i;
			}
			mi++;
			break;

		case MS_OP_BRSTAT:
			cc = pi1ppc_r_str(pi1ppc);
			pi1ppc_barrier_r(pi1ppc);
			if ((cc & ((char)mi->arg[0].i | (char)mi->arg[1].i)) ==
				(char)mi->arg[0].i) {
				mi += mi->arg[2].i;
			}
			mi++;
			break;

		case MS_OP_C_CALL:
			/*
			 * If the C call returns !0 then end the microseq.
			 * The current state of ptr is passed to the C function
			 */
			if ((error = mi->arg[0].f(mi->arg[1].p,
				pi1ppc->sc_ptr))) {
				PI1PPC_UNLOCK(pi1ppc);
				splx(s);
				return (error);
			}
			mi++;
			break;

		case MS_OP_PTR:
			pi1ppc->sc_ptr = (char *)mi->arg[0].p;
			mi++;
			break;

		case MS_OP_CALL:
			if (stack) {
				panic("%s - %s: too many calls", dev->dv_xname,
					__func__);
			}

			if (mi->arg[0].p) {
				/* store state of the actual microsequence */
				stack = mi;

				/* jump to the new microsequence */
				mi = (struct ppbus_microseq *)mi->arg[0].p;
			} else {
				mi++;
			}
			break;

		case MS_OP_SUBRET:
			/* retrieve microseq and pc state before the call */
			mi = stack;

			/* reset the stack */
			stack = 0;

			/* XXX return code */

			mi++;
			break;

		case MS_OP_PUT:
		case MS_OP_GET:
		case MS_OP_RET:
			/*
			 * Can't return to pi1ppc level during the execution
			 * of a submicrosequence.
			 */
			if (stack) {
				panic("%s: cannot return to pi1ppc level",
					__func__);
			}
			/* update pc for pi1ppc level of execution */
			*p_msq = mi;

			PI1PPC_UNLOCK(pi1ppc);
			splx(s);
			return (0);
			break;

		default:
			panic("%s: unknown microsequence "
				"opcode 0x%x", __func__, mi->opcode);
			break;
		}
	}

	/* Should not be reached! */
#ifdef PI1PPC_DEBUG
	panic("%s: unexpected code reached!\n", __func__);
#endif
}

/* General I/O routine */
static u_int8_t
pi1ppc_io(struct device *dev, int iop, u_char *addr, int cnt, u_char byte)
{
	struct pi1ppc_softc *pi1ppc = (struct pi1ppc_softc *)dev;
	u_int8_t val = 0;
	int s;

	s = splpi1ppc();
	PI1PPC_LOCK(pi1ppc);

	switch (iop) {
	case PPBUS_RDTR:
		val = r_reg(AT_DATA_REG, pi1ppc);
		break;
	case PPBUS_RSTR:
		val = r_reg(AT_STAT_REG, pi1ppc);
		break;
	case PPBUS_RCTR:
		val = r_reg(AT_CTL_REG, pi1ppc);
		break;
	case PPBUS_WDTR:
		w_reg(AT_DATA_REG, pi1ppc, byte);
		break;
	case PPBUS_WSTR:
		/* writing to the status register is weird */
		break;
	case PPBUS_WCTR:
		w_reg(AT_CTL_REG, pi1ppc, byte);
		break;
	default:
		panic("%s(%s): unknown I/O operation", dev->dv_xname,
			__func__);
		break;
	}

	pi1ppc_barrier(pi1ppc);

	PI1PPC_UNLOCK(pi1ppc);
	splx(s);

	return val;
}

/* Read "instance variables" of pi1ppc device */
static int
pi1ppc_read_ivar(struct device *dev, int index, unsigned int *val)
{
	struct pi1ppc_softc *pi1ppc = (struct pi1ppc_softc *)dev;
	int rval = 0;
	int s;

	s = splpi1ppc();
	PI1PPC_LOCK(pi1ppc);

	switch(index) {
	case PPBUS_IVAR_INTR:
		*val = ((pi1ppc->sc_use & PI1PPC_USE_INTR) != 0);
		break;

	case PPBUS_IVAR_DMA:
		*val = ((pi1ppc->sc_use & PI1PPC_USE_DMA) != 0);
		break;

	default:
		rval = ENODEV;
	}

	PI1PPC_UNLOCK(pi1ppc);
	splx(s);

	return rval;
}

/* Write "instance varaibles" of pi1ppc device */
static int
pi1ppc_write_ivar(struct device *dev, int index, unsigned int *val)
{
	struct pi1ppc_softc *pi1ppc = (struct pi1ppc_softc *)dev;
	int rval = 0;
	int s;

	s = splpi1ppc();
	PI1PPC_LOCK(pi1ppc);

	switch(index) {
	case PPBUS_IVAR_INTR:
		if (*val == 0)
			pi1ppc->sc_use &= ~PI1PPC_USE_INTR;
		else if (pi1ppc->sc_has & PI1PPC_HAS_INTR)
			pi1ppc->sc_use |= PI1PPC_USE_INTR;
		else
			rval = ENODEV;
		break;

	case PPBUS_IVAR_DMA:
		if (*val == 0)
			pi1ppc->sc_use &= ~PI1PPC_USE_DMA;
		else if (pi1ppc->sc_has & PI1PPC_HAS_DMA)
			pi1ppc->sc_use |= PI1PPC_USE_DMA;
		else
			rval = ENODEV;
		break;

	default:
		rval = ENODEV;
	}

	PI1PPC_UNLOCK(pi1ppc);
	splx(s);

	return rval;
}

/* Add a handler routine to be called by the interrupt handler */
static int
pi1ppc_add_handler(struct device *dev, void (*handler)(void *), void *arg)
{
	struct pi1ppc_softc *pi1ppc = (struct pi1ppc_softc *)dev;
	struct pi1ppc_handler_node *callback;
	int rval = 0;
	int s;

	s = splpi1ppc();
	PI1PPC_LOCK(pi1ppc);

	if (handler == NULL) {
		PI1PPC_DPRINTF(("%s(%s): attempt to register NULL handler.\n",
			__func__, dev->dv_xname));
		rval = EINVAL;
	} else {
		callback = malloc(sizeof(struct pi1ppc_handler_node), M_DEVBUF,
			M_NOWAIT);
		if (callback) {
			callback->func = handler;
			callback->arg = arg;
			SLIST_INSERT_HEAD(&(pi1ppc->sc_handler_listhead),
				callback, entries);
		} else {
			rval = ENOMEM;
		}
	}

	PI1PPC_UNLOCK(pi1ppc);
	splx(s);

	return rval;
}

/* Remove a handler added by pi1ppc_add_handler() */
static int
pi1ppc_remove_handler(struct device *dev, void (*handler)(void *))
{
	struct pi1ppc_softc *pi1ppc = (struct pi1ppc_softc *)dev;
	struct pi1ppc_handler_node *callback;
	int rval = EINVAL;
	int s;

	s = splpi1ppc();
	PI1PPC_LOCK(pi1ppc);

	if (SLIST_EMPTY(&(pi1ppc->sc_handler_listhead)))
		panic("%s(%s): attempt to remove handler from empty list.\n",
			__func__, dev->dv_xname);

	/* Search list for handler */
	SLIST_FOREACH(callback, &(pi1ppc->sc_handler_listhead), entries) {
		if (callback->func == handler) {
			SLIST_REMOVE(&(pi1ppc->sc_handler_listhead), callback,
				pi1ppc_handler_node, entries);
			free(callback, M_DEVBUF);
			rval = 0;
			break;
		}
	}

	PI1PPC_UNLOCK(pi1ppc);
	splx(s);

	return rval;
}

/* Utility functions */

/*
 * Functions that read bytes from port into buffer: called from interrupt
 * handler depending on current chipset mode and cause of interrupt. Return
 * value: number of bytes moved.
 */

/* note: BUSY is inverted in the PC world, but not on Indy, but the r_reg()
	 and w_reg() functions make the Indy look like the PC. */ 

/* Only the lower 4 bits of the final value are valid */
#define nibble2char(s) ((((s) & ~nACK) >> 3) | (~(s) & nBUSY) >> 4)


/* Read bytes in nibble mode */
static void
pi1ppc_nibble_read(struct pi1ppc_softc *pi1ppc)
{
	int i;
	u_int8_t nibble[2];
	u_int8_t ctr;
	u_int8_t str;

	/* Enable interrupts if needed */
	if (pi1ppc->sc_use & PI1PPC_USE_INTR) {

		/* XXX JOE - need code to enable interrupts 
				--> emulate PC behavior in r_reg/w_reg
		*/
#if 0
		ctr = pi1ppc_r_ctr(pi1ppc);
		pi1ppc_barrier_r(ioppc);
		if (!(ctr & IRQENABLE)) {
			ctr |= IRQENABLE;
			pi1ppc_w_ctr(pi1ppc, ctr);
			pi1ppc_barrier_w(pi1ppc);
		}
#endif
	}

	while (pi1ppc->sc_inbstart < (pi1ppc->sc_inb + pi1ppc->sc_inb_nbytes)) {
		/* Check if device has data to send in idle phase */
		str = pi1ppc_r_str(pi1ppc);
		pi1ppc_barrier_r(pi1ppc);
		if (str & nDATAVAIL) {
			return;
		}

		/* Nibble-mode handshake transfer */
		for (i = 0; i < 2; i++) {
			/* Event 7 - ready to take data (HOSTBUSY low) */
			ctr = pi1ppc_r_ctr(pi1ppc);
			pi1ppc_barrier_r(pi1ppc);
			ctr |= HOSTBUSY;
			pi1ppc_w_ctr(pi1ppc, ctr);
			pi1ppc_barrier_w(pi1ppc);

			/* Event 8 - peripheral writes the first nibble */

			/* Event 9 - peripheral set nAck low */
			pi1ppc->sc_inerr = pi1ppc_poll_str(pi1ppc, 0, PTRCLK);
			if (pi1ppc->sc_inerr)
				return;

			/* read nibble */
			nibble[i] = pi1ppc_r_str(pi1ppc);

			/* Event 10 - ack, nibble received */
			ctr &= ~HOSTBUSY;
			pi1ppc_w_ctr(pi1ppc, ctr);

			/* Event 11 - wait ack from peripherial */
			if (pi1ppc->sc_use & PI1PPC_USE_INTR)
				pi1ppc->sc_inerr = pi1ppc_wait_interrupt(pi1ppc,
					pi1ppc->sc_inb, PI1PPC_IRQ_nACK);
			else
				pi1ppc->sc_inerr = pi1ppc_poll_str(pi1ppc, PTRCLK,
					PTRCLK);
			if (pi1ppc->sc_inerr)
				return;
		}

		/* Store byte transfered */
		*(pi1ppc->sc_inbstart) = ((nibble2char(nibble[1]) << 4) & 0xf0) |
			(nibble2char(nibble[0]) & 0x0f);
		pi1ppc->sc_inbstart++;
	}
}

/* Read bytes in bidirectional mode */
static void
pi1ppc_byte_read(struct pi1ppc_softc * const pi1ppc)
{
	u_int8_t ctr;
	u_int8_t str;

	/* Check direction bit */
	ctr = pi1ppc_r_ctr(pi1ppc);
	pi1ppc_barrier_r(pi1ppc);
	if (!(ctr & PCD)) {
		PI1PPC_DPRINTF(("%s: byte-mode read attempted without direction "
			"bit set.", pi1ppc->sc_dev.dv_xname));
		pi1ppc->sc_inerr = ENODEV;
		return;
	}
	/* Enable interrupts if needed */

		/* XXX JOE - need code to enable interrupts */
#if 0	
	if (pi1ppc->sc_use & PI1PPC_USE_INTR) {
		if (!(ctr & IRQENABLE)) {
			ctr |= IRQENABLE;
			pi1ppc_w_ctr(pi1ppc, ctr);
			pi1ppc_barrier_w(pi1ppc);
		}
	}
#endif

	/* Byte-mode handshake transfer */
	while (pi1ppc->sc_inbstart < (pi1ppc->sc_inb + pi1ppc->sc_inb_nbytes)) {
		/* Check if device has data to send */
		str = pi1ppc_r_str(pi1ppc);
		pi1ppc_barrier_r(pi1ppc);
		if (str & nDATAVAIL) {
			return;
		}

		/* Event 7 - ready to take data (nAUTO low) */
		ctr |= HOSTBUSY;
		pi1ppc_w_ctr(pi1ppc, ctr);
		pi1ppc_barrier_w(pi1ppc);

		/* Event 9 - peripheral set nAck low */
		pi1ppc->sc_inerr = pi1ppc_poll_str(pi1ppc, 0, PTRCLK);
		if (pi1ppc->sc_inerr)
			return;

		/* Store byte transfered */
		*(pi1ppc->sc_inbstart) = pi1ppc_r_dtr(pi1ppc);
		pi1ppc_barrier_r(pi1ppc);

		/* Event 10 - data received, can't accept more */
		ctr &= ~HOSTBUSY;
		pi1ppc_w_ctr(pi1ppc, ctr);
		pi1ppc_barrier_w(pi1ppc);

		/* Event 11 - peripheral ack */
		if (pi1ppc->sc_use & PI1PPC_USE_INTR)
			pi1ppc->sc_inerr = pi1ppc_wait_interrupt(pi1ppc,
				pi1ppc->sc_inb, PI1PPC_IRQ_nACK);
		else
			pi1ppc->sc_inerr = pi1ppc_poll_str(pi1ppc, PTRCLK, PTRCLK);
		if (pi1ppc->sc_inerr)
			return;

		/* Event 16 - strobe */
		str |= HOSTCLK;
		pi1ppc_w_str(pi1ppc, str);
		pi1ppc_barrier_w(pi1ppc);
		DELAY(1);
		str &= ~HOSTCLK;
		pi1ppc_w_str(pi1ppc, str);
		pi1ppc_barrier_w(pi1ppc);

		/* Update counter */
		pi1ppc->sc_inbstart++;
	}
}

/*
 * Functions that write bytes to port from buffer: called from pi1ppc_write()
 * function depending on current chipset mode. Returns number of bytes moved.
 */

static void
pi1ppc_set_intr_mask(struct pi1ppc_softc * const pi1ppc, u_int8_t mask)
{
	/* invert valid bits (0 = enabled) */
	mask = ~mask;
	mask &= 0xfc;

	bus_space_write_4((pi1ppc)->sc_iot, (pi1ppc)->sc_ioh, IOC_PLP_INTMASK, mask);
	pi1ppc_barrier_w(pi1ppc);
}


#ifdef USE_INDY_ACK_HACK
static u_int8_t
pi1ppc_get_intr_mask(struct pi1ppc_softc * const pi1ppc)
{
	int val;
	val = bus_space_read_4((pi1ppc)->sc_iot, (pi1ppc)->sc_ioh, IOC_PLP_INTMASK);
	pi1ppc_barrier_r(pi1ppc);

	/* invert (0 = enabled) */
	val = ~val;

	return (val & 0xfc);
}
#endif

static u_int8_t
pi1ppc_get_intr_stat(struct pi1ppc_softc * const pi1ppc)
{
	int val;
	val = bus_space_read_4((pi1ppc)->sc_iot, (pi1ppc)->sc_ioh, IOC_PLP_INTSTAT);
	pi1ppc_barrier_r(pi1ppc);

	return (val & 0xfc);
}

/* Write bytes in std/bidirectional mode */
static void
pi1ppc_std_write(struct pi1ppc_softc * const pi1ppc)
{
	unsigned char ctr;

	ctr = pi1ppc_r_ctr(pi1ppc);
	pi1ppc_barrier_r(pi1ppc);

	/* Ensure that the data lines are in OUTPUT mode */
	ctr &= ~PCD;
	pi1ppc_w_ctr(pi1ppc, ctr);
	pi1ppc_barrier_w(pi1ppc);
	
	/* XXX JOE - need code to enable interrupts */
#if 0	
	/* Enable interrupts if needed */
	if (pi1ppc->sc_use & PI1PPC_USE_INTR) {
		if (!(ctr & IRQENABLE)) {
			ctr |= IRQENABLE;
			pi1ppc_w_ctr(pi1ppc, ctr);
			pi1ppc_barrier_w(pi1ppc);
		}
	}
#endif

	while (pi1ppc->sc_outbstart < (pi1ppc->sc_outb + pi1ppc->sc_outb_nbytes)) {

		/* Wait for peripheral to become ready for MAXBUSYWAIT */
		pi1ppc->sc_outerr = pi1ppc_poll_str(pi1ppc, SPP_READY, SPP_MASK);
		if (pi1ppc->sc_outerr) {
			printf("pi1ppc: timeout waiting for peripheral to become ready\n");
			return;
		}

		/* Put data in data register */
		pi1ppc_w_dtr(pi1ppc, *(pi1ppc->sc_outbstart));
		pi1ppc_barrier_w(pi1ppc);
		DELAY(1);

		/* If no intr, prepare to catch the rising edge of nACK */
		if (!(pi1ppc->sc_use & PI1PPC_USE_INTR)) {
			pi1ppc_get_intr_stat(pi1ppc);	/* clear any pending intr */
			pi1ppc_set_intr_mask(pi1ppc, PI1_PLP_ACK_INTR);
		}

		/* Pulse strobe to indicate valid data on lines */
		ctr |= STROBE;
		pi1ppc_w_ctr(pi1ppc, ctr);
		pi1ppc_barrier_w(pi1ppc);
		DELAY(1);
		ctr &= ~STROBE;
		pi1ppc_w_ctr(pi1ppc, ctr);
		pi1ppc_barrier_w(pi1ppc);

		/* Wait for nACK for MAXBUSYWAIT */
		if (pi1ppc->sc_use & PI1PPC_USE_INTR) {
			pi1ppc->sc_outerr = pi1ppc_wait_interrupt(pi1ppc,
				pi1ppc->sc_outb, PI1PPC_IRQ_nACK);
			if (pi1ppc->sc_outerr)
				return;
		} else {
			/* Try to catch the pulsed acknowledgement */
			pi1ppc->sc_outerr = pi1ppc_poll_interrupt_stat(pi1ppc,
				PI1_PLP_ACK_INTR);

			if (pi1ppc->sc_outerr) {
				printf("pi1ppc: timeout waiting for ACK: %02x\n",pi1ppc_r_str(pi1ppc));
				return;
			}
		}

		/* Update buffer position, byte count and counter */
		pi1ppc->sc_outbstart++;
	}
}


/*
 * Poll status register using mask and status for MAXBUSYWAIT.
 * Returns 0 if device ready, error value otherwise.
 */
static int
pi1ppc_poll_str(struct pi1ppc_softc * const pi1ppc, const u_int8_t status,
	const u_int8_t mask)
{
	unsigned int timecount;
	u_int8_t str;
	int error = EIO;

	/* Wait for str to have status for MAXBUSYWAIT */
	for (timecount = 0; timecount < ((MAXBUSYWAIT/hz)*1000000);
		timecount++) {

		str = pi1ppc_r_str(pi1ppc);
		pi1ppc_barrier_r(pi1ppc);
		if ((str & mask) == status) {
			error = 0;
			break;
		}
		DELAY(1);
	}

	return error;
}

/* Wait for interrupt for MAXBUSYWAIT: returns 0 if acknowledge received. */
static int
pi1ppc_wait_interrupt(struct pi1ppc_softc * const pi1ppc, const caddr_t where,
	const u_int8_t irqstat)
{
	int error = EIO;

	pi1ppc->sc_irqstat &= ~irqstat;

	/* Wait for interrupt for MAXBUSYWAIT */
	error = ltsleep(where, PPBUSPRI | PCATCH, __func__, MAXBUSYWAIT,
		PI1PPC_SC_LOCK(pi1ppc));

	if (!(error) && (pi1ppc->sc_irqstat & irqstat)) {
		pi1ppc->sc_irqstat &= ~irqstat;
		error = 0;
	}

	return error;
}

/*
	INDY ACK HACK DESCRIPTION

	There appears to be a bug in the Indy's PI1 hardware - it sometimes 
	*misses* the rising edge of /ACK.  Ugh!

	(Also, unlike the other status bits, /ACK doesn't generate an
	 interrupt on its falling edge.)

	So, we do something kind of skanky here.  We use a shorter timeout,
	and, if we timeout, we first check BUSY.  If BUSY is high, we go
	back to waiting for /ACK (because maybe this really is just a slow
	peripheral).

	If it's a normal printer, it will raise BUSY from when it sees our
	/STROBE until it raises its /ACK:  
		_____   _____________________ 
	/STB	     \_/
		________________   __________
	/ACK	                \_/
		       ___________
	BUSY	______/           \__________

	So, if we time out and see BUSY low, then we probably just missed
	the /ACK.

	In that case, we then check /ERROR and SELECTIN.  If both are hi, 
	(the peripheral thinks it is selected, and is not asserting /ERROR)
	we assume that the Indy's parallel port missed the /ACK, and return
	success.
 */

#ifdef USE_INDY_ACK_HACK
	#define	ACK_TIMEOUT_SCALER	1000
#else
	#define ACK_TIMEOUT_SCALER	1000000
#endif

static int
pi1ppc_poll_interrupt_stat(struct pi1ppc_softc * const pi1ppc, 
	const u_int8_t match)
{
	unsigned int timecount;
	u_int8_t cur;
	int error = EIO;

#ifdef USE_INDY_ACK_HACK
	/* retry 10000x */
	int retry_count = 10000;		

retry:
#endif

	/* Wait for intr status to have match bits set for MAXBUSYWAIT */
	for (timecount = 0; timecount < ((MAXBUSYWAIT/hz)*ACK_TIMEOUT_SCALER); 
		timecount++) {
		cur = pi1ppc_get_intr_stat(pi1ppc);
		if ((cur & match) == match) {
			error = 0;
			break;
		}
		DELAY(1);
	}

#ifdef USE_INDY_ACK_HACK
	if(error != 0) {
		cur = pi1ppc_r_str(pi1ppc);

		/* retry if BUSY is hi (inverted, so lo) and we haven't 
			waited the usual amt */

		if(((cur&nBUSY) == 0) && retry_count) {
			retry_count--;
			goto retry;
		}

		/* if /ERROR and SELECT are high, and the peripheral isn't
	   		BUSY, assume that we just missed the /ACK.
			(Remember, we emulate the PC's inverted BUSY!)
		*/

		if((cur&(nFAULT|SELECT|nBUSY)) == (nFAULT|SELECT|nBUSY))
			error = 0;

		/* if things still look bad, print out some info */
		if(error!=0)
			printf("int mask=%02x, int stat=%02x, str=%02x\n",
						pi1ppc_get_intr_mask(pi1ppc),
						pi1ppc_get_intr_stat(pi1ppc),
						cur);
	}
#endif

	return error;
}

