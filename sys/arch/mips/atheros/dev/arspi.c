/* $NetBSD: arspi.c,v 1.2 2006/10/20 06:41:46 gdamore Exp $ */

/*-
 * Copyright (c) 2006 Urbana-Champaign Independent Media Center.
 * Copyright (c) 2006 Garrett D'Amore.
 * All rights reserved.
 *
 * Portions of this code were written by Garrett D'Amore for the
 * Champaign-Urbana Community Wireless Network Project.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgements:
 *      This product includes software developed by the Urbana-Champaign
 *      Independent Media Center.
 *	This product includes software developed by Garrett D'Amore.
 * 4. Urbana-Champaign Independent Media Center's name and Garrett
 *    D'Amore's name may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER AND GARRETT D'AMORE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER OR GARRETT D'AMORE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: arspi.c,v 1.2 2006/10/20 06:41:46 gdamore Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/queue.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <mips/atheros/include/ar5315reg.h>
#include <mips/atheros/include/ar531xvar.h>
#include <mips/atheros/include/arbusvar.h>

#include <mips/atheros/dev/arspireg.h>

#include <dev/spi/spiflash.h>
#include <dev/spi/spivar.h>

/*
 * This device is intended only to operate with specific SPI flash
 * parts, and is not a general purpose SPI host.  (Or at least if it
 * is, the Linux and eCos sources do not show how to use it as such.)
 * And lack of documentation on the Atheros SoCs is less than helpful.
 *
 * So for now we just "emulate" enough of the host bus framework to
 * make the SPI flash drivers happy.
 */

struct arspi_job {
	uint8_t			job_opcode;
	struct spi_chunk	*job_chunk;
	uint32_t		job_flags;
	uint32_t		job_addr;
	uint32_t		job_data;
	int			job_rxcnt;
	int			job_txcnt;
	int			job_addrcnt;
	int			job_rresid;
	int			job_wresid;
};

#define	JOB_READ		0x1
#define	JOB_WRITE		0x2
#define	JOB_LAST		0x4
#define	JOB_WAIT		0x8	/* job must wait for WIP bits */
#define	JOB_WREN		0x10	/* WREN needed */

struct arspi_softc {
	struct device		sc_dev;
	struct spi_controller	sc_spi;
	void			*sc_ih;
	boolean_t		sc_interrupts;

	struct spi_transfer	*sc_transfer;
	struct spi_chunk	*sc_wchunk;	/* for partial writes */
	struct spi_transq	sc_transq;
	bus_space_tag_t		sc_st;
	bus_space_handle_t	sc_sh;
	bus_size_t		sc_size;
};

#define	STATIC

STATIC int arspi_match(struct device *, struct cfdata *, void *);
STATIC void arspi_attach(struct device *, struct device *, void *);
STATIC void arspi_interrupts(struct device *);
STATIC int arspi_intr(void *);
/* SPI service routines */
STATIC int arspi_configure(void *, int, int, int);
STATIC int arspi_transfer(void *, struct spi_transfer *);
/* internal support */
STATIC void arspi_poll(struct arspi_softc *);
STATIC void arspi_done(struct arspi_softc *, int);
STATIC void arspi_sched(struct arspi_softc *);
STATIC int arspi_get_byte(struct spi_chunk **, uint8_t *);
STATIC int arspi_put_byte(struct spi_chunk **, uint8_t);
STATIC int arspi_make_job(struct spi_transfer *);
STATIC void arspi_update_job(struct spi_transfer *);
STATIC void arspi_finish_job(struct spi_transfer *);


CFATTACH_DECL(arspi, sizeof(struct arspi_softc),
    arspi_match, arspi_attach, NULL, NULL);

#define	GETREG(sc, o)		bus_space_read_4(sc->sc_st, sc->sc_sh, o)
#define	PUTREG(sc, o, v)	bus_space_write_4(sc->sc_st, sc->sc_sh, o, v)

int
arspi_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct arbus_attach_args *aa = aux;

	if (strcmp(aa->aa_name, cf->cf_name) != 0)
		return 0;
	return 1;
}

