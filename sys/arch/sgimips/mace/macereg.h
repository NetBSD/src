/*	$NetBSD: macereg.h,v 1.2.26.1 2007/04/15 16:02:54 yamt Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
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

#define MACE_BASE			0x1f000000

/* PCI definitions (offset 0x080000) */

#define MACE_PCI_ERROR_ADDR		0x00
#define MACE_PCI_ERROR_FLAGS		0x04

#define MACE_PCI_CONTROL		0x08
#define MACE_PCI_CONTROL_INT_MASK	 0x000000ff
#define MACE_PCI_CONTROL_SERR_ENA	 0x00000100
#define MACE_PCI_CONTROL_ARB_N6		 0x00000200
#define MACE_PCI_CONTROL_PARITY_ERR	 0x00000400
#define MACE_PCI_CONTROL_MRMRA_ENA	 0x00000800
#define MACE_PCI_CONTROL_ARB_N3		 0x00001000
#define MACE_PCI_CONTROL_ARB_N4		 0x00002000
#define MACE_PCI_CONTROL_ARB_N5		 0x00004000
#define MACE_PCI_CONTROL_PARK_LIU	 0x00008000

#define MACE_PCI_CONTROL_INV_INT_MASK 	 0x00ff0000
#define MACE_PCI_CONTROL_OVERRUN_INT	 0x01000000
#define MACE_PCI_CONTROL_PARITY_INT	 0x02000000
#define MACE_PCI_CONTROL_SERR_INT	 0x04000000
#define MACE_PCI_CONTROL_IT_INT		 0x08000000
#define MACE_PCI_CONTROL_RE_INT		 0x10000000
#define MACE_PCI_CONTROL_DPED_INT	 0x20000000
#define MACE_PCI_CONTROL_TAR_INT	 0x40000000
#define MACE_PCI_CONTROL_MAR_INT	 0x80000000


#define MACE_PCI_REV_INFO_R	0x0c
#define MACE_PCI_FLUSH_W	0x0c
#define MACE_PCI_CONFIG_ADDR	0xcf8
#define MACE_PCI_CONFIG_DATA	0xcfc
#define MACE_PCI_LOW_MEMORY	0x1a000000
#define MACE_PCI_LOW_IO		0x18000000
#define MACE_PCI_NATIVE_VIEW	0x40000000
#define MACE_PCI_IO		0x80000000
#define MACE_PCI_HI_MEMORY	0x280000000
#define MACE_PCI_HI_IO		0x100000000

#define MACE_VIN1		0x100000
#define MACE_VIN2		0x180000
#define MACE_VOUT		0x200000
#define MACE_PERIF		0x300000
#define MACE_ISA_EXT		0x380000

#define MACE_AUDIO		(MACE_PERIF + 0x00000)
#define MACE_ISA		(MACE_PERIF + 0x10000)
#define MACE_KBDMS		(MACE_PERIF + 0x20000)
#define MACE_I2C		(MACE_PERIF + 0x30000)
#define MACE_UST_MSC		(MACE_PERIF + 0x40000)



/***********************
 * PCI_ERROR_FLAGS Bits
 */
#define MACE_PERR_MASTER_ABORT			0x80000000
#define MACE_PERR_TARGET_ABORT			0x40000000
#define MACE_PERR_DATA_PARITY_ERR		0x20000000
#define MACE_PERR_RETRY_ERR			0x10000000
#define MACE_PERR_ILLEGAL_CMD			0x08000000
#define MACE_PERR_SYSTEM_ERR			0x04000000
#define MACE_PERR_INTERRUPT_TEST		0x02000000
#define MACE_PERR_PARITY_ERR			0x01000000
#define MACE_PERR_OVERRUN			0x00800000
#define MACE_PERR_RSVD				0x00400000
#define MACE_PERR_MEMORY_ADDR			0x00200000
#define MACE_PERR_CONFIG_ADDR			0x00100000
#define MACE_PERR_MASTER_ABORT_ADDR_VALID	0x00080000
#define MACE_PERR_TARGET_ABORT_ADDR_VALID	0x00040000
#define MACE_PERR_DATA_PARITY_ADDR_VALID	0x00020000
#define MACE_PERR_RETRY_ADDR_VALID		0x00010000


