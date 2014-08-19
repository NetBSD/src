/*	$NetBSD: cpi_nubus.c,v 1.6.6.2 2014/08/20 00:03:11 tls Exp $	*/

/*-
 * Copyright (c) 2008 Hauke Fath
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,     
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpi_nubus.c,v 1.6.6.2 2014/08/20 00:03:11 tls Exp $");
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/event.h>
#include <sys/callout.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/errno.h>

#include <machine/intr.h>
#include <machine/bus.h>
#include <machine/viareg.h>

#include <dev/ic/z8536reg.h>
#include <mac68k/nubus/nubus.h>
#include <mac68k/nubus/cpi_nubusvar.h>

#include "ioconf.h"

#ifdef DEBUG
#define CPI_DEBUG
#endif

/*
 * Stuff taken from Egan/Teixeira ch 8: 'if(TRACE_FOO)' debug output 
 * statements don't break indentation, and when DEBUG is not defined, 
 * the compiler code optimizer drops them as dead code.
 */
#ifdef CPI_DEBUG
#define M_TRACE_CONFIG	0x0001
#define M_TRACE_OPEN	0x0002
#define M_TRACE_CLOSE	0x0004
#define M_TRACE_READ	0x0008
#define M_TRACE_WRITE	0x0010
#define M_TRACE_IOCTL	0x0020
#define M_TRACE_STATUS	0x0040
#define M_TRACE_TCNTR	0x0080
#define M_TRACE_ALL	0xFFFF
#define M_TRACE_NONE	0x0000

#define TRACE_CONFIG	(cpi_debug_mask & M_TRACE_CONFIG)
#define TRACE_OPEN	(cpi_debug_mask & M_TRACE_OPEN)
#define TRACE_CLOSE	(cpi_debug_mask & M_TRACE_CLOSE)
#define TRACE_READ	(cpi_debug_mask & M_TRACE_READ)
#define TRACE_WRITE	(cpi_debug_mask & M_TRACE_WRITE)
#define TRACE_IOCTL	(cpi_debug_mask & M_TRACE_IOCTL)
#define TRACE_STATUS	(cpi_debug_mask & M_TRACE_STATUS)
#define TRACE_TCNTR	(cpi_debug_mask & M_TRACE_TCNTR)
#define TRACE_ALL	(cpi_debug_mask & M_TRACE_ALL)
#define TRACE_NONE	(cpi_debug_mask & M_TRACE_NONE)

uint32_t cpi_debug_mask = M_TRACE_NONE /* | M_TRACE_TCNTR | M_TRACE_WRITE */ ;
#else /* CPI_DEBUG */
#define TRACE_CONFIG	0
#define TRACE_OPEN	0
#define TRACE_CLOSE	0
#define TRACE_READ	0
#define TRACE_WRITE	0
#define TRACE_IOCTL	0
#define TRACE_STATUS	0
#define TRACE_TCNTR	0
#define TRACE_ALL	0
#define TRACE_NONE	0
#endif /* CPI_DEBUG */

/* autoconf interface */
int cpi_nubus_match(device_t, cfdata_t, void *);
void cpi_nubus_attach(device_t, device_t, void *);
void cpi_nubus_intr(void *);

CFATTACH_DECL_NEW(cpi, sizeof(struct cpi_softc),
    cpi_nubus_match, cpi_nubus_attach, NULL, NULL);

dev_type_open(cpi_open);
dev_type_close(cpi_close);
dev_type_read(cpi_read);
dev_type_write(cpi_write);
dev_type_ioctl(cpi_ioctl);

