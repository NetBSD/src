/*	$NetBSD: ncr.c,v 1.49 2003/07/15 02:54:32 lukem Exp $	*/

/*
 * Copyright (c) 1996, 1997 Matthias Pfaller.
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
 *	This product includes software developed by Matthias Pfaller.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
__KERNEL_RCSID(0, "$NetBSD: ncr.c,v 1.49 2003/07/15 02:54:32 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_message.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/ncr5380reg.h>
#include <dev/ic/ncr5380var.h>

#include <machine/autoconf.h>
#include <machine/cpufunc.h>

/*
 * Function declarations:
 */
static int	ncr_pdma_in __P((struct ncr5380_softc *, int, int, u_char *));
static int	ncr_pdma_out __P((struct ncr5380_softc *, int, int, u_char *));
static void	ncr_intr __P((void *));
static void	ncr_attach __P((struct device *, struct device *, void *));
static int	ncr_match __P((struct device *, struct cfdata *, void *));
static int	ncr_ready __P((struct ncr5380_softc *sc));
static void	ncr_wait_not_req __P((struct ncr5380_softc *sc));

/*
 * Some constants.
 */
#define PDMA_ADDRESS	((volatile u_char *) 0xffe00000)
#define	NCR5380		((volatile u_char *) 0xffd00000)

/*
 * Bit allocation in config's sc_flags field.
 *
 * bit      0: disable disconnect/reconnect
 * bit      1: disable use of interrupts
 * bit      2: reset scsi bus in ncr_attach
 * bits  8-15: disable parity (per target)
 * bits 16-23: disable disconnect/reconnect (per target)
 */
#define NCR_DISABLE_RESELECT	1
#define NCR_DISABLE_INTERRUPTS	2
#define NCR_RESET_BUS		4

/*
 * Make the default options patchable with gdb.
 */
int ncr_default_options = 0;

CFATTACH_DECL(ncr, sizeof(struct ncr5380_softc),
    ncr_match, ncr_attach, NULL, NULL);

static int
ncr_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;
	int unit = cf->cf_unit;
	
	if (unit != 0)	/* Only one unit */
		return(0);

	ca->ca_addr = (int)NCR5380;
	ca->ca_irq  = IR_SCSI1;
	return(1);
}

static void
ncr_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct confargs *ca = aux;
	struct ncr5380_softc *sc = (struct ncr5380_softc *) self;
	int flags;

	/*
	 * For now we only support the DP8490.
	 */
	scsi_select_ctlr(DP8490);

	/* Pull in config flags. */ 
	flags = ca->ca_flags | ncr_default_options;

	if (flags)
		printf(": flags %d\n", flags);
	else
		printf("\n");

	/*
	 * Initialize NCR5380 register addresses.
	 */
	sc->sci_r0 = NCR5380 + 0;
	sc->sci_r1 = NCR5380 + 1;
	sc->sci_r2 = NCR5380 + 2;
	sc->sci_r3 = NCR5380 + 3;
	sc->sci_r4 = NCR5380 + 4;
	sc->sci_r5 = NCR5380 + 5;
	sc->sci_r6 = NCR5380 + 6;
	sc->sci_r7 = NCR5380 + 7;

	sc->sc_rev = NCR_VARIANT_DP8490;

	/*
	 * We only have to set the sc_pio_in and sc_pio_out
	 * function pointers. The rest of the MD functions are
	 * not used and default to NULL.
	 */
	sc->sc_pio_in	= ncr_pdma_in;
	sc->sc_pio_out	= ncr_pdma_out;

	/*
	 * Copy options from cf_flags to sc_flags and sc_parity_disable.
	 */
	if (flags & NCR_DISABLE_RESELECT)
		sc->sc_no_disconnect = 0xff;
	if (flags & NCR_DISABLE_INTERRUPTS)
		sc->sc_flags |= NCR5380_FORCE_POLLING;
	sc->sc_parity_disable = (flags >>  8) & 0xff;
	sc->sc_no_disconnect |= (flags >> 16) & 0xff;

	intr_establish(IR_SCSI1, ncr_intr, (void *)sc, sc->sc_dev.dv_xname,
		IPL_BIO, IPL_BIO, RISING_EDGE);

	sc->sc_channel.chan_id = 7;
	sc->sc_adapter.adapt_minphys = minphys;

	/*
	 *  Initialize the SCSI controller itself.
	 */
	ncr5380_attach(sc);
}