/*******************************
 * MACE ISA External Address Map
 */
#define MACE_ISA_EPP_BASE	(MACE_ISA_EXT + 0x00000)
#define MACE_ISA_ECP_BASE	(MACE_ISA_EXT + 0x08000)
#define MACE_ISA_SER1_BASE	(MACE_ISA_EXT + 0x10000)
#define MACE_ISA_SER2_BASE	(MACE_ISA_EXT + 0x18000)
#define MACE_ISA_RTC_BASE	(MACE_ISA_EXT + 0x20000)
#define MACE_ISA_GAME_BASE	(MACE_ISA_EXT + 0x30000)


/*************************
 * ISA Interface Registers
 */

/* ISA Ringbase Address and Reset Register */

#define MACE_ISA_RINGBASE	(MACE_ISA + 0x0000)
#define  MACE_ISA_RING_ALIGN	0x00010000

/* Flash-ROM/LED/DP-RAM/NIC Controller Register */

#define MACE_ISA_FLASH_NIC_REG	(MACE_ISA + 0x0008)
#define  MACE_ISA_FLASH_WE	0x01	/* 1=> Enable FLASH writes */
#define  MACE_ISA_PWD_CLEAR	0x02	/* 1=> PWD CLEAR jumper detected */
#define  MACE_ISA_NIC_DEASSERT	0x04
#define  MACE_ISA_NIC_DATA	0x08
#define  MACE_ISA_LED_RED	0x10	/* 1=> Illuminate RED LED */
#define  MACE_ISA_LED_GREEN	0x20	/* 1=> Illuminate GREEN LED */
#define  MACE_ISA_DP_RAM_ENABLE	0x40

/* Interrupt Status and Mask Registers (32 bits) */

#define MACE_ISA_INT_STATUS	(MACE_ISA + 0x0010)
#define MACE_ISA_INT_MASK	(MACE_ISA + 0x0018)

/* bit definitions */
#define MACE_ISA_INT_RTC_IRQ	0x00000100
#define  MACE_ISA_INT_AUDIO_SC	      0x02
#define  MACE_ISA_INT_AUDIO_DMA1      0x04
#define  MACE_ISA_INT_AUDIO_DMA2      0x10
#define  MACE_ISA_INT_AUDIO_DMA3      0x40


/********************************
 * MACE Timer Interface Registers
 *
 * Note: MSC_UST<31:0> is MSC, MSC_UST<63:32> is UST.
 */
#define MACE_UST	   (MACE_UST_MSC + 0x00) /* Universial system time */
#define MACE_COMPARE1	   (MACE_UST_MSC + 0x08) /* Interrupt compare reg 1 */
#define MACE_COMPARE2	   (MACE_UST_MSC + 0x10) /* Interrupt compare reg 2 */
#define MACE_COMPARE3	   (MACE_UST_MSC + 0x18) /* Interrupt compare reg 3 */
#define MACE_UST_PERIOD	   960			 /* UST Period in ns  */

#define MACE_AIN_MSC_UST   (MACE_UST_MSC + 0x20) /* Audio in MSC/UST pair */
#define MACE_AOUT1_MSC_UST (MACE_UST_MSC + 0x28) /* Audio out 1 MSC/UST pair */
#define MACE_AOUT2_MSC_UST (MACE_UST_MSC + 0x30) /* Audio out 2 MSC/UST pair */
#define MACE_VIN1_MSC_UST  (MACE_UST_MSC + 0x38) /* Video In 1 MSC/UST pair */
#define MACE_VIN2_MSC_UST  (MACE_UST_MSC + 0x40) /* Video In 2 MSC/UST pair */
#define MACE_VOUT_MSC_UST  (MACE_UST_MSC + 0x48) /* Video out MSC/UST pair */