const struct cdevsw cpi_cdevsw = {
	.d_open = cpi_open,
	.d_close = cpi_close,
	.d_read = noread,
	.d_write = cpi_write,
	.d_ioctl = cpi_ioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

/* prototypes */
static void cpi_lpreset(struct cpi_softc *);
static int cpi_notready(struct cpi_softc *);
static void cpi_wakeup(void *);
static int cpi_flush(struct cpi_softc *);
static void cpi_intr(void *);

static void cpi_tc_initclock(struct cpi_softc *);
static uint cpi_get_timecount(struct timecounter *);
static uint z8536_read_counter1(bus_space_tag_t, bus_space_handle_t);
static uint z8536_read_counter2(bus_space_tag_t, bus_space_handle_t);
static void z8536_reg_set(bus_space_tag_t, bus_space_handle_t,
    uint8_t, uint8_t);
static uint8_t z8536_reg_get(bus_space_tag_t, bus_space_handle_t,
    uint8_t);


const uint8_t cio_reset[] = {
	/* register	value */
	Z8536_MICR, 	0x00,
	Z8536_MICR, 	MICR_RESET,
	Z8536_MICR, 	0x00
};

const uint8_t cio_init[] = {
	/* register	value */

	/* Interrupt vectors - clear all */
	Z8536_IVRA, 	0x00,
	Z8536_IVRB, 	0x00,
	Z8536_IVRCT, 	0x20 /* ??? Do we use this? */,

	/*
	 * Port A specification - bit port, single buffered,
	 * latched output, pulsed handshake, all bits non-inverting
	 * non-special I/O
	 */
	Z8536_PMSRA, 	PMSR_PTS_OUT | PMSR_LPM,
	Z8536_PHSRA, 	PHSR_HTS_PUL,
	Z8536_DPPRA, 	0x00,
	Z8536_DDRA, 	0x00,
	Z8536_SIOCRA, 	0x00,
		
	/*
	 * Port B specification - bit port, transparent output,
	 * pulsed handshake, all bits non-inverting
	 * bits 0, 4 output; bits 1-3, 5-8 input,
	 * non-special I/O
	 * Pattern matching: Bit 6 (BUSY) matching "1"
	 * Alternatively: Bit 3 (/ACK) matching "0"
	 */
	Z8536_PMSRB, 	PMSR_PMS_OR_PEV,
	Z8536_PHSRB, 	0x00,
	Z8536_DPPRB, 	0x00,
	Z8536_DDRB, 	0xee /*11101110b*/,
	Z8536_SIOCRB, 	0x00,
	Z8536_PPRB, 	0x00,
	Z8536_PTRB, 	0x00,
	Z8536_PMRB, 	0x40 /*01000000b = PB6 */,
	
	Z8536_PDRB, 	0xFE,	/* Assign printer -RESET */
	Z8536_PCSRA, 	0x00,	/* Clear port A interrupt bits */
		
	/*
	 * Port C specification - bit 3 out, bits 0-2 in,
	 * all 4 non-inverting, non-special I/O 
	 */
	Z8536_DDRC, 	0x07 /*00000111b*/,
	Z8536_DPPRC, 	0x00,
	Z8536_SIOCRC, 	0x00,

	/*
	 * We need Timer 3 for running port A in strobed mode.
	 *
	 * Counter/Timer 3 specification -- clear IP & IUS, trigger +
	 * gate command bit, one-shot operation
	 */
	Z8536_CTCSR3, 	CTCS_CLR_IP_IUS | CTCS_GCB | CTCS_TCB,
	Z8536_CTMSR3, 	CTMS_DCS_ONESHOT,		
	Z8536_CTTCR3_MSB, 0x00,		
	Z8536_CTTCR3_LSB, 0x03,		

	/* Enable ports A+B+C+CT3 */
	Z8536_MCCR,	MCCR_PAE | MCCR_PBE | MCCR_PC_CT3E,
	
	/* Master Interrupt Enable, Disable Lower Chain,
	 * No Interrupt Vector, port A+B+CT vectors include status */
	Z8536_MICR,  	MICR_MIE | MICR_DLC | MICR_NV | MICR_PAVIS |
	MICR_PBVIS | MICR_CTVIS,
	Z8536_PDRB, 	0xFE,	/* Clear printer -RESET */
};

/* CPI default options */
/* int	cpi_options = 0 | CPI_CTC12_IS_TIMECOUNTER; */


/* 
 * Look for Creative Systems Inc. "Hurdler Centronics Parallel Interface"
 */
int
cpi_nubus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct nubus_attach_args *na;

	na = aux;
	if ((na->category == NUBUS_CATEGORY_COMMUNICATIONS) &&
	    (na->type == NUBUS_TYPE_CENTRONICS) &&
	    (na->drsw == NUBUS_DRSW_CPI) &&
	    (na->drhw == NUBUS_DRHW_CPI))
		return 1;
	else
		return 0;
}

