/*	$NetBSD: hpcreg.h,v 1.3 2001/08/19 03:16:22 wdk Exp $	*/

/*
 * Copyright (c) 2001 Rafal K. Boni
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARCH_SGIMIPS_HPC_HPCREG_H_
#define	_ARCH_SGIMIPS_HPC_HPCREG_H_

struct hpc_dma_desc {
	u_int32_t	hdd_bufptr;	/* Physical address of buffer */
	u_int32_t	hdd_ctl;	/* Control flags and byte count */
	u_int32_t	hdd_descptr;	/* Physical address of next descr. */
	u_int32_t	hdd_pad;	/* Pad out to quadword alignment */
};

/*
 * Control flags
 */
#define HDD_CTL_EOCHAIN		0x80000000	/* End of descriptor chain */
#define HDD_CTL_EOPACKET	0x40000000	/* Ethernet: end of packet */
#define HDD_CTL_INTR		0x20000000	/* Interrupt when finished */
#define HDD_CTL_XMITDONE	0x00008000	/* Ethernet transmit done */
#define HDD_CTL_OWN		0x00004000	/* CPU owns this frame */

#define HDD_CTL_BYTECNT(x)	((x) & 0x3fff)	/* Byte count: for ethernet
						 * rcv channel also doubles as
						 * length of packet received 
						 */

/* 
 * HPC memory map, as offsets from HPC base 
 *
 * XXXrkb: should each section be used as a base and have the specific
 * registers offset from there?? 
 *
 * XXX: define register values as well as their offsets.
 *
 */
#define HPC_PBUS_DMAREGS	0x00000000	/* DMA registers for PBus */
#define HPC_PBUS_DMAREGS_SIZE	0x0000ffff	/* channels 0 - 7 */

#define HPC_PBUS_CH0_BP		0x00000004	/* Chan 0 Buffer Ptr */
#define HPC_PBUS_CH0_DP		0x00000008	/* Chan 0 Descriptor Ptr */
#define HPC_PBUS_CH0_CTL	0x00001000	/* Chan 0 Control Register */

#define HPC_PBUS_CH1_BP		0x00002004	/* Chan 1 Buffer Ptr */
#define HPC_PBUS_CH1_DP		0x00002008	/* Chan 1 Descriptor Ptr */
#define HPC_PBUS_CH1_CTL	0x00003000	/* Chan 1 Control Register */

#define HPC_PBUS_CH2_BP		0x00004004	/* Chan 2 Buffer Ptr */
#define HPC_PBUS_CH2_DP		0x00004008	/* Chan 2 Descriptor Ptr */
#define HPC_PBUS_CH2_CTL	0x00005000	/* Chan 2 Control Register */

#define HPC_PBUS_CH3_BP		0x00006004	/* Chan 3 Buffer Ptr */
#define HPC_PBUS_CH3_DP		0x00006008	/* Chan 3 Descriptor Ptr */
#define HPC_PBUS_CH3_CTL	0x00007000	/* Chan 3 Control Register */

#define HPC_PBUS_CH4_BP		0x00008004	/* Chan 4 Buffer Ptr */
#define HPC_PBUS_CH4_DP		0x00008008	/* Chan 4 Descriptor Ptr */
#define HPC_PBUS_CH4_CTL	0x00009000	/* Chan 4 Control Register */

#define HPC_PBUS_CH5_BP		0x0000a004	/* Chan 5 Buffer Ptr */
#define HPC_PBUS_CH5_DP		0x0000a008	/* Chan 5 Descriptor Ptr */
#define HPC_PBUS_CH5_CTL	0x0000b000	/* Chan 5 Control Register */

#define HPC_PBUS_CH6_BP		0x0000c004	/* Chan 6 Buffer Ptr */
#define HPC_PBUS_CH6_DP		0x0000c008	/* Chan 6 Descriptor Ptr */
#define HPC_PBUS_CH6_CTL	0x0000d000	/* Chan 6 Control Register */

#define HPC_PBUS_CH7_BP		0x0000e004	/* Chan 7 Buffer Ptr */
#define HPC_PBUS_CH7_DP		0x0000e008	/* Chan 7 Descriptor Ptr */
#define HPC_PBUS_CH7_CTL	0x0000f000	/* Chan 7 Control Register */

