/*	$NetBSD: viper_reg.h,v 1.3 2007/07/24 14:41:29 pooka Exp $	*/

/*
 * Copyright (c) 2005 Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _EVBARM_VIPER_REG_H
#define _EVBARM_VIPER_REG_H

#define VIPER_SMSC_PBASE	0x08000300

/*
 * Logical mapping for onboard/integrated peripherals
 */
#define	VIPER_IO_AREA_VBASE	0xfd000000
#define VIPER_INTCTL_VBASE 	0xfd000000
#define VIPER_GPIO_VBASE	0xfd100000
#define VIPER_CLKMAN_VBASE 	0xfd200000
#define VIPER_FFUART_VBASE	0xfd300000
#define VIPER_BTUART_VBASE	0xfd400000

#define ioreg_read(a)		(*(volatile uint32_t *)(a))
#define ioreg_write(a,v)	(*(volatile uint32_t *)(a)=(v))

#endif /* _EVBARM_VIPER_REG_H */
