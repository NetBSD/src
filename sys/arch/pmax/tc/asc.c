/* $NetBSD: asc.c,v 1.1.2.3 1998/12/06 21:09:53 drochner Exp $ */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: asc.c,v 1.1.2.3 1998/12/06 21:09:53 drochner Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/scsi_message.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <dev/ic/ncr53c9xreg.h>
#include <dev/ic/ncr53c9xvar.h>

#include <pmax/tc/ascvar.h> /* XXX */

struct scsipi_device asc_dev = {
	NULL,			/* Use default error handler */
	NULL,			/* have a queue, served by this */
	NULL,			/* have no async handler */
	NULL,			/* Use default 'done' routine */
};

/*
 * Glue functions
 */
u_char
asc_read_reg(sc, reg)
	struct ncr53c9x_softc *sc;
	int reg;
{
	struct asc_softc *asc = (struct asc_softc *)sc;
	u_char v;
#if 1
	v = asc->sc_reg[reg * 4] /*& 0xff*/;
#else
	v = bus_space_read_4(asc->sc_bst, asc->sc_bsh,
		reg * sizeof(u_int32_t)) & 0xff;
#endif
	return (v);
}

void
asc_write_reg(sc, reg, val)
	struct ncr53c9x_softc *sc;
	int reg;
	u_char val;
{
	struct asc_softc *asc = (struct asc_softc *)sc;
#if 1
	asc->sc_reg[reg * 4] = val;
	wbflush();
#else
	bus_space_write_4(asc->sc_bst, asc->sc_bsh,
	    reg * sizeof(u_int32_t), val);  
#endif
}

int
asc_dma_isintr(sc)
	struct ncr53c9x_softc *sc;
{
	return !!(NCR_READ_REG(sc, NCR_STAT) & NCRSTAT_INT);
}

int
asc_dma_isactive(sc)
	struct ncr53c9x_softc *sc;
{
	struct asc_softc *asc = (struct asc_softc *)sc;

	return (asc->sc_active);
}

void
asc_clear_latched_intr(sc)
	struct ncr53c9x_softc *sc;
{
}

#ifdef __pmax__

#include <machine/locore.h>
#include <machine/pcb.h>

/*
 * When kernel stack is mapped at UADDR, any buffers on the stack need to
 * be accessible at interrupt time.  This function intercepts the I/O
 * request and replaces the stack address with the corresponding KSEG2
 * address (or KSEG0 if the kernelstack isn't a kernel managedpage.
 */
int
asc_scsi_cmd(xs)
	struct scsipi_xfer *xs;
{
	if (CPUISMIPS3 && xs->datalen != 0)
		mips3_HitFlushDCache((vaddr_t)xs->data, xs->datalen);

#ifdef KERNELSTACK
	if (xs->datalen != 0 &&
	    xs->data >= (u_char *)UADDR && xs->data < (u_char *)KERNELSTACK) {
#ifdef DEBUG
		printf("asc_scsi_md: (%d,%d) buffer %p->",
		    xs->sc_link->scsipi_scsi.scsibus,
		    xs->sc_link->scsipi_scsi.target, xs->data);
#endif
		xs->data = (xs->data - (u_char *)UADDR) +
#if 1
		    (u_char*)curpcb;
#else
		    (curpcb->pcb_k2addr != NULL) ?
			(u_char *)curpcb->pcb_k2addr : (u_char *)curpcb;
#endif
#ifdef DEBUG
		printf(" %p pcb %p k2addr %lx\n", xs->data, curpcb,
		    curpcb->pcb_k2addr);
#endif
	}
#endif

	return (ncr53c9x_scsi_cmd(xs));
}
#else
int
asc_scsi_cmd(xs)
	struct scsipi_xfer *xs;
{
	return (ncr53c9x_scsi_cmd(xs));
}
#endif