#define HPC_SCSI0_REGS		0x00010000	/* SCSI channel 0 registers */
#define HPC_SCSI0_REGS_SIZE	0x00001fff

#define HPC_SCSI0_CBP		0x00010000	/* Current buffer ptr */
#define HPC_SCSI0_NDBP		0x00010004	/* Next descriptor ptr */

#define HPC_SCSI0_BC		0x00011000	/* DMA byte count & flags */
#define HPC_SCSI0_CTL		0x00011004	/* DMA control flags */
#define HPC_SCSI0_GIO		0x00011008	/* GIO DMA FIFO pointer */
#define HPC_SCSI0_DEV		0x0001100c	/* Device DMA FIFO pointer */
#define HPC_SCSI0_DMACFG	0x00011010	/* DMA configururation */
#define HPC_SCSI0_PIOCFG	0x00011014	/* PIO configururation */

#define HPC_SCSI1_REGS		0x00012000	/* SCSI channel 1 registers */
#define HPC_SCSI1_REGS_SIZE	0x00001fff

#define HPC_SCSI1_CBP		0x00000000	/* Current buffer ptr */
#define HPC_SCSI1_NDBP		0x00000004	/* Next descriptor ptr */

#define HPC_SCSI1_BC		0x00001000	/* DMA byte count & flags */
#define HPC_SCSI1_CTL		0x00001004	/* DMA control flags */
#define HPC_SCSI1_GIO		0x00001008	/* GIO DMA FIFO pointer */
#define HPC_SCSI1_DEV		0x0000100c	/* Device DMA FIFO pointer */
#define HPC_SCSI1_DMACFG	0x00001010	/* DMA configururation */
#define HPC_SCSI1_PIOCFG	0x00001014	/* PIO configururation */

#define HPC_DMACTL_IRQ    0x01 /* IRQ asserted, either dma done or parity */
#define HPC_DMACTL_ENDIAN 0x02 /* DMA endian mode, 0=BE, 1=LE */
#define HPC_DMACTL_DIR    0x04 /* DMA direction, 0=dev->mem, 1=mem->dev */
#define HPC_DMACTL_FLUSH  0x08 /* Flush DMA FIFO's */
#define HPC_DMACTL_ACTIVE 0x10 /* DMA channel is active */
#define HPC_DMACTL_AMASK  0x20 /* DMA active inhibits PIO */
#define HPC_DMACTL_RESET  0x40 /* Resets dma channel and external controller */
#define HPC_DMACTL_PERR   0x80 /* Parity error on interface to controller */


#define HPC_ENET_REGS		0x00014000	/* Ethernet registers */
#define HPC_ENET_REGS_SIZE	0x00003fff

#define HPC_ENETR_CBP		0x00000000	/* Recv: Current buffer ptr */
#define HPC_ENETR_NDBP		0x00000004	/* Recv: Next descriptor ptr */

#define HPC_ENETR_BC		0x00001000	/* Recv: DMA byte cnt/flags */
#define HPC_ENETR_CTL		0x00001004	/* Recv: DMA control flags */

#define ENETR_CTL_STAT_5_0	0x003f		/* Seeq irq status: bits 0-5 */
#define ENETR_CTL_STAT_6	0x0040		/* Irq status: late_rxdc */
#define ENETR_CTL_STAT_7	0x0080		/* Irq status: old/new bit */
#define ENETR_CTL_LENDIAN	0x0100		/* DMA channel endian mode */
#define ENETR_CTL_ACTIVE	0x0200		/* DMA channel active? */
#define ENETR_CTL_ACTIVE_MSK	0x0400		/* DMA channel active? */
#define ENETR_CTL_RBO		0x0800		/* Recv buffer overflow */

#define HPC_ENETR_GIO		0x00001008	/* Recv: GIO DMA FIFO ptr */
#define HPC_ENETR_DEV		0x0000100c	/* Recv: Device DMA FIFO ptr */
#define HPC_ENETR_RESET		0x00001014	/* Recv: Ethernet chip reset */

#define ENETR_RESET_CH		0x0001		/* Reset controller & chan */
#define ENETR_RESET_CLRINT	0x0002		/* Clear channel interrupt */
#define ENETR_RESET_LOOPBK	0x0004		/* External loopback enable */
#define ENETR_RESET_CLRRBO	0x0008		/* Clear RBO condition (??) */

