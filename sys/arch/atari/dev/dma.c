/*	$NetBSD: dma.c,v 1.1.1.1 1995/03/26 07:12:13 leo Exp $	*/

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
 *	dmagrab:	ensure exclusive access to the DMA circuitry
 *	dmafree:	free exclusive access to the DMA circuitry
 *	dmaint:		DMA interrupt routine, switches to the current driver
 *	dmaaddr:	specify 24 bit RAM address
 *	dmardat:	set dma_mode and read word from dma_data
 *	dmawdat:	set dma_mode and write word to dma_data
 *	dmacomm:	like dmawdat, but first toggle WRBIT
 *
 * FIXME: 	The delay-loops should be done otherwise!
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <machine/cpu.h>
#include <machine/iomap.h>
#include <machine/dma.h>

#define	NDMA_DEV	10	/* Max 2 floppy's, 8 hard-disks		*/
typedef struct {
	int	(*call_func)();
	int	(*int_func)();
	void	*softc;
} DMA_DISP;

static	DMA_DISP  dispatch[NDMA_DEV];	/* dispatch table		     */
static	int	  dma_free = 0;		/* next free entry in dispatch table */
static	int	  dma_curr = 0;		/* current entry in dispatch table   */
static	int	  dmalock = 0;		/* if != 0, dma is not free	     */
static	int	  (*xxxint)();		/* current interrupt function	     */
static	void	  *dma_softc;		/* Device currently owning DMA-intr  */
static	int	  sched_soft = 0;	/* Software interrupt scheduled	     */

static	void	cdmasoft __P((void));

dmagrab(int_func, call_func, softc)
int 	(*int_func)();
int 	(*call_func)();
void	*softc;
{
	int		sps;
	DMA_DISP	*disp;

	sps = splbio();

	if(dmalock) {
		disp = &dispatch[dma_free++];
		if(dma_free >= NDMA_DEV)
			dma_free = 0;
		if(disp->call_func != NULL)
			panic("dma dispatch table overflow");
		disp->call_func = call_func;
		disp->int_func  = int_func;
		disp->softc     = softc;
		splx(sps);
		return;
	}
	dmalock++;
	xxxint    = int_func;	/* Grab DMA interrupts			*/
	dma_softc = softc;	/* Identify device which got DMA	*/
	(*call_func)(softc);	/* Call followup function		*/
	splx(sps);
	
}

dmafree()
{
	int		sps;
	DMA_DISP	*disp;
	
	sps = splbio();
	disp = &dispatch[dma_curr];
	if(disp->call_func != NULL) {
		xxxint    = disp->int_func;
		dma_softc = disp->softc;
		(*disp->call_func)(dma_softc);
		disp->call_func = NULL;
		if(++dma_curr >= NDMA_DEV)
			dma_curr = 0;
		splx(sps);
		return;
	}
	dmalock = 0;
	xxxint = NULL;		/* no more DMA interrupts */
	splx(sps);
}

cdmaint(sr)
long	sr;	/* sr at time of interrupt */
{
	if(xxxint != NULL) {
		if(!BASEPRI(sr)) {
			if(!sched_soft++)
				add_sicallback(cdmasoft, 0, 0);
		}
		else {
			spl1();
			cdmasoft();
		}
	}
	else printf("DMA interrupt discarded\r\n");
}

static void cdmasoft()
{
	sched_soft = 0;
	(*xxxint)(dma_softc);
}

dmaaddr(address)
caddr_t	address;
{
	register u_long ad = (u_long)address;

	DMA->dma_addr[AD_LOW ] = (ad     ) & 0xff;
	DMA->dma_addr[AD_MID ] = (ad >> 8) & 0xff;
	DMA->dma_addr[AD_HIGH] = (ad >>16) & 0xff;
}

int
dmardat(mode, delay)
int	mode, delay;
{
	while(--delay >= 0);
	DMA->dma_mode = mode;
	while(--delay >= 0);
	return(DMA->dma_data);
}

dmawdat(mode, data, delay)
int	mode, data, delay;
{
	DMA->dma_mode = mode;
	while(--delay >= 0);
	DMA->dma_data = data;
	while(--delay >= 0);
}

dmacomm(mode, data, delay)
int	mode, data, delay;
{
	DMA->dma_mode = mode;
	DMA->dma_mode = mode ^ WRBIT;
	DMA->dma_mode = mode;
	while(--delay >= 0);
	DMA->dma_data = data;
	while(--delay >= 0);
}

int
dmastat(mode, delay)
int	mode, delay;
{
	while(--delay >= 0);
	DMA->dma_mode = mode;
	while(--delay >= 0);
	return(DMA->dma_stat);
}
