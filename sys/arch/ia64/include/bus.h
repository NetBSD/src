/*	$NetBSD: bus.h,v 1.1.42.1 2016/10/05 20:55:29 skrll Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Copyright (c) 1996 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#ifndef _IA64_BUS_H_
#define _IA64_BUS_H_

#include <sys/systm.h>

#include <machine/cpufunc.h>
#include <machine/vmparam.h>

#ifdef BUS_SPACE_DEBUG
/*
 * Macros for sanity-checking the aligned-ness of pointers passed to
 * bus space ops.  These are not strictly necessary on the x86, but
 * could lead to performance improvements, and help catch problems
 * with drivers that would creep up on other architectures.
 */
#define	__BUS_SPACE_ALIGNED_ADDRESS(p, t)				\
	((((u_long)(p)) & (sizeof(t)-1)) == 0)

#define	__BUS_SPACE_ADDRESS_SANITY(p, t, d)				\
({									\
	if (__BUS_SPACE_ALIGNED_ADDRESS((p), t) == 0) {			\
		printf("%s 0x%lx not aligned to %d bytes %s:%d\n",	\
		    d, (u_long)(p), sizeof(t), __FILE__, __LINE__);	\
	}								\
	(void) 0;							\
})

#define BUS_SPACE_ALIGNED_POINTER(p, t) __BUS_SPACE_ALIGNED_ADDRESS(p, t)
#else
#define	__BUS_SPACE_ADDRESS_SANITY(p,t,d)	(void) 0
#define BUS_SPACE_ALIGNED_POINTER(p, t) ALIGNED_POINTER(p, t)
#endif /* BUS_SPACE_DEBUG */

/*
 * bus_space_map flags
 */

#define	BUS_SPACE_MAP_CACHEABLE		0x01
#define	BUS_SPACE_MAP_LINEAR		0x02
#define	BUS_SPACE_MAP_PREFETCHABLE	0x04

/*
 * Derived from x86 implementation, ia64 has both i/o and mem spaces
 * These values are for bus space tag.
 */
#define IA64_BUS_SPACE_IO	 0	 /* space is i/o space */
#define IA64_BUS_SPACE_MEM	 1	 /* space is mem space */

#define __BUS_SPACE_HAS_STREAM_METHODS 1

/*
 * Bus address and size types
 */
typedef u_long bus_addr_t;
typedef u_long bus_size_t;

/*
 * Access methods for bus resources and address space.
 */
typedef	int bus_space_tag_t;
typedef	u_long bus_space_handle_t;

/* map/unmap */

int	ia64_bus_space_map(bus_space_tag_t, bus_addr_t,
	    bus_size_t, int, bus_space_handle_t *);
void	ia64_bus_space_unmap(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t);
int	ia64_bus_space_subregion(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, bus_size_t, bus_space_handle_t *);
#define	bus_space_map(t, a, s, f, hp)					\
	ia64_bus_space_map((t), (a), (s), (f), (hp))
#define bus_space_unmap(t, h, s)					\
	ia64_bus_space_unmap((t), (h), (s))
#define bus_space_subregion(t, h, o, s, nhp)				\
	ia64_bus_space_subregion((t), (h), (o), (s), (nhp))

/* vaddr */

#define bus_space_vaddr(t, h) \
	((t) == IA64_BUS_SPACE_MEM ? (void *)(h) : (void *)0)

/* map to user space */

paddr_t	ia64_bus_space_mmap(bus_space_tag_t, bus_addr_t, off_t, int, int);

#define	bus_space_mmap(t, b, o, p, f)					\
	ia64_bus_space_mmap((t), (b), (o), (p), (f))

/* alloc/free */

int	ia64_bus_space_alloc(bus_space_tag_t, bus_addr_t, bus_addr_t,
	    bus_size_t, bus_size_t, bus_size_t, int, bus_addr_t *,
	    bus_space_handle_t *);
void	ia64_bus_space_free(bus_space_tag_t, bus_space_handle_t, bus_size_t);

#define bus_space_alloc(t, rs, re, s, a, b, f, ap, hp)			\
	ia64_bus_space__alloc((t), (rs), (re), (s), (a), (b), (f), (ap), (hp))
#define bus_space_free(t, h, s)						\
	ia64_bus_space_free((t), (h), (s))

/*
 *	uintN_t bus_space_read_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset);
 *
 * Read a 1, 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset.
 */

#define	bus_space_read_1(t, h, o)					\
	((t) == IA64_BUS_SPACE_IO ? (inb((h) + (o))) :\
	    (*(volatile uint8_t *)((h) + (o))))

#define	bus_space_read_2(t, h, o)					\
	 (__BUS_SPACE_ADDRESS_SANITY((h) + (o), uint16_t, "bus addr"),	\
	  ((t) == IA64_BUS_SPACE_IO ? (inw((h) + (o))) :		\
	    (*(volatile uint16_t *)((h) + (o)))))

#define	bus_space_read_4(t, h, o)					\
	 (__BUS_SPACE_ADDRESS_SANITY((h) + (o), uint32_t, "bus addr"),	\
	  ((t) == IA64_BUS_SPACE_IO ? (inl((h) + (o))) :		\
	    (*(volatile uint32_t *)((h) + (o)))))

