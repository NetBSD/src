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

#ifndef _DEV_IEEE1394_IEEE1394REG_H_
#define _DEV_IEEE1394_IEEE1394REG_H_

/* Defined IEEE 1394 speeds.
 */
#define	IEEE1394_SPD_S100	0	/* 1394-1995 */
#define	IEEE1394_SPD_S200	1	/* 1394-1995 */
#define	IEEE1394_SPD_S400	2	/* 1394-1995 */
#define	IEEE1394_SPD_S800	3	/* 1394a */
#define	IEEE1394_SPD_S1600	4	/* 1394a */
#define	IEEE1394_SPD_S3200	5	/* 1394a */
#define	IEEE1394_SPD_MAX	6

#define	IEEE1394_SPD_STRINGS	"100Mb/s", "200Mb/s", "400Mb/s", "800Mb/s", \
		"1.6Gb/s", "3.2Gb/s"

#define	IEEE1394_BCAST_PHY_ID	0x3f

/*
 * Transaction code
 */
#define	IEEE1394_TCODE_WRITE_REQ_QUAD	0x0
#define	IEEE1394_TCODE_WRITE_REQ_BLOCK	0x1
#define	IEEE1394_TCODE_WRITE_RESP	0x2
#define	IEEE1394_TCODE_READ_REQ_QUAD	0x4
#define	IEEE1394_TCODE_READ_REQ_BLOCK	0x5
#define	IEEE1394_TCODE_READ_RESP_QUAD	0x6
#define	IEEE1394_TCODE_READ_RESP_BLOCK	0x7
#define	IEEE1394_TCODE_CYCLE_START	0x8
#define	IEEE1394_TCODE_LOCK_REQ		0x9
#define	IEEE1394_TCODE_STREAM_DATA	0xa
#define	IEEE1394_TCODE_LOCK_RESP	0xb

/*
 * Response code
 */
#define	IEEE1394_RCODE_COMPLETE		0x0
#define	IEEE1394_RCODE_CONFLICT_ERROR	0x4
#define	IEEE1394_RCODE_DATA_ERROR	0x5
#define	IEEE1394_RCODE_TYPE_ERROR	0x6
#define	IEEE1394_RCODE_ADDRESS_ERROR	0x7

/*
 * Tag value
 */
#define	IEEE1394_TAG_GASP		0x3

/*
 * Control and Status Registers (IEEE1212 & IEEE1394)
 */
#define	CSR_BASE_HI			0x0000ffff
#define	CSR_BASE_LO			0xf0000000

#define	CSR_STATE_CLEAR			0x0000
#define	CSR_STATE_SET			0x0004
#define	CSR_NODE_IDS			0x0008
#define	CSR_RESET_START			0x000c
#define	CSR_INDIRECT_ADDRESS		0x0010
#define	CSR_INDIRECT_DATA		0x0014
#define	CSR_SPLIT_TIMEOUT_HI		0x0018
#define	CSR_SPLIT_TIMEOUT_LO		0x001c
#define	CSR_ARGUMENT_HI			0x0020
#define	CSR_ARGUMENT_LO			0x0024
#define	CSR_TEST_START			0x0028
#define	CSR_TEST_STATUS			0x002c
#define	CSR_INTERRUPT_TARGET		0x0050
#define	CSR_INTERRUPT_MASK		0x0054
#define	CSR_CLOCK_VALUE			0x0058
#define	CSR_CLOCK_PERIOD		0x005c
#define	CSR_CLOCK_STROBE_ARRIVED	0x0060
#define	CSR_CLOCK_INFO			0x0064
#define	CSR_MESSAGE_REQUEST		0x0080
#define	CSR_MESSAGE_RESPONSE		0x00c0

#define	CSR_SB_CYCLE_TIME		0x0200
#define	CSR_SB_BUS_TIME			0x0204
#define	CSR_SB_POWER_FAIL_IMMINENT	0x0208
#define	CSR_SB_POWER_SOURCE		0x020c
#define	CSR_SB_BUSY_TIMEOUT		0x0210
#define	CSR_SB_PRIORITY_BUDGET_HI	0x0214
#define	CSR_SB_PRIORITY_BUDGET_LO	0x0218
#define	CSR_SB_BUS_MANAGER_ID		0x021c
#define	CSR_SB_BANDWIDTH_AVAILABLE	0x0220
#define	CSR_SB_CHANNEL_AVAILABLE_HI	0x0224
#define	CSR_SB_CHANNEL_AVAILABLE_LO	0x0228
#define	CSR_SB_MAINT_CONTROL		0x022c
#define	CSR_SB_MAINT_UTILITY		0x0230
#define	CSR_SB_BROADCAST_CHANNEL	0x0234

#define	CSR_CONFIG_ROM			0x0400

#define	CSR_SB_OUTPUT_MASTER_PLUG	0x0900
#define	CSR_SB_OUTPUT_PLUG		0x0904
#define	CSR_SB_INPUT_MASTER_PLUG	0x0980
#define	CSR_SB_INPUT_PLUG		0x0984
#define	CSR_SB_FCP_COMMAND_FRAME	0x0b00
#define	CSR_SB_FCP_RESPONSE_FRAME	0x0d00
#define	CSR_SB_TOPOLOGY_MAP		0x1000
#define	CSR_SB_END			0x1400

#endif	/* _DEV_IEEE1394_IEEE1394REG_H_ */
