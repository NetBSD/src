/*	$NetBSD: bus.h,v 1.1 1997/06/08 05:10:25 jonathan Exp $	*/

/*
 * Copyright Notice:
 *
 * Copyright (c) 1995, 1996, 1997
 *     Jonathan R. Stone.  All rights reserved.
 *
 * License:
 *
 * This License applies to this software ("Software"), created
 * by Jonathan Stone ("Author").
 *
 * You may use, copy, modify and redistribute this Software without
 * charge, in either source code form, binary form, or both, on the
 * following conditions:
 *
 * 1.  (a) Binary code: (i) a complete copy of the above copyright notice
 * must be included within each copy of the Software in binary code form,
 * and (ii) a complete copy of the above copyright notice and all terms
 * of this License as presented here must be included within each copy of
 * all documentation accompanying or associated with binary code, in any
 * medium, along with a list of the software modules to which the license
 * applies.
 *
 * (b) Source Code: A complete copy of the above copyright notice and all
 * terms of this License as presented here must be included within: (i)
 * each copy of the Software in source code form, and (ii) each copy of
 * all accompanying or associated documentation, in any medium.
 *
 * 2. The following Acknowledgment must be used in communications
 * involving the Software as described below:
 *
 *      This product includes software developed by
 *      Jonathan Stone for the NetBSD Project.
 *
 * The Acknowledgment must be conspicuously and completely displayed
 * whenever the Software, or any software, products or systems containing
 * the Software, are mentioned in advertising, marketing, informational
 * or publicity materials of any kind, whether in print, electronic or
 * other media (except for information provided to support use of
 * products containing the Software by existing users or customers).
 *
 * 3. The name of the Author may not be used to endorse or promote
 * products derived from this Software without specific prior written
 * permission (conditions (1) and (2) above are not considered
 * endorsement or promotion).
 *
 * 4.  This license applies to: (a) all copies of the Software, whether
 * partial or whole, original or modified, and (b) your actions, and the
 * actions of all those who may act on your behalf.  All uses not
 * expressly permitted are reserved to the Author.
 *
 * 5.  Disclaimer.  THIS SOFTWARE IS MADE AVAILABLE BY THE AUTHOR TO THE
 * PUBLIC FOR FREE AND "AS IS.''  ALL USERS OF THIS FREE SOFTWARE ARE
 * SOLELY AND ENTIRELY RESPONSIBLE FOR THEIR OWN CHOICE AND USE OF THIS
 * SOFTWARE FOR THEIR OWN PURPOSES.  BY USING THIS SOFTWARE, EACH USER
 * AGREES THAT THE AUTHOR SHALL NOT BE LIABLE FOR DAMAGES OF ANY KIND IN
 * RELATION TO ITS USE OR PERFORMANCE.
 *
 * 6.  If you have a special need for a change in one or more of these
 * license conditions, please contact the Author via electronic mail to
 *
 *     jonathan@NetBSD.ORG
 *
 * or via the contact information on
 *
 *     http://www.NetBSD.ORG/People/Pages/jonathan.html
 */


/*
 * NetBSD machine-indepedent bus accessor macros/functions for Decstations.
 */
#ifndef _PMAX_BUS_H_
#define _PMAX_BUS_H_

/*
 * Bus address and size types
 */
typedef u_long bus_addr_t;
typedef u_long bus_size_t;

/*
 * Access types for bus resources and addresses.
 */
typedef int bus_space_tag_t;
typedef u_long bus_space_handle_t;


/*
 * Read or write a 1, 2, or 4-byte quantity from/to a bus-space
 * address, as defined by (space-tag,  handle, offset
 */
#define bus_space_read_1(t, h, o) \
	(*(volatile u_int8_t *)((h) + (o)))

#define bus_space_read_2(t, h, o) \
	(*(volatile u_int16_t *)((h) + (o)))

#define bus_space_read_4(t, h, o) \
	(*(volatile u_int32_t *)((h) + (o)))

