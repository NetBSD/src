/*	$NetBSD: pio.h,v 1.10 2022/02/16 23:49:26 riastradh Exp $ */
/*	$OpenBSD: pio.h,v 1.1 1997/10/13 10:53:47 pefo Exp $ */

/*
 * Copyright (c) 1997 Per Fogelstrom, Opsycon AB and RTMX Inc, USA.
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
 *	This product includes software developed under OpenBSD by
 *	Per Fogelstrom Opsycon AB for RTMX Inc, North Carolina, USA.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef _POWERPC_PIO_H_
#define _POWERPC_PIO_H_

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

/*
 * I/O macros.
 */

#if defined(PPC_IBM4XX) && !defined(PPC_IBM440)
/* eieio is implemented as sync */
#define IO_BARRIER() __asm volatile("sync" ::: "memory")
#else
#define IO_BARRIER() __asm volatile("eieio; sync" ::: "memory")
#endif

static __inline void __outb(volatile uint8_t *a, uint8_t v);
static __inline void __outw(volatile uint16_t *a, uint16_t v);
static __inline void __outl(volatile uint32_t *a, uint32_t v);
static __inline void __outwrb(volatile uint16_t *a, uint16_t v);
static __inline void __outlrb(volatile uint32_t *a, uint32_t v);
static __inline uint8_t __inb(volatile uint8_t *a);
static __inline uint16_t __inw(volatile uint16_t *a);
static __inline uint32_t __inl(volatile uint32_t *a);
static __inline uint16_t __inwrb(volatile uint16_t *a);
static __inline uint32_t __inlrb(volatile uint32_t *a);
static __inline void __outsb(volatile uint8_t *, const uint8_t *, size_t);
static __inline void __outsw(volatile uint16_t *, const uint16_t *, size_t);
static __inline void __outsl(volatile uint32_t *, const uint32_t *, size_t);
static __inline void __outswrb(volatile uint16_t *, const uint16_t *, size_t);
static __inline void __outslrb(volatile uint32_t *, const uint32_t *, size_t);
static __inline void __insb(volatile uint8_t *, uint8_t *, size_t);
static __inline void __insw(volatile uint16_t *, uint16_t *, size_t);
static __inline void __insl(volatile uint32_t *, uint32_t *, size_t);
static __inline void __inswrb(volatile uint16_t *, uint16_t *, size_t);
static __inline void __inslrb(volatile uint32_t *, uint32_t *, size_t);

static __inline void
__outb(volatile uint8_t *a, uint8_t v)
{
	*a = v;
	IO_BARRIER();
}

static __inline void
__outw(volatile uint16_t *a, uint16_t v)
{
	*a = v;
	IO_BARRIER();
}

static __inline void
__outl(volatile uint32_t *a, uint32_t v)
{
	*a = v;
	IO_BARRIER();
}

static __inline void
__outwrb(volatile uint16_t *a, uint16_t v)
{
	__asm volatile("sthbrx %0, 0, %1" :: "r"(v), "r"(a));
	IO_BARRIER();
}

static __inline void
__outlrb(volatile uint32_t *a, uint32_t v)
{
	__asm volatile("stwbrx %0, 0, %1" :: "r"(v), "r"(a));
	IO_BARRIER();
}

static __inline uint8_t
__inb(volatile uint8_t *a)
{
	uint8_t _v_;

	_v_ = *a;
	IO_BARRIER();
	return _v_;
}

static __inline uint16_t
__inw(volatile uint16_t *a)
{
	uint16_t _v_;

	_v_ = *a;
	IO_BARRIER();
	return _v_;
}

static __inline uint32_t
__inl(volatile uint32_t *a)
{
	uint32_t _v_;

	_v_ = *a;
	IO_BARRIER();
	return _v_;
}

static __inline uint16_t
__inwrb(volatile uint16_t *a)
{
	uint16_t _v_;

	__asm volatile("lhbrx %0, 0, %1" : "=r"(_v_) : "r"(a));
	IO_BARRIER();
	return _v_;
}

static __inline uint32_t
__inlrb(volatile uint32_t *a)
{
	uint32_t _v_;

	__asm volatile("lwbrx %0, 0, %1" : "=r"(_v_) : "r"(a));
	IO_BARRIER();
	return _v_;
}

#define	outb(a,v)	(__outb((volatile uint8_t *)(a), v))
#define	out8(a,v)	outb(a,v)
#define	outw(a,v)	(__outw((volatile uint16_t *)(a), v))
#define	out16(a,v)	outw(a,v)
#define	outl(a,v)	(__outl((volatile uint32_t *)(a), v))
#define	out32(a,v)	outl(a,v)
#define	inb(a)		(__inb((volatile uint8_t *)(a)))
#define	in8(a)		inb(a)
#define	inw(a)		(__inw((volatile uint16_t *)(a)))
#define	in16(a)		inw(a)
#define	inl(a)		(__inl((volatile uint32_t *)(a)))
#define	in32(a)		inl(a)

