/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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

#ifndef _POWERPC_RTAS_H_
#define _POWERPC_RTAS_H_

#define RTAS_MAXARGS 16

enum rtas_func {
        RTAS_FUNC_RESTART_RTAS,
	RTAS_FUNC_NVRAM_FETCH,
	RTAS_FUNC_NVRAM_STORE,
	RTAS_FUNC_GET_TIME_OF_DAY,
	RTAS_FUNC_SET_TIME_OF_DAY,
	RTAS_FUNC_SET_TIME_FOR_POWER_ON,
	RTAS_FUNC_EVENT_SCAN,
	RTAS_FUNC_CHECK_EXCEPTION,
	RTAS_FUNC_READ_PCI_CONFIG,
	RTAS_FUNC_WRITE_PCI_CONFIG,
	RTAS_FUNC_DISPLAY_CHARACTER,
	RTAS_FUNC_SET_INDICATOR,
	RTAS_FUNC_POWER_OFF,
	RTAS_FUNC_SUSPEND,
	RTAS_FUNC_HIBERNATE,
	RTAS_FUNC_SYSTEM_REBOOT,
	RTAS_FUNC_FREEZE_TIME_BASE,
	RTAS_FUNC_THAW_TIME_BASE,
	RTAS_FUNC_number
};

int rtas_call(int, int, int, ...);
int rtas_has_func(int);

#endif /* _POWERPC_RTAS_H_ */
