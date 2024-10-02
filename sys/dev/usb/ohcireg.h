/*	$NetBSD: ohcireg.h,v 1.28.20.1 2024/10/02 12:28:15 martin Exp $	*/
/*	$FreeBSD: src/sys/dev/usb/ohcireg.h,v 1.8 1999/11/17 22:33:40 n_hibma Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DEV_USB_OHCIREG_H_
#define _DEV_USB_OHCIREG_H_

/*** PCI config registers ***/

#define PCI_CBMEM		0x10	/* configuration base memory */

#define PCI_INTERFACE_OHCI	0x10

/*** OHCI registers */

#define OHCI_REVISION		0x00	/* OHCI revision # */
#define  OHCI_REV_LO_MASK	__BITS(3,0)
#define  OHCI_REV_HI_MASK	__BITS(7,4)
#define  OHCI_REV_LO(rev)	__SHIFTOUT((rev), OHCI_REV_LO_MASK)
#define  OHCI_REV_HI(rev)	__SHIFTOUT((rev), OHCI_REV_HI_MASK)
#define  OHCI_REV_LEGACY_MASK	__BIT(8)
#define  OHCI_REV_LEGACY(rev)	__SHIFTOUT((rev), OHCI_REV_LEGACY_MASK)

#define OHCI_CONTROL		0x04
#define  OHCI_CBSR_MASK		__BITS(1,0)	/* Control/Bulk Service Ratio */
#define  OHCI_CBSR_SET(x)	__SHIFTIN((x), OHCI_CBSR_MASK)
#define  OHCI_RATIO_1_1		0
#define  OHCI_RATIO_1_2		1
#define  OHCI_RATIO_1_3		2
#define  OHCI_RATIO_1_4		3
#define  OHCI_PLE		__BIT(2)	/* Periodic List Enable */
#define  OHCI_IE		__BIT(3)	/* Isochronous Enable */
#define  OHCI_CLE		__BIT(4)	/* Control List Enable */
#define  OHCI_BLE		__BIT(5)	/* Bulk List Enable */
#define  OHCI_HCFS_MASK		__BITS(7,6)	/* HostControllerFunctionalState */
#define  OHCI_SET_HCFS(x)	__SHIFTIN((x), OHCI_HCFS_MASK)
#define  OHCI_GET_HCFS(x)	__SHIFTOUT((x), OHCI_HCFS_MASK)
#define  OHCI_HCFS_RESET	0
#define  OHCI_HCFS_RESUME	1
#define  OHCI_HCFS_OPERATIONAL	2
#define  OHCI_HCFS_SUSPEND	3
#define  OHCI_IR		__BIT(8)       /* Interrupt Routing */
#define  OHCI_RWC		__BIT(9)       /* Remote Wakeup Connected */
#define  OHCI_RWE		__BIT(10)      /* Remote Wakeup Enabled */
#define OHCI_COMMAND_STATUS	0x08
#define  OHCI_HCR		__BIT(0)       /* Host Controller Reset */
#define  OHCI_CLF		__BIT(1)       /* Control List Filled */
#define  OHCI_BLF		__BIT(2)       /* Bulk List Filled */
#define  OHCI_OCR		__BIT(3)       /* Ownership Change Request */
#define  OHCI_SOC_MASK		__BITS(17,16)  /* Scheduling Overrun Count */
#define OHCI_INTERRUPT_STATUS	0x0c
#define  OHCI_SO		__BIT(0)	/* Scheduling Overrun */
#define  OHCI_WDH		__BIT(1)	/* Writeback Done Head */
#define  OHCI_SF		__BIT(2)	/* Start of Frame */
#define  OHCI_RD		__BIT(3)	/* Resume Detected */
#define  OHCI_UE		__BIT(4)	/* Unrecoverable Error */
#define  OHCI_FNO		__BIT(5)	/* Frame Number Overflow */
#define  OHCI_RHSC		__BIT(6)	/* Root Hub Status Change */
#define  OHCI_OC		__BIT(30)	/* Ownership Change */
#define  OHCI_MIE		__BIT(31)	/* Master Interrupt Enable */
#define OHCI_INTERRUPT_ENABLE	0x10
#define OHCI_INTERRUPT_DISABLE	0x14
#define OHCI_HCCA		0x18
#define OHCI_PERIOD_CURRENT_ED	0x1c
#define OHCI_PERIOD_CURRENT_ED	0x1c
#define OHCI_CONTROL_HEAD_ED	0x20
#define OHCI_CONTROL_CURRENT_ED	0x24
#define OHCI_BULK_HEAD_ED	0x28
#define OHCI_BULK_CURRENT_ED	0x2c
#define OHCI_DONE_HEAD		0x30
#define OHCI_FM_INTERVAL	0x34
#define  OHCI_FM_IVAL_MASK	__BITS(13,0)
#define  OHCI_FM_GET_IVAL(x)	__SHIFTOUT((x), OHCI_FM_IVAL_MASK)
#define  OHCI_FM_FSMPS_MASK	__BITS(30,16)
#define  OHCI_FM_GET_FSMPS(x)	__SHIFTOUT((x), OHCI_FM_FSMPS_MASK)
#define  OHCI_FM_SET_FSMPS(x)	__SHIFTIN((x), OHCI_FM_FSMPS_MASK)
#define  OHCI_FM_FIT		__BIT(31)
#define OHCI_FM_REMAINING	0x38
#define OHCI_FM_NUMBER		0x3c
#define OHCI_PERIODIC_START	0x40
#define OHCI_LS_THRESHOLD	0x44
#define OHCI_RH_DESCRIPTOR_A	0x48
#define  OHCI_RHD_NDP_MASK	__BITS(7,0)
#define  OHCI_RHD_GET_NDP(x)	__SHIFTOUT((x), OHCI_RHD_NDP_MASK)
#define  OHCI_RHD_PSM		__BIT(8)	/* Power Switching Mode */
#define  OHCI_RHD_NPS		__BIT(9)	/* No Power Switching */
#define  OHCI_RHD_DT		__BIT(10)	/* Device Type */
#define  OHCI_RHD_OCPM		__BIT(11)	/* Overcurrent Protection Mode */
#define  OHCI_RHD_NOCP		__BIT(12)	/* No Overcurrent Protection */
#define  OHCI_RHD_POTPGT_MASK	__BITS(31,24)
#define  OHCI_RHD_GET_POTPGT(x)	__SHIFTOUT((x), OHCI_RHD_POTPGT_MASK)
#define  OHCI_RHD_SET_POTPGT(x)	__SHIFTIN((x), OHCI_RHD_POTPGT_MASK)
#define OHCI_RH_DESCRIPTOR_B	0x4c
#define OHCI_RH_STATUS		0x50
#define  OHCI_RHS_LPS		__BIT(0)	/* Local Power Status */
#define  OHCI_RHS_OCI		__BIT(1)	/* OverCurrent Indicator */
#define  OHCI_RHS_DRWE		__BIT(15)	/* Device Remote Wakeup Enable */
#define  OHCI_RHS_LPSC		__BIT(16)	/* Local Power Status Change */
#define  OHCI_RHS_CCIC		__BIT(17)	/* OverCurrent Indicator Change */
#define  OHCI_RHS_CRWE		__BIT(31)	/* Clear Remote Wakeup Enable */
#define OHCI_RH_PORT_STATUS(n)	(0x50 + (n)*4)	/* 1 based indexing */