void
arspi_attach(struct device *parent, struct device *self, void *aux)
{
	struct arspi_softc *sc = device_private(self);
	struct spibus_attach_args sba;
	struct arbus_attach_args *aa = aux;

	/*
	 * Map registers.
	 */
	sc->sc_st = aa->aa_bst;
	sc->sc_size = aa->aa_size;
	if (bus_space_map(sc->sc_st, aa->aa_addr, sc->sc_size, 0,
		&sc->sc_sh) != 0) {
		printf(": unable to map registers!\n");
		return;
	}

	aprint_normal(": Atheros SPI controller\n");

	/*
	 * Initialize SPI controller.
	 */
	sc->sc_spi.sct_cookie = sc;
	sc->sc_spi.sct_configure = arspi_configure;
	sc->sc_spi.sct_transfer = arspi_transfer;
	sc->sc_spi.sct_nslaves = 1;


	/*
	 * Initialize the queue.
	 */
	spi_transq_init(&sc->sc_transq);

	/*
	 * Enable device interrupts.
	 */
	sc->sc_ih = arbus_intr_establish(aa->aa_cirq, aa->aa_mirq,
	    arspi_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error("%s: couldn't establish interrupt\n",
		    device_xname(self));
		/* just leave it in polled mode */
	} else
		config_interrupts(self, arspi_interrupts);

	/*
	 * Initialize and attach bus attach.
	 */
	sba.sba_controller = &sc->sc_spi;
	(void) config_found_ia(&sc->sc_dev, "spibus", &sba, spibus_print);
}

void
arspi_interrupts(struct device *self)
{
	/*
	 * we never leave polling mode, because, apparently, we 
	 * are missing some data about how to drive the SPI in interrupt
	 * mode.
	 */
#if 0
	struct arspi_softc *sc = device_private(self);
	int	s;

	s = splserial();
	sc->sc_interrupts = TRUE;
	splx(s);
#endif
}

int
arspi_intr(void *arg)
{
	struct arspi_softc *sc = arg;

	while (GETREG(sc, ARSPI_REG_CTL) & ARSPI_CTL_BUSY);

	arspi_done(sc, 0);

	return 1;
}

void
arspi_poll(struct arspi_softc *sc)
{

	while (sc->sc_transfer) {
		arspi_intr(sc);
	}
}

int
arspi_configure(void *cookie, int slave, int mode, int speed)
{

	/*
	 * We don't support the full SPI protocol, and hopefully the
	 * firmware has programmed a reasonable mode already.  So
	 * just a couple of quick sanity checks, then bail.
	 */
	if ((mode != 0) || (slave != 0))
		return EINVAL;

	return 0;
}

int
arspi_transfer(void *cookie, struct spi_transfer *st)
{
	struct arspi_softc *sc = cookie;
	int rv;
	int s;

	st->st_busprivate = NULL;
	if ((rv = arspi_make_job(st)) != 0) {
		if (st->st_busprivate) {
			free(st->st_busprivate, M_DEVBUF);
			st->st_busprivate = NULL;
		}
		spi_done(st, rv);
		return rv;
	}

	s = splserial();
	spi_transq_enqueue(&sc->sc_transq, st);
	if (sc->sc_transfer == NULL) {
		arspi_sched(sc);
		if (!sc->sc_interrupts)
			arspi_poll(sc);
	}
	splx(s);
	return 0;
}

void
arspi_sched(struct arspi_softc *sc) 
{
	struct spi_transfer *st;
	struct arspi_job *job;
	uint32_t ctl, cnt;

	for (;;) {
		if ((st = sc->sc_transfer) == NULL) {
			if ((st = spi_transq_first(&sc->sc_transq)) == NULL) {
				/* no work left to do */
				break;
			}
			spi_transq_dequeue(&sc->sc_transq);
			sc->sc_transfer = st;
		}

		arspi_update_job(st);
		job = st->st_busprivate;

		/* there shouldn't be anything running, but ensure it */
		do {
			ctl = GETREG(sc, ARSPI_REG_CTL);
		}  while (ctl & ARSPI_CTL_BUSY);
		/* clear all of the tx and rx bits */
		ctl &= ~(ARSPI_CTL_TXCNT_MASK | ARSPI_CTL_RXCNT_MASK);

		if (job->job_flags & JOB_WAIT) {
			PUTREG(sc, ARSPI_REG_OPCODE, SPIFLASH_CMD_RDSR);
			/* only the opcode for tx */
			ctl |= (1 << ARSPI_CTL_TXCNT_SHIFT);
			/* and one rx byte */
			ctl |= (1 << ARSPI_CTL_RXCNT_SHIFT);
		} else if (job->job_flags & JOB_WREN) {
			PUTREG(sc, ARSPI_REG_OPCODE, SPIFLASH_CMD_WREN);
			/* just the opcode */
			ctl |= (1 << ARSPI_CTL_TXCNT_SHIFT);
			/* no rx bytes */
		} else {
			/* set the data */
			PUTREG(sc, ARSPI_REG_DATA, job->job_data);

			/* set the opcode and the address */
			PUTREG(sc, ARSPI_REG_OPCODE, job->job_opcode |
			    (job->job_addr << 8));
		
			/* now set txcnt */
			cnt = 1;	/* opcode */
			cnt += job->job_addrcnt + job->job_txcnt;
			ctl |= (cnt << ARSPI_CTL_TXCNT_SHIFT);

			/* now set rxcnt */
			cnt = job->job_rxcnt;
			ctl |= (cnt << ARSPI_CTL_RXCNT_SHIFT);
		}

		/* set the start bit */
		ctl |= ARSPI_CTL_START;

		PUTREG(sc, ARSPI_REG_CTL, ctl);
		break;
	}
}

