/*	$NetBSD: ahavar.h,v 1.3 1997/03/28 23:47:10 mycroft Exp $	*/

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
 * these could be bigger but we need the aha_softc to fit on a single page..
 */
#define AHA_MBX_SIZE	16	/* mail box size  (MAX 255 MBxs) */
				/* don't need that many really */
#define AHA_CCB_MAX	16	/* store up to 16 CCBs at one time */
#define	CCB_HASH_SIZE	16	/* hash table size for phystokv */
#define	CCB_HASH_SHIFT	9
#define CCB_HASH(x)	((((long)(x))>>CCB_HASH_SHIFT) & (CCB_HASH_SIZE - 1))

#define aha_nextmbx(wmb, mbx, mbio) \
	if ((wmb) == &(mbx)->mbio[AHA_MBX_SIZE - 1])	\
		(wmb) = &(mbx)->mbio[0];		\
	else						\
		(wmb)++;

struct aha_mbx {
	struct aha_mbx_out mbo[AHA_MBX_SIZE];
	struct aha_mbx_in mbi[AHA_MBX_SIZE];
	struct aha_mbx_out *cmbo;	/* Collection Mail Box out */
	struct aha_mbx_out *tmbo;	/* Target Mail Box out */
	struct aha_mbx_in *tmbi;	/* Target Mail Box in */
};

struct aha_softc {
	struct device sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	void *sc_ih;

	struct aha_mbx sc_mbx;		/* all our mailboxes */
#define	wmbx	(&sc->sc_mbx)
	struct aha_ccb *sc_ccbhash[CCB_HASH_SIZE];
	TAILQ_HEAD(, aha_ccb) sc_free_ccb, sc_waiting_ccb;
	int sc_numccbs, sc_mbofull;
	struct scsi_link sc_link;	/* prototype for devs */

	char sc_model[18],
	     sc_firmware[4];
};

struct aha_probe_data {
	int sc_irq, sc_drq;
	int sc_scsi_dev;		/* adapters scsi id */
};

int	aha_find __P((bus_space_tag_t, bus_space_handle_t,
	    struct aha_probe_data *));
void	aha_attach __P((struct aha_softc *, struct aha_probe_data *));
int	aha_intr __P((void *));
