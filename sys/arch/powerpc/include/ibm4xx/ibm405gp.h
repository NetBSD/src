/*	$NetBSD: ibm405gp.h,v 1.3 2001/06/25 01:49:15 simonb Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge and Eduardo Horvath for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _IBM4XX_IBM405GP_H_
#define	_IBM4XX_IBM405GP_H_

/* 405GP PVR */
#define PVR_405GP      		0x40110000 
#define PVR_405GP_PASS1 	0x40110000	/* RevA */ 
#define PVR_405GP_PASS2 	0x40110040	/* RevB */ 
#define PVR_405GP_PASS2_1 	0x40110082	/* RevC */ 
#define PVR_405GP_PASS3 	0x401100c4	/* RevD */ 

/*
 * Memory and PCI addresses
 */

/* Local Memory and Peripherals */
#define	LOCAL_MEM_START		0x00000000
#define	LOCAL_MEM_END		0x7fffffff

/* PCI Memory - 1.625GB */
#define	PCI_MEM_START		0x80000000
#define	PCI_MEM_END		0xe7ffffff

/* PCI I/O - PCI I/O accesses from 0 to 64kB-1 (64kB) */
#define	PCI_IO_LOW_START	0xe8000000
#define	PCI_IO_LOW_END		0xe800ffff

/* PCI I/O - PCI I/O accesses from 8MB to 64MB-1 (56MB) */
#define	PCI_IO_HIGH_START	0xe8800000
#define	PCI_IO_HIGH_END		0xebffffff

/* PCI Configuaration Registers */
#define	PCIC0_BASE		0xeec00000
#define	PCIC0_CFGADDR		0x00
#define	PCIC0_CFGDATA		0x04

#define	PCIC0_VENDID		0x00
#define	PCIC0_DEVID		0x02
#define	PCIC0_CMD		0x04
#define	PCIC0_STATUS		0x06
#define	PCIC0_REVID		0x08
#define	PCIC0_CLS		0x09
#define	PCIC0_CACHELS		0x0c
#define	PCIC0_LATTIM		0x0d
#define	PCIC0_HDTYPE		0x0e
#define	PCIC0_BIST		0x0f
#define	PCIC0_BAR0		0x10	
#define	PCIC0_BAR1		0x14		/* PCI name */
#define	PCIC0_PTM1BAR		PCIC0_BAR1	/* 405GP name */
#define	PCIC0_BAR2		0x18		/* PCI name */
#define	PCIC0_PTM2BAR		PCIC0_BAR2	/* 405GP name */
#define	PCIC0_BAR3		0x1C	
#define	PCIC0_BAR4		0x20	
#define	PCIC0_BAR5		0x24	
#define	PCIC0_SBSYSVID		0x2c
#define	PCIC0_SBSYSID		0x2e
#define	PCIC0_CAP		0x34
#define	PCIC0_INTLN		0x3c
#define	PCIC0_INTPN		0x3d
#define	PCIC0_MINGNT		0x3e
#define	PCIC0_MAXLTNCY		0x3f

#define	PCIC0_ICS		0x44	/* 405GP specific parameters */
#define	PCIC0_ERREN		0x48
#define	PCIC0_ERRSTS		0x49
#define	PCIC0_BRDGOPT1		0x4a
#define	PCIC0_PLBBESR0		0x4c
#define	PCIC0_PLBBESR1		0x50
#define	PCIC0_PLBBEAR		0x54
#define	PCIC0_CAPID		0x58
#define	PCIC0_NEXTIPTR		0x59
#define	PCIC0_PMC		0x5a
#define	PCIC0_PMCSR		0x5c
#define	PCIC0_PMCSRBSE		0x5e
#define	PCIC0_DATA		0x5f
#define	PCIC0_BRDGOPT2		0x60
#define	PCIC0_PMSCRR		0x64


/* PCI Interrupt Acknowledge (read: 0xeed00000 0xeed00003 - 4 bytes) */
#define	PCIIA0			0xeed00000

/* PCI Special Cycle (write: 0xeed00000 0xeed00003 - 4 bytes) */
#define	PCISC0			0xeed00000

