/*	$NetBSD: xpbusvar.h,v 1.1 2022/06/10 21:42:23 tsutsui Exp $	*/

/*-
 * Copyright (c) 2016 Izumi Tsutsui.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _XPBUSVAR_H_
#define _XPBUSVAR_H_

#include <machine/board.h>

#define TRI_PORT_RAM_XP_OFFSET	0x00000

#define XP_SHM_BASE	(TRI_PORT_RAM + TRI_PORT_RAM_XP_OFFSET)
#define XP_SHM_SIZE	0x00010000	/* 64KB for XP; rest 64KB for lance */
#define XP_TAS_ADDR	OBIO_TAS

#define XP_CPU_FREQ	6144000

struct xpbus_attach_args {
	const char *xa_name;
};

/* xpbus sharing control */
#define XP_ACQ_EXCL	(1 << 31)
u_int xp_acquire(int, u_int);
void xp_release(int);
void xp_set_shm_dirty(void);
void xp_ensure_firmware(void);

/* PIO control */
uint8_t put_pio0c(uint8_t, uint8_t);

/* XP reset control */
void xp_cpu_reset_hold(void);
void xp_cpu_reset_release(void);
void xp_cpu_reset(void);

/* XP interrupt control */
void xp_intr1_enable(void);
void xp_intr1_disable(void);
void xp_intr1_acknowledge(void);

void xp_intr5_enable(void);
void xp_intr5_disable(void);
void xp_intr5_acknowledge(void);

/* XP SHM control */
void *xp_shmptr(int);
int  xp_readmem8(int);
int  xp_readmem16le(int);
void xp_writemem8(int, int);
void xp_writemem16le(int, int);

#endif /* !_XPBUSVAR_H_ */
