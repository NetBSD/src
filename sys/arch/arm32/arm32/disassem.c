/* $NetBSD: disassem.c,v 1.5 1996/10/11 00:06:38 christos Exp $ */

/*
 * Copyright (c) 1994 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * disassem.c
 *
 * Debug / Monitor disassembler
 *
 * Created      : 09/10/94
 */

/* Special compilation symbols
 *
 * DISASSEM_COLOUR - Use colour in dissassembly
 */
/*#define DISASSEM_COLOUR */

/* Include standard header files */

#include <sys/param.h>
#include <sys/systm.h>

/* Local header files */

#include <machine/katelib.h>

typedef u_int (*instruction_decoder) (u_int addr, u_int word);

typedef struct _opcodes {
	u_int mask;
	u_int pattern;
	u_int colour;
	instruction_decoder decoder;
} opcodes_struct;

static u_int instruction_swi		__P((u_int addr, u_int word));
static u_int instruction_branch		__P((u_int addr, u_int word));
static u_int instruction_mul		__P((u_int addr, u_int word));
static u_int instruction_mla		__P((u_int addr, u_int word));
static u_int instruction_ldrstr		__P((u_int addr, u_int word));
static u_int instruction_ldmstm		__P((u_int addr, u_int word));
static u_int instruction_dataproc	__P((u_int addr, u_int word));
static u_int instruction_swap		__P((u_int addr, u_int word));
static u_int instruction_mrs		__P((u_int addr, u_int word));
static u_int instruction_msr		__P((u_int addr, u_int word));
static u_int instruction_msrf		__P((u_int addr, u_int word));
static u_int instruction_mrcmcr		__P((u_int addr, u_int word));
static u_int instruction_cdp		__P((u_int addr, u_int word));
static u_int instruction_cdt		__P((u_int addr, u_int word));
static u_int instruction_fpabinop	__P((u_int addr, u_int word));
static u_int instruction_fpaunop	__P((u_int addr, u_int word));
static u_int instruction_ldfstf		__P((u_int addr, u_int word));

/* Declare global variables */

opcodes_struct opcodes[] = {
    { 0x0f000000, 0x0f000000, 7, instruction_swi },
    { 0x0e000000, 0x0a000000, 7, instruction_branch },
    { 0x0fe000f0, 0x00000090, 7, instruction_mul },
    { 0x0fe000f0, 0x00200090, 7, instruction_mla },
    { 0x0e000000, 0x04000000, 7, instruction_ldrstr },
    { 0x0c000010, 0x04000000, 7, instruction_ldrstr },
    { 0x0e000000, 0x08000000, 6, instruction_ldmstm },
    { 0x0FB00FF0, 0x01000090, 7, instruction_swap },
    { 0x0FBF0FFF, 0x010F0000, 1, instruction_mrs },
    { 0x0DBFFFF0, 0x0129F000, 1, instruction_msr },
    { 0x0DBFFFF0, 0x0128F000, 1, instruction_msrf },
    { 0x0C000000, 0x00000000, 7, instruction_dataproc },
    { 0x0F008F10, 0x0E000100, 3, instruction_fpabinop },
    { 0x0F008F10, 0x0E008100, 3, instruction_fpaunop },
    { 0x0E000F00, 0x0C000100, 3, instruction_ldfstf },
    { 0x0F000010, 0x0E000010, 2, instruction_mrcmcr },
    { 0x0F000010, 0x0E000000, 2, instruction_cdp },
    { 0x0E000000, 0x0C000000, 2, instruction_cdt },
    { 0x00000000, 0x00000000, 0, NULL }
};

char *opcode_conditions[] = {
    "EQ",
    "NE",
    "CS",
    "CC",
    "MI",
    "PL",
    "VS",
    "VC",
    "HI",
    "LS",
    "GE",
    "LT",
    "GT",
    "LE",
    "",
    "NV"
};

char *opcode_data_procs[] = {
    "AND",
    "EOR",
    "SUB",
    "RSB",
    "ADD",
    "ADC",
    "SBC",
    "RSC",
    "TST",
    "TEQ",
    "CMP",
    "CMN",
    "ORR",
    "MOV",
    "BIC",
    "MVN"
};

char *opcode_shifts[] = {
    "LSL",
    "LSR",
    "ASR",
    "ROR"
};

char *opcode_block_transfers[] = {
    "DA",
    "IA",
    "DB",
    "IB"
};

char *opcode_stack_block_transfers[] = {
    "FA",
    "EA",
    "FD",
    "FA"
};

