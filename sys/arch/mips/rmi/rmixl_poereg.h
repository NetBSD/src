/*	$NetBSD: rmixl_poereg.h,v 1.1.2.1 2012/01/19 17:35:58 matt Exp $	*/
/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifndef _MIPS_RMI_RMIXL_POEREG_H_
#define _MIPS_RMI_RMIXL_POEREG_H_

/* Class Enqueue/Dequeue and Message Base Address Registers */
#define	RMIXLP_POE_NCLASS	8
#define	RMIXLP_POE_CL_ENQ_SPILL_BASE_L(n)	_RMIXL_OFFSET(0x40+2*(n)) // Class n Enqueue Spill Base Address Low Register
#define	RMIXLP_POE_CL_ENQ_SPILL_BASE_H(n)	_RMIXL_OFFSET(0x41+2*(n)) // Class n Enqueue Spill Base Address High Register

#define	RMIXLP_POE_CL_DEQ_SPILL_BASE_L(n)		_RMIXL_OFFSET(0x50+2*(n)) // Class n Dequeue Spill Base Address Low Register
#define	RMIXLP_POE_CL_DEQ_SPILL_BASE_H(n)	_RMIXL_OFFSET(0x51+2*(n)) // Class n Dequeue Spill Base Address High Register

#define	RMIXLP_POE_MSG_STORAGE_BASE_L		_RMIXL_OFFSET(0x60) // Message Storage Base Address Low Register
#define	RMIXLP_POE_MSG_STORAGE_BASE_H		_RMIXL_OFFSET(0x61) // Message Storage Base Address High Register

#define	RMIXLP_POE_FBP_BASE_L			_RMIXL_OFFSET(0x62) // Free Buffer Pool Base Address Low Register
#define	RMIXLP_POE_FBP_BASE_H			_RMIXL_OFFSET(0x63) // Free Buffer Pool Base Address High Register

/* Class Enqueue/Dequeue Maximum Offset From Base Address Registers */
#define	RMIXLP_POE_CL_ENQ_SPILL_MAXLINE(n)	_RMIXL_OFFSET(0x64+(n)) // Class n Enqueue Spill Maximum Offset From Base Address Register
#define	RMIXLP_POE_CL_DEQ_SPILL_MAXLINE(n)	_RMIXL_OFFSET(0x6C+(n)) // Class n Dequeue Spill Maximum Offset From Base Address Register

/* Maximum POE Buffers Registers */
#define	RMIXLP_POE_NFLOW	8
#define	RMIXLP_POE_MAX_FLOW_MSG_REG(n)		_RMIXL_OFFSET(0x80+(n)) // Maximum Per-Flow POE Buffer Register n

#define	RMIXLP_POE_MAX_MSG_CL(n)		_RMIXL_OFFSET(0x88+(n)) // Maximum Class n POE Buffer Register

#define	RMIXLP_POE_MAX_LOC_BUF_STG_CL(n)	_RMIXL_OFFSET(0x90+(n)) // Maximum Class n Local POE Buffer Register

/* Enqueue Message Count Registers */
#define	RMIXLP_POE_CL_SIZE(n)			_RMIXL_OFFSET(0x98+(n)) // Class n Enqueue Message Count Register

/* Receive Message Error Registers */
#define	RMIXLP_POE_ERR_MSG_REG(n)		_RMIXL_OFFSET(0xA0+(n)) // Receive Error Message_Register n

/* Statistics Registers */
#define	RMIXLP_POE_OOO_MSG_CNT_LO		_RMIXL_OFFSET(0xA8) // Out of Order Message Count Low Register
#define	RMIXLP_POE_IN_ORDER_MSG_CNT_LO		_RMIXL_OFFSET(0xA9) // In Order Message Count Low Register
#define	RMIXLP_POE_LOC_BUF_STOR_CNT_LO		_RMIXL_OFFSET(0xAA) // Local Buffer Storage Count Low Register
#define	RMIXLP_POE_EXT_BUF_STOR_CNT_LO		_RMIXL_OFFSET(0xAB) // External Buffer Storage Count Low Register
#define	RMIXLP_POE_LOC_BUF_ALLOC_CNT_LO		_RMIXL_OFFSET(0xAC) // Local Buffer Allocation Count Low Register
#define	RMIXLP_POE_EXT_BUF_ALLOC_CNT_LO		_RMIXL_OFFSET(0xAD) // External Buffer Allocation Count Low Register
#define	RMIXLP_POE_OOO_MSG_CNT_HI		_RMIXL_OFFSET(0xAE) // Out of Order Message Count High Register
#define	RMIXLP_POE_IN_ORDER_MSG_CNT_HI		_RMIXL_OFFSET(0xAF) // In Order Message Count High Register
#define	RMIXLP_POE_LOC_BUF_STOR_CNT_HI		_RMIXL_OFFSET(0xB0) // Local Buffer Storage Count High Register
#define	RMIXLP_POE_EXT_BUF_STOR_CNT_HI		_RMIXL_OFFSET(0xB1) // External Buffer Storage Count High Register
#define	RMIXLP_POE_LOC_BUF_ALLOC_CNT_HI		_RMIXL_OFFSET(0xB2) // Local Buffer Allocation Count High Register
#define	RMIXLP_POE_EXT_BUF_ALLOC_CNT_HI		_RMIXL_OFFSET(0xB3) // External Buffer Allocation Count High Register
#define	RMIXLP_POE_MODE_ERROR_FLOW_ID		_RMIXL_OFFSET(0xB4) // Mode Error Flow ID Register
#define	RMIXLP_POE_STATISTICS_ENABLE		_RMIXL_OFFSET(0xB5) // Statistics Enable Register
#define	RMIXLP_POE_MAX_SIZE_FLOW		_RMIXL_OFFSET(0xB6) // Maximum Size Flow Register
#define	RMIXLP_POE_MAX_SIZE			_RMIXL_OFFSET(0xB7) // Maximum Size Register