#define	bus_space_read_8(t, h, o)					\
	 (__BUS_SPACE_ADDRESS_SANITY((h) + (o), uint64_t, "bus addr"),	\
	  ((t) == IA64_BUS_SPACE_IO ?					\
	    ({ printf("%s: can't read 8bytes from I/O space\n", __FUNCTION__); 0;}) : \
	    (*(volatile uint64_t *)((h) + (o)))))

#define bus_space_read_stream_1 bus_space_read_1
#define bus_space_read_stream_2 bus_space_read_2
#define bus_space_read_stream_4 bus_space_read_4
#define bus_space_read_stream_8 bus_space_read_8

/*
 *	void bus_space_read_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    uintN_t *addr, size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle/offset and copy into buffer provided.
 */

#define	bus_space_read_multi_1(t, h, o, ptr, cnt)			\
do {									\
	if ((t) == IA64_BUS_SPACE_IO) {					\
		insb((h) + (o), (ptr), (cnt));				\
	} else {							\
		int __i;						\
		volatile uint8_t *__p = (uint8_t *)((h) + (o));		\
		uint8_t *__dst = (ptr);					\
		for (__i = 0; __i < (cnt); __i++)			\
			*__dst++ = *__p;				\
	}								\
} while (/* CONSTCOND */ 0)

#define	bus_space_read_multi_2(t, h, o, ptr, cnt)			\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((ptr), uint16_t, "buffer");		\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), uint16_t, "bus addr");	\
	if ((t) == IA64_BUS_SPACE_IO) {					\
		insw((h) + (o), (ptr), (cnt));				\
	} else {							\
		int __i;						\
		volatile uint16_t *__p = (uint16_t *)((h) + (o));	\
		uint16_t *__dst = (ptr);				\
		for (__i = 0; __i < (cnt); __i++)			\
			*__dst++ = *__p;				\
	}								\
} while (/* CONSTCOND */ 0)

#define	bus_space_read_multi_4(t, h, o, ptr, cnt)			\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((ptr), uint32_t, "buffer");		\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), uint32_t, "bus addr");	\
	if ((t) == IA64_BUS_SPACE_IO) {					\
		insl((h) + (o), (ptr), (cnt));				\
	} else {							\
		int __i;						\
		volatile uint32_t *__p = (uint32_t *)((h) + (o));	\
		uint32_t *__dst = (ptr);				\
		for (__i = 0; __i < (cnt); __i++)			\
			*__dst++ = *__p;				\
	}								\
} while (/* CONSTCOND */ 0)

#define	bus_space_read_multi_8(t, h, o, ptr, cnt)			\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((ptr), uint64_t, "buffer");		\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), uint64_t, "bus addr");	\
	if ((t) == IA64_BUS_SPACE_IO) {					\
		printf("%s: can't read 8bytes from I/O space\n",	\
		    __FUNCTION__);					\
	} else {							\
		int __i;						\
		volatile uint64_t *__p = (uint64_t *)((h) + (o));	\
		uint64_t *__dst = (ptr);				\
		for (__i = 0; __i < (cnt); __i++)			\
			*__dst++ = *__p;				\
	}								\
} while (/* CONSTCOND */ 0)

#define bus_space_read_multi_stream_1 bus_space_read_multi_1
#define bus_space_read_multi_stream_2 bus_space_read_multi_2
#define bus_space_read_multi_stream_4 bus_space_read_multi_4
#define bus_space_read_multi_stream_8 bus_space_read_multi_8


/*
 *	void bus_space_read_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    uintN_t *addr, size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle and starting at `offset' and copy into
 * buffer provided.
 */

#define	bus_space_read_region_1(t, h, o, ptr, cnt)			\
do {									\
	if ((t) == IA64_BUS_SPACE_IO) {					\
		int __i;						\
		volatile bus_addr_t __port = (h) + (o);			\
		uint8_t *__dst = (ptr);					\
		for (__i = 0; __i < (cnt); __i++) {			\
			*__dst++ = inb(__port);				\
			__port++;					\
		}							\
	} else {							\
		int __i;						\
		volatile uint8_t *__p = (uint8_t *)((h) + (o));		\
		uint8_t *__dst = (ptr);					\
		for (__i = 0; __i < (cnt); __i++)			\
			*__dst++ = *__p++;				\
	}								\
} while (/* CONSTCOND */ 0)

#define	bus_space_read_region_2(t, h, o, ptr, cnt)			\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((ptr), uint16_t, "buffer");		\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), uint16_t, "bus addr");	\
	if ((t) == IA64_BUS_SPACE_IO) {					\
		int __i;						\
		volatile bus_addr_t __port = (h) + (o);			\
		uint8_t *__dst = (ptr);					\
		for (__i = 0; __i < (cnt); __i++) {			\
			*__dst++ = inb(__port);				\
			__port += 2;					\
		}							\
	} else {							\
		int __i;						\
		volatile uint16_t *__p = (uint16_t *)((h) + (o));	\
		uint16_t *__dst = (ptr);				\
		for (__i = 0; __i < (cnt); __i++)			\
			*__dst++ = *__p++;				\
	}								\
} while (/* CONSTCOND */ 0)

