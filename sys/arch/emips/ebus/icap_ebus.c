/*	$NetBSD: icap_ebus.c,v 1.6 2014/07/25 08:10:32 dholland Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was written by Alessandro Forin and Neil Pittman
 * at Microsoft Research and contributed to The NetBSD Foundation
 * by Microsoft Corporation.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: icap_ebus.c,v 1.6 2014/07/25 08:10:32 dholland Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <uvm/uvm_param.h>

#include <emips/ebus/ebusvar.h>
#include <emips/emips/machdep.h>
#include <machine/emipsreg.h>

#define DEBUG_INTR   0x01
#define DEBUG_XFERS  0x02
#define DEBUG_STATUS 0x04
#define DEBUG_FUNCS  0x08
#define DEBUG_PROBE  0x10
#define DEBUG_WRITES 0x20
#define DEBUG_READS  0x40
#define DEBUG_ERRORS 0x80
#ifdef DEBUG
int icap_debug = DEBUG_ERRORS;
#define ICAP_DEBUG(x) (icap_debug & (x))
#define DBGME(_lev_,_x_) if ((_lev_) & icap_debug) _x_
#else
#define ICAP_DEBUG(x) (0)
#define DBGME(_lev_,_x_)
#endif
#define DEBUG_PRINT(_args_,_lev_) DBGME(_lev_,printf _args_)

/*
 * Device softc
 */
struct icap_softc {
	device_t sc_dev;
	struct _Icap *sc_dp;
	struct bufq_state *sc_buflist;
	struct buf *sc_bp;
	char *sc_data;
	int sc_count;
};

/* Required funcs
 */
static int	icap_ebus_match (device_t, cfdata_t, void *);
static void	icap_ebus_attach (device_t, device_t, void *);

static dev_type_open(icapopen);
static dev_type_close(icapclose);
static dev_type_read(icapread);
static dev_type_write(icapwrite);
static dev_type_ioctl(icapioctl);
static dev_type_strategy(icapstrategy);

/* Other functions
 */
extern paddr_t kvtophys(vaddr_t);
static void icapstart(struct icap_softc *sc);
static int  icap_ebus_intr(void *cookie, void *f);
static void icap_reset(struct icap_softc *sc);

/* Config stuff
 */
extern struct cfdriver icap_cd;

CFATTACH_DECL_NEW(icap_ebus, sizeof (struct icap_softc),
    icap_ebus_match, icap_ebus_attach, NULL, NULL);

static int
icap_ebus_match(device_t parent, cfdata_t match, void *aux)
{
	struct ebus_attach_args *ia = aux;
    struct _Icap *f = (struct _Icap *)ia->ia_vaddr;

	DEBUG_PRINT(("icap_match %x\n", (f) ? f->Tag : 0), DEBUG_PROBE);
	if (strcmp("icap", ia->ia_name) != 0)
		return (0);
    if ((f == NULL) ||
        (! (f->Tag == PMTTAG_ICAP)))
		return (0);

	return (1);
}

static void
icap_ebus_attach(device_t parent, device_t self, void *aux)
{
	struct icap_softc *sc = device_private(self);
	struct ebus_attach_args *ia =aux;

	DEBUG_PRINT(("icap_attach %p\n", sc), DEBUG_PROBE);

	sc->sc_dev = self;
	sc->sc_dp = (struct _Icap*)ia->ia_vaddr;
	bufq_alloc(&sc->sc_buflist, "fcfs", 0);
	sc->sc_bp = NULL;
	sc->sc_data = NULL;
	sc->sc_count = 0;

#if DEBUG
    printf(" virt=%p", (void*)sc->sc_dp);
#endif
	printf(": %s\n", "Internal Configuration Access Port");

	ebus_intr_establish(parent, (void*)ia->ia_cookie, IPL_BIO,
	    icap_ebus_intr, sc);

	icap_reset(sc);
}

/* The character device handlers
 */
