/*	$NetBSD: db_memrw.c,v 1.2 2003/07/15 03:36:00 lukem Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon 
 * the rights to redistribute these changes.
 */

/*
 * Interface to the debugger for virtual memory read/write.
 * This is a simple version for kernels with writable text.
 * For an example of read-only kernel text, see the file:
 * sys/arch/sun3/sun3/db_memrw.c
 *
 * ALERT!  If you want to access device registers with a
 * specific size, then the read/write functions have to
 * make sure to do the correct sized pointer access.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_memrw.c,v 1.2 2003/07/15 03:36:00 lukem Exp $");

#include <sys/param.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

#include <machine/db_machdep.h>

#include <ddb/db_access.h>

/*
 * Read bytes from kernel address space for debugger.
 */
void
db_read_bytes(db_addr_t addr, size_t size, char *data)
{
	char *src = (char *)addr;

	if (size == 8) {
		*((register_t*)data) = *((register_t*)src);
		return;
	}

	if (size == 4) {
		*((int*)data) = *((int*)src);
		return;
	}

	if (size == 2) {
		*((short*)data) = *((short*)src);
		return;
	}

	while (size > 0) {
		--size;
		*data++ = *src++;
	}
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(db_addr_t addr, size_t size, char *data)
{
	char *dst = (char *)addr;
	extern char etext[];
	int in_text = 0;

	if (dst < etext && (dst + size) >= (char *)SH5_KSEG0_BASE) {
		char *dp;
		for (dp = dst; dp < &dst[size]; dp += 32)
			asm volatile("ocbp %0, 0; icbi %0, 0; synci"
			    : : "r"(dp));
		in_text = 1;
	}

	if (size == 8) {
		*((register_t*)dst) = *((register_t*)data);
		if (in_text)
			asm volatile("ocbp %0, 0; synci" :: "r"(dst));
		return;
	}

	if (size == 4) {
		*((int*)dst) = *((int*)data);
		if (in_text)
			asm volatile("ocbp %0, 0; synci" :: "r"(dst));
		return;
	}

	if (size == 2) {
		*((short*)dst) = *((short*)data);
		if (in_text)
			asm volatile("ocbp %0, 0; synci" :: "r"(dst));
		return;
	}

	while (size > 0) {
		--size;
		*dst = *data++;
		if (in_text)
			asm volatile("ocbp %0, 0; synci" :: "r"(dst));
		dst++;
	}
}
