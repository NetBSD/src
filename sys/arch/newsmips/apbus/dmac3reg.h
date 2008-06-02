/*	$NetBSD: dmac3reg.h,v 1.1.128.1 2008/06/02 13:22:29 mjf Exp $	*/

/*-
 * Copyright (c) 2000 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

struct dmac3reg {
	volatile uint32_t csr;
	volatile uint32_t intr;
	volatile uint32_t len;
	volatile uint32_t addr;
	volatile uint32_t conf;
};

#define DMAC3_CSR_DBURST	0x0020
#define DMAC3_CSR_MBURST	0x0010
#define DMAC3_CSR_APAD		0x0008
#define DMAC3_CSR_RESET		0x0004
#define DMAC3_CSR_RECV		0x0002
#define DMAC3_CSR_SEND		0x0000
#define DMAC3_CSR_ENABLE	0x0001

#define DMAC3_INTR_PERR		0x8000
#define DMAC3_INTR_DRQI		0x4000
#define DMAC3_INTR_DRQIE	0x2000
#define DMAC3_INTR_DREQ		0x1000
#define DMAC3_INTR_EOPI		0x0400
#define DMAC3_INTR_EOPIE	0x0200
#define DMAC3_INTR_EOP		0x0100
#define DMAC3_INTR_TCI		0x0040
#define DMAC3_INTR_TCIE		0x0020
#define DMAC3_INTR_INTEN	0x0002
#define DMAC3_INTR_INT		0x0001

#define DMAC3_CONF_IPER		0x8000
#define DMAC3_CONF_MPER		0x4000
#define DMAC3_CONF_PCEN		0x2000
#define DMAC3_CONF_DERR		0x1000
#define DMAC3_CONF_DCEN		0x0800
#define DMAC3_CONF_ODDP		0x0200
#define DMAC3_CONF_WIDTH	0x00ff
#define DMAC3_CONF_SLOWACCESS	0x0020
#define DMAC3_CONF_FASTACCESS	0x0001


#define DMAC3_PAGEMAP	0xb4c20000
#define DMAC3_MAPSIZE	0x20000

struct dma_pte {
	uint32_t pad1;
	uint32_t valid:1,
	      coherent:1,	/* ? */
	      pad2:10,		/* ? */
	      pfnum:20;
};