char *opcode_fpabinops[] = {
    "ADF",
    "MUF",
    "SUF",
    "RSF",
    "DVF",
    "RDF",
    "POW",
    "RPW",
    "RMF",
    "FML",
    "FDV",
    "FRD",
    "POL",
    "???",
    "???",
    "???",
    "???"
};

char *opcode_fpaunops[] = {
    "MVF",
    "MNF",
    "ABS",
    "RND",
    "SQT",
    "LOG",
    "LGN",
    "EXP",
    "SIN",
    "COS",
    "TAN",
    "ASN",
    "ACS",
    "ATN",
    "???",
    "???",
    "???"
};

char *opcode_fpaconstants[] = {
    "0.0",
    "1.0",
    "2.0",
    "3.0",
    "4.0",
    "5.0",
    "0.5",
    "10.0"
};

char *opcode_fpa_rounding[] = {
    "",
    "P",
    "M",
    "Z"
};

char *opcode_fpa_precision[] = {
    "S",
    "D",
    "E",
    "P"
};

#define opcode_condition(x) opcode_conditions[x >> 28]

#define opcode_s(x) ((x & 0x00100000) ? "S" : "")

#define opcode_b(x) ((x & 0x00400000) ? "B" : "")

#define opcode_t(x) ((x & 0x01200000) == 0x00200000 ? "T" : "")

#define opcode_dataproc(x) opcode_data_procs[(x >> 21) & 0x0f]

#define opcode_shift(x) opcode_shifts[(x >> 5) & 3]

#define opcode_blktrans(x) opcode_block_transfers[(x >> 23) & 3]

#define opcode_stkblktrans(x) opcode_stack_block_transfers[(x >> 23) & 3]

#define opcode_fpabinop(x) opcode_fpabinops[(x >> 20) & 0x0f]

#define opcode_fpaunop(x) opcode_fpaunops[(x >> 20) & 0x0f]

#define opcode_fpaimm(x) opcode_fpaconstants[x & 0x07]

#define opcode_fparnd(x) opcode_fpa_rounding[(x >> 5) & 0x03]

#define opcode_fpaprec(x) opcode_fpa_precision[(((x >> 18) & 2)|(x >> 7)) & 3]

/* Declare external variables */

extern caddr_t shell_ident;

/* Local function prototypes */

u_int disassemble	__P((u_char *addr));

/* Now for the main code */

static void
printascii(byte)
	int byte;
{
	byte &= 0x7f;
	if (byte < 0x20)
#ifdef DISASSEM_COLOUR
		kprintf("\x1b[31m%c\x1b[0m", byte + '@');
#else
		kprintf("%c", byte + '@');
#endif
	else if (byte == 0x7f)
#ifdef DISASSEM_COLOUR
		kprintf("\x1b[31m?\x1b[0m");
#else
		kprintf("?");
#endif
	else
		kprintf("%c", byte);
}


u_int
disassemble(addr)
	u_char *addr;
{
	int loop;
	u_int word;
	u_int result = 0;

	kprintf("%08x : ", (u_int)addr);

	word = *((u_int *)addr);

	for (loop = 0; loop < 4; ++loop)
		printascii(addr[loop]);

	kprintf(" : %08x :    ", word);

	loop = 0;

	while (opcodes[loop].mask != 0) {
		if ((word & opcodes[loop].mask) == opcodes[loop].pattern) {
#ifdef DISASSEM_COLOUR
			kprintf("\x1b[3%dm", opcodes[loop].colour);
#endif
			result = (*opcodes[loop].decoder)((u_int )addr, word);
#ifdef DISASSEM_COLOUR
			kprintf("\x1b[0m");
#endif
			break;
		}
		++loop;
	}

	if (opcodes[loop].mask == 0)
		kprintf("Undefined instruction");

	kprintf("\n\r");
	return(result);
}


static u_int
instruction_swi(addr, word)
	u_int addr;
	u_int word;
{
	kprintf("SWI%s\t0x%08x", opcode_condition(word), (word & 0x00ffffff));
	return(addr);
}


static u_int
instruction_branch(addr, word)
	u_int addr;
	u_int word;
{
	u_int branch;

	branch = ((word << 2) & 0x03ffffff);
	if (branch & 0x02000000)
		branch |= 0xfc000000;

	branch += addr + 8;

	if (word & 0x01000000)
		kprintf("BL%s\t0x%08x", opcode_condition(word), branch);
	else
		kprintf("B%s\t0x%08x", opcode_condition(word), branch);

	return(branch);
}