#define OHCI_LES (OHCI_PLE | OHCI_IE | OHCI_CLE | OHCI_BLE)
#define OHCI_ALL_INTRS (OHCI_SO | OHCI_WDH | OHCI_SF | OHCI_RD | OHCI_UE | \
			OHCI_FNO | OHCI_RHSC | OHCI_OC)
#define OHCI_NORMAL_INTRS (OHCI_SO | OHCI_WDH | OHCI_RD | OHCI_UE | OHCI_RHSC)

#define OHCI_FSMPS(i) (((i-210)*6/7) << 16)
#define OHCI_PERIODIC(i) ((i)*9/10)

typedef uint32_t ohci_physaddr_t;

#define OHCI_NO_INTRS 32
struct ohci_hcca {
	volatile ohci_physaddr_t	hcca_interrupt_table[OHCI_NO_INTRS];
	volatile uint32_t		hcca_frame_number;
	volatile ohci_physaddr_t	hcca_done_head;
#define OHCI_DONE_INTRS 1
};
#define OHCI_HCCA_SIZE 256
#define OHCI_HCCA_ALIGN 256

#define OHCI_PAGE_SIZE 0x1000
#define OHCI_PAGE(x) ((x) &~ 0xfff)
#define OHCI_PAGE_OFFSET(x) ((x) & 0xfff)

