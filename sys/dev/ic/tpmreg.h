/*	$NetBSD: tpmreg.h,v 1.11 2022/01/29 12:27:30 riastradh Exp $	*/

/*
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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

#ifndef	DEV_IC_TPMREG_H
#define	DEV_IC_TPMREG_H

#include <sys/types.h>

#include <sys/cdefs.h>
#include <sys/endian.h>

struct tpm_header {
	uint16_t tag;
	uint32_t length;
	uint32_t code;
} __packed;

/* -------------------------------------------------------------------------- */

/*
 * TPM Interface Specification 1.2 (TIS12).
 */

#define	TPM_ACCESS			0x0000	/* 8bit register */
#define		TPM_ACCESS_VALID		__BIT(7)
#define		TPM_ACCESS_ACTIVE_LOCALITY	__BIT(5)
#define		TPM_ACCESS_BEEN_SEIZED		__BIT(4)
#define		TPM_ACCESS_SEIZE		__BIT(3)
#define		TPM_ACCESS_PENDING_REQUEST	__BIT(2)
#define		TPM_ACCESS_REQUEST_USE		__BIT(1)
#define		TPM_ACCESS_ESTABLISHMENT	__BIT(0)

#define	TPM_INT_ENABLE			0x0008	/* 32bit register */
#define		TPM_GLOBAL_INT_ENABLE		__BIT(31)
#define		TPM_CMD_READY_INT		__BIT(7)
#define		TPM_TYPE_POLARITY		__BITS(4,3)
#define		TPM_INT_LEVEL_HIGH		__SHIFTIN(0, TPM_TYPE_POLARITY)
#define		TPM_INT_LEVEL_LOW		__SHIFTIN(1, TPM_TYPE_POLARITY)
#define		TPM_INT_EDGE_RISING		__SHIFTIN(2, TPM_TYPE_POLARITY)
#define		TPM_INT_EDGE_FALLING		__SHIFTIN(3, TPM_TYPE_POLARITY)
#define		TPM_LOCALITY_CHANGE_INT		__BIT(2)
#define		TPM_STS_VALID_INT		__BIT(1)
#define		TPM_DATA_AVAIL_INT		__BIT(0)

#define	TPM_INT_VECTOR			0x000c	/* 8bit register */
#define	TPM_INT_STATUS			0x0010	/* 32bit register */

#define	TPM_INTF_CAPABILITY		0x0014	/* 32bit register */
#define		TPM_INTF_BURST_COUNT_STATIC	__BIT(8)
#define		TPM_INTF_CMD_READY_INT		__BIT(7)
#define		TPM_INTF_INT_EDGE_FALLING	__BIT(6)
#define		TPM_INTF_INT_EDGE_RISING	__BIT(5)
#define		TPM_INTF_INT_LEVEL_LOW		__BIT(4)
#define		TPM_INTF_INT_LEVEL_HIGH		__BIT(3)
#define		TPM_INTF_LOCALITY_CHANGE_INT	__BIT(2)
#define		TPM_INTF_STS_VALID_INT		__BIT(1)
#define		TPM_INTF_DATA_AVAIL_INT		__BIT(0)
#define	TPM_INTF_CAPABILITY_BITS \
    "\020\01IDRDY\02ISTSV\03ILOCH\04IHIGH\05ILOW\06IRISE\07IFALL\010IRDY\011BCST"

#define	TPM_STS				0x0018	/* 24bit register */
#define		TPM_STS_BURST_COUNT		__BITS(23,8)
#define		TPM_STS_STATUS_BITS		__BITS(7,0)
#define		TPM_STS_VALID			__BIT(7)
#define		TPM_STS_CMD_READY		__BIT(6)
#define		TPM_STS_GO			__BIT(5)
#define		TPM_STS_DATA_AVAIL		__BIT(4)
#define		TPM_STS_DATA_EXPECT		__BIT(3)
#define		TPM_STS_SELFTEST_DONE		__BIT(2)
#define		TPM_STS_RESP_RETRY		__BIT(1)

#define	TPM_DATA			0x0024	/* 32bit register */
#define	TPM_ID				0x0f00	/* 32bit register */
#define	TPM_REV				0x0f04	/* 8bit register */

/*
 * Five localities, 4K per locality.  But we only use the registers for
 * the first locality, so this is 0x1000 rather than 0x5000.
 */
#define	TPM_SPACE_SIZE	0x1000

#define	TPM_TAG_RQU_COMMAND		0x00c1
#define	TPM_TAG_RSP_COMMAND		0x00c4

#define	TPM_ORD_GetRandom		0x00000046

/* TPM_RESULT return codes */
#define	TPM_AUTHFAIL			1
#define	TPM_BADINDEX			2
#define	TPM_BAD_PARAMETER		3
#define	TPM_AUDITFAILURE		4
#define	TPM_CLEAR_DISABLED		5
#define	TPM_DEACTIVATED			6
#define	TPM_DISABLED			7
#define	TPM_DISABLED_CMD		8
#define	TPM_FAIL			9
#define	TPM_BAD_ORDINAL			10
/* ... */

#define	TPM_NON_FATAL			0x800

/* -------------------------------------------------------------------------- */

/*
 * Trusted Platform Module Library Specification, Family "2.0",
 * Level 00, Revision 01.59 -- November 2019
 *
 * https://trustedcomputinggroup.org/resource/tpm-library-specification/
 *
 * Where this spec names things TPM_* that don't obviously coincide
 * with the 1.2 things, we name them TPM2_*.
 */

/* https://trustedcomputinggroup.org/wp-content/uploads/TPM-Rev-2.0-Part-4-Supporting-Routines-01.38-code.pdf#page=172 */
#define	TPM2_ST_RSP_COMMAND		0x00c4
#define	TPM2_ST_NULL			0x8000
#define	TPM2_ST_NO_SESSIONS		0x8001
#define	TPM2_ST_SESSIONS		0x8002
/* ... */

/* https://trustedcomputinggroup.org/wp-content/uploads/TPM-Rev-2.0-Part-2-Structures-01.38.pdf#page=45 */
#define	TPM2_CC_GetRandom		0x0000017b

/* https://trustedcomputinggroup.org/wp-content/uploads/TPM-Rev-2.0-Part-2-Structures-01.38.pdf#page=53 */
#define	TPM2_RC_SUCCESS			0x000
#define	TPM2_RC_BAD_TAG			0x01e

#define	TPM2_RC_VER1			0x100

#define	TPM2_RC_FMT1			0x080

#define	TPM2_RC_WARN			0x900
#define	TPM2_RC_TESTING			(TPM2_RC_WARN + 0x00a)
#define	TPM2_RC_RETRY			(TPM2_RC_WARN + 0x022)

#endif	/* DEV_IC_TPMREG_H */
