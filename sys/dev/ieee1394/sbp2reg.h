/*	$NetBSD: sbp2reg.h,v 1.6.12.1 2005/03/19 08:34:33 yamt Exp $	*/

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

/* Max data size in a command orb (add 20 for command section) */

#define SBP2_MAX_ORB		1004
#define SBP2_CMD_ORB_SIZE	32

#define SBP2_MIN_MGMT_OFFSET 0x4000

#define SBP_READ	0x1
#define SBP_WRITE	0x2

#define SBP2_LOGGED_IN	0x1

#define SBP2_MAX_RECONNECT	0xf
#define SBP2_RECONNECT	0x4

#define SBP2_CMDREG_AGENT_STATE			0x00
#define SBP2_CMDREG_AGENT_RESET			0x04
#define SBP2_CMDREG_ORB_POINTER			0x08
#define SBP2_CMDREG_DOORBELL			0x10
#define SBP2_CMDREG_UNSOLICITED_STATUS_ENABLE	0x14

#define SBP2_LOGIN_MAX_RESP	16
#define SBP2_LOGIN_SIZE		32
#define SBP2_STATUS_SIZE	32

#define SBP2_LOGIN_EXCLUSIVE_MASK 0x10000000
#define SBP2_LOGIN_RECONNECT_MASK 0x00f00000

#define SBP2_LOGIN_EXCLUSIVE_SHIFT 28
#define SBP2_LOGIN_RECONNECT_SHIFT 20

#define SBP2_LOGIN_SET_EXCLUSIVE (1 << SBP2_LOGIN_EXCLUSIVE_SHIFT)
#define SBP2_LOGIN_SET_RECONNECT(x) (x << SBP2_LOGIN_RECONNECT_SHIFT)

#define SBP2_LOGINRESP_LENGTH_MASK		0xffff0000
#define SBP2_LOGINRESP_LOGINID_MASK		0x0000ffff
#define SBP2_LOGINRESP_RECONNECT_MASK	0x0000ffff

#define SBP2_LOGINRESP_LENGTH_SHIFT		16

#define SBP2_LOGINRESP_GET_LENGTH(x)	((x & SBP2_LOGINRESP_LENGTH_MASK) >> \
						SBP2_LOGINRESP_LENGTH_SHIFT)
#define SBP2_LOGINRESP_GET_LOGINID(x)	(x & SBP2_LOGINRESP_LOGINID_MASK)
#define SBP2_LOGINRESP_GET_RECONNECT(x)	(x & SBP2_LOGINRESP_RECONNECT_MASK)

#define SBP2_LOGINRESP_CREATE_CMDREG(x,y) (((u_int64_t)x << 32) | y)

#define SBP2_STATUS_REQUEST_COMPLETE	0x0
#define SBP2_STATUS_TRANSPORT_FAIL	0x1
#define SBP2_STATUS_ILLEGAL_REQUEST	0x2
#define SBP2_STATUS_VENDOR_DEPENDENT	0x3

#define SBP2_STATUS_UNSPEC_ERROR	0xff

#define SBP2_STATUS_NOERROR		0x0

#define SBP2_NUM_ALLOC		0x8

#define SBP2_ORB_FMT_SBP2	0x0
#define SBP2_ORB_FMT_RESERVED	0x1
#define SBP2_ORB_FMT_VENDOR	0x2
#define SBP2_ORB_FMT_DUMMY	0x3

#define SBP2_ORB_FMT_SBP2_MASK		0x00000000
#define SBP2_ORB_FMT_RESERVED_MASK	0x20000000
#define SBP2_ORB_FMT_VENDOR_MASK	0x40000000
#define SBP2_ORB_FMT_DUMMY_MASK		0x60000000

#define SBP2_ORB_NULL_POINTER	0x80000000

#define SBP2_ORB_NOTIFY_MASK	0x80000000
#define SBP2_ORB_FMT_MASK	0x60000000
#define SBP2_ORB_RW_MASK	0x08000000
#define SBP2_ORB_SPD_MASK	0x07000000
#define SBP2_ORB_PAYLOAD_MASK   0x00f00000
#define SBP2_ORB_PAGETABLE_MASK	0x00080000
#define SBP2_ORB_PAGESIZE_MASK	0x00070000
#define SBP2_ORB_DATASIZE_MASK	0x0000ffff