void
cpi_nubus_attach(device_t parent, device_t self, void *aux)
{
	struct cpi_softc *sc;
	struct nubus_attach_args *na;
	int err, ii;

	sc = device_private(self);
	sc->sc_options = (device_cfdata(self)->cf_flags & CPI_OPTIONS_MASK);

	na = aux;
	sc->sc_bst = na->na_tag;
	memcpy(&sc->sc_slot, na->fmt, sizeof(nubus_slot));
	sc->sc_basepa = (bus_addr_t)NUBUS_SLOT2PA(na->slot);

	/*
	 * The CIO sits eight bit wide on the top byte lane of
	 * Nubus, so map 16 byte.
	 */
	if (TRACE_CONFIG) {
		printf("\n");
		printf("\tcpi_nubus_attach() mapping 8536 CIO at 0x%lx.\n",
		    sc->sc_basepa + CIO_BASE_OFFSET);
	}

	err = bus_space_map(sc->sc_bst, sc->sc_basepa + CIO_BASE_OFFSET,
	    (Z8536_IOSIZE << 4), 0, &sc->sc_bsh);
	if (err) {
		aprint_normal(": failed to map memory space.\n");
		return;
	}
	
	sc->sc_lpstate = LP_INITIAL;
	sc->sc_intcount = 0;
	sc->sc_bytestoport = 0;

	if (TRACE_CONFIG)
		printf("\tcpi_nubus_attach() about to set up 8536 CIO.\n");

	for (ii = 0; ii < sizeof(cio_reset); ii += 2)
		z8536_reg_set(sc->sc_bst, sc->sc_bsh, cio_reset[ii],
		    cio_reset[ii + 1]);
	
	delay(1000);		/* Give the CIO time to set itself up */
	for (ii = 0; ii < sizeof(cio_init); ii += 2) {
		z8536_reg_set(sc->sc_bst, sc->sc_bsh, cio_init[ii],
		    cio_init[ii + 1]);
	}
	
	if (TRACE_CONFIG)
		printf("\tcpi_nubus_attach() done with 8536 CIO setup.\n");
		
	/* XXX Get information strings from the card ROM */
	aprint_normal(": CSI Hurdler II Centronics\n");

	/* Attach CIO timers 1+2 as timecounter */
	if (sc->sc_options & CPI_CTC12_IS_TIMECOUNTER) {
		cpi_tc_initclock(sc);
	}

	callout_init(&sc->sc_wakeupchan, 0);	/* XXX */
	
	/* make sure interrupts are vectored to us */
	add_nubus_intr(na->slot, cpi_nubus_intr, sc);
}

void
cpi_nubus_intr(void *arg)
{
        struct cpi_softc *sc;
	int s;

	sc = (struct cpi_softc *)arg;

	s = spltty();

	sc->sc_intcount++;
	
	/* Check for interrupt source, and clear interrupt */

	/*
	 * Clear port A interrupt
	 * Interrupt from register A, clear "pending"
	 * and set "under service"
	 */
	z8536_reg_set(sc->sc_bst, sc->sc_bsh, Z8536_PCSRA, PCSR_CLR_IE);
	z8536_reg_set(sc->sc_bst, sc->sc_bsh, Z8536_PCSRA, PCSR_CLR_IP);
	z8536_reg_set(sc->sc_bst, sc->sc_bsh, Z8536_PCSRA, PCSR_SET_IUS);

	cpi_intr(sc);

	/* Interrupt from register A, mark serviced */
	z8536_reg_set(sc->sc_bst, sc->sc_bsh, Z8536_PCSRA, PCSR_CLR_IUS);
	z8536_reg_set(sc->sc_bst, sc->sc_bsh, Z8536_PCSRA, PCSR_SET_IE);

	splx(s);
}


/* cpi nuts and bolts */

