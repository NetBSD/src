/*
 * Copyright (c) 2025 Stephen M. Rumble <rumble@ephemeral.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _ARCH_SGIMIPS_HPC_HAUDREG_H_
#define _ARCH_SGIMIPS_HPC_HAUDREG_H_

#define HAUD_DMA_XFER_WORD_LEN	0x00000000      /* DMA transfer length (words)*/
#define HAUD_GIO_ADDR_16LSB	0x00000004      /* Lower 16 bits of GIO addr */
#define HAUD_GIO_ADDR_16MSB	0x00000008      /* Upper 16 bits of GIO addr */
#define HAUD_PBUS_ADDR		0x0000000c      /* 16-bit PBus address */
#define HAUD_DMA_CTL		0x00000010      /* DMA control register */
#define   HAUD_DMA_CTL_START	       0x1      /* Start DMA engine */
#define   HAUD_DMA_CTL_TO_CPU	       0x2      /* 0: DSP->CPU, 1: CPU->DSP */
#define   HAUD_DMA_CTL_MODE	       0x4      /* 0: normal, 1: folded */
#define HAUD_COUNTER		0x00000014      /* Read-only counter (24-bit) */
#define HAUD_TX_HANDSHAKE	0x00000018      /* 16-bit tx handshake */
#define HAUD_RX_HANDSHAKE	0x0000001c      /* 16-bit rx handshake */
#define HAUD_CPU_INTR_STAT	0x00000020      /* CPU interrupt status */
#define   HAUD_CPU_INTR_STAT_DMA       0x1      /* DSP completed DMA */
#define   HAUD_CPU_INTR_STAT_TX        0x2      /* DSP wrote to TX_HANDSHAKE */
#define   HAUD_CPU_INTR_STAT_RX        0x4      /* DSP read from RX_HANDSHAKE */
#define HAUD_CPU_INTR_MASK	0x00000024      /* CPU interrupt mask */
#define   HAUD_CPU_INTR_MASK_DMA_ENBL  0x1      /* Enable post-DMA interrupt */
#define   HAUD_CPU_INTR_MASK_TX_ENBL   0x2      /* Enable post-TX interrupt */
#define   HAUD_CPU_INTR_MASK_RX_ENBL   0x4      /* Enable post-RX interrupt */
#define HAUD_MISC_CSR		0x00000030      /* Misc control & status */
#define   HAUD_MISC_CSR_RESET	      0x01	/* Hard reset the DSP */
#define   HAUD_MISC_CSR_IRQA          0x02	/* Set /IRQA line per polarity*/
#define   HAUD_MISC_CSR_IRQA_POL      0x04	/* Polarity, 0: low, 1: high */
#define   HAUD_MISC_CSR_32K_SRAM      0x08	/* 0: 8K-word SRAM, 1: 32K */
#define   HAUD_MISC_CSR_PBUS_MASK     0x70	/* PBus sign extension bits */
#define HAUD_BURST_CTL		0x00000034      /* DMA burst control */

#endif /* _ARCH_SGIMIPS_HPC_HAUDREG_H_ */