#define	bus_space_read_region_4(t, h, o, ptr, cnt)			\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((ptr), uint32_t, "buffer");		\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), uint32_t, "bus addr");	\
	if ((t) == IA64_BUS_SPACE_IO) {					\
		int __i;						\
		volatile bus_addr_t __port = (h) + (o);			\
		uint8_t *__dst = (ptr);					\
		for (__i = 0; __i < (cnt); __i++) {			\
			*__dst++ = inb(__port);				\
			__port += 4;					\
		}							\
	} else {							\
		volatile uint32_t *__p = (uint32_t *)((h) + (o));	\
		uint32_t *__dst = (ptr);				\
		for (__i = 0; __i < (cnt); __i++)			\
			*__dst++ = *__p++;				\
	}								\
} while (/* CONSTCOND */ 0)

#define	bus_space_read_region_8(t, h, o, ptr, cnt)			\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((ptr), uint64_t, "buffer");		\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), uint64_t, "bus addr");	\
	if ((t) == IA64_BUS_SPACE_IO) {					\
		printf("%s: can't read 8bytes from I/O space\n",	\
		    __FUNCTION__);					\
	} else {							\
		volatile uint64_t *__p = (uint64_t *)((h) + (o));	\
		uint64_t *__dst = (ptr);				\
		for (__i = 0; __i < (cnt); __i++)			\
			*__dst++ = *__p++;				\
	}								\
} while (/* CONSTCOND */ 0)

#define bus_space_read_region_stream_1 bus_space_read_region_1
#define bus_space_read_region_stream_2 bus_space_read_region_2
#define bus_space_read_region_stream_4 bus_space_read_region_4
#define bus_space_read_region_stream_8 bus_space_read_region_8


/*
 *	void bus_space_write_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    uintN_t value);
 *
 * Write the 1, 2, 4, or 8 byte value `value' to bus space
 * described by tag/handle/offset.
 */

#define	bus_space_write_1(t, h, o, v)					\
do {									\
	if ((t) == IA64_BUS_SPACE_IO)					\
		outb((h) + (o), (v));					\
	else								\
		((void)(*(volatile uint8_t *)((h) + (o)) = (v)));	\
} while (/* CONSTCOND */ 0)

#define	bus_space_write_2(t, h, o, v)					\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), uint16_t, "bus addr");	\
	if ((t) == IA64_BUS_SPACE_IO)					\
		outw((h) + (o), (v));					\
	else								\
		((void)(*(volatile uint16_t *)((h) + (o)) = (v)));	\
} while (/* CONSTCOND */ 0)

#define	bus_space_write_4(t, h, o, v)					\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), uint32_t, "bus addr");	\
	if ((t) == IA64_BUS_SPACE_IO)					\
		outl((h) + (o), (v));					\
	else								\
		((void)(*(volatile uint32_t *)((h) + (o)) = (v)));	\
} while (/* CONSTCOND */ 0)

#define	bus_space_write_8(t, h, o, v)					\
	 __BUS_SPACE_ADDRESS_SANITY((h) + (o), uint64_t, "bus addr");	\
	 if ((t) == IA64_BUS_SPACE_IO ?					\
	     printf("%s: can't write 8bytes to I/O space\n", __FUNCTION__); : \
	     (*(volatile uint64_t *)((h) + (o)) = (v)))

#define bus_space_write_stream_1 bus_space_write_1
#define bus_space_write_stream_2 bus_space_write_2
#define bus_space_write_stream_4 bus_space_write_4
#define bus_space_write_stream_8 bus_space_write_8

/*
 *	void bus_space_write_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const uintN_t *addr, size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer
 * provided to bus space described by tag/handle/offset.
 */

#define	bus_space_write_multi_1(t, h, o, ptr, cnt)			\
do {									\
	if ((t) == IA64_BUS_SPACE_IO) {					\
		outsb((h) + (o), (ptr), (cnt));				\
	} else {							\
		int __i;						\
		volatile uint8_t *__p = (uint8_t *)((h) + (o));		\
		uint8_t *__src = (ptr);					\
		for (__i = 0; __i < (cnt); __i++)			\
			*__p = *__src++;				\
	}								\
} while (/* CONSTCOND */ 0)

#define bus_space_write_multi_2(t, h, o, ptr, cnt)			\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((ptr), uint16_t, "buffer");		\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), uint16_t, "bus addr");	\
	if ((t) == IA64_BUS_SPACE_IO) {					\
		outsw((h) + (o), (ptr), (cnt));				\
	} else {							\
		int __i;						\
		volatile uint16_t *__p = (uint16_t *)((h) + (o));	\
		uint16_t *__src = (ptr);				\
		for (__i = 0; __i < (cnt); __i++)			\
			*__p = *__src++;				\
	}								\
} while (/* CONSTCOND */ 0)

#define bus_space_write_multi_4(t, h, o, ptr, cnt)			\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((ptr), uint32_t, "buffer");		\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), uint32_t, "bus addr");	\
	if ((t) == IA64_BUS_SPACE_IO) {					\
		outsl((h) + (o), (ptr), (cnt));				\
	} else {							\
		int __i;						\
		volatile uint32_t *__p = (uint32_t *)((h) + (o));	\
		uint32_t *__src = (ptr);				\
		for (__i = 0; __i < (cnt); __i++)			\
			*__p = *__src++;				\
	}								\
} while (/* CONSTCOND */ 0)

#define bus_space_write_multi_8(t, h, o, ptr, cnt)			\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((ptr), uint64_t, "buffer");		\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), uint64_t, "bus addr");	\
	if ((t) == IA64_BUS_SPACE_IO) {					\
		printf("%s: can't write 8bytes to I/O space\n", __FUNCTION__);\
	} else {							\
		int __i;						\
		volatile uint64_t *__p = (uint64_t *)((h) + (o));	\
		uint64_t *__src = (ptr);				\
		for (__i = 0; __i < (cnt); __i++)			\
			*__p = *__src++;				\
	}								\
} while (/* CONSTCOND */ 0)

