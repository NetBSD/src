/*	$NetBSD: pio.h,v 1.1 2001/06/19 00:20:12 fvdl Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * XXXfvdl plain copy of i386. Since pio didn't change, this should
 * probably be shared.
 */

#ifndef _X86_64_PIO_H_
#define _X86_64_PIO_H_

/*
 * Functions to provide access to i386 programmed I/O instructions.
 *
 * The in[bwl]() and out[bwl]() functions are split into two varieties: one to
 * use a small, constant, 8-bit port number, and another to use a large or
 * variable port number.  The former can be compiled as a smaller instruction.
 */


#ifdef __OPTIMIZE__

#define	__use_immediate_port(port) \
	(__builtin_constant_p((port)) && (port) < 0x100)

#else

#define	__use_immediate_port(port)	0

#endif


#define	inb(port) \
	(__use_immediate_port(port) ? __inbc(port) : __inb(port))

static __inline u_int8_t
__inbc(int port)
{
	u_int8_t data;
	__asm __volatile("inb %1,%0" : "=a" (data) : "id" (port));
	return data;
}

static __inline u_int8_t
__inb(int port)
{
	u_int8_t data;
	__asm __volatile("inb %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static __inline void
insb(int port, void *addr, int cnt)
{
	void *dummy1;
	int dummy2;
	__asm __volatile("cld\n\trepne\n\tinsb"			:
			 "=D" (dummy1), "=c" (dummy2) 		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "memory");
}

#define	inw(port) \
	(__use_immediate_port(port) ? __inwc(port) : __inw(port))

static __inline u_int16_t
__inwc(int port)
{
	u_int16_t data;
	__asm __volatile("inw %1,%0" : "=a" (data) : "id" (port));
	return data;
}

static __inline u_int16_t
__inw(int port)
{
	u_int16_t data;
	__asm __volatile("inw %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static __inline void
insw(int port, void *addr, int cnt)
{
	void *dummy1;
	int dummy2;
	__asm __volatile("cld\n\trepne\n\tinsw"			:
			 "=D" (dummy1), "=c" (dummy2)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "memory");
}

#define	inl(port) \
	(__use_immediate_port(port) ? __inlc(port) : __inl(port))

static __inline u_int32_t
__inlc(int port)
{
	u_int32_t data;
	__asm __volatile("inl %w1,%0" : "=a" (data) : "id" (port));
	return data;
}

static __inline u_int32_t
__inl(int port)
{
	u_int32_t data;
	__asm __volatile("inl %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static __inline void
insl(int port, void *addr, int cnt)
{
	void *dummy1;
	int dummy2;
	__asm __volatile("cld\n\trepne\n\tinsl"			:
			 "=D" (dummy1), "=c" (dummy2)		:
			 "d" (port), "0" (addr), "1" (cnt)	:
			 "memory");
}

#define	outb(port, data) \
	(__use_immediate_port(port) ? __outbc(port, data) : __outb(port, data))

static __inline void
__outbc(int port, u_int8_t data)
{
	__asm __volatile("outb %0,%w1" : : "a" (data), "id" (port));
}

static __inline void
__outb(int port, u_int8_t data)
{
	__asm __volatile("outb %0,%w1" : : "a" (data), "d" (port));
}

static __inline void
outsb(int port, void *addr, int cnt)
{
	void *dummy1;
	int dummy2;
	__asm __volatile("cld\n\trepne\n\toutsb"		:
			 "=S" (dummy1), "=c" (dummy2)		:
			 "d" (port), "0" (addr), "1" (cnt));
}

#define	outw(port, data) \
	(__use_immediate_port(port) ? __outwc(port, data) : __outw(port, data))

static __inline void
__outwc(int port, u_int16_t data)
{
	__asm __volatile("outw %0,%1" : : "a" (data), "id" (port));
}

static __inline void
__outw(int port, u_int16_t data)
{
	__asm __volatile("outw %0,%w1" : : "a" (data), "d" (port));
}

static __inline void
outsw(int port, void *addr, int cnt)
{
	void *dummy1;
	int dummy2;
	__asm __volatile("cld\n\trepne\n\toutsw"		:
			 "=S" (dummy1), "=c" (dummy2)		:
			 "d" (port), "0" (addr), "1" (cnt));
}

#define	outl(port, data) \
	(__use_immediate_port(port) ? __outlc(port, data) : __outl(port, data))

static __inline void
__outlc(int port, u_int32_t data)
{
	__asm __volatile("outl %0,%1" : : "a" (data), "id" (port));
}

static __inline void
__outl(int port, u_int32_t data)
{
	__asm __volatile("outl %0,%w1" : : "a" (data), "d" (port));
}

static __inline void
outsl(int port, void *addr, int cnt)
{
	void *dummy1;
	int dummy2;
	__asm __volatile("cld\n\trepne\n\toutsl"		:
			 "=S" (dummy1), "=c" (dummy2)		:
			 "d" (port), "0" (addr), "1" (cnt));
}

#endif /* _X86_64_PIO_H_ */
