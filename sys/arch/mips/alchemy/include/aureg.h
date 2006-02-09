/* $NetBSD: aureg.h,v 1.10 2006/02/09 01:20:18 gdamore Exp $ */

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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

/*  *********************************************************************
    *  Naming schemes for constants in these files:
    *
    *  M_xxx            MASK constant (identifies bits in a register).
    *                   For multi-bit fields, all bits in the field will
    *                   be set.
    *
    *  K_xxx            "Code" constant (value for data in a multi-bit
    *                   field).  The value is right justified.
    *
    *  V_xxx            "Value" constant.  This is the same as the
    *                   corresponding "K_xxx" constant, except it is
    *                   shifted to the correct position in the register.
    *
    *  S_xxx            SHIFT constant.  This is the number of bits that
    *                   a field value (code) needs to be shifted
    *                   (towards the left) to put the value in the right
    *                   position for the register.
    *
    *  A_xxx            ADDRESS constant.  This will be a physical
    *                   address.  Use the MIPS_PHYS_TO_KSEG1 macro to
    *                   generate a K1SEG address.
    *
    *  R_xxx            RELATIVE offset constant.  This is an offset from
    *                   an A_xxx constant (usually the first register in
    *                   a group).
    *
    *  G_xxx(X)         GET value.  This macro obtains a multi-bit field
    *                   from a register, masks it, and shifts it to
    *                   the bottom of the register (retrieving a K_xxx
    *                   value, for example).
    *
    *  V_xxx(X)         VALUE.  This macro computes the value of a
    *                   K_xxx constant shifted to the correct position
    *                   in the register.
    ********************************************************************* */

#if !defined(__ASSEMBLER__)
#define _MAKE64(x) ((uint64_t)(x))
#define _MAKE32(x) ((uint32_t)(x))
#else
#define _MAKE64(x) (x)  
#define _MAKE32(x) (x)
#endif 

/* Make a mask for 1 bit at position 'n' */
#define _MAKEMASK1_64(n) (_MAKE64(1) << _MAKE64(n))
#define _MAKEMASK1_32(n) (_MAKE32(1) << _MAKE32(n))

/* Make a mask for 'v' bits at position 'n' */
#define _MAKEMASK_64(v,n) (_MAKE64((_MAKE64(1)<<(v))-1) << _MAKE64(n))
#define _MAKEMASK_32(v,n) (_MAKE32((_MAKE32(1)<<(v))-1) << _MAKE32(n))

/* Make a value at 'v' at bit position 'n' */
#define _MAKEVALUE_64(v,n) (_MAKE64(v) << _MAKE64(n))
#define _MAKEVALUE_32(v,n) (_MAKE32(v) << _MAKE32(n))

#define _GETVALUE_64(v,n,m) ((_MAKE64(v) & _MAKE64(m)) >> _MAKE64(n))
#define _GETVALUE_32(v,n,m) ((_MAKE32(v) & _MAKE32(m)) >> _MAKE32(n))


/************************************************************************/
/********************   AC97 Controller registers   *********************/
/************************************************************************/
#define	AC97_BASE		0x10000000

#define	AC97_CONFIG		0x00

#define	  M_AC97CFG_RS		  _MAKEMASK1_32(0)
#define	  M_AC97CFG_SN		  _MAKEMASK1_32(1)
#define	  M_AC97CFG_SG		  _MAKEMASK1_32(2)

#define	  S_AC97CFG_XS		  _MAKE32(12)
#define	  M_AC97CFG_XS		  _MAKEMASK_32(10)
#define	  V_AC97CFG_XS(x)	  _MAKEVALUE_32(x, S_AC97CFG_XS)
#define	  G_AC97CFG_XS(x)	  _GETVALUE_32(x, S_AC97CFG_XS, M_AC97CFG_XS)

#define	  S_AC97CFG_RC		  _MAKE32(12)
#define	  M_AC97CFG_RC		  _MAKEMASK_32(10)
#define	  V_AC97CFG_RC(x)	  _MAKEVALUE_32(x, S_AC97CFG_RC)
#define	  G_AC97CFG_RC(x)	  _GETVALUE_32(x, S_AC97CFG_RC, M_AC97CFG_RC)

#define	AC97_STATUS		0x04