void
arspi_done(struct arspi_softc *sc, int err)
{
	struct spi_transfer *st;
	struct arspi_job *job;

	if ((st = sc->sc_transfer) != NULL) {
		job = st->st_busprivate;

		if (job->job_flags & JOB_WAIT) {
			if (err == 0) {
				if ((GETREG(sc, ARSPI_REG_DATA) &
				    SPIFLASH_SR_BUSY) == 0) {
					/* intermediate wait done */
					job->job_flags &= ~JOB_WAIT;
					goto done;
				}
			}
		} else if (job->job_flags & JOB_WREN) {
			if (err == 0) {
				job->job_flags &= ~JOB_WREN;
				goto done;
			}
		} else if (err == 0) {
			/*
			 * When breaking up write jobs, we have to wait until
			 * the WIP bit is clear, and we have to seperately
			 * send WREN for each chunk.  These flags facilitate
			 * that.
			 */
			if (job->job_flags & JOB_WRITE)
				job->job_flags |= (JOB_WAIT | JOB_WREN);
			job->job_data = GETREG(sc, ARSPI_REG_DATA);
			arspi_finish_job(st);
		}

		if (err || (job->job_flags & JOB_LAST)) {
			sc->sc_transfer = NULL;
			st->st_busprivate = NULL;
			spi_done(st, err);
			free(job, M_DEVBUF);
		}
	}
done:
	arspi_sched(sc);
}

int
arspi_get_byte(struct spi_chunk **chunkp, uint8_t *bytep)
{
	struct spi_chunk *chunk;

	chunk = *chunkp;

	/* skip leading empty (or already consumed) chunks */
	while (chunk && chunk->chunk_wresid == 0)
		chunk = chunk->chunk_next;

	if (chunk == NULL) {
		return ENODATA;
	}

	/*
	 * chunk must be write only.  SPI flash doesn't support
	 * any full duplex operations.
	 */
	if ((chunk->chunk_rptr) || !(chunk->chunk_wptr)) {
		return EINVAL;
	}

	*bytep = *chunk->chunk_wptr;
	chunk->chunk_wptr++;
	chunk->chunk_wresid--;
	chunk->chunk_rresid--;
	/* clearing wptr and rptr makes sanity checks later easier */
	if (chunk->chunk_wresid == 0)
		chunk->chunk_wptr = NULL;
	if (chunk->chunk_rresid == 0)
		chunk->chunk_rptr = NULL;
	while (chunk && chunk->chunk_wresid == 0)
		chunk = chunk->chunk_next;

	*chunkp = chunk;
	return 0;
}

int
arspi_put_byte(struct spi_chunk **chunkp, uint8_t byte)
{
	struct spi_chunk *chunk;

	chunk = *chunkp;

	/* skip leading empty (or already consumed) chunks */
	while (chunk && chunk->chunk_rresid == 0)
		chunk = chunk->chunk_next;

	if (chunk == NULL) {
		return EOVERFLOW;
	}

	/*
	 * chunk must be read only.  SPI flash doesn't support
	 * any full duplex operations.
	 */
	if ((chunk->chunk_wptr) || !(chunk->chunk_rptr)) {
		return EINVAL;
	}

	*chunk->chunk_rptr = byte;
	chunk->chunk_rptr++;
	chunk->chunk_wresid--;	/* technically this was done at send time */
	chunk->chunk_rresid--;
	while (chunk && chunk->chunk_rresid == 0)
		chunk = chunk->chunk_next;

	*chunkp = chunk;
	return 0;
}

