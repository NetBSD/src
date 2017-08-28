/*	$NetBSD: db_disasm.c,v 1.21.6.1 2017/08/28 17:51:55 skrll Exp $ */
/*
 * Copyright (c) 1996 Ludd, University of Lule}, Sweden.
 * All rights reserved.
 *
 * This code is derived from software contributed to Ludd by
 * Bertram Barth.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_disasm.c,v 1.21.6.1 2017/08/28 17:51:55 skrll Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_variables.h>
#include <ddb/db_interface.h>
#include <ddb/db_output.h>

#include <vax/vax/db_disasm.h>

#ifdef VMS_MODE
#define DEFERRED   '@'
#define LITERAL	   '#'
#else
#define DEFERRED   '*'
#define LITERAL	   '$'
#endif
/*
 * disassembling vax instructions works as follows:
 *
 * 1.	get first byte as opcode (check for two-byte opcodes!)
 * 2.	lookup in op-table for mnemonic and operand-list
 * 2.a	store the mnemonic
 * 3.	for each operand in list: get the size/type
 * 3.a	evaluate addressing mode for this operand
 * 3.b	store each operand(s)
 * 4.	db_printf the opcode and the (value of the) operands
 * 5.	return the start of the next instruction
 *
 * - if jump/branch calculate (and display) the target-address
 */

/* 
#define BROKEN_DB_REGS
*/
#ifdef	BROKEN_DB_REGS
const struct {		/* Due to order and contents of db_regs[], we can't */
	const char *name;	/* use this array to extract register-names. */
	void *valuep;	/* eg. "psl" vs "pc", "pc" vs "sp" */
} my_db_regs[16] = {
	{ "r0",		NULL },
	{ "r1",		NULL },
	{ "r2",		NULL },
	{ "r3",		NULL },
	{ "r4",		NULL },
	{ "r5",		NULL },
	{ "r6",		NULL },
	{ "r7",		NULL },
	{ "r8",		NULL },
	{ "r9",		NULL },
	{ "r10",	NULL },
	{ "r11",	NULL },
	{ "ap",		NULL },		/* aka "r12" */
	{ "fp",		NULL },		/* aka "r13" */
	{ "sp",		NULL },		/* aka "r14" */
	{ "pc",		NULL },		/* aka "r15" */
};
#else
#define my_db_regs db_regs
#endif

typedef struct {
	char		dasm[256];	/* disassebled instruction as text */
	char	       *curp;	/* pointer into result */
	char	       *ppc;	/* pseudo PC */
	int		opc;	/* op-code */
	const char	*argp;	/* pointer into argument-list */
	int		itype;	/* instruction-type, eg. branch, call, unspec */
	int		atype;	/* argument-type, eg. byte, long, address */
	int		off;	/* offset specified by last argument */
	int		addr;	/* address specified by last argument */
}	inst_buffer;

#define ITYPE_INVALID  -1
#define ITYPE_UNSPEC	0
#define ITYPE_BRANCH	1
#define ITYPE_CALL	2

static inline int get_byte(inst_buffer * ib);
static inline int get_word(inst_buffer * ib);
static inline int get_long(inst_buffer * ib);

static int get_opcode(inst_buffer * ib);
static int get_operands(inst_buffer * ib);
static int get_operand(inst_buffer * ib, int size);

static inline void add_char(inst_buffer * ib, char c);
static inline void add_str(inst_buffer * ib, const char *s);
static void add_int(inst_buffer * ib, int i);
static void add_xint(inst_buffer * ib, int i);
static void add_sym(inst_buffer * ib, int i);
static void add_off(inst_buffer * ib, int i);

#define err_print  printf

/*
 * Disassemble instruction at 'loc'.  'altfmt' specifies an
 * (optional) alternate format (altfmt for vax: don't assume
 * that each external label is a procedure entry mask).
 * Return address of start of next instruction.
 * Since this function is used by 'examine' and by 'step'
 * "next instruction" does NOT mean the next instruction to
 * be executed but the 'linear' next instruction.
 */
db_addr_t
db_disasm(db_addr_t loc, bool altfmt)
{
	db_expr_t	diff;
	db_sym_t	sym;
	const char	*symname;

	inst_buffer	ib;

	memset(&ib, 0, sizeof(ib));
	ib.ppc = (void *) loc;
	ib.curp = ib.dasm;

	if (!altfmt) {		/* ignore potential entry masks in altfmt */
		diff = INT_MAX;
		symname = NULL;
		sym = db_search_symbol(loc, DB_STGY_PROC, &diff);
		db_symbol_values(sym, &symname, 0);

		if (symname && !diff) { /* symbol at loc */
			db_printf("function \"%s()\", entry-mask 0x%x\n\t\t",
				  symname, (unsigned short) get_word(&ib));
			ib.ppc += 2;
		}
	}
	get_opcode(&ib);
	get_operands(&ib);
	db_printf("%s\n", ib.dasm);

	return ((u_int) ib.ppc);
}