#define	  M_AC97STAT_RF		  _MAKEMASK1_32(0)
#define	  M_AC97STAT_RE		  _MAKEMASK1_32(1)
#define	  M_AC97STAT_TF		  _MAKEMASK1_32(3)
#define	  M_AC97STAT_TE		  _MAKEMASK1_32(4)
#define	  M_AC97STAT_CP		  _MAKEMASK1_32(6)
#define	  M_AC97STAT_RD		  _MAKEMASK1_32(7)
#define	  M_AC97STAT_RO		  _MAKEMASK1_32(8)
#define	  M_AC97STAT_RU		  _MAKEMASK1_32(9)
#define	  M_AC97STAT_XO		  _MAKEMASK1_32(10)
#define	  M_AC97STAT_XU		  _MAKEMASK1_32(11)

#define	AC97_DATA		0x08

#define	  S_AC97DATA_DATA	  _MAKE32(0)
#define	  M_AC97DATA_DATA	  _MAKEMASK_32(16)
#define	  V_AC97DATA_DATA(x)	  _MAKEVALUE_32(x, S_AC97DATA_DATA)
#define	  G_AC97DATA_DATA(x)	  _GETVALUE_32(x, S_AC97DATA_DATA, M_AC97DATA_DATA)

#define	AC97_COMMAND		0x0c

#define	  S_AC97CMD_INDEX	  _MAKE32(0)
#define	  M_AC97CMD_INDEX	  _MAKEMASK_32(7)
#define	  V_AC97CMD_INDEX(x)	  _MAKEVALUE_32(x, S_AC97CMD_INDEX)
#define	  G_AC97CMD_INDEX(x)	  _GETVALUE_32(x, S_AC97CMD_INDEX, M_AC97CMD_INDEX)

#define	  M_AC97CMD_RW		  _MAKEMASK1_32(7)

#define	  S_AC97CMD_DATA	  _MAKE32(16)
#define	  M_AC97CMD_DATA	  _MAKEMASK_32(16)
#define	  V_AC97CMD_DATA(x)	  _MAKEVALUE_32(x, S_AC97CMD_DATA)
#define	  G_AC97CMD_DATA(x)	  _GETVALUE_32(x, S_AC97CMD_DATA, M_AC97CMD_DATA)

#define	AC97_COMMAND_RESPONSE	0x0c

#define	  S_AC97CMDRESP_DATA	  _MAKE32(0)
#define	  M_AC97CMDRESP_DATA	  _MAKEMASK_32(16)
#define	  V_AC97CMDRESP_DATA(x)	  _MAKEVALUE_32(x, S_AC97CMDRESP_DATA)
#define	  G_AC97CMDRESP_DATA(x)	  _GETVALUE_32(x, S_AC97CMDRESP_DATA, M_AC97CMDRESP_DATA)

#define	AC97_ENABLE		0x10

#define	  M_AC97EN_CE		  _MAKEMASK1_32(0)
#define	  M_AC97EN_D		  _MAKEMASK1_32(1)

#define	AC97_SIZE		0x14		/* size of register set */

/************************************************************************/
/***********************   USB Host registers   *************************/
/************************************************************************/
#define	USBH_BASE		0x10100000

#define	USBH_ENABLE		0x7fffc
#define	  UE_RD			  0x00000010	/* reset done */
#define	  UE_CE			  0x00000008	/* clock enable */
#define	  UE_E			  0x00000004	/* enable */
#define	  UE_C			  0x00000002	/* coherent */
#define	  UE_BE			  0x00000001	/* big-endian */

#define	USBH_SIZE		0x100000	/* size of register set */

#define	AU1550_USBH_BASE	0x14020000
#define	AU1550_USBH_ENABLE	0x7ffc
#define AU1550_USBH_SIZE	0x60000

/************************************************************************/
/**********************   USB Device registers   ************************/
/************************************************************************/
#define	USBD_BASE		0x10200000

