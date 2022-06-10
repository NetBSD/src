/*	$NetBSD: xpcmd.c,v 1.1 2022/06/10 21:42:23 tsutsui Exp $	*/

/*
 * Copyright (c) 2018 Yosuke Sugahara. All rights reserved.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>

#if defined(_KERNEL)

#include <sys/param.h>

#include <luna68k/dev/xpbusvar.h>
#include <luna68k/dev/xpcmd.h>
#include <luna68k/dev/xplx/xplxdefs.h>
#else

#include <stdint.h>
#include "xpbusvar.h"
#include "xpcmd.h"
#include "xplx/xplxdefs.h"

#endif

enum {
	READY = 0,
	CMD = 1,
	RESULT = 2,
};

static int
xp_waitfor_ready(int xplx_devid, int timeout)
{
	uint16_t addr = XPLX_VAR_BASE + xplx_devid * 16;

	do {
		int r;
		r = xp_readmem8(addr + READY);
		if (r)
			return 1;
		delay(100);
	} while (timeout--);
	return 0;
}

int
xp_cmd_nowait(int xplx_devid, uint8_t cmd)
{
	uint8_t r;

	uint16_t addr = XPLX_VAR_BASE + xplx_devid * 16;
	r = xp_waitfor_ready(xplx_devid, 1000);
	if (r == 0)
		return 0;
	xp_writemem8(addr + RESULT, 0);
	xp_writemem8(addr + CMD, cmd);
	return XPLX_R_OK;
}

int
xp_cmd(int xplx_devid, uint8_t cmd)
{
	uint8_t rv;
	uint8_t r;
	uint16_t addr = XPLX_VAR_BASE + xplx_devid * 16;

	r = xp_waitfor_ready(xplx_devid, 1000);
	if (r == 0)
		return 0;
	xp_writemem8(addr + RESULT, 0);
	xp_writemem8(addr + CMD, cmd);
	while ((rv = xp_readmem8(addr + RESULT)) == 0)
		continue;
	return rv;
}