/* Free Buffer Pool Configuration Registers */
#define	RMIXLP_POE_FBP_SP			_RMIXL_OFFSET(0xB8) // Free Buffer Pool Stack Pointer Register
#define	RMIXLP_POE_FBP_SP_EN			_RMIXL_OFFSET(0xB9) // Free Buffer Pool Stack Pointer Enable Register
#define	RMIXLP_POE_LOC_ALLOC_EN			_RMIXL_OFFSET(0xBA) // Local Message Allocation Enable Register
#define	RMIXLP_POE_EXT_ALLOC_EN			_RMIXL_OFFSET(0xBB) // External Message Allocation Enable Register

#define	RMIXLP_POE_NDISTR			16
#define	RMIXLP_POE_DISTR_DROP_CNT(n)		_RMIXL_OFFSET(0x100+(n)) // Distribution n Vector Drop Count Register

#define	RMIXLP_POE_CLASS_DROP_CNT(n)		_RMIXL_OFFSET(0x110+(n)) // Class n Drop Count Register

#define	RMIXLP_POE_DISTR_C_DROP_CNT(n)		_RMIXL_OFFSET(0x118+(n)) // Distribution Class 0 Drop Count Register
#define	RMIXLP_POE_CPU_DROP_CNT			_RMIXL_OFFSET(0x120) // CPU Drop Count Register
#define	RMIXLP_POE_MAX_FLOW_DROP_CNT		_RMIXL_OFFSET(0x121) // Maximum Flow Drop Count Register

/* Interrupt Registers */
#define	RMIXLP_POE_INTERRUPT_VEC		_RMIXL_OFFSET(0x180) // Packet Ordering Engine Interrupt Vector Register
#define	RMIXLP_POE_INTERRUPT_MASK		_RMIXL_OFFSET(0x181) // Packet Ordering Engine Interrupt Mask Register
#define	RMIXLP_POE_FATAL_ERROR_MASK		_RMIXL_OFFSET(0x182) // Packet Ordering Engine Fatal Error Mask
#define	RMIXLP_POE_IODATA_IC_CONFIG		_RMIXL_OFFSET(0x183) // Packet Ordering Engine I/O Data Interconnect Configuration
#define	RMIXLP_POE_TIMEOUT_TIMER		_RMIXL_OFFSET(0x184) // Packet Ordering Engine Time-out Timer
#define	RMIXLP_POE_CACHE_ALLOC_EN		_RMIXL_OFFSET(0x185) // Packet Ordering Engine Cache Allocation Enable Register

/* ECC Error Count Registers */
#define	RMIXLP_POE_FBP_ECC_ERR_CNT		_RMIXL_OFFSET(0x186) // Free Buffer Pool ECC Error Count
#define	RMIXLP_POE_MSG_STOR_ECC_ERR_CNT		_RMIXL_OFFSET(0x187) // Message Storage Errors Register
#define	RMIXLP_POE_FID_INFO_ECC_ERR_CNT		_RMIXL_OFFSET(0x188) // Flow Information Errors Register
#define	RMIXLP_POE_MSG_INFO_ECC_ERR_CNT		_RMIXL_OFFSET(0x189) // Message Information Errors Register
#define	RMIXLP_POE_LL_ECC_ERR_CNT		_RMIXL_OFFSET(0x18A) // Linked List Errors Register
#define	RMIXLP_POE_SIZE_ECC_ERR_CNT		_RMIXL_OFFSET(0x18B) // Size Errors Register
#define	RMIXLP_POE_FMN_TXCR_ECC_ERR_CNT		_RMIXL_OFFSET(0x18C) // Fast Message Network Transmit Errors Register
#define	RMIXLP_POE_ENQ_INSPIL_ECC_ERR_CNT	_RMIXL_OFFSET(0x18D) // Enqueue Spill Input Errors Register
#define	RMIXLP_POE_ENQ_OUTSPIL_ECC_ERR_CNT	_RMIXL_OFFSET(0x18E) // Enqueue Spill Output Errors Register
#define	RMIXLP_POE_DEQ_OUTSPIL_ECC_ERR_CNT	_RMIXL_OFFSET(0x18F) // Dequeue Spill Output Errors Register
#define	RMIXLP_POE_ENQD_MSG_SENT		_RMIXL_OFFSET(0x190) // Enqueue Message Sent Register
#define	RMIXLP_POE_ENQD_MSG_CNT			_RMIXL_OFFSET(0x191) // Enqueue Message Count Register