#define bus_space_write_multi_stream_1 bus_space_write_multi_1
#define bus_space_write_multi_stream_2 bus_space_write_multi_2
#define bus_space_write_multi_stream_4 bus_space_write_multi_4
#define bus_space_write_multi_stream_8 bus_space_write_multi_8


/*
 *	void bus_space_write_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const uintN_t *addr, size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer provided
 * to bus space described by tag/handle starting at `offset'.
 */

#define	bus_space_write_region_1(t, h, o, ptr, cnt)			\
do {									\
	if ((t) == IA64_BUS_SPACE_IO) {					\
		int __i;						\
		volatile bus_addr_t __port = (h) + (o);			\
		uint8_t *__src = (ptr);					\
		for (__i = 0; __i < (cnt); __i++) {			\
			outb(__port, *__src);				\
			__port++;					\
			__src++;					\
		}							\
	} else {							\
		int __i;						\
		volatile uint8_t *__p = (uint8_t *)((h) + (o));		\
		uint8_t *__src = (ptr);					\
		for (__i = 0; __i < (cnt); __i++)			\
			*__p++ = *__src++;				\
	}								\
} while (/* CONSTCOND */ 0)

#define	bus_space_write_region_2(t, h, o, ptr, cnt)			\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((ptr), uint16_t, "buffer");		\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), uint16_t, "bus addr");	\
	if ((t) == IA64_BUS_SPACE_IO) {					\
		int __i;						\
		volatile bus_addr_t __port = (h) + (o);			\
		uint16_t *__src = (ptr);				\
		for (__i = 0; __i < (cnt); __i++) {			\
			outw(__port, *__src);				\
			__port += 2;					\
			__src++;					\
		}							\
	} else {							\
		int __i;						\
		volatile uint16_t *__p = (uint16_t *)((h) + (o));	\
		uint16_t *__src = (ptr);				\
		for (__i = 0; __i < (cnt); __i++)			\
			*__p++ = *__src++;				\
	}								\
} while (/* CONSTCOND */ 0)

#define	bus_space_write_region_4(t, h, o, ptr, cnt)			\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((ptr), uint32_t, "buffer");		\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), uint32_t, "bus addr");	\
	if ((t) == IA64_BUS_SPACE_IO) {					\
		int __i;						\
		volatile bus_addr_t __port = (h) + (o);			\
		uint32_t *__src = (ptr);				\
		for (__i = 0; __i < (cnt); __i++) {			\
			outl(__port, *__src);				\
			__port += 4;					\
			__src++;					\
		}							\
	} else {							\
		int __i;						\
		volatile uint32_t *__p = (uint32_t *)(h) + (o);		\
		uint32_t *__src = (ptr);				\
		for (__i = 0; __i < (cnt); __i++)			\
			*__p++ = *__src++;				\
	}								\
} while (/* CONSTCOND */ 0)

#define	bus_space_write_region_8(t, h, o, ptr, cnt)			\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((ptr), uint64_t, "buffer");		\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), uint64_t, "bus addr");	\
	if ((t) == IA64_BUS_SPACE_IO) {					\
		printf("%s: can't write 8bytes to I/O space\n", __FUNCTION__);\
	} else {							\
		int __i;						\
		volatile uint64_t *__p = (uint64_t *)((h) + (o));	\
		uint64_t *__src = (ptr);				\
		for (__i = 0; __i < (cnt); __i++)			\
			*__p++ = *__src++;				\
	}								\
} while (/* CONSTCOND */ 0)

#define bus_space_write_region_stream_1 bus_space_write_region_1
#define bus_space_write_region_stream_2 bus_space_write_region_2
#define bus_space_write_region_stream_4 bus_space_write_region_4
#define bus_space_write_region_stream_8 bus_space_write_region_8


/*
 *	void bus_space_set_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, uintN_t val,
 *	    size_t count);
 *
 * Write the 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle/offset `count' times.
 */

static __inline void ia64_bus_space_set_multi_1(bus_space_tag_t,
	bus_space_handle_t, bus_size_t, uint8_t, size_t);
static __inline void ia64_bus_space_set_multi_2(bus_space_tag_t,
	bus_space_handle_t, bus_size_t, uint16_t, size_t);
static __inline void ia64_bus_space_set_multi_4(bus_space_tag_t,
	bus_space_handle_t, bus_size_t, uint32_t, size_t);
static __inline void ia64_bus_space_set_multi_8(bus_space_tag_t,
	bus_space_handle_t, bus_size_t, uint64_t, size_t);

#define	bus_space_set_multi_1(t, h, o, v, c)				\
	ia64_bus_space_set_multi_1((t), (h), (o), (v), (c))

#define	bus_space_set_multi_2(t, h, o, v, c)				\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), uint16_t, "bus addr");	\
	ia64_bus_space_set_multi_2((t), (h), (o), (v), (c));		\
} while (/* CONSTCOND */ 0)

#define	bus_space_set_multi_4(t, h, o, v, c)				\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), uint32_t, "bus addr");	\
	ia64_bus_space_set_multi_4((t), (h), (o), (v), (c));		\
} while (/* CONSTCOND */ 0)

