/* $NetBSD: sh_ipl.h,v 1.1 2025/11/17 19:09:58 uwe Exp $ */
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

#ifndef _SH3_SH_IPL_H_
#define _SH3_SH_IPL_H_

#define SH_IPL_BIOS_TRAP	0x3f
#define     SH_IPL_WRITE	    0
#define     SH_IPL_GET_CONFIG	    3
#define     SH_IPL_GET_RAM_SIZE	    4
#define     SH_IPL_PUTC		    31
#define     SH_IPL_PUTS		    32

#ifndef _LOCORE

void sh_ipl_write(const char *buf, unsigned int len);
unsigned int sh_ipl_get_config(void);
int sh_ipl_get_ram_size(void);
void sh_ipl_putc(char c);
void sh_ipl_puts(const char *s);

#endif /* !_LOCORE */
#endif /* _SH3_SH_IPL_H_ */