#define HPC_ENETR_DMACFG	0x00001018	/* Recv: DMA configururation */

#define	ENETR_DMACFG_D1		0x0000f		/* DMA D1 state cycles */
#define	ENETR_DMACFG_D2		0x000f0		/* DMA D2 state cycles */
#define	ENETR_DMACFG_D3		0x00f00		/* DMA D3 state cycles */
#define	ENETR_DMACFG_WRCTL	0x01000		/* Enable IPG write */

/* 
 * The following three bits work around bugs in the Seeq 8003; if you 
 * don't set them, the Seeq gets wonky pretty often.
 */
#define	ENETR_DMACFG_FIX_RXDC	0x02000		/* Clear EOP bits on RXDC */
#define	ENETR_DMACFG_FIX_EOP	0x04000		/* Enable rxintr timeout */
#define	ENETR_DMACFG_FIX_INTR	0x08000		/* Enable EOP timeout */
#define	ENETR_DMACFG_TIMO	0x30000		/* Timeout for above two */

#define HPC_ENETR_PIOCFG	0x0000101c	/* Recv: PIO configururation */

#define HPC_ENETX_CBP		0x00002000	/* Xmit: Current buffer ptr */
#define HPC_ENETX_NDBP		0x00002004	/* Xmit: Next descriptor ptr */

#define HPC_ENETX_BC		0x00003000	/* Xmit: DMA byte cnt/flags */
#define HPC_ENETX_CTL		0x00003004	/* Xmit: DMA control flags */

#define ENETX_CTL_STAT_5_0	0x003f		/* Seeq irq status: bits 0-5 */
#define ENETX_CTL_STAT_6	0x0040		/* Irq status: late_rxdc */
#define ENETX_CTL_STAT_7	0x0080		/* Irq status: old/new bit */
#define ENETX_CTL_LENDIAN	0x0100		/* DMA channel endian mode */
#define ENETX_CTL_ACTIVE	0x0200		/* DMA channel active? */
#define ENETX_CTL_ACTIVE_MSK	0x0400		/* DMA channel active? */
#define ENETX_CTL_RBO		0x0800		/* Recv buffer overflow */

#define HPC_ENETX_GIO		0x00003008	/* Xmit: GIO DMA FIFO ptr */
#define HPC_ENETX_DEV		0x0000300c	/* Xmit: Device DMA FIFO ptr */

#define HPC_PBUS_FIFO		0x00020000	/* PBus DMA FIFO */
#define HPC_PBUS_FIFO_SIZE	0x00007fff	/* PBus DMA FIFO size */

#define HPC_SCSI0_FIFO		0x00028000	/* SCSI0 DMA FIFO */
#define HPC_SCSI0_FIFO_SIZE	0x00001fff	/* SCSI0 DMA FIFO size */

#define HPC_SCSI1_FIFO		0x0002a000	/* SCSI1 DMA FIFO */
#define HPC_SCSI1_FIFO_SIZE	0x00001fff	/* SCSI1 DMA FIFO size */

#define HPC_ENETR_FIFO		0x0002c000	/* Ether recv DMA FIFO */
#define HPC_ENETR_FIFO_SIZE	0x00001fff	/* Ether recv DMA FIFO size */

#define HPC_ENETX_FIFO		0x0002e000	/* Ether xmit DMA FIFO */
#define HPC_ENETX_FIFO_SIZE	0x00001fff	/* Ether xmit DMA FIFO size */

/* 
 * HPCBUG: The interrupt status is split amongst two registers, and they're
 * not even consecutive in the HPC address space.  This is documented as a
 * bug by SGI.
 */
#define HPC_INTRSTAT_40		0x00030000	/* Interrupt stat, bits 4:0 */
#define HPC_INTRSTAT_95		0x0003000c	/* Interrupt stat, bits 9:5 */

#define HPC_GIO_MISC		0x00030004	/* GIO64 misc register */

#define HPC_EEPROM_DATA		0x00030008	/* Serial EEPROM data reg. */

#define HPC_GIO_BUSERR		0x00030010	/* GIO64 bus error intr stat */

