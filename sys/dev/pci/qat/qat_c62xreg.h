/*	$NetBSD: qat_c62xreg.h,v 1.1 2019/11/20 09:37:46 hikaru Exp $	*/

/*
 * Copyright (c) 2019 Internet Initiative Japan, Inc.
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

/*
 *   Copyright(c) 2014 Intel Corporation.
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DEV_PCI_QAT_C62XREG_H_
#define _DEV_PCI_QAT_C62XREG_H_

/* Max number of accelerators and engines */
#define MAX_ACCEL_C62X			5
#define MAX_AE_C62X			10

/* PCIe BAR index */
#define BAR_SRAM_ID_C62X		0
#define BAR_PMISC_ID_C62X		1
#define BAR_ETR_ID_C62X			2

/* BAR PMISC sub-regions */
#define AE_OFFSET_C62X			0x20000
#define AE_LOCAL_OFFSET_C62X		0x20800
#define CAP_GLOBAL_OFFSET_C62X		0x30000

#define SOFTSTRAP_REG_C62X			0x2EC
#define SOFTSTRAP_SS_POWERGATE_CY_C62X		__BIT(23)
#define SOFTSTRAP_SS_POWERGATE_PKE_C62X		__BIT(24)

#define ACCEL_REG_OFFSET_C62X		16
#define ACCEL_MASK_C62X			0x1F
#define AE_MASK_C62X			0x3FF

#define SMIAPF0_C62X			0x3A028
#define SMIAPF1_C62X			0x3A030
#define SMIA0_MASK_C62X			0xFFFF
#define SMIA1_MASK_C62X			0x1

/* Error detection and correction */
#define AE_CTX_ENABLES_C62X(i)		((i) * 0x1000 + 0x20818)
#define AE_MISC_CONTROL_C62X(i)		((i) * 0x1000 + 0x20960)
#define ENABLE_AE_ECC_ERR_C62X		__BIT(28)
#define ENABLE_AE_ECC_PARITY_CORR_C62X	(__BIT(24) | __BIT(12))
#define ERRSSMSH_EN_C62X		__BIT(3)
/* BIT(2) enables the logging of push/pull data errors. */
#define PPERR_EN_C62X			(__BIT(2))

/* Mask for VF2PF interrupts */
#define VF2PF1_16_C62X			(0xFFFF << 9)
#define ERRSOU3_VF2PF_C62X(errsou3)	(((errsou3) & 0x01FFFE00) >> 9)
#define ERRMSK3_VF2PF_C62X(vf_mask)	(((vf_mask) & 0xFFFF) << 9)

/* Masks for correctable error interrupts. */
#define ERRMSK0_CERR_C62X	(__BIT(24) | __BIT(16) | __BIT(8) | __BIT(0))
#define ERRMSK1_CERR_C62X	(__BIT(24) | __BIT(16) | __BIT(8) | __BIT(0))
#define ERRMSK3_CERR_C62X	(__BIT(7))
#define ERRMSK4_CERR_C62X	(__BIT(8) | __BIT(0))
#define ERRMSK5_CERR_C62X	(0)

/* Masks for uncorrectable error interrupts. */
#define ERRMSK0_UERR_C62X	(__BIT(25) | __BIT(17) | __BIT(9) | __BIT(1))
#define ERRMSK1_UERR_C62X	(__BIT(25) | __BIT(17) | __BIT(9) | __BIT(1))
#define ERRMSK3_UERR_C62X	(__BIT(8) | __BIT(6) | __BIT(5) | __BIT(4) | \
				 __BIT(3) | __BIT(2) | __BIT(0))
#define ERRMSK4_UERR_C62X	(__BIT(9) | __BIT(1))
#define ERRMSK5_UERR_C62X	(__BIT(18) | __BIT(17) | __BIT(16))

/* RI CPP control */
#define RICPPINTCTL_C62X		(0x3A000 + 0x110)
/*
 * BIT(2) enables error detection and reporting on the RI Parity Error.
 * BIT(1) enables error detection and reporting on the RI CPP Pull interface.
 * BIT(0) enables error detection and reporting on the RI CPP Push interface.
 */
#define RICPP_EN_C62X			(__BIT(2) | __BIT(1) | __BIT(0))

/* TI CPP control */
#define TICPPINTCTL_C62X		(0x3A400 + 0x138)
/*
 * BIT(3) enables error detection and reporting on the ETR Parity Error.
 * BIT(2) enables error detection and reporting on the TI Parity Error.
 * BIT(1) enables error detection and reporting on the TI CPP Pull interface.
 * BIT(0) enables error detection and reporting on the TI CPP Push interface.
 */
#define TICPP_EN_C62X		\
	(__BIT(4) | __BIT(3) | __BIT(2) | __BIT(1) | __BIT(0))

/* CFC Uncorrectable Errors */
#define CPP_CFC_ERR_CTRL_C62X	(0x30000 + 0xC00)
/*
 * BIT(1) enables interrupt.
 * BIT(0) enables detecting and logging of push/pull data errors.
 */
#define CPP_CFC_UE_C62X		(__BIT(1) | __BIT(0))

/* Correctable SecureRAM Error Reg */
#define SECRAMCERR_C62X			(0x3AC00 + 0x00)
/* BIT(3) enables fixing and logging of correctable errors. */
#define SECRAM_CERR_C62X		(__BIT(3))

/* Uncorrectable SecureRAM Error Reg */
/*
 * BIT(17) enables interrupt.
 * BIT(3) enables detecting and logging of uncorrectable errors.
 */
#define SECRAM_UERR_C62X		(__BIT(17) | __BIT(3))

/* Miscellaneous Memory Target Errors Register */
/*
 * BIT(3) enables detecting and logging push/pull data errors.
 * BIT(2) enables interrupt.
 */
#define TGT_UERR_C62X			(__BIT(3) | __BIT(2))


#define SLICEPWRDOWN_C62X(i)	((i) * 0x4000 + 0x2C)
/* Enabling PKE4-PKE0. */
#define MMP_PWR_UP_MSK_C62X		\
	(__BIT(20) | __BIT(19) | __BIT(18) | __BIT(17) | __BIT(16))

/* CPM Uncorrectable Errors */
#define INTMASKSSM_C62X(i)		((i) * 0x4000 + 0x0)
/* Disabling interrupts for correctable errors. */
#define INTMASKSSM_UERR_C62X	\
	(__BIT(11) | __BIT(9) | __BIT(7) | __BIT(5) | __BIT(3) | __BIT(1))

/* MMP */
/* BIT(3) enables correction. */
#define CERRSSMMMP_EN_C62X		(__BIT(3))

/* BIT(3) enables logging. */
#define UERRSSMMMP_EN_C62X		(__BIT(3))

/* ETR */
#define ETR_MAX_BANKS_C62X		16
#define ETR_TX_RX_GAP_C62X		8
#define ETR_TX_RINGS_MASK_C62X		0xFF
#define ETR_BUNDLE_SIZE_C62X		0x1000

/* AE firmware */
#define AE_FW_PROD_TYPE_C62X		0x01000000
#define AE_FW_MOF_NAME_C62X	"qat_c62x.bin"
#define AE_FW_MMP_NAME_C62X	"qat_c62x_mmp.bin"
#define AE_FW_UOF_NAME_C62X	"icp_qat_ae.suof"

/* Clock frequency */
#define CLOCK_PER_SEC_C62X		(685 * 1000000 / 16)

#endif