static u_int
instruction_mul(addr, word)
	u_int addr;
	u_int word;
{
	kprintf("MUL%s%s\t", opcode_condition(word), opcode_s(word));

	kprintf("r%d, r%d, r%d", (word >> 16) & 0x0f, word & 0x0f,
		(word >> 8) & 0x0f);
	return(addr);
}


static u_int
instruction_mla(addr, word)
	u_int addr;
	u_int word;
{
	kprintf("MLA%s%s\t", opcode_condition(word), opcode_s(word));

	kprintf("r%d, r%d, r%d, r%d", (word >> 16) & 0x0f, word & 0x0f,
		(word >> 8) & 0x0f, (word >> 12) & 0x0f);
	return(addr);
}


static void
register_shift(word)
	u_int word;
{
	kprintf("r%d", (word & 0x0f));
	if ((word & 0x00000ff0) == 0)
		;
	else if ((word & 0x00000ff0) == 0x00000060)
 		kprintf(", RRX");
	else {
		if (word & 0x10)
			kprintf(", %s r%d", opcode_shift(word), (word >> 8) & 0x0f);
		else
			kprintf(", %s #%d", opcode_shift(word), (word >> 7) & 0x1f);
	}
}


static u_int
instruction_ldrstr(addr, word)
	u_int addr;
	u_int word;
{
    kprintf("%s%s%s%s\t", (word & 0x00100000) ? "LDR" : "STR",
      opcode_condition(word), opcode_b(word), opcode_t(word));

    kprintf("r%d, ", (word >> 12) & 0x0f);

    if (((word >> 16) & 0x0f) == 16)
      {
/*        u_int location;

        location = addr + 8;

        addr = */
      }
    else
      {
        kprintf("[r%d", (word >> 16) & 0x0f);

        kprintf("%s, ", (word & (1 << 24)) ? "" : "]");

        if (!(word & 0x00800000))
          kprintf("-");

        if (word & (1 << 25))
          register_shift(word);
        else
          kprintf("#0x%04x", word & 0xfff);

        if (word & (1 << 24))
          kprintf("]");

        if (word & (1 << 21))
          kprintf("!");
      }

    return(addr);
}


static u_int
instruction_ldmstm(addr, word)
	u_int addr;
	u_int word;
{
    int loop;
    int start;

    kprintf("%s%s%s\t", (word & 0x00100000) ? "LDM" : "STM",
      opcode_condition(word), opcode_blktrans(word));

    kprintf("r%d", (word >> 16) & 0x0f);

    if (word & (1 << 21))
      kprintf("!");

    kprintf(", {");

    start = -1;

    for (loop = 0; loop < 17; ++loop)
      {
        if (start != -1)
          {
            if (!(word & (1 << loop)) || loop == 16)
              {
                if (start == loop - 1)
                  kprintf("r%d, ", start);
                else
                  kprintf("r%d-r%d, ", start, loop - 1);
                start = -1;
              }
          }
        else
          {
            if (word & (1 << loop))
              start = loop;
          }

      }
    kprintf("\x7f\x7f}");

    if (word & (1 << 22))
      kprintf("^");
    return(addr);
}


static u_int
instruction_dataproc(addr, word)
	u_int addr;
	u_int word;
{
    if ((word & 0x01800000) == 0x01000000)
      word = word & ~(1<<20);

    kprintf("%s%s%s\t", opcode_dataproc(word), opcode_condition(word),
      opcode_s(word));

    if ((word & 0x01800000) != 0x01000000)
      kprintf("r%d, ", (word >> 12) & 0x0f);

    if ((word & 0x01a00000) != 0x01a00000)
      kprintf("r%d, ", (word >> 16) & 0x0f);

    if (word & 0x02000000)
      {
        kprintf("#&%08x", (word & 0xff) << (((word >> 7) & 0x1e)));
      }
    else
      {
        register_shift(word);
      }
    return(addr);
}


static u_int
instruction_swap(addr, word)
	u_int addr;
	u_int word;
{
	kprintf("SWP%s%s\t", opcode_condition(word), opcode_b(word));

	kprintf("r%d, r%d, [r%d]", (word >> 12) & 0x0f, (word & 0x0f),
		(word >> 16) & 0x0f);
	return(addr);
}


static u_int
instruction_mrs(addr, word)
	u_int addr;
	u_int word;
{
	kprintf("MRS%s\tr%d, ", opcode_condition(word), (word >> 12) & 0x0f);

	kprintf("%s", (word & 0x00400000) ? "SPSR" : "CPSR");
	return(addr);
}