/* PCI Bridge Local Configuation Registers (0xef400000 0xef40003f - 64 bytes) */
#define	PCIL0_BASE		0xef400000
#define	PCIL0_PMM0LA		0x00	/* PCI Master Map 0: Local Address */
#define	PCIL0_PMM0MA		0x04	/*		     Mask/Attribute */
#define	PCIL0_PMM0PCILA		0x08	/*		     PCI Low Address */
#define	PCIL0_PMM0PCIHA		0x0c	/*		     PCI High Address */
#define	PCIL0_PMM1LA		0x10
#define	PCIL0_PMM1MA		0x14
#define	PCIL0_PMM1PCILA		0x18
#define	PCIL0_PMM1PCIHA		0x1c
#define	PCIL0_PMM2LA		0x20
#define	PCIL0_PMM2MA		0x24
#define	PCIL0_PMM2PCILA		0x28
#define	PCIL0_PMM2PCIHA		0x2c
#define	PCIL0_PTM1MS		0x30
#define	PCIL0_PTM1LA		0x34
#define	PCIL0_PTM2MS		0x38
#define	PCIL0_PTM2LA		0x3c

/*
 * Internal Peripherals
 */

/* UART0 Registers */
#define	UART0_BASE		0xef600300
#define	UART0_RBR		0x00	/* R    Receiver Buffer Register */
#define	UART0_THR		0x00	/*   W  Transmitter Holding Register */
#define	UART0_IER		0x01	/* R/W  Interrupt Enable Register */
#define	UART0_IIR		0x02	/* R    Interrupt Identification Register */
#define	UART0_FCR		0x02	/*   W  FIFO Control Register */
#define	UART0_LCR		0x03	/* R/W  Line Control Register */
#define	UART0_MCR		0x04	/* R/W  Modem Control Register */
#define	UART0_LSR		0x05	/* R/W  Line Status Register */
#define	UART0_MSR		0x06	/* R/W  Modem Status Register */
#define	UART0_SCR		0x07	/* R/W  Scratch Register */
#define	UART0_DLL		0x00	/* R/W* Divisor Latch (LSB) */
#define	UART0_DLM		0x01	/* R/W* Divisor Latch (MSB) */

/* UART1 Registers */
#define	UART1_BASE		0xef600400
#define	UART1_RBR		UART0_RBR /* R    Receiver Buffer Register */
#define	UART1_THR		UART0_THR /*   W  Transmitter Holding Register */
#define	UART1_IER		UART0_IER /* R/W  Interrupt Enable Register */
#define	UART1_IIR		UART0_IIR /* R    Interrupt Identification Register */
#define	UART1_FCR		UART0_FCR /*   W  FIFO Control Register */
#define	UART1_LCR		UART0_LCR /* R/W  Line Control Register */
#define	UART1_MCR		UART0_MCR /* R/W  Modem Control Register */
#define	UART1_LSR		UART0_LSR /* R/W  Line Status Register */
#define	UART1_MSR		UART0_MSR /* R/W  Modem Status Register */
#define	UART1_SCR		UART0_SCR /* R/W  Scratch Register */
#define	UART1_DLL		UART0_DLL /* R/W* Divisor Latch (LSB) */
#define	UART1_DLM		UART0_DLM /* R/W* Divisor Latch (MSB) */

/* IIC Registers */
#define	IIC0_BASE		0xef600500
#define	IIC0_MDBUF		0x00	/* Master Data Buffer */
#define	IIC0_SDBUF		0x02	/* Slave Data Buffer */
#define	IIC0_LMADR		0x04	/* Low Master Address */
#define	IIC0_HMADR		0x05	/* High Master Address */
#define	IIC0_CNTL		0x06	/* Control */
#define	IIC0_MDCNTL		0x07	/* Mode Control */
#define	IIC0_STS		0x08	/* Status */
#define	IIC0_EXTSTS		0x09	/* Extended Status */
#define	IIC0_LSADR		0x0a	/* Low Slave Address */
#define	IIC0_HSADR		0x0b	/* High Slave Address */
#define	IIC0_CLKDIV		0x0c	/* Clock Divide */
#define	IIC0_INTRMSK		0x0d	/* Interrupt Mask */
#define	IIC0_XFRCNT		0x0e	/* Transfer Count */
#define	IIC0_XTCNTLSS		0x0f	/* Extended Control and Slave Status */
#define	IIC0_DIRECTCNTL		0x10	/* Direct Control */