int
cpi_open(dev_t device, int flag, int mode, struct lwp *l)
{
	int err, ii, s;
        struct cpi_softc *sc;

	if (TRACE_OPEN)
		printf("\tcpi_open() called...\n");

	/* Consistency checks: Valid unit number, softc, device state */
	sc = device_lookup_private(&cpi_cd, CPI_UNIT(device));
	if (NULL == sc) {
		if (TRACE_OPEN)
			printf("Tried to cpi_open() with NULL softc\n");
		return ENXIO;
	}
	if (sc->sc_lpstate != LP_INITIAL) {
		if (TRACE_OPEN)
			printf("Not in initial state (%x).\n",
			    sc->sc_lpstate);
		return EBUSY;
	}
	sc->sc_lpstate = LP_OPENING;

	if (TRACE_OPEN)
		printf("\tcpi_open() resetting the printer...\n");
	cpi_lpreset(sc);

	if (TRACE_OPEN)
		printf("\tcpi_open() waiting for printer ready...\n");
	
	/* Wait max 15 sec for printer to get ready */
	for (ii = 15; cpi_notready(sc); ii--) {
		if (0 == ii) {
			sc->sc_lpstate = LP_INITIAL;
			return EBUSY;
		}
		/* sleep for a second, unless we get a signal */
		err = tsleep(sc, PZERO | PCATCH, "cpi_open", hz);
		if (err != EWOULDBLOCK) {
			sc->sc_lpstate = LP_INITIAL;
			return err;
		}
	}
	if (TRACE_OPEN)
		printf("\tcpi_open() allocating printer buffer...\n");
	
	/* Allocate the driver's line buffer */
	sc->sc_printbuf = malloc(CPI_BUFSIZE, M_DEVBUF, M_WAITOK);
	sc->sc_bufbytes = 0;
	sc->sc_lpstate = LP_OPEN;

	/* Statistics */
	sc->sc_intcount = 0;
	sc->sc_bytestoport = 0;

	/* Kick off transfer */
	cpi_wakeup(sc);

	/*
	 * Reset "interrupt {pending, under service}" bits, then
	 * enable Port A interrupts
	 */
	s = spltty();
	
	z8536_reg_set(sc->sc_bst, sc->sc_bsh, Z8536_PCSRA, PCSR_CLR_IP_IUS);
	z8536_reg_set(sc->sc_bst, sc->sc_bsh, Z8536_PCSRA, PCSR_SET_IE);
	splx(s);
	
	if (TRACE_OPEN)
		printf("\tcpi_open() done...\n");

	return 0;
}

int
cpi_close(dev_t device, int flag, int mode, struct lwp *l)
{
        struct cpi_softc *sc;
	
	sc = device_lookup_private(&cpi_cd, CPI_UNIT(device));

	if (TRACE_CLOSE)
		printf("\tcpi_close() called (%lu hard, %lu bytes to port)\n",
		    sc->sc_intcount, sc->sc_bytestoport);

	/* Flush the remaining buffer content, ignoring any errors */
	if (0 < sc->sc_bufbytes)
		(void)cpi_flush(sc);
	
	callout_stop(&sc->sc_wakeupchan);

	/* Disable Port A interrupts */
	z8536_reg_set(sc->sc_bst, sc->sc_bsh, Z8536_PCSRA, PCSR_CLR_IE);
	z8536_reg_set(sc->sc_bst, sc->sc_bsh, Z8536_PCSRA, PCSR_CLR_IP_IUS);

	sc->sc_lpstate = LP_INITIAL;
	free(sc->sc_printbuf, M_DEVBUF);
	
	return 0;
}

int
cpi_write(dev_t device, struct uio *uio, int flags)
{
	int err;
	size_t numbytes;
        struct cpi_softc *sc;

	err = 0;
	
	if (TRACE_WRITE)
		printf("\tcpi_write() called for %u bytes\n", uio->uio_resid);

	sc = device_lookup_private(&cpi_cd, CPI_UNIT(device));

	/* Send data to printer, a line buffer full at a time */
	while (uio->uio_resid > 0) {
		numbytes = min(CPI_BUFSIZE, uio->uio_resid);
		sc->sc_cp = sc->sc_printbuf;
		uiomove(sc->sc_cp, numbytes, uio);
		sc->sc_bufbytes = numbytes;

		if (TRACE_WRITE)
			printf("\tQueuing %u bytes\n", numbytes);
		err = cpi_flush(sc);
		if (err) {
			/* Failure; adjust residual counter */
			if (TRACE_WRITE)
				printf("\tQueuing failed with %d\n", err);
			uio->uio_resid += sc->sc_bufbytes;
			sc->sc_bufbytes = 0;
			break;
		}
	}
	return err;
}

