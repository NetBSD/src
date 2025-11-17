/* $NetBSD: sh_ipl.S,v 1.1 2025/11/17 19:09:58 uwe Exp $ */
/*
 * Copyright (c) 2025 Valery Ushakov
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

#include <sh3/asm.h>
#include <sh3/sh_ipl.h>


/**
 * LINTSTUB: Func: void sh_ipl_write(const char *buf, unsigned int len);
 */
NENTRY(sh_ipl_write)
	bra	.L_sh_ipl_bios_call
	 mov	#SH_IPL_WRITE, r0
	SET_ENTRY_SIZE(sh_ipl_write)

/**
 * LINTSTUB: Func: unsigned int sh_ipl_get_config(void);
 */
NENTRY(sh_ipl_get_config)
	bra	.L_sh_ipl_bios_call
	 mov	#SH_IPL_GET_CONFIG, r0
	SET_ENTRY_SIZE(sh_ipl_get_config)

/**
 * LINTSTUB: Func: int sh_ipl_get_ram_size(void);
 */
NENTRY(sh_ipl_get_ram_size)
	bra	.L_sh_ipl_bios_call
	 mov	#SH_IPL_GET_RAM_SIZE, r0
	SET_ENTRY_SIZE(sh_ipl_write)

/**
 * LINTSTUB: Func: void sh_ipl_putc(char c);
 */
NENTRY(sh_ipl_putc)
	bra	.L_sh_ipl_bios_call
	 mov	#SH_IPL_PUTC, r0
	SET_ENTRY_SIZE(sh_ipl_putc)

/**
 * LINTSTUB: Func: void sh_ipl_puts(const char *s);
 */
NENTRY(sh_ipl_puts)
	bra	.L_sh_ipl_bios_call
	 mov	#SH_IPL_PUTS, r0
	SET_ENTRY_SIZE(sh_ipl_puts)


.L_sh_ipl_bios_call:
	trapa	#SH_IPL_BIOS_TRAP
	rts
	 nop