/* OPB Arbiter Registers */
#define	OPBA0_BASE		0xef600600
#define	OPBA0_PR		0x00	/* Priority Register */
#define	OPBA0_CR		0x01	/* Control Register */

/* GPIO Registers */
#define	GPIO0_BASE		0xef600700
#define	GPIO0_OR		0x00	/* Output */
#define	GPIO0_TCR		0x04	/* Three-State Control */
#define	GPIO0_ODR		0x18	/* Open Drain */
#define	GPIO0_IR		0x1c	/* Input */

/* Ethernet MAC Registers */
#define	EMAC0_BASE		0xef600800

#define	EMAC0_MR0		0x00	/* Mode Register 0 */
#define	  MR0_RXI		  0x80000000	/* Receive MAC Idle */
#define	  MR0_TXI		  0x40000000	/* Transmit MAC Idle */
#define	  MR0_SRST		  0x20000000	/* Soft Reset */
#define	  MR0_TXE		  0x10000000	/* Transmit MAC Enable */
#define	  MR0_RXE		  0x08000000	/* Receive MAC Enable */
#define	  MR0_WKE		  0x04000000	/* Wake-up Enable */

#define	EMAC0_MR1		0x04	/* Mode Register 1 */
#define	  MR1_FDE		  0x80000000	/* Full-Duplex Enable */
#define	  MR1_ILE		  0x40000000	/* Internal Loop-back Enable */
#define	  MR1_VLE		  0x20000000	/* VLAN Enable */
#define	  MR1_EIFC		  0x10000000	/* Enable Integrated Flow Control */
#define	  MR1_APP		  0x08000000	/* Allow Pause Packet */
#define	  MR1_IST		  0x01000000	/* Ignore SQE Test */
#define	  MR1_MF_MASK		  0x00c00000	/* Medium Frequency mask */
#define	  MR1_MF_10MBS		  0x00000000	/* 10MB/sec */
#define	  MR1_MF_100MBS		  0x00400000	/* 100MB/sec */
#define	  MR1_RFS_MASK		  0x00300000	/* Receive FIFO size */
#define	  MR1_RFS_512		  0x00000000	/* 512 bytes */
#define	  MR1_RFS_1KB		  0x00100000	/* 1kByte */
#define	  MR1_RFS_2KB		  0x00200000	/* 2kByte */
#define	  MR1_RFS_4KB		  0x00300000	/* 4kByte */
#define	  MR1_TFS_MASK		  0x000c0000	/* Transmit FIFO size */
#define	  MR1_TFS_1KB		  0x00080000	/* 1kByte */
#define	  MR1_TFS_2KB		  0x00040000	/* 2kByte */
#define	  MR1_TR0_MASK		  0x00018000	/* Transmit Request 0 */
#define	  MR1_TR0_SINGLE	  0x00000000	/* Single Packet mode */
#define	  MR1_TR0_MULTIPLE	  0x00008000	/* Multiple Packet mode */
#define	  MR1_TR0_DEPENDANT	  0x00010000	/* Dependent Mode */
#define	  MR1_TR1_MASK		  0x00006000	/* Transmit Request 1 */
#define	  MR1_TR1_SINGLE	  0x00000000	/* Single Packet mode */
#define	  MR1_TR1_MULTIPLE	  0x00002000	/* Multiply Packet mode */
#define	  MR1_TR1_DEPENDANT	  0x00004000	/* Dependent Mode */

#define	EMAC0_TMR0		0x08	/* Transmit Mode Register 0 */
#define	  TMR0_GNP0		  0x80000000	/* Get New Packet for Channel 0 */
#define	  TMR0_GNP1		  0x40000000	/* Get New Packet for Channel 1 */
#define	  TMR0_GNPD		  0x20000000	/* Get New Packet for Dependent mode */
#define	  TMR0_FC_MASK		  0x10000000	/* First Channel */
#define	  TMR0_FC_CHAN0		  0x00000000	/* Channel 0 */
#define	  TMR0_FC_CHAN1		  0x10000000	/* Channel 1 */

