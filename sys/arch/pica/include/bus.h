/*	$NetBSD: bus.h,v 1.3 1997/06/23 02:46:45 jonathan Exp $	*/

/*
 * Copyright (c) 1997 Jonathan Stone (hereinafter referred to as the author)
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Jonathan Stone for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _PICA_BUS_H_
#define _PICA_BUS_H_

/*
 * ISA  "I/O space" macros
 */
#include <machine/pio.h>

/*
 * I/O addresses (in bus space)
 */
typedef u_int32_t bus_io_addr_t;
typedef u_int32_t bus_io_size_t;

/*
 * Memory addresses (in bus space)
 */
typedef u_int32_t bus_mem_addr_t;
typedef u_int32_t bus_mem_size_t;

/*
 * Access methods for bus resources, I/O space, and memory space.
 */
typedef void *bus_chipset_tag_t;
typedef u_int32_t bus_io_handle_t;
typedef caddr_t bus_mem_handle_t;

#endif	/* _PICA_BUS_H_ */