int
get_opcode(inst_buffer *ib)
{
	ib->opc = get_byte(ib);
	if (ib->opc >> 2 == 0x3F) {	/* two byte op-code */
		ib->opc = ib->opc << 8;
		ib->opc += get_byte(ib);
	}
	switch (ib->opc) {
	case 0xFA:		/* CALLG */
	case 0xFB:		/* CALLS */
	case 0xFC:		/* XFC */
		ib->itype = ITYPE_CALL;
		break;
	case 0x16:		/* JSB */
	case 0x17:		/* JMP */
		ib->itype = ITYPE_BRANCH;
		break;
	default:
		ib->itype = ITYPE_UNSPEC;
	}
	if (ib->opc < 0 || ib->opc > 0xFF) {
		add_str(ib, "invalid or two-byte opcode ");
		add_xint(ib, ib->opc);
		ib->itype = ITYPE_INVALID;
	} else {
		add_str(ib, vax_inst[ib->opc].mnemonic);
		add_char(ib, '\t');
	}
	return (ib->opc);
}

int
get_operands(inst_buffer *ib)
{
	int		aa = 0; /* absolute address mode ? */
	int		size;

	if (ib->opc < 0 || ib->opc > 0xFF) {
		/* invalid or two-byte opcode */
		ib->argp = NULL;
		return (-1);
	}
	ib->argp = vax_inst[ib->opc].argdesc;
	if (ib->argp == NULL)
		return 0;

	while (*ib->argp) {
		switch (*ib->argp) {

		case 'b':	/* branch displacement */
			switch (*(++ib->argp)) {
			case 'b':
				ib->off = (signed char) get_byte(ib);
				break;
			case 'w':
				ib->off = (short) get_word(ib);
				break;
			case 'l':
				ib->off = get_long(ib);
				break;
			default:
				err_print("XXX eror\n");
			}
			/* add_int(ib, ib->off); */
			ib->addr = (u_int) ib->ppc + ib->off;
			add_off(ib, ib->addr);
			break;

		case 'a':	/* absolute addressing mode */
			aa = 1; /* do not break here ! */

		default:
			switch (*(++ib->argp)) {
			case 'b':	/* Byte */
				size = SIZE_BYTE;
				break;
			case 'w':	/* Word */
				size = SIZE_WORD;
				break;
			case 'l':	/* Long-Word */
			case 'f':	/* F_Floating */
				size = SIZE_LONG;
				break;
			case 'q':	/* Quad-Word */
			case 'd':	/* D_Floating */
			case 'g':	/* G_Floating */
				size = SIZE_QWORD;
				break;
			case 'o':	/* Octa-Word */
			case 'h':	/* H_Floating */
				size = SIZE_OWORD;
				break;
			default:
				err_print("invalid op-type %X (%c) found.\n",
					  *ib->argp, *ib->argp);
				size = 0;
			}
			if (aa) {
				/* get the address */
				ib->addr = get_operand(ib, size);
				add_sym(ib, ib->addr);
			} else {
				/* get the operand */
				ib->addr = get_operand(ib, size);
				add_off(ib, ib->addr);
			}
		}

		if (!*ib->argp || !*++ib->argp)
			break;
		if (*ib->argp++ == ',') {
			add_char(ib, ',');
			add_char(ib, ' ');
		} else {
			err_print("XXX error\n");
			add_char(ib, '\0');
			return (-1);
		}
	}

	add_char(ib, '\0');
	return (0);
}