static void
ncr_intr(arg)
	void *arg;
{
	struct ncr5380_softc *sc = arg;

	if (*sc->sci_csr & SCI_CSR_INT) {
		if (ncr5380_intr(sc) == 0) {
			printf("%s: ", sc->sc_dev.dv_xname);
			if ((*sc->sci_bus_csr & ~SCI_BUS_RST) == 0)
				printf("BUS RESET\n");
			else
				printf("spurious interrupt\n");
			SCI_CLR_INTR(sc);
		}
	}
}

/*
 * PDMA stuff
 */

#define byte_data ((volatile u_char *)pdma)
#define word_data ((volatile u_short *)pdma)
#define long_data ((volatile u_long *)pdma)

#define W1(n)	*byte_data = *(data + n)
#define W2(n)	*word_data = *((u_short *)data + n)
#define W4(n)	*long_data = *((u_long *)data + n)
#define R1(n)	*(data + n) = *byte_data
#define R4(n)	*((u_long *)data + n) = *long_data

#ifndef NCR_TSIZE_OUT
#define NCR_TSIZE_OUT	512
#endif

#ifndef NCR_TSIZE_IN
#define NCR_TSIZE_IN	512
#define NCR_UNROLL_TIMES   8
#define NCR_UNROLL_SIZE (NCR_UNROLL_TIMES * sizeof(u_long))
#endif

#define TIMEOUT	1000000

static __inline int
ncr_ready(sc)
	struct ncr5380_softc *sc;
{
	int i;

	for (i = TIMEOUT; i > 0; i--) {
		if ((*sc->sci_csr & (SCI_CSR_DREQ | SCI_CSR_PHASE_MATCH)) ==
		    (SCI_CSR_DREQ | SCI_CSR_PHASE_MATCH))
		    	return(1);

		if ((*sc->sci_csr & SCI_CSR_PHASE_MATCH) == 0 ||
		    SCI_BUSY(sc) == 0)
			return(0);
	}
	printf("%s: ready timeout\n", sc->sc_dev.dv_xname);
	return(0);
}

/* Return zero on success. */
static __inline void ncr_wait_not_req(sc)
	struct ncr5380_softc *sc;
{
	int timo;
	for (timo = TIMEOUT; timo; timo--) {
		if ((*sc->sci_bus_csr & SCI_BUS_REQ) == 0 ||
		    (*sc->sci_csr & SCI_CSR_PHASE_MATCH) == 0 ||
		    SCI_BUSY(sc) == 0) {
			return;
		}
	}
	printf("%s: pdma not_req timeout\n", sc->sc_dev.dv_xname);
}

