/*	$NetBSD: twereg.h,v 1.2 2000/10/26 14:43:50 ad Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

/*-
 * Copyright (c) 2000 Michael Smith
 * Copyright (c) 2000 BSDi
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from FreeBSD: twereg.h,v 1.1 2000/05/24 23:35:23 msmith Exp
 */

#ifndef _PCI_TWEREG_H_
#define	_PCI_TWEREG_H_

/* Board registers. */
#define	TWE_REG_CTL			0x00
#define	TWE_REG_STS			0x04
#define	TWE_REG_CMD_QUEUE		0x08
#define	TWE_REG_RESP_QUEUE		0x0c

/* Control register bit definitions. */
#define	TWE_CTL_CLEAR_HOST_INTR		0x00080000
#define	TWE_CTL_CLEAR_ATTN_INTR		0x00040000
#define	TWE_CTL_MASK_CMD_INTR		0x00020000
#define	TWE_CTL_MASK_RESP_INTR		0x00010000
#define	TWE_CTL_UNMASK_CMD_INTR		0x00008000
#define	TWE_CTL_UNMASK_RESP_INTR	0x00004000
#define	TWE_CTL_CLEAR_ERROR_STS		0x00000200
#define	TWE_CTL_ISSUE_SOFT_RESET	0x00000100
#define	TWE_CTL_ENABLE_INTRS		0x00000080
#define	TWE_CTL_DISABLE_INTRS		0x00000040
#define	TWE_CTL_ISSUE_HOST_INTR		0x00000020

/* Status register bit definitions. */
#define	TWE_STS_MAJOR_VERSION_MASK	0xf0000000
#define	TWE_STS_MINOR_VERSION_MASK	0x0f000000
#define	TWE_STS_PCI_PARITY_ERROR	0x00800000
#define	TWE_STS_QUEUE_ERROR		0x00400000
#define	TWE_STS_MICROCONTROLLER_ERROR	0x00200000
#define	TWE_STS_PCI_ABORT		0x00100000
#define	TWE_STS_HOST_INTR		0x00080000
#define	TWE_STS_ATTN_INTR		0x00040000
#define	TWE_STS_CMD_INTR		0x00020000
#define	TWE_STS_RESP_INTR		0x00010000
#define	TWE_STS_CMD_QUEUE_FULL		0x00008000
#define	TWE_STS_RESP_QUEUE_EMPTY	0x00004000
#define	TWE_STS_MICROCONTROLLER_READY	0x00002000
#define	TWE_STS_CMD_QUEUE_EMPTY		0x00001000

#define	TWE_STS_ALL_INTRS		0x000f0000
#define	TWE_STS_CLEARABLE_BITS		0x00d00000
#define	TWE_STS_EXPECTED_BITS		0x00002000
#define	TWE_STS_UNEXPECTED_BITS		0x00f80000

/* Command packet opcodes. */
#define	TWE_OP_NOP			0x00
#define	TWE_OP_INIT_CONNECTION		0x01
#define	TWE_OP_READ			0x02
#define	TWE_OP_WRITE			0x03
#define	TWE_OP_VERIFY			0x04
#define	TWE_OP_GET_PARAM		0x12
#define	TWE_OP_SET_PARAM		0x13
#define	TWE_OP_SECTOR_INFO		0x1a
#define	TWE_OP_AEN_LISTEN		0x1c

/* Asynchronous event notification (AEN) codes. */
#define	TWE_AEN_QUEUE_EMPTY		0x0000
#define	TWE_AEN_SOFT_RESET		0x0001
#define	TWE_AEN_DEGRADED_MIRROR		0x0002
#define	TWE_AEN_CONTROLLER_ERROR	0x0003
#define	TWE_AEN_REBUILD_FAIL		0x0004
#define	TWE_AEN_REBUILD_DONE		0x0005
#define	TWE_AEN_TABLE_UNDEFINED		0x0015
#define	TWE_AEN_QUEUE_FULL		0x00ff

/* Response queue entries.  Masking and shifting yields request ID. */
#define	TWE_RESP_MASK			0x00000ff0
#define	TWE_RESP_SHIFT			4

/* Miscellenous constants. */
#define	TWE_ALIGNMENT			512
#define	TWE_MAX_UNITS			16
#define	TWE_INIT_MESSAGE_CREDITS	0x100
#define	TWE_INIT_CMD_PACKET_SIZE	0x3
#define	TWE_SG_SIZE			62
#define	TWE_MAX_CMDS			255
#define	TWE_Q_START			0
#define	TWE_UNIT_INFORMATION_TABLE_BASE	0x300
#define	TWE_IOCTL			0x80
#define	TWE_MAX_AEN_TRIES		100
#define	TWE_SECTOR_SIZE			512

/* Maximum transfer size.  XXX This is an arbitrarily chosen value. */ 
#define	TWE_MAX_XFER			1048576

/* Scatter/gather block. */
struct twe_sgb {
	u_int32_t	tsg_address;
	u_int32_t	tsg_length;
} __attribute__ ((packed));

/*
 * Command block.  This is 512 (really 508) bytes in size, and must be
 * aligned on a 512 byte boundary.
 */
struct twe_cmd {
	u_int8_t	tc_opcode;	/* high 3 bits is S/G list offset */
	u_int8_t	tc_size;
	u_int8_t	tc_cmdid;
	u_int8_t	tc_unit;	/* high nybble is host ID */
	u_int8_t	tc_status;
	u_int8_t	tc_flags;
	u_int16_t	tc_count;	/* block & param count, msg credits */
	union {
		struct {
			u_int32_t	lba;
			struct	twe_sgb sgl[TWE_SG_SIZE];
		} io __attribute__ ((packed));
		struct {
			struct	twe_sgb sgl[TWE_SG_SIZE];
		} param  __attribute__ ((packed));
		struct {
			u_int32_t	response_queue_pointer;
		} init_connection  __attribute__ ((packed));
	} tc_args __attribute__ ((packed));
	int32_t		tc_pad;
} __attribute__ ((packed));

/* Get/set parameter block. */
struct twe_param {
	u_int16_t	tp_table_id;
	u_int8_t	tp_param_id;
	u_int8_t	tp_param_size;
	u_int8_t	tp_data[1];
} __attribute__ ((packed));

#endif	/* !_PCI_TWEREG_H_ */
