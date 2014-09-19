/* $NetBSD: setjmp.h,v 1.1 2014/09/19 17:36:26 matt Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

			/* magic + 16 reg + 1 fcsr + 16 fp + 4 sigmask */
#define _JBLEN		(_JB_SIGMASK + 4)
#define _JB_MAGIC	0
#define	_JB_RA		1
#define _JB_S0		2
#define _JB_S1		3
#define _JB_S2		4
#define _JB_S3		5
#define _JB_S4		6
#define _JB_S5		7
#define _JB_S6		8
#define _JB_S7		9
#define _JB_S8		10
#define _JB_S9		11
#define _JB_S10		12
#define _JB_S11		13
#define _JB_SP		14
#define _JB_TP		15
#define _JB_GP		16
#define _JB_FCSR	17

#define	_JB_F0		18
#define	_JB_F1		(_JB_F0 + sizeof(double) / sizeof(_BSD_JBSLOT_T_))
#define	_JB_F2		(_JB_F1 + sizeof(double) / sizeof(_BSD_JBSLOT_T_))
#define	_JB_F3		(_JB_F2 + sizeof(double) / sizeof(_BSD_JBSLOT_T_))
#define	_JB_F4		(_JB_F3 + sizeof(double) / sizeof(_BSD_JBSLOT_T_))
#define	_JB_F5		(_JB_F4 + sizeof(double) / sizeof(_BSD_JBSLOT_T_))
#define	_JB_F6		(_JB_F5 + sizeof(double) / sizeof(_BSD_JBSLOT_T_))
#define	_JB_F7		(_JB_F6 + sizeof(double) / sizeof(_BSD_JBSLOT_T_))
#define	_JB_F8		(_JB_F7 + sizeof(double) / sizeof(_BSD_JBSLOT_T_))
#define	_JB_F9		(_JB_F8 + sizeof(double) / sizeof(_BSD_JBSLOT_T_))
#define	_JB_F10		(_JB_F9 + sizeof(double) / sizeof(_BSD_JBSLOT_T_))
#define	_JB_F11		(_JB_F10 + sizeof(double) / sizeof(_BSD_JBSLOT_T_))
#define	_JB_F12		(_JB_F11 + sizeof(double) / sizeof(_BSD_JBSLOT_T_))
#define	_JB_F13		(_JB_F12 + sizeof(double) / sizeof(_BSD_JBSLOT_T_))
#define	_JB_F14		(_JB_F13 + sizeof(double) / sizeof(_BSD_JBSLOT_T_))
#define	_JB_F15		(_JB_F14 + sizeof(double) / sizeof(_BSD_JBSLOT_T_))

#define _JB_SIGMASK	(_JB_F15 + sizeof(double) / sizeof(_BSD_JBSLOT_T_))

#ifndef _BSD_JBSLOT_T_
#define	_BSD_JBSLOT_T_	long long
#endif