#define USBD_EP0RD		0x00		/* Read from endpoint 0 */
#define USBD_EP0WR		0x04		/* Write to endpoint 0 */
#define USBD_EP1WR		0x08		/* Write to endpoint 1 */
#define USBD_EP2WR		0x0c		/* Write to endpoint 2 */
#define USBD_EP3RD		0x10		/* Read from endpoint 3 */
#define USBD_EP4RD		0x14		/* Read from endpoint 4 */
#define USBD_INTEN		0x18		/* Interrupt Enable Register */
#define USBD_INTSTAT		0x1c		/* Interrupt Status Register */
#define USBD_CONFIG		0x20		/* Write Configuration Register */
#define USBD_EP0CS		0x24		/* Endpoint 0 control and status */
#define USBD_EP1CS		0x28		/* Endpoint 1 control and status */
#define USBD_EP2CS		0x2c		/* Endpoint 2 control and status */
#define USBD_EP3CS		0x30		/* Endpoint 3 control and status */
#define USBD_EP4CS		0x34		/* Endpoint 4 control and status */
#define USBD_FRAMENUM		0x38		/* Current frame number */
#define USBD_EP0RDSTAT		0x40		/* EP0 Read FIFO Status */
#define USBD_EP0WRSTAT		0x44		/* EP0 Write FIFO Status */
#define USBD_EP1WRSTAT		0x48		/* EP1 Write FIFO Status */
#define USBD_EP2WRSTAT		0x4c		/* EP2 Write FIFO Status */
#define USBD_EP3RDSTAT		0x50		/* EP3 Read FIFO Status */
#define USBD_EP4RDSTAT		0x54		/* EP4 Read FIFO Status */
#define USBD_ENABLE		0x58		/* USB Device Controller Enable */

/************************************************************************/
/*************************   IRDA registers   ***************************/
/************************************************************************/
#define	IRDA_BASE		0x10300000

/************************************************************************/
/******************   Interrupt Controller registers   ******************/
/************************************************************************/

#define	IC0_BASE	0x10400000
#define	IC1_BASE	0x11800000

/*
 * The *_READ registers read the current value of the register
 * The *_SET registers set to 1 all bits that are written 1
 * The *_CLEAR registers clear to zero all bits that are written as 1
 */
#define	IC_CONFIG0_READ			0x40	/* See table below */
#define	IC_CONFIG0_SET			0x40
#define	IC_CONFIG0_CLEAR		0x44

#define	IC_CONFIG1_READ			0x48	/* See table below */
#define	IC_CONFIG1_SET			0x48
#define	IC_CONFIG1_CLEAR		0x4c

#define	IC_CONFIG2_READ			0x50	/* See table below */
#define	IC_CONFIG2_SET			0x50
#define	IC_CONFIG2_CLEAR		0x54

#define	IC_REQUEST0_INT			0x54	/* Show active interrupts on request 0 */

#define	IC_SOURCE_READ			0x58	/* Interrupt source */
#define	IC_SOURCE_SET			0x58	/*  0 - test bit used as source */
#define	IC_SOURCE_CLEAR			0x5c	/*  1 - peripheral/GPIO used as source */

#define	IC_REQUEST1_INT			0x5c	/* Show active interrupts on request 1 */

#define	IC_ASSIGN_REQUEST_READ		0x60	/* Assigns the interrupt to one of the */
#define	IC_ASSIGN_REQUEST_SET		0x60	/* CPU requests (0 - assign to request 1, */
#define	IC_ASSIGN_REQUEST_CLEAR		0x64	/* 1 - assign to request 0) */

#define	IC_WAKEUP_READ			0x68	/* Controls whether the interrupt can */
#define	IC_WAKEUP_SET			0x68	/* cause a wakeup from IDLE */
#define	IC_WAKEUP_CLEAR			0x6c

#define	IC_MASK_READ			0x70	/* Enables/Disables the interrupt */
#define	IC_MASK_SET			0x70
#define	IC_MASK_CLEAR			0x74

#define	IC_RISING_EDGE_DETECT		0x78	/* Check/clear rising edge interrupts */
#define	IC_RISING_EDGE_DETECT_CLEAR	0x78

#define	IC_FAILLING_EDGE_DETECT		0x7c	/* Check/clear falling edge interrupts */
#define	IC_FAILLING_EDGE_DETECT_CLEAR	0x7c

#define	IC_TEST_BIT			0x80	/* single bit source select testing register */

/*
 *	Interrupt Configuration Register Functions
 *
 *	Cfg2[n]	Cfg1[n]	Cfg0[n]		Function
 *	   0	   0	   0		Interrupts Disabled
 *	   0	   0	   1		Rising Edge Enabled
 *	   0	   1	   0		Falling Edge Enabled
 *	   0	   1	   1		Rising and Falling Edge Enabled
 *	   1	   0	   0		Interrupts Disabled
 *	   1	   0	   1		High Level Enabled
 *	   1	   1	   0		Low Level Enabled
 *	   1	   1	   1		Both Levels and Both Edges Enabled
 */

/************************************************************************/
/**********************   Ethernet MAC registers   **********************/
/************************************************************************/

