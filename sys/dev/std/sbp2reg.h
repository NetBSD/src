/*	$NetBSD: sbp2reg.h,v 1.3 2001/04/23 00:57:32 jmc Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by 
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _DEV_STD_SBP2REG_H_
#define _DEV_STD_SBP2REG_H_

#define	SBP2_UNIT_SPEC_ID	0x00609E	/* NCITS OUI */
#define	SBP2_UNIT_SW_VERSION	0x010483

/*      0x38/8, oui/24
 */
#define SBP2_KEYVALUE_Command_Set_Spec_Id       0x38    /* immediate (OUI) */

/*	0x39/8, oui/24						Unit Directory
 */
#define	SBP2_KEYVALUE_Command_Set		0x39	/* immediate (OUI) */

/*	0x54/8, offset/24					Unit Directory
 */
#define	SBP2_KEYVALUE_Management_Agent \
		P1212_KEYVALUE_Unit_Dependent_Info	/* offset */

/*	0x3A/8, reserved/8, msg_ORB_timeout/8, ORB_size/8	Unit Directory
 */
#define	SBP2_KEYVALUE_Unit_Characteristics	0x3A	/* immediate */

/*	0x3B/8, reserver/8, max_reconnect_hold/16		Unit Directory
 */
#define	SBP2_KEYVALUE_Command_Set_Revision	0x3B	/* immediate */

/*	0x3C/8, firmware_revision/24				Unit Directory
 */
#define	SBP2_KEYVALUE_Firmware_Revision		0x3C	/* immediate */

/*	0x3D/8, reserver/8, max_reconnect_hold/16		Unit Directory
 */
#define	SBP2_KEYVALUE_Reconnect_Timeout		0x3D	/* immediate */

/*	0xD4/8, indirect_offset/24				Unit Directory
 */
#define	SBP2_KEYVALUE_Logical_Unit_Directory \
		P1212_KEYVALUE_Unit_Dependent_Info	/* directory */

/*	0x14/8, reserved/1, ordered/1, reserved/1		Unit Directory
 *	device_type/5, lun/16
 */
#define	SBP2_KEYVALUE_Logical_Unit_Number \
		P1212_KEYVALUE_Unit_Dependent_Info	/* immediate */

/*	0x8D/8, indirect_offset/24				Unit Directory
 */
#define	SBP2_KEYVALUE_Unit_Unique_Id \
		P1212_KEYVALUE_Node_Unique_Id		/* leaf */

#define	SBP2_ORB_LOGIN		0
#define	SBP2_ORB_QUERY_LOGINS	1
#define	SBP2_ORB_RECONNECT	3
#define	SBP2_ORB_LOGOUT		7

struct sbp2_pte_u {
	uint32_t seg_hi_length;
	uint32_t seg_lo;
};

struct sbp2_pte_n {
	uint32_t seg_hi_length;
	uint32_t seg_lo_offset;
};

#endif	/* _DEV_STD_SBP2REG_H_ */