#define HPC_SCSI0_DEVREGS	0x00044000	/* SCSI channel 0 chip regs */
#define HPC_SCSI0_DEVREGS_SIZE	0x000003ff	/* Size of chip registers */

#define HPC_SCSI1_DEVREGS	0x0004c000	/* SCSI channel 1 chip regs */
#define HPC_SCSI1_DEVREGS_SIZE	0x000003ff	/* Size of chip registers */

#define HPC_ENET_DEVREGS	0x00054000	/* Ethernet chip registers */
#define HPC_ENET_DEVREGS_SIZE	0x000004ff	/* Size of chip registers */

#define HPC_PBUS_DEVREGS	0x00054000	/* PBus PIO chip registers */
#define HPC_PBUS_DEVREGS_SIZE	0x000003ff	/* PBus PIO chip registers */

#define HPC_PBUS_CH0_DEVREGS	0x00058000	/* PBus ch. 0 chip registers */
#define HPC_PBUS_CH0_DEVREGS_SIZE   0x03ff

#define HPC_PBUS_CH1_DEVREGS	0x00058400	/* PBus ch. 1 chip registers */
#define HPC_PBUS_CH1_DEVREGS_SIZE   0x03ff

#define HPC_PBUS_CH2_DEVREGS	0x00058800	/* PBus ch. 2 chip registers */
#define HPC_PBUS_CH2_DEVREGS_SIZE   0x03ff

#define HPC_PBUS_CH3_DEVREGS	0x00058c00	/* PBus ch. 3 chip registers */
#define HPC_PBUS_CH3_DEVREGS_SIZE   0x03ff

#define HPC_PBUS_CH4_DEVREGS	0x00059000	/* PBus ch. 4 chip registers */
#define HPC_PBUS_CH4_DEVREGS_SIZE   0x03ff

#define HPC_PBUS_CH5_DEVREGS	0x00059400	/* PBus ch. 5 chip registers */
#define HPC_PBUS_CH5_DEVREGS_SIZE   0x03ff

#define HPC_PBUS_CH6_DEVREGS	0x00059800	/* PBus ch. 6 chip registers */
#define HPC_PBUS_CH6_DEVREGS_SIZE   0x03ff

#define HPC_PBUS_CH7_DEVREGS	0x00059c00	/* PBus ch. 7 chip registers */
#define HPC_PBUS_CH7_DEVREGS_SIZE   0x03ff

#define HPC_PBUS_CH8_DEVREGS	0x0005a000	/* PBus ch. 8 chip registers */
#define HPC_PBUS_CH8_DEVREGS_SIZE   0x03ff

#define HPC_PBUS_CH9_DEVREGS	0x0005a400	/* PBus ch. 9 chip registers */
#define HPC_PBUS_CH9_DEVREGS_SIZE   0x03ff

#define HPC_PBUS_CH8_DEVREGS_2	0x0005a800	/* PBus ch. 8 chip registers */
#define HPC_PBUS_CH8_DEVREGS_2_SIZE 0x03ff

#define HPC_PBUS_CH9_DEVREGS_2	0x0005ac00	/* PBus ch. 9 chip registers */
#define HPC_PBUS_CH9_DEVREGS_2_SIZE 0x03ff

#define HPC_PBUS_CH8_DEVREGS_3	0x0005b000	/* PBus ch. 8 chip registers */
#define HPC_PBUS_CH8_DEVREGS_3_SIZE 0x03ff

#define HPC_PBUS_CH9_DEVREGS_3	0x0005b400	/* PBus ch. 9 chip registers */
#define HPC_PBUS_CH9_DEVREGS_3_SIZE 0x03ff

#define HPC_PBUS_CH8_DEVREGS_4	0x0005b800	/* PBus ch. 8 chip registers */
#define HPC_PBUS_CH8_DEVREGS_4_SIZE 0x03ff

#define HPC_PBUS_CH9_DEVREGS_4	0x0005bc00	/* PBus ch. 9 chip registers */
#define HPC_PBUS_CH9_DEVREGS_4_SIZE 0x03ff

#define HPC_PBUS_CFGDMA_REGS	0x0005c000	/* PBus DMA config registers */
#define HPC_PBUS_CFGDMA_REGS_SIZE   0x0fff	