#define	bus_space_set_multi_8(t, h, o, v, c)				\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), uint64_t, "bus addr");	\
	ia64_bus_space_set_multi_8((t), (h), (o), (v), (c));		\
} while (/* CONSTCOND */ 0)

static __inline void
ia64_bus_space_set_multi_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint8_t v, size_t c)
{
	bus_addr_t addr = h + o;

	if (t == IA64_BUS_SPACE_IO)
		while (c--)
			outb(addr, v);
	else
		while (c--)
			*(volatile uint8_t *)(addr) = v;
}

static __inline void
ia64_bus_space_set_multi_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint16_t v, size_t c)
{
	bus_addr_t addr = h + o;

	if (t == IA64_BUS_SPACE_IO)
		while (c--)
			outw(addr, v);
	else
		while (c--)
			*(volatile uint16_t *)(addr) = v;
}

static __inline void
ia64_bus_space_set_multi_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint32_t v, size_t c)
{
	bus_addr_t addr = h + o;

	if (t == IA64_BUS_SPACE_IO)
		while (c--)
			outl(addr, v);
	else
		while (c--)
			*(volatile uint32_t *)(addr) = v;
}

static __inline void
ia64_bus_space_set_multi_8(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint64_t v, size_t c)
{
	bus_addr_t addr = h + o;

	if (t == IA64_BUS_SPACE_IO)
		printf("%s: can't write 8bytes to I/O space\n", __FUNCTION__);
	else
		while (c--)
			*(volatile uint64_t *)(addr) = v;
}


/*
 *	void bus_space_set_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, uintN_t val,
 *	    size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle starting at `offset'.
 */

static __inline void ia64_bus_space_set_region_1(bus_space_tag_t,
	bus_space_handle_t, bus_size_t, uint8_t, size_t);
static __inline void ia64_bus_space_set_region_2(bus_space_tag_t,
	bus_space_handle_t, bus_size_t, uint16_t, size_t);
static __inline void ia64_bus_space_set_region_4(bus_space_tag_t,
	bus_space_handle_t, bus_size_t, uint32_t, size_t);

#define	bus_space_set_region_1(t, h, o, v, c)				\
	ia64_bus_space_set_region_1((t), (h), (o), (v), (c))

#define	bus_space_set_region_2(t, h, o, v, c)				\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), uint16_t, "bus addr");	\
	ia64_bus_space_set_region_2((t), (h), (o), (v), (c));		\
} while (/* CONSTCOND */ 0)

#define	bus_space_set_region_4(t, h, o, v, c)				\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), uint32_t, "bus addr");	\
	ia64_bus_space_set_region_4((t), (h), (o), (v), (c));		\
} while (/* CONSTCOND */ 0)

#define	bus_space_set_region_8(t, h, o, v, c)				\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), uint64_t, "bus addr");	\
	ia64_bus_space_set_region_8((t), (h), (o), (v), (c));		\
} while (/* CONSTCOND */ 0)

static __inline void
ia64_bus_space_set_region_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint8_t v, size_t c)
{
	bus_addr_t addr = h + o;

	if (t == IA64_BUS_SPACE_IO)
		for (; c != 0; c--, addr++)
			outb(addr, v);
	else
		for (; c != 0; c--, addr++)
			*(volatile uint8_t *)(addr) = v;
}

static __inline void
ia64_bus_space_set_region_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint16_t v, size_t c)
{
	bus_addr_t addr = h + o;

	if (t == IA64_BUS_SPACE_IO)
		for (; c != 0; c--, addr += 2)
			outw(addr, v);
	else
		for (; c != 0; c--, addr += 2)
			*(volatile uint16_t *)(addr) = v;
}

static __inline void
ia64_bus_space_set_region_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint32_t v, size_t c)
{
	bus_addr_t addr = h + o;

	if (t == IA64_BUS_SPACE_IO)
		for (; c != 0; c--, addr += 4)
			outl(addr, v);
	else
		for (; c != 0; c--, addr += 4)
			*(volatile uint32_t *)(addr) = v;
}

static __inline void
ia64_bus_space_set_region_8(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint64_t v, size_t c)
{
	bus_addr_t addr = h + o;

	if (t == IA64_BUS_SPACE_IO)
		printf("%s: can't write 8bytes to I/O space\n", __FUNCTION__);
	else
		for (; c != 0; c--, addr += 8)
			*(volatile uint32_t *)(addr) = v;
}


/*
 *	void bus_space_copy_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh1, bus_size_t off1,
 *	    bus_space_handle_t bsh2, bus_size_t off2,
 *	    size_t count);
 *
 * Copy `count' 1, 2, 4, or 8 byte values from bus space starting
 * at tag/bsh1/off1 to bus space starting at tag/bsh2/off2.
 */

static __inline void ia64_bus_space_copy_region_1(bus_space_tag_t,
	bus_space_handle_t, bus_size_t, bus_space_handle_t,
	bus_size_t, size_t);
static __inline void ia64_bus_space_copy_region_2(bus_space_tag_t,
	bus_space_handle_t, bus_size_t, bus_space_handle_t,
	bus_size_t, size_t);
static __inline void ia64_bus_space_copy_region_4(bus_space_tag_t,
	bus_space_handle_t, bus_size_t, bus_space_handle_t,
	bus_size_t, size_t);