#define	out8rb(a,v)	outb(a,v)
#define	outwrb(a,v)	(__outwrb((volatile uint16_t *)(a), v))
#define	out16rb(a,v)	outwrb(a,v)
#define	outlrb(a,v)	(__outlrb((volatile uint32_t *)(a), v))
#define	out32rb(a,v)	outlrb(a,v)
#define	in8rb(a)	inb(a)
#define	inwrb(a)	(__inwrb((volatile uint16_t *)(a)))
#define	in16rb(a)	inwrb(a)
#define	inlrb(a)	(__inlrb((volatile uint32_t *)(a)))
#define	in32rb(a)	inlrb(a)


static __inline void
__outsb(volatile uint8_t *a, const uint8_t *s, size_t c)
{
	while (c--)
		*a = *s++;
	IO_BARRIER();
}

static __inline void
__outsw(volatile uint16_t *a, const uint16_t *s, size_t c)
{
	while (c--)
		*a = *s++;
	IO_BARRIER();
}

static __inline void
__outsl(volatile uint32_t *a, const uint32_t *s, size_t c)
{
	while (c--)
		*a = *s++;
	IO_BARRIER();
}

static __inline void
__outswrb(volatile uint16_t *a, const uint16_t *s, size_t c)
{
	while (c--)
		__asm volatile("sthbrx %0, 0, %1" :: "r"(*s++), "r"(a));
	IO_BARRIER();
}

static __inline void
__outslrb(volatile uint32_t *a, const uint32_t *s, size_t c)
{
	while (c--)
		__asm volatile("stwbrx %0, 0, %1" :: "r"(*s++), "r"(a));
	IO_BARRIER();
}

static __inline void
__insb(volatile uint8_t *a, uint8_t *d, size_t c)
{
	while (c--)
		*d++ = *a;
	IO_BARRIER();
}

static __inline void
__insw(volatile uint16_t *a, uint16_t *d, size_t c)
{
	while (c--)
		*d++ = *a;
	IO_BARRIER();
}

static __inline void
__insl(volatile uint32_t *a, uint32_t *d, size_t c)
{
	while (c--)
		*d++ = *a;
	IO_BARRIER();
}

static __inline void
__inswrb(volatile uint16_t *a, uint16_t *d, size_t c)
{
	while (c--)
		__asm volatile("lhbrx %0, 0, %1" : "=r"(*d++) : "r"(a));
	IO_BARRIER();
}

static __inline void
__inslrb(volatile uint32_t *a, uint32_t *d, size_t c)
{
	while (c--)
		__asm volatile("lwbrx %0, 0, %1" : "=r"(*d++) : "r"(a));
	IO_BARRIER();
}

#define	outsb(a,s,c)	(__outsb((volatile uint8_t *)(a), s, c))
#define	outs8(a,s,c)	outsb(a,s,c)
#define	outsw(a,s,c)	(__outsw((volatile uint16_t *)(a), s, c))
#define	outs16(a,s,c)	outsw(a,s,c)
#define	outsl(a,s,c)	(__outsl((volatile uint32_t *)(a), s, c))
#define	outs32(a,s,c)	outsl(a,s,c)
#define	insb(a,d,c)	(__insb((volatile uint8_t *)(a), d, c))
#define	ins8(a,d,c)	insb(a,d,c)
#define	insw(a,d,c)	(__insw((volatile uint16_t *)(a), d, c))
#define	ins16(a,d,c)	insw(a,d,c)
#define	insl(a,d,c)	(__insl((volatile uint32_t *)(a), d, c))
#define	ins32(a,d,c)	insl(a,d,c)

#define	outs8rb(a,s,c)	outsb(a,s,c)
#define	outswrb(a,s,c)	(__outswrb((volatile uint16_t *)(a), s, c))
#define	outs16rb(a,s,c)	outswrb(a,s,c)
#define	outslrb(a,s,c)	(__outslrb((volatile uint32_t *)(a), s, c))
#define	outs32rb(a,s,c)	outslrb(a,s,c)
#define	ins8rb(a,d,c)	insb(a,d,c)
#define	inswrb(a,d,c)	(__inswrb((volatile uint16_t *)(a), d, c))
#define	ins16rb(a,d,c)	inswrb(a,d,c)
#define	inslrb(a,d,c)	(__inslrb((volatile uint32_t *)(a), d, c))
#define	ins32rb(a,d,c)	inslrb(a,d,c)

#undef IO_BARRIER

#endif /*_POWERPC_PIO_H_*/