#define	MAC0_BASE		0x10500000
#define	MAC1_BASE		0x10510000
#define	MACx_SIZE		0x28

#define	AU1500_MAC0_BASE	0x11500000	/* Grr, difference on Au1500 */
#define	AU1500_MAC1_BASE	0x11510000	/* Grr, difference on Au1500 */

#define	MAC0_ENABLE		0x10520000
#define	MAC1_ENABLE		0x10520004
#define	MACENx_SIZE		0x04

#define	AU1500_MAC0_ENABLE	0x11520000	/* Grr, difference on Au1500 */
#define	AU1500_MAC1_ENABLE	0x11520004	/* Grr, difference on Au1500 */

#define	MAC0_DMA_BASE		0x14004000
#define	MAC1_DMA_BASE		0x14004200
#define	MACx_DMA_SIZE		0x140

/************************************************************************/
/********************   Secure Digital registers   **********************/
/************************************************************************/
#define	SD0_BASE		0x10600000
#define	SD1_BASE		0x10680000

/************************************************************************/
/*************************   I^2S registers   ***************************/
/************************************************************************/
#define	I2S_BASE		0x11000000

/************************************************************************/
/**************************   UART registers   **************************/
/************************************************************************/

#define	UART0_BASE	0x11100000
#define	UART1_BASE	0x11200000
#define	UART2_BASE	0x11300000
#define	UART3_BASE	0x11400000

/************************************************************************/
/*************************   SSI registers   ****************************/
/************************************************************************/
#define	SSI0_BASE		0x11600000
#define	SSI1_BASE		0x11680000

/************************************************************************/
/************************   GPIO2 registers   ***************************/
/************************************************************************/
#define	GPIO2_BASE		0x11700000

/************************************************************************/
/*************************   PCI registers   ****************************/
/************************************************************************/
#define	PCI_BASE		0x14005000
#define	PCI_HEADER		0x14005100
#define	PCI_MEM_BASE		0x400000000ULL
#define	PCI_IO_BASE		0x500000000ULL
#define	PCI_CONFIG_BASE		0x600000000ULL

/************************************************************************/
/******************   Programmable Counter registers   ******************/
/************************************************************************/

#define	SYS_BASE		0x11900000

#define	PC_BASE			SYS_BASE

#define	PC_TRIM0		0x00		/* PC0 Divide (16 bits) */
#define	PC_COUNTER_WRITE0	0x04		/* set PC0 */
#define	PC_MATCH0_0		0x08		/* match counter & interrupt */
#define	PC_MATCH1_0		0x0c		/* match counter & interrupt */
#define	PC_MATCH2_0		0x10		/* match counter & interrupt */
#define	PC_COUNTER_CONTROL	0x14		/* Programmable Counter Control */
#define	  CC_E1S		  0x00800000	/* Enable PC1 write status */
#define	  CC_T1S		  0x00100000	/* Trim PC1 write status */
#define	  CC_M21		  0x00080000	/* Match 2 of PC1 write status */
#define	  CC_M11		  0x00040000	/* Match 1 of PC1 write status */
#define	  CC_M01		  0x00020000	/* Match 0 of PC1 write status */
#define	  CC_C1S		  0x00010000	/* PC1 write status */
#define	  CC_BP			  0x00004000	/* Bypass OSC (use GPIO1) */
#define	  CC_EN1		  0x00002000	/* Enable PC1 */
#define	  CC_BT1		  0x00001000	/* Bypass Trim on PC1 */
#define	  CC_EN0		  0x00000800	/* Enable PC0 */
#define	  CC_BT0		  0x00000400	/* Bypass Trim on PC0 */
#define	  CC_EO			  0x00000100	/* Enable Oscillator */
#define	  CC_E0S		  0x00000080	/* Enable PC0 write status */
#define	  CC_32S		  0x00000020	/* 32.768kHz OSC status */
#define	  CC_T0S		  0x00000010	/* Trim PC0 write status */
#define	  CC_M20		  0x00000008	/* Match 2 of PC0 write status */
#define	  CC_M10		  0x00000004	/* Match 1 of PC0 write status */
#define	  CC_M00		  0x00000002	/* Match 0 of PC0 write status */
#define	  CC_C0S		  0x00000001	/* PC0 write status */
#define	PC_COUNTER_READ_0	0x40		/* get PC0 */
#define	PC_TRIM1		0x44		/* PC1 Divide (16 bits) */
#define	PC_COUNTER_WRITE1	0x48		/* set PC1 */
#define	PC_MATCH0_1		0x4c		/* match counter & interrupt */
#define	PC_MATCH1_1		0x50		/* match counter & interrupt */
#define	PC_MATCH2_1		0x54		/* match counter & interrupt */
#define	PC_COUNTER_READ_1	0x58		/* get PC1 */

