/*	$NetBSD: aicavar.h,v 1.1 2003/08/24 17:33:29 marcus Exp $	*/

/*
 * Copyright (c) 2003 SHIMIZU Ryo <ryo@misakimix.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _AICAVAR_H_
#define _AICAVAR_H_

typedef struct {
	u_int32_t serial;
	u_int32_t command;
	u_int32_t blocksize;
	u_int32_t channel;
	u_int32_t rate;
	u_int32_t precision;
	u_int32_t l_param;	/* volume,etc... for left */
	u_int32_t r_param;	/* volume,etc... for right */
} aica_cmd_t;

#define	AICA_COMMAND_NOP	0
#define	AICA_COMMAND_PLAY	1
#define	AICA_COMMAND_STOP	2
#define	AICA_COMMAND_INIT	3
#define	AICA_COMMAND_MVOL	4
#define	AICA_COMMAND_VOL	5

#define	AICA_ARM_CODE		0x00000000	/* text+data+bss+stack
						   0x00000000-0x0000ff00 */
#define	AICA_ARM_CMD		0x0000ff00	/* SH4<->ARM work for
						   communication */
#define	AICA_ARM_CMDADDR(x)	(AICA_ARM_CMD + offsetof(aica_cmd_t, x))
#define	AICA_ARM_CMD_SERIAL	AICA_ARM_CMDADDR(serial)
#define	AICA_ARM_CMD_COMMAND	AICA_ARM_CMDADDR(command)
#define	AICA_ARM_CMD_BLOCKSIZE	AICA_ARM_CMDADDR(blocksize)
#define	AICA_ARM_CMD_CHANNEL	AICA_ARM_CMDADDR(channel)
#define	AICA_ARM_CMD_RATE	AICA_ARM_CMDADDR(rate)
#define	AICA_ARM_CMD_PRECISION	AICA_ARM_CMDADDR(precision)
#define	AICA_ARM_CMD_LPARAM	AICA_ARM_CMDADDR(l_param)
#define	AICA_ARM_CMD_RPARAM	AICA_ARM_CMDADDR(r_param)

#define	AICA_ARM_END		0x00010000

#define	AICA_DMABUF_START	0x00010000
#define	AICA_DMABUF_LEFT	0x00010000	/* DMA buffer for PLAY
						   0x00010000-0x0001FFFF */
#define	AICA_DMABUF_RIGHT	0x00020000	/* DMA buffer for PLAY
						   0x00020000-0x0002FFFF */
#define	AICA_DMABUF_MONO	AICA_DMABUF_LEFT
#define	AICA_DMABUF_END		0x00030000

#define	AICA_DMABUF_SIZE	0x0000ffc0

#define	AICA_MEMORY_END		0x00200000


#define L256TO16(l)	(((l) >> 4) & 0x0f)
#define L16TO256(l)	((((l) << 4) & 0xf0) + ((l) & 0x0f))


enum MIXER_CLASS {
	AICA_MASTER_VOL = 0,
	AICA_OUTPUT_GAIN,
	AICA_OUTPUT_CLASS,

	AICA_NDEVS
};

#endif /* _AICAVAR_H_ */