int
cpi_ioctl(dev_t device, unsigned long cmd, void *data,
    int flag, struct lwp *l)
{
	int err;

	err = 0;

	if (TRACE_IOCTL)
		printf("\tcpi_ioctl() called with %ld...\n", cmd);
	
	switch (cmd) {
	default:
		if (TRACE_IOCTL)
			printf("\tcpi_ioctl() unknown ioctl %ld\n", cmd);
		err = ENODEV;
		break;
	}
	return err;
}

/*
 * Flush the print buffer that our top half uses to provide data to
 * our bottom, interrupt-driven half.
 */
static int
cpi_flush(struct cpi_softc *sc)
{
	int err, s;

	err = 0;
	while (0 < sc->sc_bufbytes) {
		/* Feed the printer a char, if it's ready */
		if ( !cpi_notready(sc)) {
			if (TRACE_WRITE)
				printf("\tcpi_flush() writes %u bytes "
				    "(%lu hard, %lu bytes to port)\n",
				    sc->sc_bufbytes, sc->sc_intcount,
				    sc->sc_bytestoport);
			s = spltty();
			cpi_intr(sc);
			splx(s);
		}
		/* XXX Sure we want to wait forever for the printer? */
		err = tsleep((void *)sc, PZERO | PCATCH,
		    "cpi_flush", (60 * hz));
	}
	return err;
}

	
static void
cpi_wakeup(void *param)
{
	struct cpi_softc *sc;
	int s;
	
	sc = param;

	s = spltty();
	cpi_intr(sc);
	splx(s);
	
	callout_reset(&sc->sc_wakeupchan, hz, cpi_wakeup, sc);
}
	

static void
cpi_lpreset(struct cpi_softc *sc)
{
	uint8_t portb;		/* Centronics -RESET is on port B, bit 0 */

	portb = z8536_reg_get(sc->sc_bst, sc->sc_bsh, Z8536_PDRB);
	z8536_reg_set(sc->sc_bst, sc->sc_bsh, Z8536_PDRB, portb & ~CPI_RESET);
	delay(100);
	portb = z8536_reg_get(sc->sc_bst, sc->sc_bsh, Z8536_PDRB);
	z8536_reg_set(sc->sc_bst, sc->sc_bsh, Z8536_PDRB, portb | CPI_RESET);
}


/*
 * Centronics BUSY 		is on port B, bit 6
 *	      SELECT		is on Port B, bit 5
 *	      /FAULT		is on Port B, bit 1
 *            PAPER EMPTY	is on Port C, bit 1
 */
static int
cpi_notready(struct cpi_softc *sc)
{
	uint8_t portb, portc;
	int is_busy, is_select, is_fault, is_paper_empty;

	if (TRACE_STATUS)
		printf("\tcpi_notready() checking printer status...\n");
	
	portb = bus_space_read_1(sc->sc_bst, sc->sc_bsh, CIO_PORTB);
	if (TRACE_STATUS)
		printf("\tPort B has 0x0%X\n", portb);
	
	is_busy = CPI_BUSY & portb;
	if (TRACE_STATUS)
		printf("\t\tBUSY = %d\n", is_busy);
	
	is_select = CPI_SELECT & portb;
	if (TRACE_STATUS)
		printf("\t\tSELECT = %d\n", is_select);
	
	is_fault = CPI_FAULT & portb;
	if (TRACE_STATUS)
		printf("\t\t/FAULT = %d\n", is_fault);
	
	portc = bus_space_read_1(sc->sc_bst, sc->sc_bsh, CIO_PORTC);
	if (TRACE_STATUS)
		printf("\tPort C has 0x0%X\n", portc);

	is_paper_empty = CPI_PAPER_EMPTY & portc;
	if (TRACE_STATUS)
		printf("\t\tPAPER EMPTY = %d\n", is_paper_empty);

	return (is_busy || !is_select || !is_fault || is_paper_empty);
}

static void
cpi_intr(void *arg)
{
	struct cpi_softc *sc;
	
	sc = arg;

	/* Printer ready for output? */
	if (cpi_notready(sc))
		return;

	if (0 && TRACE_WRITE)
		printf("\tcpi_soft_intr() has %u bytes.\n", sc->sc_bufbytes);
	
	/* Anything to print? */
	if (sc->sc_bufbytes) {
		/* Data byte */
		bus_space_write_1(sc->sc_bst, sc->sc_bsh,
		    CIO_PORTA, *sc->sc_cp++);
		sc->sc_bufbytes--;
		sc->sc_bytestoport++;
	}
	if (0 == sc->sc_bufbytes)
		/* line buffer empty, wake up our top half */
		wakeup((void *)sc);
}

