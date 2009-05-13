/*	$NetBSD: iomd_dma.h,v 1.2.140.1 2009/05/13 17:16:17 jym Exp $	*/

/*
 * Copyright (c) 1997 Scott Stevens
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
 *      This product includes software developed by Scott Stevens.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *      
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)dmavar.h
 */     

/*
 * dma structures
 */

struct dma_ctrl {
	int			dc_flags;
	int			dc_channel;
	int			dc_dmasize;
	volatile u_int		*dc_cura;
	volatile u_int		*dc_enda;
	volatile u_int		*dc_curb;
	volatile u_int		*dc_endb;
	volatile u_char		*dc_cr;
	volatile u_char		*dc_st;

	u_char			*dc_nextaddr;
	int			dc_len;

	irqhandler_t		dc_ih;
};

#define DMA_FL_ACTIVE	0x01
#define DMA_FL_READY	0x02

#define DMA_CR_CLEAR	0x80
#define DMA_CR_DIR	0x40
#define DMA_CR_ENABLE	0x20
#define DMA_CR_INC	0x1f

#define DMA_ST_CHAN	0x01
#define DMA_ST_INT	0x02
#define DMA_ST_OVER	0x04
#define DMA_ST_MASK	(DMA_ST_CHAN | DMA_ST_INT | DMA_ST_OVER)

#define DMA_END_STOP	0x80000000
#define DMA_END_LAST	0x40000000

/* Prototypes */
struct dma_ctrl *dma_init(int, int, int, int);
void dma_go(struct dma_ctrl *);
int dma_reset(struct dma_ctrl *);
int dma_setup(struct dma_ctrl *, u_char *, int, int);
int dma_isactive(struct dma_ctrl *);
int dma_isintr(struct dma_ctrl *);
int dma_intr(void *);