static u_int
instruction_msr(addr, word)
	u_int addr;
	u_int word;
{
	kprintf("MSR%s\t", opcode_condition(word));

	kprintf("%s, r%d", (word & 0x00400000) ? "SPSR" : "CPSR", word & 0x0f);

	return(addr);
}


static u_int
instruction_msrf(addr, word)
	u_int addr;
	u_int word;
{
	kprintf("MSR%s\t", opcode_condition(word));

	kprintf("%s_flg, ", (word & 0x00400000) ? "SPSR" : "CPSR");

	if (word & 0x02000000)
		kprintf("#0x%08x", (word & 0xff) << (32 - ((word >> 7) & 0x1e)));
	else
		kprintf("r%d", word &0x0f);
	return(addr);
}


static u_int
instruction_mrcmcr(addr, word)
	u_int addr;
	u_int word;
{
	kprintf("%s%s\t", (word & (1 << 20)) ? "MRC" : "MCR",
		opcode_condition(word));

	kprintf("CP #%d, %d, ", (word >> 8) & 0x0f, (word >> 21) & 0x07);

	kprintf("r%d, cr%d, cr%d", (word >> 12) & 0x0f, (word >> 16) & 0x0f,
		word & 0x0f);

	if (((word >> 5) & 0x07) != 0)
		kprintf(", %d", (word >> 5) & 0x07);

	return(addr);
}


static u_int
instruction_cdp(addr, word)
	u_int addr;
	u_int word;
{
	kprintf("CDP%s\t", opcode_condition(word));

	kprintf("CP #%d, %d, ", (word >> 8) & 0x0f, (word >> 20) & 0x0f);

	kprintf("cr%d, cr%d, cr%d", (word >> 12) & 0x0f, (word >> 16) & 0x0f,
		word & 0x0f);

	kprintf(", %d", (word >> 5) & 0x07);

	return(addr);
}


static u_int
instruction_cdt(addr, word)
	u_int addr;
	u_int word;
{
    kprintf("%s%s%s\t", (word & (1 << 20)) ? "LDC" : "STC",
      opcode_condition(word), (word & (1 << 22)) ? "L" : "");

    kprintf("CP #%d, cr%d, ", (word >> 8) & 0x0f, (word >> 12) & 0x0f);

    kprintf("[r%d", (word >> 16) & 0x0f);

    kprintf("%s, ", (word & (1 << 24)) ? "" : "]");

    if (!(word & (1 << 23)))
      kprintf("-");

    kprintf("#0x%02x", word & 0xff);

    if (word & (1 << 24))
      kprintf("]");

    if (word & (1 << 21))
      kprintf("!");

    return(addr);
}


static u_int
instruction_fpabinop(addr, word)
	u_int addr;
	u_int word;
{
	kprintf("%s%s%s%s\t", opcode_fpabinop(word), opcode_condition(word),
		opcode_fpaprec(word), opcode_fparnd(word));

	kprintf("f%d, f%d, ", (word >> 12) & 0x07, (word >> 16) & 0x07);

	if (word & (1 << 3))
		kprintf("#%s", opcode_fpaimm(word));
	else
		kprintf("f%d", word & 0x07);

	return(addr);
}


static u_int
instruction_fpaunop(addr, word)
	u_int addr;
	u_int word;
{
	kprintf("%s%s%s%s\t", opcode_fpaunop(word), opcode_condition(word),
		opcode_fpaprec(word), opcode_fparnd(word));

	kprintf("f%d, ", (word >> 12) & 0x07);

	if (word & (1 << 3))
		kprintf("#%s", opcode_fpaimm(word));
	else
		kprintf("f%d", word & 0x07);

	return(addr);
}


static u_int
instruction_ldfstf(addr, word)
	u_int addr;
	u_int word;
{
    kprintf("%s%s%s\t", (word & (1 << 20)) ? "LDF" : "STF",
      opcode_condition(word), (word & (1 << 22)) ? "L" : "");

    kprintf("f%d, [r%d", (word >> 12) & 0x07, (word >> 16) & 0x0f);

    kprintf("%s, ", (word & (1 << 24)) ? "" : "]");

    if (!(word & (1 << 23)))
      kprintf("-");

    kprintf("#0x%03x", (word & 0xff) << 2);

    if (word & (1 << 24))
      kprintf("]");

    if (word & (1 << 21))
      kprintf("!");

    return(addr);
}

/* End of disassem.c */
