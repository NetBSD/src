/*	$NetBSD: bhavar.h,v 1.6.2.1 1997/05/13 03:04:44 thorpej Exp $	*/

/*
 * Copyright (c) 1994, 1996, 1997 Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

/*
 * Mail box defs  etc.
 * these could be bigger but we need the bha_softc to fit on a single page..
 */
#define BHA_MBX_SIZE	32	/* mail box size  (MAX 255 MBxs) */
				/* don't need that many really */
#define BHA_CCB_MAX	32	/* store up to 32 CCBs at one time */
#define	CCB_HASH_SIZE	32	/* hash table size for phystokv */
#define	CCB_HASH_SHIFT	9
#define CCB_HASH(x)	((((long)(x))>>CCB_HASH_SHIFT) & (CCB_HASH_SIZE - 1))

#define bha_nextmbx(wmb, mbx, mbio) \
	if ((wmb) == &(mbx)->mbio[BHA_MBX_SIZE - 1])	\
		(wmb) = &(mbx)->mbio[0];		\
	else						\
		(wmb)++;

struct bha_mbx {
	struct bha_mbx_out mbo[BHA_MBX_SIZE];
	struct bha_mbx_in mbi[BHA_MBX_SIZE];
	struct bha_mbx_out *cmbo;	/* Collection Mail Box out */
	struct bha_mbx_out *tmbo;	/* Target Mail Box out */
	struct bha_mbx_in *tmbi;	/* Target Mail Box in */
};

struct bha_softc {
	struct device sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_dma_tag_t sc_dmat;
	bus_dmamap_t sc_dmamap_mbox;	/* maps the mailbox */
	int sc_dmaflags;		/* bus-specific dma map flags */
	void *sc_ih;

	struct bha_mbx *sc_mbx;		/* all our mailboxes */
#define	wmbx	(sc->sc_mbx)
	struct bha_ccb *sc_ccbhash[CCB_HASH_SIZE];
	TAILQ_HEAD(, bha_ccb) sc_free_ccb, sc_waiting_ccb;
	int sc_numccbs, sc_mbofull;
	struct scsi_link sc_link;	/* prototype for devs */

	char sc_model[7],
	     sc_firmware[6];
};

struct bha_probe_data {
	int sc_irq, sc_drq;
	int sc_scsi_dev;		/* adapters scsi id */
	int sc_iswide;			/* adapter is wide */
};

#define	ISWIDE(sc)	(sc->sc_link.max_target >= 8)

int	bha_find __P((bus_space_tag_t, bus_space_handle_t,
	    struct bha_probe_data *));
void	bha_attach __P((struct bha_softc *, struct bha_probe_data *));
int	bha_intr __P((void *));

int	bha_disable_isacompat __P((struct bha_softc *));
void	bha_inquire_setup_information __P((struct bha_softc *));