const struct cdevsw icap_cdevsw = {
	.d_open = icapopen,
	.d_close = icapclose,
	.d_read = icapread,
	.d_write = icapwrite,
	.d_ioctl = icapioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

/*
 * Handle an open request on the device.
 */
static int
icapopen(dev_t device, int flags, int fmt, struct lwp *process)
{
	struct icap_softc *sc;

	DEBUG_PRINT(("icapopen\n"), DEBUG_FUNCS);
	sc = device_lookup_private(&icap_cd, minor(device));
	if (sc == NULL)
		return (ENXIO);

	return 0;
}

/*
 * Handle the close request for the device.
 */
static int
icapclose(dev_t device, int flags, int fmt, struct lwp *process)
{
	DEBUG_PRINT(("icapclose\n"), DEBUG_FUNCS);
	return 0; /* this always succeeds */
}

/*
 * Handle the read request for the device.
 */
static int
icapread(dev_t dev, struct uio *uio, int flags)
{
	DEBUG_PRINT(("icapread\n"), DEBUG_READS);
	return (physio(icapstrategy, NULL, dev, B_READ, minphys, uio));
}

/*
 * Handle the write request for the device.
 */
static int
icapwrite(dev_t dev, struct uio *uio, int flags)
{
	DEBUG_PRINT(("icapwrite\n"), DEBUG_WRITES);
	return (physio(icapstrategy, NULL, dev, B_WRITE, minphys, uio));
}

/*
 * Handle the ioctl request for the device.
 */
static int
icapioctl(dev_t dev, u_long xfer, void *addr, int flag, struct lwp *l)
{

	return ENOTTY;
}

/*
 * Strategy function for the device.
 */
static void
icapstrategy(struct buf *bp)
{
	struct icap_softc *sc;
	int s;

	DEBUG_PRINT(("icapstrategy\n"), DEBUG_FUNCS);

	/* We did nothing lest we did */
	bp->b_resid = bp->b_bcount;

	/* Do we know you.  */
	sc = device_lookup_private(&icap_cd, minor(bp->b_dev));
	if (sc == NULL) {
		DEBUG_PRINT(("icapstrategy: nodev %" PRIx64 "\n",bp->b_dev),
		    DEBUG_ERRORS);
		bp->b_error = ENXIO;
		biodone(bp);
		return;
	    }

	/* Add to Q. If Q was empty get it started */
	s = splbio();
	bufq_put(sc->sc_buflist, bp);
	if (bufq_peek(sc->sc_buflist) == bp) {
		icapstart(sc);
	}
	splx(s);
}

/*
 * Get the next I/O request started
 */
static void
icapstart(struct icap_softc *sc)
{
	paddr_t phys, phys2;
	vaddr_t virt;
	size_t count;
	uint32_t fl;
	struct buf *bp = sc->sc_bp;

	DEBUG_PRINT(("icapstart %p %p\n",sc,bp), DEBUG_FUNCS);

    /* Were we idle?
     */
 recheck:
    if (bp == NULL) {

        /* Yes, get the next request if any
         */
        bp = bufq_get(sc->sc_buflist);
        DEBUG_PRINT(("icapnext: %p\n",bp), DEBUG_XFERS);
        if (bp == NULL)
            return;
    }

    /* Done with this request?
     */
    if ((bp->b_resid == 0) || bp->b_error) {

        /* Yes, complete and move to next, if any
         */
        sc->sc_bp = NULL;
        biodone(bp);
        DEBUG_PRINT(("icapdone %p\n",bp), DEBUG_XFERS);
        bp = NULL;
        goto recheck;
    }

    /* If new request init the xfer info
     */
    if (sc->sc_bp == NULL) {
        sc->sc_bp = bp;
        sc->sc_data = bp->b_data;
        sc->sc_count = bp->b_resid;
    }

    /* Loop filling as many buffers as will fit in the FIFO
     */
    fl = (bp->b_flags & B_READ) ? ICAPS_F_RECV : ICAPS_F_XMIT;
    for (;;) {

        /* Make sure there's still room in the FIFO, no errors.
         */
        if (sc->sc_dp->Control & (ICAPC_IF_FULL|ICAPC_ERROR))
            break;

        /* How much data do we xfer and where
         */
        virt = (vaddr_t)sc->sc_data;
        phys = kvtophys(virt);
        count = round_page(virt) - virt;
        if (count == 0) count = PAGE_SIZE;/* could(will) be aligned */

        /* How much of it is contiguous
         */
        while (count < sc->sc_count) {
            phys2 = kvtophys(virt + count);
            if (phys2 != (phys + count)) {

                /* No longer contig, ship it
                 */
                break;
            }
            count += PAGE_SIZE;
        }

        /* Trim if we went too far 
         */
        if (count > sc->sc_count)
            count = sc->sc_count;

        /* Ship it
         */
        DEBUG_PRINT(("icapship %" PRIxPADDR " %d\n",phys,count), DEBUG_XFERS);
        sc->sc_dp->SizeAndFlags = fl | count;
        sc->sc_dp->BufferAddressHi32 = 0; /* BUGBUG 64bit */
        sc->sc_dp->BufferAddressLo32 = phys; /* this pushes the fifo */

        /* Adjust pointers and continue 
         */
        sc->sc_data  += count;
        sc->sc_count -= count;

        if (sc->sc_count <= 0)
            break;
    }
}

/*
 * Interrupt handler
 */
static int
icap_ebus_intr(void *cookie, void *f)
{
	struct icap_softc *sc = cookie;
    struct buf *bp = sc->sc_bp;
	u_int32_t isr, saf = 0, hi, lo;

	isr = sc->sc_dp->Control;

	DEBUG_PRINT(("i %x\n",isr), DEBUG_INTR);

    /* Make sure there is an interrupt and that we should take it
     */
	if ((isr & (ICAPC_INTEN|ICAPC_DONE)) != (ICAPC_INTEN|ICAPC_DONE))
		return (0);

    /* Pull out all completed buffers
     */
    while ((isr & ICAPC_OF_EMPTY) == 0) {

        if (isr & ICAPC_ERROR) {
            printf("%s: internal error (%x)\n", device_xname(sc->sc_dev),isr);
            icap_reset(sc);
            if (bp) {
                bp->b_error = EIO;
                icapstart(sc);
            }
            return (1);
        }

        /* Beware, order matters */
        saf = sc->sc_dp->SizeAndFlags;
        hi  = sc->sc_dp->BufferAddressHi32; /* BUGBUG 64bit */
        lo  = sc->sc_dp->BufferAddressLo32; /* this pops the fifo */
	__USE(hi);
	__USE(lo);

        /* Say its done that much (and sanity)
         */
        if (bp) {
            size_t count = saf & ICAPS_S_MASK;
            /* more sanity */
            if (count > bp->b_resid)
                count = bp->b_resid;
            bp->b_resid -= count;
        }

        /* More? */
        isr = sc->sc_dp->Control;
    }

    /* Did we pop at least one */
    if (saf)
        icapstart(sc);

	return (1);
}

/*
 * HW (re)Initialization
 */
static void 
icap_reset(struct icap_softc *sc)
{
	DEBUG_PRINT(("icap_reset %x\n",sc->sc_dp->Control), DEBUG_STATUS);
    sc->sc_dp->Control = ICAPC_RESET;
    DELAY(2);
    sc->sc_dp->Control = ICAPC_INTEN;    
}