#define HPC_PBUS_CH0_CFGDMA	0x0005c000	/* PBus Ch. 0 DMA config */
#define HPC_PBUS_CH0_CFGDMA_SIZE    0x01ff	

#define HPC_PBUS_CH1_CFGDMA	0x0005c200	/* PBus Ch. 1 DMA config */
#define HPC_PBUS_CH1_CFGDMA_SIZE    0x01ff	

#define HPC_PBUS_CH2_CFGDMA	0x0005c400	/* PBus Ch. 2 DMA config */
#define HPC_PBUS_CH2_CFGDMA_SIZE    0x01ff	

#define HPC_PBUS_CH3_CFGDMA	0x0005c600	/* PBus Ch. 3 DMA config */
#define HPC_PBUS_CH3_CFGDMA_SIZE    0x01ff	

#define HPC_PBUS_CH4_CFGDMA	0x0005c800	/* PBus Ch. 4 DMA config */
#define HPC_PBUS_CH4_CFGDMA_SIZE    0x01ff	

#define HPC_PBUS_CH5_CFGDMA	0x0005ca00	/* PBus Ch. 5 DMA config */
#define HPC_PBUS_CH5_CFGDMA_SIZE    0x01ff	

#define HPC_PBUS_CH6_CFGDMA	0x0005cc00	/* PBus Ch. 6 DMA config */
#define HPC_PBUS_CH6_CFGDMA_SIZE    0x01ff	

#define HPC_PBUS_CH7_CFGDMA	0x0005ce00	/* PBus Ch. 7 DMA config */
#define HPC_PBUS_CH7_CFGDMA_SIZE    0x01ff	

#define HPC_PBUS_CFGPIO_REGS	0x0005d000	/* PBus PIO config registers */
#define HPC_PBUS_CFGPIO_REGS_SIZE   0x0fff

#define HPC_PBUS_CH0_CFGPIO	0x0005d000	/* PBus Ch. 0 PIO config */
#define HPC_PBUS_CH1_CFGPIO	0x0005d100	/* PBus Ch. 1 PIO config */
#define HPC_PBUS_CH2_CFGPIO	0x0005d200	/* PBus Ch. 2 PIO config */
#define HPC_PBUS_CH3_CFGPIO	0x0005d300	/* PBus Ch. 3 PIO config */
#define HPC_PBUS_CH4_CFGPIO	0x0005d400	/* PBus Ch. 4 PIO config */
#define HPC_PBUS_CH5_CFGPIO	0x0005d500	/* PBus Ch. 5 PIO config */
#define HPC_PBUS_CH6_CFGPIO	0x0005d600	/* PBus Ch. 6 PIO config */
#define HPC_PBUS_CH7_CFGPIO	0x0005d700	/* PBus Ch. 7 PIO config */
#define HPC_PBUS_CH8_CFGPIO	0x0005d800	/* PBus Ch. 8 PIO config */
#define HPC_PBUS_CH9_CFGPIO	0x0005d900	/* PBus Ch. 9 PIO config */
#define HPC_PBUS_CH8_CFGPIO_2	0x0005da00	/* PBus Ch. 8 PIO config */
#define HPC_PBUS_CH9_CFGPIO_2	0x0005db00	/* PBus Ch. 9 PIO config */
#define HPC_PBUS_CH8_CFGPIO_3	0x0005dc00	/* PBus Ch. 8 PIO config */
#define HPC_PBUS_CH9_CFGPIO_3	0x0005dd00	/* PBus Ch. 9 PIO config */
#define HPC_PBUS_CH8_CFGPIO_4	0x0005de00	/* PBus Ch. 8 PIO config */
#define HPC_PBUS_CH9_CFGPIO_4	0x0005df00	/* PBus Ch. 9 PIO config */

#define HPC_PBUS_PROM_WE	0x0005e000	/* PBus boot-prom write
						 * enable register 
						 */

#define HPC_PBUS_PROM_SWAP	0x0005e800	/* PBus boot-prom chip-select 
						 * swap register
						 */

#define HPC_PBUS_GEN_OUT	0x0005f000	/* PBus general-purpose output
						 * register 
						 */

#define HPC_PBUS_BBRAM		0x00060000	/* PBus battery-backed RAM
						 * external registers 
						 */
#endif	/* _ARCH_SGIMIPS_HPC_HPCREG_H_ */