#define	EMAC0_TMR1		0x0c	/* Transmit Mode Register 1 */
#define	  TMR1_TLR_MASK		  0xf8000000	/* Transmit Low Request */
#define	  TMR1_TLR_SHIFT	  27
#define	  TMR1_TUR_MASK		  0x00ff0000	/* Transmit Urgent Request */
#define	  TMR1_TUR_SHIFT	  16

#define	EMAC0_RMR		0x10	/* Receive Mode Register */
#define	  RMR_SP		  0x80000000	/* Strip Padding */
#define	  RMR_SFCS		  0x40000000	/* Strip FCS */
#define	  RMR_RRP		  0x20000000	/* Receive Runt Packets */
#define	  RMR_RFP		  0x10000000	/* Receive FCS Packets */
#define	  RMR_ROP		  0x08000000	/* Receive Oversize Packets */
#define	  RMR_RPIR		  0x04000000	/* Receive Packets with In Range Error */
#define	  RMR_PPP		  0x02000000	/* Propagate Pause Packet */
#define	  RMR_PME		  0x01000000	/* Promiscuous Mode Enable */
#define	  RMR_PMME		  0x00800000	/* Promiscuous Multicast Mode Enable */
#define	  RMR_IAE		  0x00400000	/* Individual Address Enable */
#define	  RMR_MIAE		  0x00200000	/* Multiple Individual Address Enable */
#define	  RMR_BAE		  0x00100000	/* Broadcast Address Enable */
#define	  RMR_MAE		  0x00080000	/* Multicast Address Enable */

#define	EMAC0_ISR		0x14	/* Interrupt Status Register */
#define	  ISR_OVR		  0x02000000	/* Overrun Error */
#define	  ISR_PP		  0x01000000	/* Pause Packet */
#define	  ISR_BP		  0x00800000	/* Bad Packet */
#define	  ISR_RP		  0x00400000	/* Runt Packet */
#define	  ISR_SE		  0x00200000	/* Short Event */
#define	  ISR_ALE		  0x00100000	/* Alignment Error */
#define	  ISR_BFCS		  0x00080000	/* Bad FCS */
#define	  ISR_PTLE		  0x00040000	/* Packet Too Long Error */
#define	  ISR_ORE		  0x00020000	/* Out of Range Error */
#define	  ISR_IRE		  0x00010000	/* In Range Error */
#define	  ISR_DBDM		  0x00000200	/* Dead Bit Dependent Mode */
#define	  ISR_DB0		  0x00000100	/* Dead Bit 0 */
#define	  ISR_SE0		  0x00000080	/* SQE Error 0 */
#define	  ISR_TE0		  0x00000040	/* Transmit Error 0 */
#define	  ISR_DB1		  0x00000020	/* Dead Bit 1 */
#define	  ISR_SE1		  0x00000010	/* SQE Error 1 */
#define	  ISR_TE1		  0x00000008	/* Transmit Error 1 */
#define	  ISR_MOS		  0x00000002	/* MMA Operation Succeeded */
#define	  ISR_MOF		  0x00000001	/* MMA Operation Failed */

#define	EMAC0_ISER		0x18	/* Interrupt Status Enable Register */
#define	  ISER_OVR		  ISR_OVR
#define	  ISER_PP		  ISR_PP
#define	  ISER_BP		  ISR_BP
#define	  ISER_RP		  ISR_RP
#define	  ISER_SE		  ISR_SE
#define	  ISER_ALE		  ISR_ALE
#define	  ISER_BFCS		  ISR_BFCS
#define	  ISER_PTLE		  ISR_PTLE
#define	  ISER_ORE		  ISR_ORE
#define	  ISER_IRE		  ISR_IRE
#define	  ISER_DBDM		  ISR_DBDM
#define	  ISER_DB0		  ISR_DB0
#define	  ISER_SE0		  ISR_SE0
#define	  ISER_TE0		  ISR_TE0
#define	  ISER_DB1		  ISR_DB1
#define	  ISER_SE1		  ISR_SE1
#define	  ISER_TE1		  ISR_TE1
#define	  ISER_MOS		  ISR_MOS
#define	  ISER_MOF		  ISR_MOF