static void
cpi_tc_initclock(struct cpi_softc *sc)
{
	uint8_t reg;
	
	/*
	 * Set up c/t 1 and 2 as a single, free-running 32 bit counter
	 */

	/* Disable counters 1 and 2 */
	reg = z8536_reg_get(sc->sc_bst, sc->sc_bsh, Z8536_MCCR);
	z8536_reg_set(sc->sc_bst, sc->sc_bsh, Z8536_MCCR,
	    reg & ~(MCCR_CT1E | MCCR_CT2E));

	/* Make sure interrupt enable bits are cleared */
	z8536_reg_set(sc->sc_bst, sc->sc_bsh, Z8536_CTCSR1, CTCS_CLR_IE);
	z8536_reg_set(sc->sc_bst, sc->sc_bsh, Z8536_CTCSR2, CTCS_CLR_IE);
	
	/* Initialise counter start values, and set to continuous cycle */
	z8536_reg_set(sc->sc_bst, sc->sc_bsh, Z8536_CTMSR1,
	    CTMS_CSC | CTMS_DCS_PULSE);
	z8536_reg_set(sc->sc_bst, sc->sc_bsh, Z8536_CTTCR1_MSB, 0x00);
	z8536_reg_set(sc->sc_bst, sc->sc_bsh, Z8536_CTTCR1_LSB, 0x00);
	
	z8536_reg_set(sc->sc_bst, sc->sc_bsh, Z8536_CTMSR2,
	    CTMS_CSC | CTMS_DCS_PULSE);
	z8536_reg_set(sc->sc_bst, sc->sc_bsh, Z8536_CTTCR2_MSB, 0x00);
	z8536_reg_set(sc->sc_bst, sc->sc_bsh, Z8536_CTTCR2_LSB, 0x00);

	/* Link counters 1 and 2 */
	reg = z8536_reg_get(sc->sc_bst, sc->sc_bsh, Z8536_MCCR);
	z8536_reg_set(sc->sc_bst, sc->sc_bsh, Z8536_MCCR, reg | MCCR_CT1CT2);

	/* Enable and counter pair */
	reg = z8536_reg_get(sc->sc_bst, sc->sc_bsh, Z8536_MCCR);
	z8536_reg_set(sc->sc_bst, sc->sc_bsh, Z8536_MCCR,
	    reg | (MCCR_CT1E | MCCR_CT2E));

	/* Start c/t 1; c/t 2 gets started by c/t 1 pulse */
	reg = z8536_reg_get(sc->sc_bst, sc->sc_bsh, Z8536_CTCSR1);
	z8536_reg_set(sc->sc_bst, sc->sc_bsh, Z8536_CTCSR1,
	    CTCSR_MASK(reg | CTCS_TCB | CTCS_GCB));

	if (TRACE_TCNTR) {
		printf("Before tc_init():\n");
		reg = z8536_reg_get(sc->sc_bst, sc->sc_bsh, Z8536_CTCSR1);
		printf("Counter 1 CTCSR setup bits are 0x%03x\n", reg);
		printf("Counter 1 (LSW) is now 0x%05x\n",
		    z8536_read_counter1(sc->sc_bst, sc->sc_bsh));
		reg = z8536_reg_get(sc->sc_bst, sc->sc_bsh, Z8536_CTCSR2);
		printf("Counter 2 CTCSR setup bits are 0x%03x\n", reg);
		printf("Counter 2 (MSW) is now 0x%05x\n",
		    z8536_read_counter2(sc->sc_bst, sc->sc_bsh));

		delay(1000);
	}
	
	sc->sc_timecounter.tc_get_timecount = cpi_get_timecount;
	sc->sc_timecounter.tc_poll_pps      = 0;
	sc->sc_timecounter.tc_counter_mask  = ~0u;
	sc->sc_timecounter.tc_frequency     = CPI_CLK_FREQ;
	sc->sc_timecounter.tc_name          = "Nubus CPI";
	sc->sc_timecounter.tc_quality       = 1000;
	/*
	 * Squirrel away the device's sc so we can talk
	 * to the CIO later
	 */
	sc->sc_timecounter.tc_priv          = sc;
	sc->sc_timecounter.tc_next          = NULL;
        
        tc_init(&(sc->sc_timecounter));
	
	if (TRACE_TCNTR) {
		delay(1000);

		printf("After tc_init():\n");
		reg = z8536_reg_get(sc->sc_bst, sc->sc_bsh, Z8536_CTCSR1);
		printf("Counter 1 CTCSR setup bits are 0x%03x\n", reg);
		printf("Counter 1 (LSW) is now 0x%05x\n",
		    z8536_read_counter1(sc->sc_bst, sc->sc_bsh));
		reg = z8536_reg_get(sc->sc_bst, sc->sc_bsh, Z8536_CTCSR2);
		printf("Counter 2 CTCSR setup bits are 0x%03x\n", reg);
		printf("Counter 2 (MSW) is now 0x%05x\n",
		    z8536_read_counter2(sc->sc_bst, sc->sc_bsh));
	}
}