/* FID Info RAM Indirect Access Registers */
#define	RMIXLP_POE_FID_RDATA			_RMIXL_OFFSET(0x192) // FID Flow Read Data Register
#define	RMIXLP_POE_FID_WDATA			_RMIXL_OFFSET(0x193) // FID Flow Read Write Register
#define	RMIXLP_POE_FID_CMD			_RMIXL_OFFSET(0x194) // FID Indirect Access Command Register
#define	RMIXLP_POE_FID_ADR			_RMIXL_OFFSET(0x195) // FID Flow Info Address Register

/* Message Information SRAM Indirect Access Registers */
#define	RMIXLP_POE_MSG_INFO_CMD			_RMIXL_OFFSET(0x196) // Message Information Command Register
#define	RMIXLP_POE_MSG_INFO_ADR			_RMIXL_OFFSET(0x197) // Message Information Read Address Register
#define	RMIXLP_POE_MSG_INFO_RDATA		_RMIXL_OFFSET(0x198) // Message Information Read Data Register

/* Linked List SRAM Indirect Access Registers */
#define	RMIXLP_POE_LL_CMD			_RMIXL_OFFSET(0x199) // Linked List Command Register
#define	RMIXLP_POE_LL_ADR			_RMIXL_OFFSET(0x19A) // Linked List Read Address Register
#define	RMIXLP_POE_LL_RDATA			_RMIXL_OFFSET(0x19B) // Linked List Read Data Register

/* Message Storage SRAM Indirect Access Registers */
#define	RMIXLP_POE_MSG_STG_CMD			_RMIXL_OFFSET(0x19C) // Message Storage Command Register
#define	RMIXLP_POE_MSG_STG_ADR			_RMIXL_OFFSET(0x19D) // Message Storage Read Address Register
#define	RMIXLP_POE_MSG_STG_RDATA		_RMIXL_OFFSET(0x19E) // Message Storage Read Data Register

/* Distribution Vector Masking Thresholds Registers */
#define	RMIXLP_POE_NDISTR_THRESHOLD		4
#define	RMIXLP_POE_DISTR_THRESHOLD(n)		_RMIXL_OFFSET(0x200+(n)) // Distribution Vector Threshold n Register
#define	RMIXLP_POE_DEST_THRESHOLD		_RMIXL_OFFSET(0x204) // Destination Threshold Register
#define	RMIXLP_POE_DISTR_LOGIC_EN		_RMIXL_OFFSET(0x205) // Distribution Logic Enable Register
#define	RMIXLP_POE_ENQ_SPILL_THOLD		_RMIXL_OFFSET(0x208) // Distribution Vector Enqueue Spill Threshold Register
#define	RMIXLP_POE_DEQ_SPILL_THOLD		_RMIXL_OFFSET(0x209) // Distribution Vector Dequeue Spill Threshold Register
#define	RMIXLP_POE_DEQ_SPILL_TIMER		_RMIXL_OFFSET(0x20A) // Distribution Vector Dequeue Spill Timer Register
#define	RMIXLP_POE_DISTR_CLASS_DROP_EN		_RMIXL_OFFSET(0x20B) // Distribution Vector Class Drop Enable Register
#define	RMIXLP_POE_DISTR_VEC_DROP_EN		_RMIXL_OFFSET(0x20C) // Distribution Vector Drop Enable Register
#define	RMIXLP_POE_DISTR_DROP_TIMER		_RMIXL_OFFSET(0x20D) // Distribution Vector Drop Timer Register

/* Error Registers */
#define	RMIXLP_POE_NERROR_LOG			3
#define	RMIXLP_POE_ERROR_LOG(n)			_RMIXL_OFFSET(0x20E+(n)) // POE Error Log Word n
#define	RMIXLP_POE_NERROR_INJ_CTL		2
#define	RMIXLP_POE_ERROR_INJ_CTL_0		_RMIXL_OFFSET(0x211+(n)) // POE Error Injection Control Word 0
#define	RMIXLP_POE_RESET			_RMIXL_OFFSET(0x213) // POE Reset Register
#define	RMIXLP_POE_TRANSMIT_TIMER		_RMIXL_OFFSET(0x214) // POE Transmit Timer Register

#endif /* _MIPS_RMI_RMIXL_POEREG_H_ */
