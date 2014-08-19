/*	$NetBSD: eevar.h,v 1.4.6.2 2014/08/20 00:03:18 tls Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

/*
 * EE embeded device's memory mapped register access method.
 */

#define _PLAYSTATION2_BUS_SPACE_PRIVATE
#include <machine/bus.h>

#define _reg_read_1(a)		__read_1(a)
#define _reg_read_2(a)		__read_2(a)
#define _reg_read_4(a)		__read_4(a)
#define _reg_read_8(a)		__read_8(a)
#define _reg_read_16(a)		__read_16(a)
#define _reg_write_1(a, v)	__write_1(a, v)
#define _reg_write_2(a, v)	__write_2(a, v)
#define _reg_write_4(a, v)	__write_4(a, v)
#define _reg_write_8(a, v)	__write_8(a, v)
#define _reg_write_16(a, v)	__write_16(a, v)

#define qwctobyte(x)		((x) << 4)
#define bytetoqwc(x)		((x) >> 4)