static u_int
cpi_get_timecount(struct timecounter *tc)
{
        int s;
	uint msw, msw2, lsw;
	uint8_t reg;
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	
	bst = ((struct cpi_softc *)tc->tc_priv)->sc_bst;
	bsh = ((struct cpi_softc *)tc->tc_priv)->sc_bsh;
	/*
	 * We run CIO counters 1 and 2 in an internally coupled mode,
	 * where the output of counter 1 (LSW) clocks counter 2 (MSW).
	 * The counters are buffered, and the buffers have to be
	 * locked before we can read out a consistent counter
	 * value. Reading the LSB releases the buffer lock.
	 *
	 * Unfortunately, there is no such mechanism between MSW and
	 * LSW of the coupled counter. To ensure a consistent
	 * read-out, we read the MSW, then the LSW, then re-read the
	 * MSW and compare with the old value. If we find that the MSW
	 * has just been incremented, we re-read the LSW. This avoids
	 * a race that could leave us with a new (just wrapped) LSW
	 * and an old MSW value.
	 *
	 * For simplicity, we roll the procedure into a loop - the
	 * rollover case is rare.
	 */
	do {
		
#define delay(a)
		
		/* Guard HW timer access */
		s = splhigh();

		/* Lock counter 2 latch in preparation for read-out */
		bus_space_write_1(bst, bsh, CIO_CTRL, Z8536_CTCSR2);
		delay(1);
		reg = bus_space_read_1(bst, bsh, CIO_CTRL);
		bus_space_write_1(bst, bsh, CIO_CTRL, Z8536_CTCSR2);
		bus_space_write_1(bst, bsh, CIO_CTRL, CTCSR_MASK(reg | CTCS_RCC));
		
		/* Read out counter 2 MSB,then LSB (releasing the latch) */
		bus_space_write_1(bst, bsh, CIO_CTRL, Z8536_CTCCR2_MSB);
		delay(1);
		msw = bus_space_read_1(bst, bsh, CIO_CTRL) << 8;
		bus_space_write_1(bst, bsh, CIO_CTRL, Z8536_CTCCR2_LSB);
		delay(1);
		msw |= bus_space_read_1(bst, bsh, CIO_CTRL);
		
		/* Lock counter 1 latch in preparation for read-out */
		bus_space_write_1(bst, bsh, CIO_CTRL, Z8536_CTCSR1);
		delay(1);
		reg = bus_space_read_1(bst, bsh, CIO_CTRL);
		bus_space_write_1(bst, bsh, CIO_CTRL, Z8536_CTCSR1);
		bus_space_write_1(bst, bsh, CIO_CTRL, CTCSR_MASK(reg |CTCS_RCC));
		
		/* Read out counter 1 MSB,then LSB (releasing the latch) */
		bus_space_write_1(bst, bsh, CIO_CTRL, Z8536_CTCCR1_MSB);
		delay(1);
		lsw = bus_space_read_1(bst, bsh, CIO_CTRL) << 8;
		bus_space_write_1(bst, bsh, CIO_CTRL, Z8536_CTCCR1_LSB);
		delay(1);
		lsw |= bus_space_read_1(bst, bsh, CIO_CTRL);
		
		/* Lock counter 2 latch in preparation for read-out */
		bus_space_write_1(bst, bsh, CIO_CTRL, Z8536_CTCSR2);
		delay(1);
		reg = bus_space_read_1(bst, bsh, CIO_CTRL);
		bus_space_write_1(bst, bsh, CIO_CTRL, Z8536_CTCSR2);
		bus_space_write_1(bst, bsh, CIO_CTRL, CTCSR_MASK(reg | CTCS_RCC));
		
		/* Read out counter 2 MSB,then LSB (releasing the latch) */
		bus_space_write_1(bst, bsh, CIO_CTRL, Z8536_CTCCR2_MSB);
		delay(1);
		msw2 = bus_space_read_1(bst, bsh, CIO_CTRL) << 8;
		bus_space_write_1(bst, bsh, CIO_CTRL, Z8536_CTCCR2_LSB);
		delay(1);
		msw2 |= bus_space_read_1(bst, bsh, CIO_CTRL);
		
		splx(s);

	} while (msw2 != msw);

	/* timecounter expects an upward counter */
	return ~0u - ((msw << 16) | lsw);
}