#define	EMAC0_IAHR		0x1c	/* Individual Address High Register */
#define	EMAC0_IALR		0x20	/* Individual Address Low Register */
#define	EMAC0_VTPID		0x24	/* VLAN TPID Register */
#define	EMAC0_VTCI		0x28	/* VLAN TCI Register */
#define	EMAC0_PTR		0x2c	/* Pause Timer Register */
#define	EMAC0_IAHT1		0x30	/* Individual Address Hash Table 1 */
#define	EMAC0_IAHT2		0x34	/* Individual Address Hash Table 2 */
#define	EMAC0_IAHT3		0x38	/* Individual Address Hash Table 3 */
#define	EMAC0_IAHT4		0x3c	/* Individual Address Hash Table 4 */
#define	EMAC0_GAHT1		0x40	/* Group Address Hash Table 1 */
#define	EMAC0_GAHT2		0x44	/* Group Address Hash Table 2 */
#define	EMAC0_GAHT3		0x48	/* Group Address Hash Table 3 */
#define	EMAC0_GAHT4		0x4c	/* Group Address Hash Table 4 */
#define	EMAC0_LSAH		0x50	/* Last Source Address High */
#define	EMAC0_LSAL		0x54	/* Last Source Address Low */
#define	EMAC0_IPGVR		0x58	/* Inter-Packet Gap Value Register */

#define	EMAC0_STACR		0x5c	/* STA Control Register */
#define	  STACR_PHYD		  0xffff0000	/* PHY data mask */
#define	  STACR_PHYDSHIFT	  16
#define	  STACR_OC		  0x00008000	/* operation complete */
#define	  STACR_PHYE		  0x00004000	/* PHY error */
#define	  STACR_WRITE		  0x00002000	/* STA command - write */
#define	  STACR_READ		  0x00001000	/* STA command - read */
#define	  STACR_OPBC_MASK	  0x00000c00	/* OPB bus clock freq mask */
#define	  STACR_OPBC_50MHZ	  0x00000000	/* OPB bus clock freq -  50MHz */
#define	  STACR_OPBC_66MHZ	  0x00000400	/* OPB bus clock freq -  66MHz */
#define	  STACR_OPBC_83MHZ	  0x00000800	/* OPB bus clock freq -  83MHz */
#define	  STACR_OPBC_100MHZ	  0x00000c00	/* OPB bus clock freq - 100MHz */
#define	  STACR_PCDA		  0x000003e0	/* PHY cmd dest address mask */
#define	  STACR_PDCASHIFT	  5
#define	  STACR_PRA		  0x0000001f	/* PHY register address mask */
#define	  STACR_PRASHIFT	  0

#define	EMAC0_TRTR		0x60	/* Transmit Request Threshold Register */
#define	  TRTR_64		  0x00000000	/* 64 bytes */
#define	  TRTR_128		  0x08000000	/* 128 bytes */
#define	  TRTR_192		  0x10000000	/* 192 bytes */
#define	  TRTR_256		  0x18000000	/* 256 bytes */
/* ... and so on +64 until ... */
#define	  TRTR_2048		  0xf8000000	/* 2048 bytes */

#define	EMAC0_RWMR		0x64	/* Receive Low/High Water Mark Register */
#define	  RWMR_RLWM_MASK	  0xff800000	/* Receive Low Water Mark */
#define	  RWMR_RLWM_SHIFT	    23
#define	  RWMR_RHWM_MASK	  0x0000ff80	/* Receive High Water Mark */
#define	  RWMR_RHWM_SHIFT	    7

#define	EMAC0_OCTX		0x68	/* Number of Octets Transmitted */
#define	EMAC0_OCRX		0x6c	/* Number of Octets Received */


/* Expansion ROM - 254MB */
#define	EXPANSION_ROM_START	0xf0000000
#define	EXPANSION_ROM_END	0xffdfffff

/* Boot ROM - 2MB */
#define	BOOT_ROM_START		0xffe00000
#define	BOOT_ROM_END		0xffffffff

#ifndef _LOCORE
void galaxy_show_pci_map(void);
void galaxy_setup_pci(void);
#endif /* _LOCORE */
#endif	/* _IBM4XX_IBM405GP_H_ */