#define bus_space_write_1(t, h, o, v) \
	do { ((void)(*(volatile u_int8_t *)((h) + (o)) = (v))); } while (0)

#define bus_space_write_2(t, h, o, v) \
	do { ((void)(*(volatile u_int16_t *)((h) + (o)) = (v))); } while (0)

#define bus_space_write_4(t, h, o, v) \
	do { ((void)(*(volatile u_int32_t *)((h) + (o)) = (v))); } while (0)

/*
 * Read `count'  1, 2, or 4-byte quantities from bus-space
 * address, defined by (space-tag,  handle, offset).
 * Copy to the specified buffer address.
 */
#define	bus_space_read_multi_1(t, h, o, a, c) \
    do {								\
    	register int __i ;						\
	for (__i = 0; i < (c); i++)					\
	  ((u_char *)(a))[__i] = bus_space_read_1(t, h, o);		\
    } while (0)


#define	bus_space_read_multi_2(t, h, o, a, c) \
    do {								\
    	register int __i ;						\
	for (__i = 0; i < (c); i++)					\
	  ((u_int16t_t *)(a))[__i] = bus_space_read_2(t, h, o);		\
    } while (0)

#define	bus_space_read_multi_4(t, h, o, a, c) \
    do {								\
    	register int __i ;						\
	for (__i = 0; i < (c); i++)					\
	  ((u_int32_t *)(a))[__i] = bus_space_read_4(t, h, o);		\
    } while (0)

/*
 * Write `count'  1, 2, or 4-byte quantities to a bus-space
 * address, defined by (space-tag,  handle, offset).
 * Copy from the specified buffer address.
 */
#define	bus_space_write_multi_1(t, h, o, a, c)  \
    do {								\
    	register int __i ;						\
	for (__i = 0; i < (c); i++)					\
	  bus_space_write_1(t, h, o, ((u_char *)(a))[__i]);		\
    } while (0)

#define	bus_space_write_multi_2(t, h, o, a, c)  \
    do {								\
    	register int __i ;						\
	for (__i = 0; i < (c); i++)					\
	  bus_space_write_2(t, h, o, ((u_int16_t *)(a))[__i]);		\
    } while (0)

#define	bus_space_write_multi_4(t, h, o, a, c)  \
    do {								\
    	register int __i ;						\
	for (__i = 0; i < (c); i++)					\
	  bus_space_write_4(t, h, o, ((u_int32_t *)(a))[__i]);		\
    } while (0)

/*
 * Copy `count' 1, 2, or 4-byte values from one bus-space address
 * (t,  h, o triple) to another.
 */
#define	bus_space_copy_multi_1(t, h1, h2, o1, o2, c) \
    do {								\
    	register int __i ;						\
	for (__i = 0; i < (c); i++)					\
	  bus_space_write_1(t, h1, o1, bus_space_read_1(t, h2, o2));	\
    } while (0)

#define	bus_space_copy_multi_2(t, h1, h2, o1, o2, c) \
    do {								\
    	register int __i ;						\
	for (__i = 0; i < (c); i++)					\
	  bus_space_write_2(t, h1, o1, bus_space_read_2(t, h2, o2));	\
    while (0)

#define	bus_space_copy_multi_4(t,  h1, h2, o1, o2, c) \
    do {								\
    	register int __i ;						\
	for (__i = 0; i < (c); i++)					\
	  bus_space_write_4(t, h1, o1, bus_space_read_4(t, h2, o2));	\
    } while (0)


/*
 * Bus-space barriers.
 * Since DECstation DMA is non-cache-coherent, we have to handle
 * consistency in software anyway (e.g., via bus -DMA, or by ensuring
 * that DMA buffers are referenced via  uncached address space.
 * For now, simply do CPU writebuffer flushes and export the flags
 * to  MI code.
 */
#define bus_space_barrier(t, h, o, l, f) \
	((void)  wbflush();

#define BUS_BARRIER_READ 	0x01
#define BUS_BARRIER_WRITE	0x02

#endif /* _PMAX_BUS_H_ */
