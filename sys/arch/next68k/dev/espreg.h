/*	$NetBSD: espreg.h,v 1.3.14.1 2001/10/01 12:41:15 fvdl Exp $ */

/*
 * Copyright (c) 1995 Rolf Grossmann. All rights reserved.
 * Copyright (c) 1994 Peter Galbavy.  All rights reserved.
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
 *	This product includes software developed by Peter Galbavy.
 *	This product includes software developed by Rolf Grossmann.
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
 * Register addresses, relative to some base address.
 */

#define	ESP_DCTL	0x20		/* RW - DMA Control		*/
#define	 ESPDCTL_CLKMSK	0xc0		/*	Clock Selection Bits	*/
#define	 ESPDCTL_10MHZ	0x00		/*	10 MHz Clock		*/
#define	 ESPDCTL_12MHZ	0x40		/*	12.5 MHz Clock		*/
#define	 ESPDCTL_16MHZ	0xc0		/*	16.6 MHz Clock		*/
#define	 ESPDCTL_20MHZ	0x80		/*	20 MHz Clock		*/
#define	 ESPDCTL_INTENB	0x20		/*	Interrupt Enable	*/
#define	 ESPDCTL_DMAMOD	0x10		/*	1 = Enable DMA 		*/
#define	 ESPDCTL_DMARD	0x08		/*	1 = scsi->mem (read)	*/
#define	 ESPDCTL_FLUSH	0x04		/*	Flush Fifo		*/
#define	 ESPDCTL_RESET	0x02		/*	Reset SCSI Chip		*/
#define	 ESPDCTL_WD3392	0x01		/*	0 = NCR 5390		*/

#define ESP_DCTL_BITS \
"\20\06INTENB\05DMAMOD\04DMARD\03FLUSH\02RESET\01WD3392"

#define	ESP_DSTAT	0x21		/* RW - DMA Status		*/
#define	 ESPDSTAT_STATE	0xc0		/*	DMA/SCSI Bank State	*/
#define	 ESPDSTAT_D0S0	0x00		/*	DMA rdy b. 0, SCSI b. 0	*/
#define	 ESPDSTAT_D0S1	0x40		/*	DMA req b. 0, SCSI b. 1	*/
#define	 ESPDSTAT_D1S1	0x80		/*	DMA rdy b. 0, SCSI b. 1 */


#define ESP_DEVICE_SIZE (ESP_DSTAT+1)
