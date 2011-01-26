/*	$NetBSD: lcd_ebus.c,v 1.1 2011/01/26 01:18:50 pooka Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was written by Alessandro Forin and Neil Pittman
 * at Microsoft Research and contributed to The NetBSD Foundation
 * by Microsoft Corporation.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: lcd_ebus.c,v 1.1 2011/01/26 01:18:50 pooka Exp $");

#define STUBNAME         lcd
#define STUBSTRING       "lcd"
#define STUBBANNER       "LCD"
#define STUBSTRUCT       _Lcd
#define STUBMATCH(_f_)   (((_f_)->TypeAndTag & LCDBT_TAG) == PMTTAG_LCD)

#define stub_ebus_match         __CONCAT(lcd,_ebus_match)
#define stub_ebus_attach        __CONCAT(lcd,_ebus_attach)
#define stub_ebus               __CONCAT(lcd,_ebus)
#define stub_softc              __CONCAT(lcd,_softc)
#define stubopen                __CONCAT(lcd,open)
#define stubclose               __CONCAT(lcd,close)
#define stub_cdevsw             __CONCAT(lcd,_cdevsw)

#include "stub_ebus.c"