#define	PC_SIZE			0x5c		/* size of register set */
#define	PC_RATE			32768		/* counter rate is 32.768kHz */

/************************************************************************/
/*******************   Frequency Generator Registers   ******************/
/************************************************************************/

#define SYS_FREQCTRL0		(SYS_BASE + 0x20)
#define SFC_FRDIV2(f)		(f<<22)		/* 29:22. Freq Divider 2 */
#define SFC_FE2			(1<<21)		/* Freq generator output enable 2 */
#define SFC_FS2			(1<<20)		/* Freq generator source 2 */
#define SFC_FRDIV1(f)		(f<<12)		/* 19:12. Freq Divider 1 */
#define SFC_FE1			(1<<11)		/* Freq generator output enable 1 */
#define SFC_FS1			(1<<10)		/* Freq generator source 1 */
#define SFC_FRDIV0(f)		(f<<2)		/* 9:2. Freq Divider 0 */
#define SFC_FE0			2		/* Freq generator output enable 0 */
#define SFC_FS0			1		/* Freq generator source 0 */

#define SYS_FREQCTRL1		(SYS_BASE + 0x24)
#define SFC_FRDIV5(f)		(f<<22)		/* 29:22. Freq Divider 5 */
#define SFC_FE5			(1<<21)		/* Freq generator output enable 5 */
#define SFC_FS5			(1<<20)		/* Freq generator source 5 */
#define SFC_FRDIV4(f)		(f<<12)		/* 19:12. Freq Divider 4 */
#define SFC_FE4			(1<<11)		/* Freq generator output enable 4 */
#define SFC_FS4			(1<<10)		/* Freq generator source 4 */
#define SFC_FRDIV3(f)		(f<<2)		/* 9:2. Freq Divider 3 */
#define SFC_FE3			2		/* Freq generator output enable 3 */
#define SFC_FS3			1		/* Freq generator source 3 */

/************************************************************************/
/******************   Clock Source Control Registers   ******************/
/************************************************************************/

#define SYS_CLKSRC		(SYS_BASE + 0x28)
#define  SCS_ME1(n)		(n<<27)		/* EXTCLK1 Clock Mux input select */
#define  SCS_ME0(n)		(n<<22)		/* EXTCLK0 Clock Mux input select */
#define  SCS_MPC(n)		(n<<17)		/* PCI clock mux input select */
#define  SCS_MUH(n)		(n<<12)		/* USB Host clock mux input select */
#define  SCS_MUD(n)		(n<<7)		/* USB Device clock mux input select */
#define   SCS_MEx_AUX		0x1		/* Aux clock */
#define   SCS_MEx_FREQ0		0x2		/* FREQ0 */
#define   SCS_MEx_FREQ1		0x3		/* FREQ1 */
#define   SCS_MEx_FREQ2		0x4		/* FREQ2 */
#define   SCS_MEx_FREQ3		0x5		/* FREQ3 */
#define   SCS_MEx_FREQ4		0x6		/* FREQ4 */
#define   SCS_MEx_FREQ5		0x7		/* FREQ5 */
#define  SCS_DE1		(1<<26)		/* EXTCLK1 clock divider select */
#define  SCS_CE1		(1<<25)		/* EXTCLK1 clock select */
#define  SCS_DE0		(1<<21)		/* EXTCLK0 clock divider select */
#define  SCS_CE0		(1<<20)		/* EXTCLK0 clock select */
#define  SCS_DPC		(1<<16)		/* PCI clock divider select */
#define  SCS_CPC		(1<<15)		/* PCI clock select */
#define  SCS_DUH		(1<<11)		/* USB Host clock divider select */
#define  SCS_CUH		(1<<10)		/* USB Host clock select */
#define  SCS_DUD		(1<<6)		/* USB Device clock divider select */
#define  SCS_CUD		(1<<5)		/* USB Device clock select */

/************************************************************************/
/***************************   PLL Control  *****************************/
/************************************************************************/

#define SYS_CPUPLL		(SYS_BASE + 0x60)
#define SYS_AUXPLL              (SYS_BASE + 0x64)