typedef struct {
	volatile uint32_t	ed_flags;
#define OHCI_ED_ADDR_MASK	__BITS(6,0)
#define OHCI_ED_GET_FA(x)	__SHIFTOUT((x), OHCI_ED_ADDR_MASK)
#define OHCI_ED_SET_FA(x)	__SHIFTIN((x), OHCI_ED_ADDR_MASK)
#define OHCI_ED_EN_MASK		__BITS(10,7)
#define OHCI_ED_GET_EN(x)	__SHIFTOUT((x), OHCI_ED_EN_MASK)
#define OHCI_ED_SET_EN(x)	__SHIFTIN((x), OHCI_ED_EN_MASK)
#define OHCI_ED_DIR_MASK	__BITS(12,11)
#define OHCI_ED_GET_DIR(x)	__SHIFTOUT((x), OHCI_ED_DIR_MASK)
#define OHCI_ED_SET_DIR(x)	__SHIFTIN((x), OHCI_ED_DIR_MASK)
#define  OHCI_ED_DIR_TD		0
#define  OHCI_ED_DIR_OUT	1
#define  OHCI_ED_DIR_IN		2
#define OHCI_ED_SPEED		__BIT(13)
#define OHCI_ED_SKIP		__BIT(14)
#define OHCI_ED_FORMAT_MASK	__BIT(15)
#define OHCI_ED_GET_FORMAT(x)	__SHIFTOUT((x), OHCI_ED_FORMAT_MASK)
#define OHCI_ED_SET_FORMAT(x)	__SHIFTIN((x), OHCI_ED_FORMAT_MASK)
#define  OHCI_ED_FORMAT_GEN	0
#define  OHCI_ED_FORMAT_ISO	1
#define OHCI_ED_MAXP_MASK	__BITS(26,16)
#define OHCI_ED_GET_MAXP(x)	__SHIFTOUT((x), OHCI_ED_MAXP_MASK)
#define OHCI_ED_SET_MAXP(x)	__SHIFTIN((x), OHCI_ED_MAXP_MASK)
	volatile ohci_physaddr_t	ed_tailp;
	volatile ohci_physaddr_t	ed_headp;
#define OHCI_HALTED		__BIT(0)
#define OHCI_TOGGLECARRY	__BIT(1)
#define OHCI_HEADMASK		__BITS(31,2)
	volatile ohci_physaddr_t	ed_nexted;
} ohci_ed_t;
/* #define OHCI_ED_SIZE 16 */
#define OHCI_ED_ALIGN 16
#define OHCI_ED_ALLOC_ALIGN	MAX(OHCI_ED_ALIGN, CACHE_LINE_SIZE)
#define OHCI_ED_SIZE		(roundup(sizeof(ohci_ed_t), OHCI_ED_ALLOC_ALIGN))
#define OHCI_ED_CHUNK (PAGE_SIZE / OHCI_ED_SIZE)


typedef struct {
	volatile uint32_t	td_flags;
#define OHCI_TD_R		__BIT(18)	/* Buffer Rounding  */
#define OHCI_TD_DP_MASK		__BITS(20,19)	/* Direction / PID */
#define OHCI_TD_GET_DP(x)	__SHIFTOUT((x), OHCI_TD_DP_MASK)
#define OHCI_TD_SET_DP(x)	__SHIFTIN((x), OHCI_TD_DP_MASK)
#define  OHCI_TD_DP_SETUP	0
#define  OHCI_TD_DP_OUT		1
#define  OHCI_TD_DP_IN		2
#define OHCI_TD_DI_MASK		__BITS(23,21)	/* Delay Interrupt */
#define OHCI_TD_GET_DI(x)	__SHIFTOUT((x), OHCI_TD_DI_MASK)
#define OHCI_TD_SET_DI(x)	__SHIFTIN((x), OHCI_TD_DI_MASK)
#define  OHCI_TD_NOINTR		__SHIFTOUT_MASK(OHCI_TD_DI_MASK)
#define OHCI_TD_TOGGLE_MASK	__BITS(25,24)	/* Toggle */
#define OHCI_TD_GET_TOGGLE(x)	__SHIFTOUT((x), OHCI_TD_TOGGLE_MASK)
#define OHCI_TD_SET_TOGGLE(x)	__SHIFTIN((x), OHCI_TD_TOGGLE_MASK)
#define  OHCI_TD_TOGGLE_CARRY	0
#define  OHCI_TD_TOGGLE_0	2
#define  OHCI_TD_TOGGLE_1	3
#define OHCI_TD_EC_MASK		__BITS(27,26)	/* Error Count */
#define OHCI_TD_GET_EC(x)	__SHIFTOUT((x), OHCI_TD_EC_MASK)
#define OHCI_TD_CC_MASK		__BITS(31,28)	/* Condition Code */
#define OHCI_TD_GET_CC(x)	__SHIFTOUT((x), OHCI_TD_CC_MASK)
#define OHCI_TD_SET_CC(x)	__SHIFTIN((x), OHCI_TD_CC_MASK)

#define  OHCI_TD_NOCC		__SHIFTOUT_MASK(OHCI_TD_CC_MASK)

	volatile ohci_physaddr_t td_cbp;	/* Current Buffer Pointer */
	volatile ohci_physaddr_t td_nexttd;	/* Next TD */
	volatile ohci_physaddr_t td_be;		/* Buffer End */
} ohci_td_t;
/* #define OHCI_TD_SIZE 16 */
#define OHCI_TD_ALIGN 16
#define OHCI_TD_ALLOC_ALIGN	MAX(OHCI_TD_ALIGN, CACHE_LINE_SIZE)
#define OHCI_TD_SIZE		(roundup(sizeof(ohci_td_t), OHCI_TD_ALLOC_ALIGN))
#define OHCI_TD_CHUNK (PAGE_SIZE / OHCI_TD_SIZE)


