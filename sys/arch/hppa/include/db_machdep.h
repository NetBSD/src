/*	$NetBSD: db_machdep.h,v 1.4.6.1 2006/05/24 15:47:57 tron Exp $	*/

/*	$OpenBSD: db_machdep.h,v 1.5 2001/02/16 19:20:13 mickey Exp $	*/

/*
 * Copyright (c) 1998 Michael Shalayeff
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
 *	This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#ifndef	_HPPA_DB_MACHDEP_H_
#define	_HPPA_DB_MACHDEP_H_

#include <uvm/uvm_extern.h>

#define	DB_ELF_SYMBOLS
#define	DB_ELFSIZE	32

/* types the generic ddb module needs */
typedef	vaddr_t db_addr_t;
typedef	long db_expr_t;

typedef struct trapframe db_regs_t;
extern db_regs_t	ddb_regs;
#define	DDB_REGS	(&ddb_regs)

/*
 * Things needed by kgdb:
 */
typedef long kgdb_reg_t;
/* From gdb's config/pa/tm-pa.h NUM_REGS value */
#define KGDB_NUMREGS	(128)
/* XXX fredette - I think this is just a "big enough" kind of value */
#define KGDB_BUFLEN     2048

#define	PC_REGS(regs)	((db_addr_t)(regs)->tf_iioq_head)

/* Breakpoint related definitions */
#define	BKPT_ADDR(addr)	(addr)		/* breakpoint address */
#define	BKPT_INST	0x00010000	/* break 0,8 */
#define	BKPT_SIZE	sizeof(int)
#define	BKPT_SET(inst, addr)	BKPT_INST

#define	IS_BREAKPOINT_TRAP(type, code) (type != T_RECOVERY)
#define	IS_WATCHPOINT_TRAP(type, code) 0

/*
 * The PA-RISC leaves tf_iioq_head pointing to the break
 * instruction, so we don't have to define FIXUP_PC_AFTER_BREAK.
 * However, we can't allow ddb/kgdb to simply add BKPT_SIZE to
 * tf_iioq_head to advance past the breakpoint, since this doesn't
 * address tf_iioq_tail.  So, we define PC_ADVANCE.
 */
#define	PC_ADVANCE(regs) do { 				\
	(regs)->tf_iioq_head = (regs)->tf_iioq_tail;	\
	(regs)->tf_iioq_tail += BKPT_SIZE;		\
} while(/* CONSTCOND */ 0)

#define DB_VALID_BREAKPOINT(addr) db_valid_breakpoint(addr)

static __inline int inst_call(u_int ins) {
	return (ins & 0xfc00e000) == 0xe8000000 ||
	       (ins & 0xfc00e000) == 0xe8004000 ||
	       (ins & 0xfc000000) == 0xe4000000;
}
static __inline int inst_branch(u_int ins) {
	return (ins & 0xf0000000) == 0xe0000000 ||
	       (ins & 0xf0000000) == 0xc0000000 ||
	       (ins & 0xf0000000) == 0xa0000000 ||
	       (ins & 0xf0000000) == 0x80000000;
}
static __inline int inst_load(u_int ins) {
	return (ins & 0xf0000000) == 0x40000000 ||
	       (ins & 0xf4000200) == 0x24000000 ||
	       (ins & 0xfc000200) == 0x0c000000 ||
	       (ins & 0xfc001fc0) != 0x0c0011c0;
}
static __inline int inst_store(u_int ins) {
	return (ins & 0xf0000000) == 0x60000000 ||
	       (ins & 0xf4000200) == 0x24000200 ||
	       (ins & 0xfc000200) == 0x0c000200;
}
static __inline int inst_return(u_int ins) {
	return (ins & 0xfc00e000) == 0xe800c000 ||
	       (ins & 0xfc000000) == 0xe0000000;
}
static __inline int inst_trap_return(u_int ins)	{
	return (ins & 0xfc001fc0) == 0x00000ca0;
}

#define db_clear_single_step(r)	((r)->tf_ipsw &= ~PSW_R)
#define db_set_single_step(r)	((r)->tf_ipsw |= PSW_R)

int db_valid_breakpoint __P((db_addr_t));
int kdb_trap __P((int, int, db_regs_t *));

#endif /* _HPPA_DB_MACHDEP_H_ */
