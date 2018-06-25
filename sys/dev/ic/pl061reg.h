/* $NetBSD: pl061reg.h,v 1.1.2.2 2018/06/25 07:25:50 pgoyette Exp $ */

/*
 * Copyright (c) 2018 Jonathan A. Kollasch
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _IC_PL061REG_H
#define _IC_PL061REG_H

#define PL061_GPIODATA_REG(m)	(0x000 + (m) * 4)
#define PL061_GPIODIR_REG	0x400
#define PL061_GPIOIS_REG	0x404
#define PL061_GPIOIBE_REG	0x408
#define PL061_GPIOIEV_REG	0x40c
#define PL061_GPIOIE_REG	0x410
#define PL061_GPIORIS_REG	0x414
#define PL061_GPIOMIS_REG	0x418
#define PL061_GPIOIC_REG	0x41c
#define PL061_GPIOAFSEL_REG	0x420
#define PL061_GPIOPeriphID(n)	(0xfe0 + (n) * 4)
#define PL061_GPIOCellID(n)	(0xff0 + (n) * 4)

#endif /* _IC_PL061REG_H */