#define	bus_space_copy_region_1(t, h1, o1, h2, o2, c)			\
	ia64_bus_space_copy_region_1((t), (h1), (o1), (h2), (o2), (c))

#define	bus_space_copy_region_2(t, h1, o1, h2, o2, c)			\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((h1) + (o1), uint16_t, "bus addr 1"); \
	__BUS_SPACE_ADDRESS_SANITY((h2) + (o2), uint16_t, "bus addr 2"); \
	ia64_bus_space_copy_region_2((t), (h1), (o1), (h2), (o2), (c));	\
} while (/* CONSTCOND */ 0)

#define	bus_space_copy_region_4(t, h1, o1, h2, o2, c)			\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((h1) + (o1), uint32_t, "bus addr 1"); \
	__BUS_SPACE_ADDRESS_SANITY((h2) + (o2), uint32_t, "bus addr 2"); \
	ia64_bus_space_copy_region_4((t), (h1), (o1), (h2), (o2), (c));	\
} while (/* CONSTCOND */ 0)

#define	bus_space_copy_region_8(t, h1, o1, h2, o2, c)			\
do {									\
	__BUS_SPACE_ADDRESS_SANITY((h1) + (o1), uint64_t, "bus addr 1"); \
	__BUS_SPACE_ADDRESS_SANITY((h2) + (o2), uint64_t, "bus addr 2"); \
	ia64_bus_space_copy_region_8((t), (h1), (o1), (h2), (o2), (c));	\
} while (/* CONSTCOND */ 0)

static __inline void
ia64_bus_space_copy_region_1(bus_space_tag_t t,
    bus_space_handle_t h1, bus_size_t o1,
    bus_space_handle_t h2, bus_size_t o2, size_t c)
{
	bus_addr_t addr1 = h1 + o1;
	bus_addr_t addr2 = h2 + o2;

	if (t == IA64_BUS_SPACE_IO) {
		if (addr1 >= addr2) {
			/* src after dest: copy forward */
			for (; c != 0; c--, addr1++, addr2++)
				outb(addr2, inb(addr1));
		} else {
			/* dest after src: copy backwards */
			for (addr1 += (c - 1), addr2 += (c - 1);
			    c != 0; c--, addr1--, addr2--)
				outb(addr2, inb(addr1));
		}
	} else {
		if (addr1 >= addr2) {
			/* src after dest: copy forward */
			for (; c != 0; c--, addr1++, addr2++)
				*(volatile uint8_t *)(addr2) =
				    *(volatile uint8_t *)(addr1);
		} else {
			/* dest after src: copy backwards */
			for (addr1 += (c - 1), addr2 += (c - 1);
			    c != 0; c--, addr1--, addr2--)
				*(volatile uint8_t *)(addr2) =
				    *(volatile uint8_t *)(addr1);
		}
	}
}

static __inline void
ia64_bus_space_copy_region_2(bus_space_tag_t t,
    bus_space_handle_t h1, bus_size_t o1,
    bus_space_handle_t h2, bus_size_t o2, size_t c)
{
	bus_addr_t addr1 = h1 + o1;
	bus_addr_t addr2 = h2 + o2;

	if (t == IA64_BUS_SPACE_IO) {
		if (addr1 >= addr2) {
			/* src after dest: copy forward */
			for (; c != 0; c--, addr1 += 2, addr2 += 2)
				outw(addr2, inw(addr1));
		} else {
			/* dest after src: copy backwards */
			for (addr1 += 2 * (c - 1), addr2 += 2 * (c - 1);
			    c != 0; c--, addr1 -= 2, addr2 -= 2)
				outw(addr2, inw(addr1));
		}
	} else {
		if (addr1 >= addr2) {
			/* src after dest: copy forward */
			for (; c != 0; c--, addr1 += 2, addr2 += 2)
				*(volatile uint16_t *)(addr2) =
				    *(volatile uint16_t *)(addr1);
		} else {
			/* dest after src: copy backwards */
			for (addr1 += 2 * (c - 1), addr2 += 2 * (c - 1);
			    c != 0; c--, addr1 -= 2, addr2 -= 2)
				*(volatile uint16_t *)(addr2) =
				    *(volatile uint16_t *)(addr1);
		}
	}
}

static __inline void
ia64_bus_space_copy_region_4(bus_space_tag_t t,
    bus_space_handle_t h1, bus_size_t o1,
    bus_space_handle_t h2, bus_size_t o2, size_t c)
{
	bus_addr_t addr1 = h1 + o1;
	bus_addr_t addr2 = h2 + o2;

	if (t == IA64_BUS_SPACE_IO) {
		if (addr1 >= addr2) {
			/* src after dest: copy forward */
			for (; c != 0; c--, addr1 += 4, addr2 += 4)
				outl(addr2, inl(addr1));
		} else {
			/* dest after src: copy backwards */
			for (addr1 += 4 * (c - 1), addr2 += 4 * (c - 1);
			    c != 0; c--, addr1 -= 4, addr2 -= 4)
				outl(addr2, inl(addr1));
		}
	} else {
		if (addr1 >= addr2) {
			/* src after dest: copy forward */
			for (; c != 0; c--, addr1 += 4, addr2 += 4)
				*(volatile uint32_t *)(addr2) =
				    *(volatile uint32_t *)(addr1);
		} else {
			/* dest after src: copy backwards */
			for (addr1 += 4 * (c - 1), addr2 += 4 * (c - 1);
			    c != 0; c--, addr1 -= 4, addr2 -= 4)
				*(volatile uint32_t *)(addr2) =
				    *(volatile uint32_t *)(addr1);
		}
	}
}

