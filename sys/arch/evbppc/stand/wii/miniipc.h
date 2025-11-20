/* $NetBSD: miniipc.h,v 1.1.2.2 2025/11/20 19:14:48 martin Exp $ */

/*-
 * Copyright (c) 2025 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _WII_MINIIPC_H
#define _WII_MINIIPC_H

#include <lib/libsa/stand.h>

#define IPC_NUM_ARGS	6

typedef struct {
	union {
		struct {
			uint8_t		flags;
			uint8_t		device;
			uint16_t	req;
		};
		uint32_t		code;
	};
	uint32_t			tag;
	uint32_t			args[IPC_NUM_ARGS];
} ipc_request_t;

typedef struct {
	char				magic[3];
	char				version;
	void				*mem2_boundary;
	volatile ipc_request_t		*ipc_in;
	uint32_t			ipc_in_size;
	volatile ipc_request_t		*ipc_out;
	uint32_t			ipc_out_size;
} ipc_info_header_t;

bool	miniipc_probe(void);
int	miniipc_ping(void);
int	miniipc_sdmmc_ack(uint32_t *);
int	miniipc_sdmmc_state(uint32_t *);
int	miniipc_sdmmc_size(uint32_t *);
int	miniipc_sdmmc_read(uint32_t, uint32_t, void *);
int	miniipc_sdmmc_write(uint32_t, uint32_t, const void *);

#endif /* !_WII_MINIIPC_H */
