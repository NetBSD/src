/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nia Alarie.
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

#ifndef _VIS_TYPES_H
#define _VIS_TYPES_H

/*
 * This should be compatible with what was shipped with SunPro.
 *
 * VIS Instruction Set User's Manual
 * Sun Microsystems
 * Part Number: 805-1394-03
 * May 2001
 *
 * Page 32 describes data types.
 */

typedef signed char		vis_s8 __attribute__ ((__may_alias__));
typedef unsigned char		vis_u8 __attribute__ ((__may_alias__));
typedef signed short		vis_s16 __attribute__ ((__may_alias__));
typedef unsigned short		vis_u16 __attribute__ ((__may_alias__));
typedef signed int		vis_s32 __attribute__ ((__may_alias__));
typedef unsigned int		vis_u32 __attribute__ ((__may_alias__));
typedef double			vis_d64 __attribute__ ((__may_alias__));
typedef float			vis_f32 __attribute__ ((__may_alias__));
typedef unsigned long		vis_addr __attribute__ ((__may_alias__));

#ifdef _LP64
typedef signed long		vis_s64 __attribute__ ((__may_alias__));
typedef unsigned long		vis_u64 __attribute__ ((__may_alias__));
#else
typedef signed long long	vis_s64 __attribute__ ((__may_alias__));
typedef unsigned long long	vis_u64 __attribute__ ((__may_alias__));
#endif

#endif