static __inline void
ia64_bus_space_copy_region_8(bus_space_tag_t t,
    bus_space_handle_t h1, bus_size_t o1,
    bus_space_handle_t h2, bus_size_t o2, size_t c)
{
	bus_addr_t addr1 = h1 + o1;
	bus_addr_t addr2 = h2 + o2;

	if (t == IA64_BUS_SPACE_IO) {
		printf("%s: can't write 8bytes to I/O space\n", __FUNCTION__);
	} else {
		if (addr1 >= addr2) {
			/* src after dest: copy forward */
			for (; c != 0; c--, addr1 += 8, addr2 += 8)
				*(volatile uint64_t *)(addr2) =
				    *(volatile uint64_t *)(addr1);
		} else {
			/* dest after src: copy backwards */
			for (addr1 += 8 * (c - 1), addr2 += 8 * (c - 1);
			    c != 0; c--, addr1 -= 8, addr2 -= 8)
				*(volatile uint64_t *)(addr2) =
				    *(volatile uint64_t *)(addr1);
		}
	}
}


/*
 * Bus read/write barrier methods.
 *
 *	void bus_space_barrier(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    bus_size_t len, int flags);
 *
 * Note: the x86 does not currently require barriers, but we must
 * provide the flags to MI code.
 */
#define	bus_space_barrier(t, h, o, l, f)	\
	ia64_bus_space_barrier((t), (h), (o), (l), (f))

#define	BUS_SPACE_BARRIER_READ	0x01
#define	BUS_SPACE_BARRIER_WRITE	0x02

static __inline void
ia64_bus_space_barrier(bus_space_tag_t t, bus_space_handle_t handle,
    bus_size_t offset, bus_size_t length, int flags)
{
	if (t == IA64_BUS_SPACE_IO)
		return;

	switch (flags) {
	case BUS_SPACE_BARRIER_READ:
		__asm volatile("mf" ::: "memory");
		break;
	case BUS_SPACE_BARRIER_WRITE:
		__asm volatile("mf" ::: "memory");
		break;
	case BUS_SPACE_BARRIER_READ | BUS_SPACE_BARRIER_WRITE:
		__asm volatile("mf" ::: "memory");
		break;
	default:
		printf("%s: Unknown barrier %d\n", __FUNCTION__, flags);
		break;
	}
}


/*
 * Flags used in various bus DMA methods.
 */
#define	BUS_DMA_WAITOK		0x000	/* safe to sleep (pseudo-flag) */
#define	BUS_DMA_NOWAIT		0x001	/* not safe to sleep */
#define	BUS_DMA_ALLOCNOW	0x002	/* perform resource allocation now */
#define	BUS_DMA_COHERENT	0x004	/* hint: map memory DMA coherent */
#define	BUS_DMA_STREAMING	0x008	/* hint: sequential, unidirectional */
#define	BUS_DMA_BUS1		0x010	/* placeholders for bus functions... */
#define	BUS_DMA_BUS2		0x020
#define	BUS_DMA_BUS3		0x040
#define	BUS_DMA_BUS4		0x080
#define	BUS_DMA_READ		0x100	/* mapping is device -> memory only */
#define	BUS_DMA_WRITE		0x200	/* mapping is memory -> device only */
#define	BUS_DMA_NOCACHE		0x400	/* hint: map non-cached memory */

/* Forwards needed by prototypes below. */
struct mbuf;
struct uio;

/*
 * Operations performed by bus_dmamap_sync().
 */
#define	BUS_DMASYNC_PREREAD	0x01	/* pre-read synchronization */
#define	BUS_DMASYNC_POSTREAD	0x02	/* post-read synchronization */
#define	BUS_DMASYNC_PREWRITE	0x04	/* pre-write synchronization */
#define	BUS_DMASYNC_POSTWRITE	0x08	/* post-write synchronization */

typedef struct ia64_bus_dma_tag		*bus_dma_tag_t;
typedef struct ia64_bus_dmamap		*bus_dmamap_t;

#define BUS_DMA_TAG_VALID(t)    ((t) != (bus_dma_tag_t)0)

/*
 *	bus_dma_segment_t
 *
 *	Describes a single contiguous DMA transaction.  Values
 *	are suitable for programming into DMA registers.
 */
struct ia64_bus_dma_segment {
	bus_addr_t	ds_addr;	/* DMA address */
	bus_size_t	ds_len;		/* length of transfer */
};
typedef struct ia64_bus_dma_segment	bus_dma_segment_t;

/*
 *	bus_dma_tag_t
 *
 *	A machine-dependent opaque type describing the implementation of
 *	DMA for a given bus.
 */

struct ia64_bus_dma_tag {
	/*
	 * The `bounce threshold' is checked while we are loading
	 * the DMA map.  If the physical address of the segment
	 * exceeds the threshold, an error will be returned.  The
	 * caller can then take whatever action is necessary to
	 * bounce the transfer.  If this value is 0, it will be
	 * ignored.
	 */
	bus_addr_t _bounce_thresh;
	bus_addr_t _bounce_alloc_lo;
	bus_addr_t _bounce_alloc_hi;
	int	(*_may_bounce)(bus_dma_tag_t, bus_dmamap_t, int, int *);