#define SBP2_ORB_NOTIFY_SHIFT	31
#define SBP2_ORB_FMT_SHIFT	29
#define SBP2_ORB_RW_SHIFT	27
#define SBP2_ORB_SPD_SHIFT	24
#define SBP2_ORB_PAYLOAD_SHIFT	20
#define SBP2_ORB_PAGETABLE_SHIFT	19
#define SBP2_ORB_PAGESIZE_SHIFT	16
#define SBP2_ORB_DATASIZE_SHIFT	0

#define SBP2_ORB_SET_SPEED(x)   (x << SBP2_ORB_SPD_SHIFT)
#define SBP2_ORB_SET_MAXTRANS(x) ((x + 7) << SBP2_ORB_PAYLOAD_SHIFT)

#define SBP2_ORB_INIT_STATE	0
#define SBP2_ORB_SENT_STATE	1
#define SBP2_ORB_STATUS_STATE	2
#define SBP2_ORB_NEXT_ORB_STATE	3
#define SBP2_ORB_FREE_STATE	0xff

#define SBP2_STATUS_SRC_MASK		0xc0000000
#define SBP2_STATUS_RESP_MASK		0x30000000
#define SBP2_STATUS_DEAD_MASK		0x08000000
#define SBP2_STATUS_LEN_MASK		0x07000000
#define SBP2_STATUS_STATUS_MASK		0x00ff0000
#define SBP2_STATUS_ORB_HIGH_MASK	0x0000ffff

/* Contained within status if needed. */
#define SBP2_STATUS_OBJECT_MASK		0xc0
#define SBP2_STATUS_BUS_ERROR_MASK	0x0f

#define SBP2_STATUS_SRC_SHIFT		30
#define SBP2_STATUS_RESP_SHIFT		28
#define SBP2_STATUS_DEAD_SHIFT		27
#define SBP2_STATUS_LEN_SHIFT		24
#define SBP2_STATUS_STATUS_SHIFT	16
#define SBP2_STATUS_ORB_HIGH_SHIFT	0

#define SBP2_STATUS_OBJECT_SHIFT	6
#define SBP2_STATUS_BUS_ERROR_SHIFT	0

#define SBP2_STATUS_GET_SRC(x) \
	(((x) & SBP2_STATUS_SRC_MASK) >> SBP2_STATUS_SRC_SHIFT)
#define SBP2_STATUS_GET_RESP(x) \
	(((x) & SBP2_STATUS_RESP_MASK) >> SBP2_STATUS_RESP_SHIFT)
#define SBP2_STATUS_GET_DEAD(x) \
	(((x) & SBP2_STATUS_DEAD_MASK) >> SBP2_STATUS_DEAD_SHIFT)
#define SBP2_STATUS_GET_LEN(x) \
	(((x) & SBP2_STATUS_LEN_MASK) >> SBP2_STATUS_LEN_SHIFT)
#define SBP2_STATUS_GET_STATUS(x) \
	(((x) & SBP2_STATUS_STATUS_MASK) >> SBP2_STATUS_STATUS_SHIFT)
#define SBP2_STATUS_GET_ORBHIGH(x) \
	(((x) & SBP2_STATUS_ORB_HIGH_MASK))
#define SBP2_STATUS_GET_OBJECT(x) \
	(((x) & SBP2_STATUS_OBJECT_MASK) >> SBP2_STATUS_OBJECT_SHIFT)
#define SBP2_STATUS_GET_BUS_ERROR(x) \
	(((x) & SBP2_STATUS_BUS_ERROR_MASK))

#define SBP2_STATE_RESET	0x0
#define SBP2_STATE_ACTIVE	0x1
#define SBP2_STATE_SUSPENDED	0x2
#define SBP2_STATE_DEAD		0x3

#define SBP2_MAXPHYS		0xffff
#define SBP2_PHYS_SEGMENT	(0x10000 - 512)

#define SBP2_PTENT_SIZE		8
#define SBP2_PTENT_SIZEQ	2

#define SBP2_PT_MAKELEN(x)	(htonl(x << 16))

struct sbp2_pte_u {
	uint32_t seg_hi_length;
	uint32_t seg_lo;
};

struct sbp2_pte_n {
	uint32_t seg_hi_length;
	uint32_t seg_lo_offset;
};

#endif	/* _DEV_STD_SBP2REG_H_ */
