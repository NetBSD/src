/*	$NetBSD: kgdb.h,v 1.1 1996/10/09 07:45:06 matthias Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1995
 *	The President and Fellows of Harvard College. All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgements:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 *	This product includes software developed by Harvard University.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <machine/db_machdep.h>

/*
 * Message types.
 */
#define KGDB_MEM_R	'm'
#define KGDB_MEM_W	'M'
#define KGDB_REG_R	'g'
#define KGDB_REG_W	'G'
#define KGDB_CONT	'c'
#define KGDB_STEP	's'
#define KGDB_KILL	'k'
#define KGDB_SIGNAL	'?'
#define KGDB_DEBUG	'd'

/*
 * start of frame/end of frame
 */
#define KGDB_START	'$'
#define KGDB_END	'#'
#define KGDB_GOODP	'+'
#define KGDB_BADP	'-'

/*
 * Functions and variables exported from kgdb_stub.c
 */
void kgdb_attach __P((int (*)(void *), void (*)(void *, int), void *ioarg));
void kgdb_connect __P((int));
void kgdb_panic __P(());
int kgdb_trap __P((int, db_regs_t *));
extern int kgdb_dev, kgdb_rate, kgdb_active, kgdb_debug_init, kgdb_debug_panic;

/*
 * Machine dependent functions needed by kgdb_stub.c
 */
void db_read_bytes __P((vm_offset_t, size_t, char *));
void db_write_bytes __P((vm_offset_t, size_t, char *));
int kgdb_signal __P((int));
int kgdb_acc __P((vm_offset_t, size_t));
void kgdb_getregs __P((db_regs_t *, kgdb_reg_t *));
void kgdb_setregs __P((db_regs_t *, kgdb_reg_t *));