#define OHCI_ITD_NOFFSET 8
typedef struct {
	volatile uint32_t	itd_flags;
#define OHCI_ITD_SF_MASK	__BITS(15,0)
#define OHCI_ITD_GET_SF(x)	__SHIFTOUT((x), OHCI_ITD_SF_MASK)
#define OHCI_ITD_SET_SF(x)	__SHIFTIN((x), OHCI_ITD_SF_MASK)
#define OHCI_ITD_DI_MASK	__BITS(23,21)	/* Delay Interrupt */
#define OHCI_ITD_GET_DI(x)	__SHIFTOUT((x), OHCI_ITD_DI_MASK)
#define OHCI_ITD_SET_DI(x)	__SHIFTIN((x), OHCI_ITD_DI_MASK)
#define OHCI_ITD_FC_MASK	__BITS(26,24)	/* Frame Count */
#define OHCI_ITD_GET_FC(x)	(__SHIFTOUT((x), OHCI_ITD_FC_MASK) + 1)
#define OHCI_ITD_SET_FC(x)	__SHIFTIN(((x) - 1), OHCI_ITD_FC_MASK)
#define OHCI_ITD_CC_MASK	__BITS(31,28)	/* Condition Code */
#define OHCI_ITD_GET_CC(x)	__SHIFTOUT((x), OHCI_ITD_CC_MASK)
#define OHCI_ITD_SET_CC(x)	__SHIFTIN((x), OHCI_ITD_CC_MASK)
#define  OHCI_ITD_NOCC		__SHIFTOUT_MASK(OHCI_ITD_CC_MASK)
	volatile ohci_physaddr_t itd_bp0;	/* Buffer Page 0 */
	volatile ohci_physaddr_t itd_nextitd;	/* Next ITD */
	volatile ohci_physaddr_t itd_be;	/* Buffer End */
	volatile uint16_t itd_offset[OHCI_ITD_NOFFSET];/* Buffer offsets */
#define itd_pswn itd_offset			/* Packet Status Word*/
#define OHCI_ITD_PAGE_SELECT	__BIT(12)
#define OHCI_ITD_MK_OFFS(len)	(0xe000 | ((len) & 0x1fff))

#define OHCI_ITD_PSW_SIZE_MASK	__BITS(10,0)	/* Transfer length */
#define OHCI_ITD_PSW_SIZE(x)	__SHIFTOUT((x), OHCI_ITD_PSW_SIZE_MASK)
#define OHCI_ITD_PSW_CC_MASK	__BITS(15,12)	/* Condition Code */
#define OHCI_ITD_PSW_GET_CC(x)	__SHIFTOUT((x), OHCI_ITD_PSW_CC_MASK)
} ohci_itd_t;
/* #define OHCI_ITD_SIZE 32 */
#define OHCI_ITD_ALIGN 32
#define OHCI_ITD_ALLOC_ALIGN	MAX(OHCI_ITD_ALIGN, CACHE_LINE_SIZE)
#define OHCI_ITD_SIZE		(roundup(sizeof(ohci_itd_t), OHCI_ITD_ALLOC_ALIGN))
#define OHCI_ITD_CHUNK		(PAGE_SIZE / OHCI_ITD_SIZE)


#define OHCI_CC_NO_ERROR		0
#define OHCI_CC_CRC			1
#define OHCI_CC_BIT_STUFFING		2
#define OHCI_CC_DATA_TOGGLE_MISMATCH	3
#define OHCI_CC_STALL			4
#define OHCI_CC_DEVICE_NOT_RESPONDING	5
#define OHCI_CC_PID_CHECK_FAILURE	6
#define OHCI_CC_UNEXPECTED_PID		7
#define OHCI_CC_DATA_OVERRUN		8
#define OHCI_CC_DATA_UNDERRUN		9
#define OHCI_CC_BUFFER_OVERRUN		12
#define OHCI_CC_BUFFER_UNDERRUN		13
#define OHCI_CC_NOT_ACCESSED		14
#define OHCI_CC_NOT_ACCESSED_MASK	14

/* Some delay needed when changing certain registers. */
#define OHCI_ENABLE_POWER_DELAY	5
#define OHCI_READ_DESC_DELAY	5

#endif /* _DEV_USB_OHCIREG_H_ */
