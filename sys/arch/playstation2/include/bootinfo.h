/*	$NetBSD: bootinfo.h,v 1.5.6.2 2014/08/20 00:03:18 tls Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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

#define PS2_MEMORY_SIZE		(32 * 1024 * 1024)

#define BOOTINFO_BLOCK_SIZE	0x1000
#define BOOTINFO_BLOCK_BASE	(PS2_MEMORY_SIZE - BOOTINFO_BLOCK_SIZE)

#define BOOTINFO_DEVCONF	0x00
#define BOOTINFO_DEVCONF_SPD_PRESENT	0x100

#define BOOTINFO_OPTION_PTR	0x04
#define BOOTINFO_RTC		0x10
#define BOOTINFO_PCMCIA_TYPE	0x1c
#define BOOTINFO_SYSCONF	0x20

#define BOOTINFO_REF(x)							\
    (*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(BOOTINFO_BLOCK_BASE + (x)))

struct bootinfo_rtc {
	u_int8_t __reserved1;
	u_int8_t sec;
	u_int8_t min;
	u_int8_t hour;
	u_int8_t __reserved2;
	u_int8_t day;
	u_int8_t mon;
	u_int8_t year;
} __attribute__((__packed__));