	/*
	 * DMA mapping methods.
	 */
	int	(*_dmamap_create)(bus_dma_tag_t, bus_size_t, int,
		    bus_size_t, bus_size_t, int, bus_dmamap_t *);
	void	(*_dmamap_destroy)(bus_dma_tag_t, bus_dmamap_t);
	int	(*_dmamap_load)(bus_dma_tag_t, bus_dmamap_t, void *,
		    bus_size_t, struct proc *, int);
	int	(*_dmamap_load_mbuf)(bus_dma_tag_t, bus_dmamap_t,
		    struct mbuf *, int);
	int	(*_dmamap_load_uio)(bus_dma_tag_t, bus_dmamap_t,
		    struct uio *, int);
	int	(*_dmamap_load_raw)(bus_dma_tag_t, bus_dmamap_t,
		    bus_dma_segment_t *, int, bus_size_t, int);
	void	(*_dmamap_unload)(bus_dma_tag_t, bus_dmamap_t);
	void	(*_dmamap_sync)(bus_dma_tag_t, bus_dmamap_t,
		    bus_addr_t, bus_size_t, int);

	/*
	 * DMA memory utility functions.
	 */
	int	(*_dmamem_alloc)(bus_dma_tag_t, bus_size_t, bus_size_t,
		    bus_size_t, bus_dma_segment_t *, int, int *, int);
	void	(*_dmamem_free)(bus_dma_tag_t, bus_dma_segment_t *, int);
	int	(*_dmamem_map)(bus_dma_tag_t, bus_dma_segment_t *,
		    int, size_t, void **, int);
	void	(*_dmamem_unmap)(bus_dma_tag_t, void *, size_t);
	paddr_t	(*_dmamem_mmap)(bus_dma_tag_t, bus_dma_segment_t *,
		    int, off_t, int, int);
};

static __inline void bus_dmamap_sync(bus_dma_tag_t, bus_dmamap_t,
    bus_addr_t, bus_size_t, int) __attribute__((__unused__));

#define	bus_dmamap_create(t, s, n, m, b, f, p)			\
	(*(t)->_dmamap_create)((t), (s), (n), (m), (b), (f), (p))
#define	bus_dmamap_destroy(t, p)				\
	(*(t)->_dmamap_destroy)((t), (p))
#define	bus_dmamap_load(t, m, b, s, p, f)			\
	(*(t)->_dmamap_load)((t), (m), (b), (s), (p), (f))
#define	bus_dmamap_load_mbuf(t, m, b, f)			\
	(*(t)->_dmamap_load_mbuf)((t), (m), (b), (f))
#define	bus_dmamap_load_uio(t, m, u, f)				\
	(*(t)->_dmamap_load_uio)((t), (m), (u), (f))
#define	bus_dmamap_load_raw(t, m, sg, n, s, f)			\
	(*(t)->_dmamap_load_raw)((t), (m), (sg), (n), (s), (f))
#define	bus_dmamap_unload(t, p)					\
	(*(t)->_dmamap_unload)((t), (p))
static __inline void
bus_dmamap_sync(bus_dma_tag_t t, bus_dmamap_t p, bus_addr_t o, bus_size_t l,
    int ops)
{
	/* if (ops & BUS_DMASYNC_POSTREAD)
	   x86_lfence(); */
	if (t->_dmamap_sync)
		(*t->_dmamap_sync)(t, p, o, l, ops);
}

#define	bus_dmamem_alloc(t, s, a, b, sg, n, r, f)		\
	(*(t)->_dmamem_alloc)((t), (s), (a), (b), (sg), (n), (r), (f))
#define	bus_dmamem_free(t, sg, n)				\
	(*(t)->_dmamem_free)((t), (sg), (n))
#define	bus_dmamem_map(t, sg, n, s, k, f)			\
	(*(t)->_dmamem_map)((t), (sg), (n), (s), (k), (f))
#define	bus_dmamem_unmap(t, k, s)				\
	(*(t)->_dmamem_unmap)((t), (k), (s))
#define	bus_dmamem_mmap(t, sg, n, o, p, f)			\
	(*(t)->_dmamem_mmap)((t), (sg), (n), (o), (p), (f))

/*
 *	bus_dmamap_t
 *
 *	Describes a DMA mapping.
 */
struct ia64_bus_dmamap {
	/*
	 * PRIVATE MEMBERS: not for use by machine-independent code.
	 */
	bus_size_t	_dm_size;	/* largest DMA transfer mappable */
	int		_dm_segcnt;	/* number of segs this map can map */
	bus_size_t	_dm_maxmaxsegsz; /* fixed largest possible segment */
	bus_size_t	_dm_boundary;	/* don't cross this */
	bus_addr_t	_dm_bounce_thresh; /* bounce threshold; see tag */
	int		_dm_flags;	/* misc. flags */

	void		*_dm_cookie;	/* cookie for bus-specific functions */

	/*
	 * PUBLIC MEMBERS: these are used by machine-independent code.
	 */
	bus_size_t	dm_maxsegsz;	/* largest possible segment */
	bus_size_t	dm_mapsize;	/* size of the mapping */
	int		dm_nsegs;	/* # valid segments in mapping */
	bus_dma_segment_t dm_segs[1];	/* segments; variable length */
};

#endif /* _IA64_BUS_H_ */
