/* $NetBSD: htif_var.h,v 1.1.18.2 2017/12/03 11:36:39 jdolecek Exp $ */
/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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

#ifndef _RISCV_HTIF_HTIF_VAR_H_
#define _RISCV_HTIF_HTIF_VAR_H_

#define	HTIF_CMD_READ_MEM		0
#define	HTIF_CMD_WRITE_MEM		1
#define	HTIF_CMD_READ_CONTROL_REG	2
#define	HTIF_CMD_WRITE_CONTROL_REG	3
#define	HTIF_CMD_ACK			4
#define	HTIF_CMD_NACK			5

struct htif_packet_header {
	uint64_t hphp_hdr;
#define	HTIF_PHDR_CMD			__BITS(3,0)
#define	HTIF_PHDR_DATA_DWORDS		__BITS(15,4)
#define	HTIF_PHDR_SEQNO			__BITS(23,16)
#define	HTIF_PHDR_ADDR			__BITS(63,24)
};

struct htif_attach_args {
	const char *haa_name;
};

#endif /* _RISCV_HTIF_HTIF_VAR_H_ */