static int
ncr_pdma_in(sc, phase, datalen, data)
	struct ncr5380_softc *sc;
	int phase, datalen;
	u_char *data;
{
	volatile u_char *pdma = PDMA_ADDRESS;
	int resid, s;

	s = splbio();
	*sc->sci_mode |= SCI_MODE_DMA;
	*sc->sci_irecv = 0;

	for (resid = datalen; resid >= NCR_TSIZE_IN; resid -= NCR_TSIZE_IN) {
		int i;
		if (ncr_ready(sc) == 0) {
			goto interrupt;
		}
		for (i = (NCR_TSIZE_IN/NCR_UNROLL_SIZE); i--;
		     data += NCR_UNROLL_SIZE) {
			di();
			R4(0); R4(1); R4(2); R4(3);
			R4(4); R4(5); R4(6); R4(7);
			ei();
		}
	}

	if (resid) {
		int t = resid/sizeof(u_long);
		if (ncr_ready(sc) == 0) {
			goto interrupt;
		}
		/* Use duffs device to copy by 4 byte words */
		{
			int rem, nchunk;
			rem = t % NCR_UNROLL_TIMES;
			nchunk = t / NCR_UNROLL_TIMES;
			data += (rem - 1) * sizeof(u_long);
			di();
			switch(rem) {
				while(nchunk--) {
					di();
					data += NCR_UNROLL_SIZE;
					R4(-7);
				case 7: R4(-6);
				case 6: R4(-5);
				case 5: R4(-4);
				case 4: R4(-3);
				case 3: R4(-2);
				case 2: R4(-1);
				case 1: R4(0);
				case 0:
					ei();
				}
			}
			data += sizeof(u_long);
		}

		t *= sizeof(int);
		resid -= t;

		di();
		for(; resid--; data++)
			R1(0);
		ei();
		resid = 0;
	}
	ncr_wait_not_req(sc);
interrupt:
	SCI_CLR_INTR(sc);
	*sc->sci_mode &= ~SCI_MODE_DMA;
	splx(s);
	return(datalen - resid);
}

static int
ncr_pdma_out(sc, phase, datalen, data)
	struct ncr5380_softc *sc;
	int phase, datalen;
	u_char *data; 
{
	volatile u_char *pdma = PDMA_ADDRESS;
	int i, s, resid;
	u_char icmd;

	s = splbio();
	icmd = *(sc->sci_icmd) & SCI_ICMD_RMASK;
	*sc->sci_icmd = icmd | SCI_ICMD_DATA;
	*sc->sci_mode |= SCI_MODE_DMA;
	*sc->sci_dma_send = 0;

	resid = datalen;
	if (ncr_ready(sc) == 0) {
		goto interrupt;
	}
	if (resid > NCR_TSIZE_OUT) {
		/* Because of the chips DMA prefetch, phase changes
		 * etc, won't be detected until we have written at
		 * least one byte more. We pre-write 4 bytes so
		 * subsequent transfers will be aligned to a 4 byte
		 * boundary. Assuming disconects will only occur on
		 * block boundaries, we then correct for the pre-write
		 * when and if we get a phase change. If the chip had
		 * DMA byte counting hardware, the assumption would not
		 * be necessary.
		 */
		W4(0);
		data += 4;
		resid -= 4;
		
		for (; resid >= NCR_TSIZE_OUT; resid -= NCR_TSIZE_OUT) {
			if (ncr_ready(sc) == 0) {
				resid += 4; /* Overshot */
				goto interrupt;
			}
			movsd(data, (u_char *)pdma, NCR_TSIZE_OUT / 4);
		}
		if (ncr_ready(sc) == 0) {
			resid += 4; /* Overshot */
			goto interrupt;
		}
	}

	if (resid) {
		int t;
		t = resid / sizeof(int);
		movsd(data, (u_char *)pdma, t);
		t *= sizeof(int);
		resid -= t;

		movsb(data, (u_char *)pdma, resid);
		resid = 0;
	}
	for (i = TIMEOUT; i > 0; i--) {
		if ((*sc->sci_csr & (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH))
		    != SCI_CSR_DREQ)
			break;
	}
	if (i != 0)
		*byte_data = 0;
	else
		printf("%s: timeout waiting for final SCI_DSR_DREQ.\n",
			sc->sc_dev.dv_xname);

	ncr_wait_not_req(sc);
interrupt:
	SCI_CLR_INTR(sc);
	*sc->sci_mode &= ~SCI_MODE_DMA;
	*sc->sci_icmd = icmd;
	splx(s);
	return(datalen - resid);
}