int
arspi_make_job(struct spi_transfer *st)
{
	struct arspi_job *job;
	struct spi_chunk *chunk;
	uint8_t byte;
	int i, rv;

	job = malloc(sizeof (struct arspi_job), M_DEVBUF, M_ZERO);
	if (job == NULL) {
		return ENOMEM;
	}

	st->st_busprivate = job;

	/* skip any leading empty chunks (should not be any!) */
	chunk = st->st_chunks;

	/* get transfer opcode */
	if ((rv = arspi_get_byte(&chunk, &byte)) != 0)
		return rv;

	job->job_opcode = byte;
	switch (job->job_opcode) {
	case SPIFLASH_CMD_WREN:
	case SPIFLASH_CMD_WRDI:
	case SPIFLASH_CMD_CHIPERASE:
		break;
	case SPIFLASH_CMD_RDJI:
		job->job_rxcnt = 3;
		break;
	case SPIFLASH_CMD_RDSR:
		job->job_rxcnt = 1;
		break;
	case SPIFLASH_CMD_WRSR:
		/*
		 * is this in data, or in address?  stick it in data
		 * for now.
		 */
		job->job_txcnt = 1;
		break;
	case SPIFLASH_CMD_RDID:
		job->job_addrcnt = 3;	/* 3 dummy bytes */
		job->job_rxcnt = 1;
		break;
	case SPIFLASH_CMD_ERASE:
		job->job_addrcnt = 3;
		break;
	case SPIFLASH_CMD_READ:
		job->job_addrcnt = 3;
		job->job_flags |= JOB_READ;
		break;
	case SPIFLASH_CMD_PROGRAM:
		job->job_addrcnt = 3;
		job->job_flags |= JOB_WRITE;
		break;
	case SPIFLASH_CMD_READFAST:
		/*
		 * This is a pain in the arse to support, so we will
		 * rewrite as an ordinary read.  But later, after we
		 * obtain the address.
		 */
		job->job_addrcnt = 3;	/* 3 address */
		job->job_flags |= JOB_READ;
		break;
	default:
		return EINVAL;
	}

	for (i = 0; i < job->job_addrcnt; i++) {
		if ((rv = arspi_get_byte(&chunk, &byte)) != 0)
			return rv;
		job->job_addr <<= 8;
		job->job_addr |= byte;
	}


	if (job->job_opcode == SPIFLASH_CMD_READFAST) {
		/* eat the dummy timing byte */
		if ((rv = arspi_get_byte(&chunk, &byte)) != 0)
			return rv;
		/* rewrite this as a read */
		job->job_opcode = SPIFLASH_CMD_READ;
	}

	job->job_chunk = chunk;

	/*
	 * Now quickly check a few other things.   Namely, we are not
	 * allowed to have both READ and WRITE.
	 */
	for (chunk = job->job_chunk; chunk; chunk = chunk->chunk_next) {
		if (chunk->chunk_wptr) {
			job->job_wresid += chunk->chunk_wresid;
		}
		if (chunk->chunk_rptr) {
			job->job_rresid += chunk->chunk_rresid;
		}
	}

	if (job->job_rresid && job->job_wresid) {
		return EINVAL;
	}

	return 0;
}

/*
 * NB: The Atheros SPI controller runs in little endian mode. So all
 * data accesses must be swapped appropriately.
 *
 * The controller auto-swaps read accesses done through the mapped memory
 * region, but when using SPI directly, we have to do the right thing to
 * swap to or from little endian.
 */

void
arspi_update_job(struct spi_transfer *st)
{
	struct arspi_job *job = st->st_busprivate;
	uint8_t byte;
	int i;

	if (job->job_flags & (JOB_WAIT|JOB_WREN))
		return;

	job->job_rxcnt = 0;
	job->job_txcnt = 0;
	job->job_data = 0;

	job->job_txcnt = min(job->job_wresid, 4);
	job->job_rxcnt = min(job->job_rresid, 4);

	job->job_wresid -= job->job_txcnt;
	job->job_rresid -= job->job_rxcnt;

	for (i = 0; i < job->job_txcnt; i++) {
		arspi_get_byte(&job->job_chunk, &byte);
		job->job_data |= (byte << (i * 8));
	}

	if ((!job->job_wresid) && (!job->job_rresid)) {
		job->job_flags |= JOB_LAST;
	}
}

void
arspi_finish_job(struct spi_transfer *st)
{
	struct arspi_job *job = st->st_busprivate;
	uint8_t	byte;
	int i;

	job->job_addr += job->job_rxcnt;
	job->job_addr += job->job_txcnt;
	for (i = 0; i < job->job_rxcnt; i++) {
		byte = job->job_data & 0xff;
		job->job_data >>= 8;
		arspi_put_byte(&job->job_chunk, byte);
	}
}