/*
 * Z8536 CIO convenience atomic register getter/setter
 */

static uint
z8536_read_counter1(bus_space_tag_t bst, bus_space_handle_t bsh)
{
	uint8_t reg;
	uint32_t lsw;
	int s;

	s = splhigh();
	
	/* Lock counter 1 latch in preparation for read-out */
	bus_space_write_1(bst, bsh, CIO_CTRL, Z8536_CTCSR1);
	delay(1);
	reg = bus_space_read_1(bst, bsh, CIO_CTRL);
	bus_space_write_1(bst, bsh, CIO_CTRL, Z8536_CTCSR1);
	bus_space_write_1(bst, bsh, CIO_CTRL, CTCSR_MASK(reg | CTCS_RCC));
		
	/* Read out counter 1 MSB,then LSB (releasing the latch) */
	bus_space_write_1(bst, bsh, CIO_CTRL, Z8536_CTCCR1_MSB);
	delay(1);
	lsw = bus_space_read_1(bst, bsh, CIO_CTRL) << 8;
	bus_space_write_1(bst, bsh, CIO_CTRL, Z8536_CTCCR1_LSB);
	delay(1);
	lsw |= bus_space_read_1(bst, bsh, CIO_CTRL);

	splx(s);

	return lsw;
}

static uint
z8536_read_counter2(bus_space_tag_t bst, bus_space_handle_t bsh)
{
	uint8_t reg;
	uint32_t msw;
	int s;

	s = splhigh();

	/* Lock counter 2 latch in preparation for read-out */
	bus_space_write_1(bst, bsh, CIO_CTRL, Z8536_CTCSR2);
	delay(1);
	reg = bus_space_read_1(bst, bsh, CIO_CTRL);
	bus_space_write_1(bst, bsh, CIO_CTRL, Z8536_CTCSR2);
	bus_space_write_1(bst, bsh, CIO_CTRL, CTCSR_MASK(reg | CTCS_RCC));

	/* Read out counter 2 MSB,then LSB (releasing the latch) */
	bus_space_write_1(bst, bsh, CIO_CTRL, Z8536_CTCCR2_MSB);
	delay(1);
	msw = bus_space_read_1(bst, bsh, CIO_CTRL) << 8;
	bus_space_write_1(bst, bsh, CIO_CTRL, Z8536_CTCCR2_LSB);
	delay(1);
	msw |= bus_space_read_1(bst, bsh, CIO_CTRL);

	splx(s);

	return msw;
}

static void
z8536_reg_set(bus_space_tag_t bspace, bus_space_handle_t bhandle,
    uint8_t reg, uint8_t val)
{
	int s;

	s = splhigh();
	bus_space_write_1(bspace, bhandle, CIO_CTRL, reg);
	delay(1);
	bus_space_write_1(bspace, bhandle, CIO_CTRL, val);
	splx(s);
}

static uint8_t
z8536_reg_get(bus_space_tag_t bspace, bus_space_handle_t bhandle, uint8_t reg)
{
	int s;
	uint8_t val;

	s = splhigh();
	bus_space_write_1(bspace, bhandle, CIO_CTRL, reg);
	delay(1);
	val = bus_space_read_1(bspace, bhandle, CIO_CTRL);
	splx(s);
	
	return val;
}