int
get_operand(inst_buffer *ib, int size)
{
	int		c = get_byte(ib);
	int		mode = c >> 4;
	int		reg = c & 0x0F;
	int		lit = c & 0x3F;
	int		tmp = 0;
	char		buf[16];

	switch (mode) {
	case 0:		/* literal */
	case 1:		/* literal */
	case 2:		/* literal */
	case 3:		/* literal */
		add_char(ib, LITERAL);
		add_int(ib, lit);
		tmp = lit;
		break;

	case 4:		/* indexed */
		snprintf(buf, sizeof(buf), "[%s]", my_db_regs[reg].name);
		get_operand(ib, 0);
		add_str(ib, buf);
		break;

	case 5:		/* register */
		add_str(ib, my_db_regs[reg].name);
		break;

	case 6:		/* register deferred */
		add_char(ib, '(');
		add_str(ib, my_db_regs[reg].name);
		add_char(ib, ')');
		break;

	case 7:		/* autodecrement */
		add_char(ib, '-');
		add_char(ib, '(');
		add_str(ib, my_db_regs[reg].name);
		add_char(ib, ')');
		if (reg == 0x0F) {	/* pc is not allowed in this mode */
			err_print("autodecrement not allowd for PC.\n");
		}
		break;

	case 9:		/* autoincrement deferred */
		add_char(ib, DEFERRED);
		if (reg == 0x0F) {	/* pc: immediate deferred */
			/*
			 * addresses are always longwords!
			 */
			tmp = get_long(ib);
			add_off(ib, tmp);
			break;
		}
		/* fall through */
	case 8:		/* autoincrement */
		if (reg == 0x0F) {	/* pc: immediate ==> special syntax */
			switch (size) {
			case SIZE_BYTE:
				tmp = (signed char) get_byte(ib);
				break;
			case SIZE_WORD:
				tmp = (signed short) get_word(ib);
				break;
			case SIZE_LONG:
				tmp = get_long(ib);
				break;
			default:
				err_print("illegal op-type %d\n", size);
				tmp = -1;
			}
			if (mode == 8)
				add_char(ib, LITERAL);
			add_int(ib, tmp);
			break;
		}
		add_char(ib, '(');
		add_str(ib, my_db_regs[reg].name);
		add_char(ib, ')');
		add_char(ib, '+');
		break;

	case 11:	/* byte displacement deferred/ relative deferred  */
		add_char(ib, DEFERRED);
	case 10:	/* byte displacement / relative mode */
		tmp = (signed char) get_byte(ib);
		if (reg == 0x0F) {
			add_off(ib, (u_int) ib->ppc + tmp);
			break;
		}
		/* add_str (ib, "b^"); */
		add_int(ib, tmp);
		add_char(ib, '(');
		add_str(ib, my_db_regs[reg].name);
		add_char(ib, ')');
		break;

	case 13:		/* word displacement deferred */
		add_char(ib, DEFERRED);
	case 12:		/* word displacement */
		tmp = (signed short) get_word(ib);
		if (reg == 0x0F) {
			add_off(ib, (u_int) ib->ppc + tmp);
			break;
		}
		/* add_str (ib, "w^"); */
		add_int(ib, tmp);
		add_char(ib, '(');
		add_str(ib, my_db_regs[reg].name);
		add_char(ib, ')');
		break;

	case 15:		/* long displacement referred */
		add_char(ib, DEFERRED);
	case 14:		/* long displacement */
		tmp = get_long(ib);
		if (reg == 0x0F) {
			add_off(ib, (u_int) ib->ppc + tmp);
			break;
		}
		/* add_str (ib, "l^"); */
		add_int(ib, tmp);
		add_char(ib, '(');
		add_str(ib, my_db_regs[reg].name);
		add_char(ib, ')');
		break;

	default:
		err_print("can\'t evaluate operand (%02X).\n", lit);
		break;
	}

	return (0);
}

int
get_byte(inst_buffer *ib)
{
	return ((unsigned char) *(ib->ppc++));
}

int
get_word(inst_buffer *ib)
{
	int tmp = *(uint16_t *)ib->ppc;
	ib->ppc += 2;
	return tmp;
}

int
get_long(inst_buffer *ib)
{
	int tmp = *(int *)ib->ppc;
	ib->ppc += 4;
	return (tmp);
}

void
add_char(inst_buffer *ib, char c)
{
	*ib->curp++ = c;
}

void
add_str(inst_buffer *ib, const char *s)
{
	while ((*ib->curp++ = *s++));
	--ib->curp;
}

void
add_int(inst_buffer *ib, int i)
{
	char buf[32];
	if (i < 100 && i > -100)
		snprintf(ib->curp, sizeof(buf), "%d", i);
	else
		snprintf(buf, sizeof(buf), "0x%x", i);
	add_str(ib, buf);
}

void
add_xint(inst_buffer *ib, int val)
{
	char buf[32];
	snprintf(buf, sizeof(buf), "0x%x", val);
	add_str(ib, buf);
}

void
add_sym(inst_buffer *ib, int loc)
{
	db_expr_t	diff;
	db_sym_t	sym;
	const char	*symname;

	if (!loc)
		return;

	diff = INT_MAX;
	symname = NULL;
	sym = db_search_symbol(loc, DB_STGY_ANY, &diff);
	db_symbol_values(sym, &symname, 0);

	if (symname && !diff) {
		/* add_char(ib, '<'); */
		add_str(ib, symname);
		/* add_char(ib, '>'); */
	} else
		add_xint(ib, loc);
}

void
add_off(inst_buffer *ib, int loc)
{
	db_expr_t	diff;
	db_sym_t	sym;
	const char	*symname;

	if (!loc)
		return;
	  
	diff = INT_MAX;
	symname = NULL;
	sym = db_search_symbol(loc, DB_STGY_ANY, &diff);
	db_symbol_values(sym, &symname, 0);

	if (symname) {
		/* add_char(ib, '<'); */
		add_str(ib, symname);
		if (diff) {
			add_char(ib, '+');
			add_xint(ib, diff);
		}
		/* add_char(ib, '>'); */
	} else
		add_xint(ib, loc);
}
