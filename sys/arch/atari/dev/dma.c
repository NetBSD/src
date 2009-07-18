/*	$NetBSD: dma.c,v 1.18.44.2 2009/07/18 14:52:52 yamt Exp $	*/

/*
 * Copyright (c) 1995 Leo Weppelman.
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
 *      This product includes software developed by Leo Weppelman.
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

/*
 * This file contains special code dealing with the DMA interface
 * on the Atari ST.
 *
 * The DMA circuitry requires some special treatment for the peripheral
 * devices which make use of the ST's DMA feature (the hard disk and the
 * floppy drive).
 * All devices using DMA need mutually exclusive access and can follow some
 * standard pattern which will be provided in this file.
 *
 * The file contains the following entry points:
 *
 *	st_dmagrab:	ensure exclusive access to the DMA circuitry
 *	st_dmafree:	free exclusive access to the DMA circuitry
 *	st_dmawanted:	somebody is queued waiting for DMA-access
 *	dmaint:		DMA interrupt routine, switches to the current driver
 *	st_dmaaddr_set:	specify 24 bit RAM address
 *	st_dmaaddr_get:	get address of last DMA-op
 *	st_dmacomm:	program DMA, flush FIFO first
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dma.c,v 1.18.44.2 2009/07/18 14:52:52 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/queue.h>

#include <machine/cpu.h>
#include <machine/iomap.h>
#include <machine/dma.h>
#include <machine/intr.h>

#define	NDMA_DEV	10	/* Max 2 floppy's, 8 hard-disks		*/
typedef struct dma_entry {
	TAILQ_ENTRY(dma_entry)	entries;	/* List pointers	   */
	void		(*call_func)(void *);	/* Call when lock granted  */
	void		(*int_func)(void *);	/* Call on DMA interrupt   */
	void		*softc;			/* Arg. to int_func	   */
	int		*lock_stat;		/* status of DMA lock	   */
} DMA_ENTRY;

/*
 * Preallocated entries. An allocator seem an overkill here.
 */
static	DMA_ENTRY dmatable[NDMA_DEV];	/* preallocated entries		*/

/*
 * Heads of free and active lists:
 */
static  TAILQ_HEAD(freehead, dma_entry)	dma_free;
static  TAILQ_HEAD(acthead, dma_entry)	dma_active;

static	int	must_init = 1;		/* Must initialize		*/

int	cdmaint(void *, int);

static	void	st_dma_init(void);

static void
st_dma_init(void)
{
	int	i;

	TAILQ_INIT(&dma_free);
	TAILQ_INIT(&dma_active);

	for(i = 0; i < NDMA_DEV; i++)
		TAILQ_INSERT_HEAD(&dma_free, &dmatable[i], entries);

	if (intr_establish(7, USER_VEC, 0, cdmaint, NULL) == NULL)
		panic("st_dma_init: Can't establish interrupt");
}

int
st_dmagrab(dma_farg int_func, dma_farg call_func, void *softc, int *lock_stat, int rcaller)
{
	int		sps;
	DMA_ENTRY	*req;

	if(must_init) {
		st_dma_init();
		must_init = 0;
	}
	*lock_stat = DMA_LOCK_REQ;

	sps = splhigh();

	/*
	 * Create a request...
	 */
	if(dma_free.tqh_first == NULL)
		panic("st_dmagrab: Too many outstanding requests");
	req = dma_free.tqh_first;
	TAILQ_REMOVE(&dma_free, dma_free.tqh_first, entries);
	req->call_func = call_func;
	req->int_func  = int_func;
	req->softc     = softc;
	req->lock_stat = lock_stat;
	TAILQ_INSERT_TAIL(&dma_active, req, entries);

	if(dma_active.tqh_first != req) {
		if (call_func == NULL) {
			do {
				tsleep(&dma_active, PRIBIO, "dmalck", 0);
			} while (*req->lock_stat != DMA_LOCK_GRANT);
			splx(sps);
			return(1);
		}
		splx(sps);
		return(0);
	}
	splx(sps);

	/*
	 * We're at the head of the queue, ergo: we got the lock.
	 */
	*lock_stat = DMA_LOCK_GRANT;

	if(rcaller || (call_func == NULL)) {
		/*
		 * Just return to caller immediately without going
		 * through 'call_func' first.
		 */
		return(1);
	}

	(*call_func)(softc);	/* Call followup function		*/
	return(0);
}

void
st_dmafree(void *softc, int *lock_stat)
{
	int		sps;
	DMA_ENTRY	*req;
	
	sps = splhigh();

	/*
	 * Some validity checks first.
	 */
	if((req = dma_active.tqh_first) == NULL)
		panic("st_dmafree: empty active queue");
	if(req->softc != softc)
		printf("Caller of st_dmafree is not lock-owner!\n");

	/*
	 * Clear lock status, move request from active to free queue.
	 */
	*lock_stat = 0;
	TAILQ_REMOVE(&dma_active, req, entries);
	TAILQ_INSERT_HEAD(&dma_free, req, entries);

	if((req = dma_active.tqh_first) != NULL) {
		*req->lock_stat = DMA_LOCK_GRANT;

		if (req->call_func == NULL)
			wakeup((void *)&dma_active);
		else {
		    /*
		     * Call next request through softint handler. This avoids
		     * spl-conflicts.
		     */
		    add_sicallback((si_farg)req->call_func, req->softc, 0);
		}
	}
	splx(sps);
	return;
}

int
st_dmawanted(void)
{
	return(dma_active.tqh_first->entries.tqe_next != NULL);
}

int
cdmaint(void *unused, int sr)
	/* sr:	 sr at time of interrupt */
{
	dma_farg	int_func;
	void		*softc;

	if(dma_active.tqh_first != NULL) {
		/*
		 * Due to the logic of the ST-DMA chip, it is not possible to
		 * check for stray interrupts here...
		 */
		int_func = dma_active.tqh_first->int_func;
		softc    = dma_active.tqh_first->softc;

		if(!BASEPRI(sr))
			add_sicallback((si_farg)int_func, softc, 0);
		else {
			spl1();
			(*int_func)(softc);
			spl0();
		}
		return 1;
	}
	return 0;
}

/*
 * Setup address for DMA-transfer.
 * Note: The order _is_ important!
 */
void
st_dmaaddr_set(void * address)
{
	register u_long ad = (u_long)address;

	DMA->dma_addr[AD_LOW ] = (ad     ) & 0xff;
	DMA->dma_addr[AD_MID ] = (ad >> 8) & 0xff;
	DMA->dma_addr[AD_HIGH] = (ad >>16) & 0xff;
}

/*
 * Get address from DMA unit.
 */
u_long
st_dmaaddr_get(void)
{
	register u_long ad = 0;

	ad  = (DMA->dma_addr[AD_LOW ] & 0xff);
	ad |= (DMA->dma_addr[AD_MID ] & 0xff) << 8;
	ad |= (DMA->dma_addr[AD_HIGH] & 0xff) <<16;
	return(ad);
}

/*
 * Program the DMA-controller to transfer 'nblk' blocks of 512 bytes.
 * The DMA_WRBIT trick flushes the FIFO before doing DMA.
 */
void
st_dmacomm(int mode, int nblk)
{
	DMA->dma_mode = mode;
	DMA->dma_mode = mode ^ DMA_WRBIT;
	DMA->dma_mode = mode;
	DMA->dma_data = nblk;
	delay(2);	/* Needed for Falcon */
	DMA->dma_mode = DMA_SCREG | (mode & DMA_WRBIT);
}
