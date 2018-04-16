#line 1 "rl78-decode.opc"
/* -*- c -*- */
/* Copyright (C) 2012-2018 Free Software Foundation, Inc.
   Contributed by Red Hat.
   Written by DJ Delorie.

   This file is part of the GNU opcodes library.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ansidecl.h"
#include "opcode/rl78.h"

static int trace = 0;

typedef struct
{
  RL78_Opcode_Decoded * rl78;
  int (* getbyte)(void *);
  void * ptr;
  unsigned char * op;
} LocalData;

#define ID(x) rl78->id = RLO_##x, rl78->lineno = __LINE__
#define OP(n,t,r,a) (rl78->op[n].type = t, \
		     rl78->op[n].reg = r,	     \
		     rl78->op[n].addend = a )
#define OPX(n,t,r1,r2,a) \
	(rl78->op[n].type = t, \
	rl78->op[n].reg = r1, \
	rl78->op[n].reg2 = r2, \
	rl78->op[n].addend = a )

#define W() rl78->size = RL78_Word

#define AU ATTRIBUTE_UNUSED

#define OP_BUF_LEN 20
#define GETBYTE() (ld->rl78->n_bytes < (OP_BUF_LEN - 1) ? ld->op [ld->rl78->n_bytes++] = ld->getbyte (ld->ptr): 0)
#define B ((unsigned long) GETBYTE())

#define SYNTAX(x) rl78->syntax = x

#define UNSUPPORTED() \
  rl78->syntax = "*unknown*"

#define RB(x) ((x)+RL78_Reg_X)
#define RW(x) ((x)+RL78_Reg_AX)

#define Fz	rl78->flags = RL78_PSW_Z
#define Fza	rl78->flags = RL78_PSW_Z | RL78_PSW_AC
#define Fzc	rl78->flags = RL78_PSW_Z | RL78_PSW_CY
#define Fzac	rl78->flags = RL78_PSW_Z | RL78_PSW_AC | RL78_PSW_CY
#define Fa	rl78->flags = RL78_PSW_AC
#define Fc	rl78->flags = RL78_PSW_CY
#define Fac	rl78->flags = RL78_PSW_AC | RL78_PSW_CY

#define IMMU(bytes)   immediate (bytes, 0, ld)
#define IMMS(bytes)   immediate (bytes, 1, ld)

static int
immediate (int bytes, int sign_extend, LocalData * ld)
{
  unsigned long i = 0;

  switch (bytes)
    {
    case 1:
      i |= B;
      if (sign_extend && (i & 0x80))
	i -= 0x100;
      break;
    case 2:
      i |= B;
      i |= B << 8;
      if (sign_extend && (i & 0x8000))
	i -= 0x10000;
      break;
    case 3:
      i |= B;
      i |= B << 8;
      i |= B << 16;
      if (sign_extend && (i & 0x800000))
	i -= 0x1000000;
      break;
    default:
      fprintf (stderr, "Programmer error: immediate() called with invalid byte count %d\n", bytes);
      abort();
    }
  return i;
}

#define DC(c)		OP (0, RL78_Operand_Immediate, 0, c)
#define DR(r)		OP (0, RL78_Operand_Register, RL78_Reg_##r, 0)
#define DRB(r)		OP (0, RL78_Operand_Register, RB(r), 0)
#define DRW(r)		OP (0, RL78_Operand_Register, RW(r), 0)
#define DM(r,a)		OP (0, RL78_Operand_Indirect, RL78_Reg_##r, a)
#define DM2(r1,r2,a)	OPX (0, RL78_Operand_Indirect, RL78_Reg_##r1, RL78_Reg_##r2, a)
#define DE()		rl78->op[0].use_es = 1
#define DB(b)		set_bit (rl78->op, b)
#define DCY()		DR(PSW); DB(0)
#define DPUSH()		OP (0, RL78_Operand_PreDec, RL78_Reg_SP, 0);

#define SC(c)		OP (1, RL78_Operand_Immediate, 0, c)
#define SR(r)		OP (1, RL78_Operand_Register, RL78_Reg_##r, 0)
#define SRB(r)		OP (1, RL78_Operand_Register, RB(r), 0)
#define SRW(r)		OP (1, RL78_Operand_Register, RW(r), 0)
#define SM(r,a)		OP (1, RL78_Operand_Indirect, RL78_Reg_##r, a)
#define SM2(r1,r2,a)	OPX (1, RL78_Operand_Indirect, RL78_Reg_##r1, RL78_Reg_##r2, a)
#define SE()		rl78->op[1].use_es = 1
#define SB(b)		set_bit (rl78->op+1, b)
#define SCY()		SR(PSW); SB(0)
#define COND(c)		rl78->op[1].condition = RL78_Condition_##c
#define SPOP()		OP (1, RL78_Operand_PostInc, RL78_Reg_SP, 0);

static void
set_bit (RL78_Opcode_Operand *op, int bit)
{
  op->bit_number = bit;
  switch (op->type) {
  case RL78_Operand_Register:
    op->type = RL78_Operand_Bit;
    break;
  case RL78_Operand_Indirect:
    op->type = RL78_Operand_BitIndirect;
    break;
  default:
    break;
  }
}

static int
saddr (int x)
{
  if (x < 0x20)
    return 0xfff00 + x;
  return 0xffe00 + x;
}

static int
sfr (int x)
{
  return 0xfff00 + x;
}

#define SADDR saddr (IMMU (1))
#define SFR sfr (IMMU (1))

int
rl78_decode_opcode (unsigned long pc AU,
		  RL78_Opcode_Decoded * rl78,
		  int (* getbyte)(void *),
		  void * ptr,
		  RL78_Dis_Isa isa)
{
  LocalData lds, * ld = &lds;
  unsigned char op_buf[OP_BUF_LEN] = {0};
  unsigned char *op = op_buf;
  int op0, op1;

  lds.rl78 = rl78;
  lds.getbyte = getbyte;
  lds.ptr = ptr;
  lds.op = op;

  memset (rl78, 0, sizeof (*rl78));

 start_again:

/* Byte registers, not including A.  */
/* Word registers, not including AX.  */

/*----------------------------------------------------------------------*/
/* ES: prefix								*/

  GETBYTE ();
  switch (op[0] & 0xff)
  {
    case 0x00:
        {
          /** 0000 0000			nop					*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0000 0000			nop					*/",
                     op[0]);
            }
          SYNTAX("nop");
#line 913 "rl78-decode.opc"
          ID(nop);

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0x01:
    case 0x03:
    case 0x05:
    case 0x07:
        {
          /** 0000 0rw1			addw	%0, %1				*/
#line 276 "rl78-decode.opc"
          int rw AU = (op[0] >> 1) & 0x03;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0000 0rw1			addw	%0, %1				*/",
                     op[0]);
              printf ("  rw = 0x%x\n", rw);
            }
          SYNTAX("addw	%0, %1");
#line 276 "rl78-decode.opc"
          ID(add); W(); DR(AX); SRW(rw); Fzac;

        }
      break;
    case 0x02:
        {
          /** 0000 0010			addw	%0, %e!1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0000 0010			addw	%0, %e!1			*/",
                     op[0]);
            }
          SYNTAX("addw	%0, %e!1");
#line 267 "rl78-decode.opc"
          ID(add); W(); DR(AX); SM(None, IMMU(2)); Fzac;

        }
      break;
    case 0x04:
        {
          /** 0000 0100			addw	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0000 0100			addw	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("addw	%0, #%1");
#line 273 "rl78-decode.opc"
          ID(add); W(); DR(AX); SC(IMMU(2)); Fzac;

        }
      break;
    case 0x06:
        {
          /** 0000 0110			addw	%0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0000 0110			addw	%0, %1				*/",
                     op[0]);
            }
          SYNTAX("addw	%0, %1");
#line 279 "rl78-decode.opc"
          ID(add); W(); DR(AX); SM(None, SADDR); Fzac;

        }
      break;
    case 0x08:
        {
          /** 0000 1000			xch	a, x				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0000 1000			xch	a, x				*/",
                     op[0]);
            }
          SYNTAX("xch	a, x");
#line 1236 "rl78-decode.opc"
          ID(xch); DR(A); SR(X);

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0x09:
        {
          /** 0000 1001			mov	%0, %e1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0000 1001			mov	%0, %e1				*/",
                     op[0]);
            }
          SYNTAX("mov	%0, %e1");
#line 680 "rl78-decode.opc"
          ID(mov); DR(A); SM(B, IMMU(2));

        }
      break;
    case 0x0a:
        {
          /** 0000 1010			add	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0000 1010			add	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("add	%0, #%1");
#line 230 "rl78-decode.opc"
          ID(add); DM(None, SADDR); SC(IMMU(1)); Fzac;

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0x0b:
        {
          /** 0000 1011			add	%0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0000 1011			add	%0, %1				*/",
                     op[0]);
            }
          SYNTAX("add	%0, %1");
#line 224 "rl78-decode.opc"
          ID(add); DR(A); SM(None, SADDR); Fzac;

        }
      break;
    case 0x0c:
        {
          /** 0000 1100			add	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0000 1100			add	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("add	%0, #%1");
#line 218 "rl78-decode.opc"
          ID(add); DR(A); SC(IMMU(1)); Fzac;

        }
      break;
    case 0x0d:
        {
          /** 0000 1101			add	%0, %e1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0000 1101			add	%0, %e1				*/",
                     op[0]);
            }
          SYNTAX("add	%0, %e1");
#line 206 "rl78-decode.opc"
          ID(add); DR(A); SM(HL, 0); Fzac;

        }
      break;
    case 0x0e:
        {
          /** 0000 1110			add	%0, %ea1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0000 1110			add	%0, %ea1			*/",
                     op[0]);
            }
          SYNTAX("add	%0, %ea1");
#line 212 "rl78-decode.opc"
          ID(add); DR(A); SM(HL, IMMU(1)); Fzac;

        }
      break;
    case 0x0f:
        {
          /** 0000 1111			add	%0, %e!1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0000 1111			add	%0, %e!1			*/",
                     op[0]);
            }
          SYNTAX("add	%0, %e!1");
#line 203 "rl78-decode.opc"
          ID(add); DR(A); SM(None, IMMU(2)); Fzac;

        }
      break;
    case 0x10:
        {
          /** 0001 0000			addw	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0001 0000			addw	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("addw	%0, #%1");
#line 282 "rl78-decode.opc"
          ID(add); W(); DR(SP); SC(IMMU(1)); Fzac;

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0x11:
        {
          /** 0001 0001			es:					*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0001 0001			es:					*/",
                     op[0]);
            }
          SYNTAX("es:");
#line 195 "rl78-decode.opc"
          DE(); SE();
          op ++;
          pc ++;
          goto start_again;

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0x12:
    case 0x14:
    case 0x16:
        {
          /** 0001 0ra0			movw	%0, %1				*/
#line 861 "rl78-decode.opc"
          int ra AU = (op[0] >> 1) & 0x03;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0001 0ra0			movw	%0, %1				*/",
                     op[0]);
              printf ("  ra = 0x%x\n", ra);
            }
          SYNTAX("movw	%0, %1");
#line 861 "rl78-decode.opc"
          ID(mov); W(); DRW(ra); SR(AX);

        }
      break;
    case 0x13:
    case 0x15:
    case 0x17:
        {
          /** 0001 0ra1			movw	%0, %1				*/
#line 858 "rl78-decode.opc"
          int ra AU = (op[0] >> 1) & 0x03;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0001 0ra1			movw	%0, %1				*/",
                     op[0]);
              printf ("  ra = 0x%x\n", ra);
            }
          SYNTAX("movw	%0, %1");
#line 858 "rl78-decode.opc"
          ID(mov); W(); DR(AX); SRW(ra);

        }
      break;
    case 0x18:
        {
          /** 0001 1000			mov	%e0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0001 1000			mov	%e0, %1				*/",
                     op[0]);
            }
          SYNTAX("mov	%e0, %1");
#line 731 "rl78-decode.opc"
          ID(mov); DM(B, IMMU(2)); SR(A);

        }
      break;
    case 0x19:
        {
          /** 0001 1001			mov	%e0, #%1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0001 1001			mov	%e0, #%1			*/",
                     op[0]);
            }
          SYNTAX("mov	%e0, #%1");
#line 728 "rl78-decode.opc"
          ID(mov); DM(B, IMMU(2)); SC(IMMU(1));

        }
      break;
    case 0x1a:
        {
          /** 0001 1010			addc	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0001 1010			addc	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("addc	%0, #%1");
#line 262 "rl78-decode.opc"
          ID(addc); DM(None, SADDR); SC(IMMU(1)); Fzac;

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0x1b:
        {
          /** 0001 1011			addc	%0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0001 1011			addc	%0, %1				*/",
                     op[0]);
            }
          SYNTAX("addc	%0, %1");
#line 259 "rl78-decode.opc"
          ID(addc); DR(A); SM(None, SADDR); Fzac;

        }
      break;
    case 0x1c:
        {
          /** 0001 1100			addc	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0001 1100			addc	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("addc	%0, #%1");
#line 250 "rl78-decode.opc"
          ID(addc); DR(A); SC(IMMU(1)); Fzac;

        }
      break;
    case 0x1d:
        {
          /** 0001 1101			addc	%0, %e1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0001 1101			addc	%0, %e1				*/",
                     op[0]);
            }
          SYNTAX("addc	%0, %e1");
#line 238 "rl78-decode.opc"
          ID(addc); DR(A); SM(HL, 0); Fzac;

        }
      break;
    case 0x1e:
        {
          /** 0001 1110			addc	%0, %ea1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0001 1110			addc	%0, %ea1			*/",
                     op[0]);
            }
          SYNTAX("addc	%0, %ea1");
#line 247 "rl78-decode.opc"
          ID(addc); DR(A); SM(HL, IMMU(1)); Fzac;

        }
      break;
    case 0x1f:
        {
          /** 0001 1111			addc	%0, %e!1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0001 1111			addc	%0, %e!1			*/",
                     op[0]);
            }
          SYNTAX("addc	%0, %e!1");
#line 235 "rl78-decode.opc"
          ID(addc); DR(A); SM(None, IMMU(2)); Fzac;

        }
      break;
    case 0x20:
        {
          /** 0010 0000			subw	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0010 0000			subw	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("subw	%0, #%1");
#line 1200 "rl78-decode.opc"
          ID(sub); W(); DR(SP); SC(IMMU(1)); Fzac;

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0x21:
    case 0x23:
    case 0x25:
    case 0x27:
        {
          /** 0010 0rw1			subw	%0, %1				*/
#line 1194 "rl78-decode.opc"
          int rw AU = (op[0] >> 1) & 0x03;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0010 0rw1			subw	%0, %1				*/",
                     op[0]);
              printf ("  rw = 0x%x\n", rw);
            }
          SYNTAX("subw	%0, %1");
#line 1194 "rl78-decode.opc"
          ID(sub); W(); DR(AX); SRW(rw); Fzac;

        }
      break;
    case 0x22:
        {
          /** 0010 0010			subw	%0, %e!1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0010 0010			subw	%0, %e!1			*/",
                     op[0]);
            }
          SYNTAX("subw	%0, %e!1");
#line 1185 "rl78-decode.opc"
          ID(sub); W(); DR(AX); SM(None, IMMU(2)); Fzac;

        }
      break;
    case 0x24:
        {
          /** 0010 0100			subw	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0010 0100			subw	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("subw	%0, #%1");
#line 1191 "rl78-decode.opc"
          ID(sub); W(); DR(AX); SC(IMMU(2)); Fzac;

        }
      break;
    case 0x26:
        {
          /** 0010 0110			subw	%0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0010 0110			subw	%0, %1				*/",
                     op[0]);
            }
          SYNTAX("subw	%0, %1");
#line 1197 "rl78-decode.opc"
          ID(sub); W(); DR(AX); SM(None, SADDR); Fzac;

        }
      break;
    case 0x28:
        {
          /** 0010 1000			mov	%e0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0010 1000			mov	%e0, %1				*/",
                     op[0]);
            }
          SYNTAX("mov	%e0, %1");
#line 743 "rl78-decode.opc"
          ID(mov); DM(C, IMMU(2)); SR(A);

        }
      break;
    case 0x29:
        {
          /** 0010 1001			mov	%0, %e1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0010 1001			mov	%0, %e1				*/",
                     op[0]);
            }
          SYNTAX("mov	%0, %e1");
#line 686 "rl78-decode.opc"
          ID(mov); DR(A); SM(C, IMMU(2));

        }
      break;
    case 0x2a:
        {
          /** 0010 1010			sub	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0010 1010			sub	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("sub	%0, #%1");
#line 1148 "rl78-decode.opc"
          ID(sub); DM(None, SADDR); SC(IMMU(1)); Fzac;

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0x2b:
        {
          /** 0010 1011			sub	%0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0010 1011			sub	%0, %1				*/",
                     op[0]);
            }
          SYNTAX("sub	%0, %1");
#line 1142 "rl78-decode.opc"
          ID(sub); DR(A); SM(None, SADDR); Fzac;

        }
      break;
    case 0x2c:
        {
          /** 0010 1100			sub	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0010 1100			sub	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("sub	%0, #%1");
#line 1136 "rl78-decode.opc"
          ID(sub); DR(A); SC(IMMU(1)); Fzac;

        }
      break;
    case 0x2d:
        {
          /** 0010 1101			sub	%0, %e1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0010 1101			sub	%0, %e1				*/",
                     op[0]);
            }
          SYNTAX("sub	%0, %e1");
#line 1124 "rl78-decode.opc"
          ID(sub); DR(A); SM(HL, 0); Fzac;

        }
      break;
    case 0x2e:
        {
          /** 0010 1110			sub	%0, %ea1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0010 1110			sub	%0, %ea1			*/",
                     op[0]);
            }
          SYNTAX("sub	%0, %ea1");
#line 1130 "rl78-decode.opc"
          ID(sub); DR(A); SM(HL, IMMU(1)); Fzac;

        }
      break;
    case 0x2f:
        {
          /** 0010 1111			sub	%0, %e!1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0010 1111			sub	%0, %e!1			*/",
                     op[0]);
            }
          SYNTAX("sub	%0, %e!1");
#line 1121 "rl78-decode.opc"
          ID(sub); DR(A); SM(None, IMMU(2)); Fzac;

        }
      break;
    case 0x30:
    case 0x32:
    case 0x34:
    case 0x36:
        {
          /** 0011 0rg0			movw	%0, #%1				*/
#line 855 "rl78-decode.opc"
          int rg AU = (op[0] >> 1) & 0x03;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0011 0rg0			movw	%0, #%1				*/",
                     op[0]);
              printf ("  rg = 0x%x\n", rg);
            }
          SYNTAX("movw	%0, #%1");
#line 855 "rl78-decode.opc"
          ID(mov); W(); DRW(rg); SC(IMMU(2));

        }
      break;
    case 0x31:
        GETBYTE ();
        switch (op[1] & 0x8f)
        {
          case 0x00:
              {
                /** 0011 0001 0bit 0000		btclr	%s1, $%a0			*/
#line 418 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0011 0001 0bit 0000		btclr	%s1, $%a0			*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("btclr	%s1, $%a0");
#line 418 "rl78-decode.opc"
                ID(branch_cond_clear); SM(None, SADDR); SB(bit); DC(pc+IMMS(1)+4); COND(T);

              /*----------------------------------------------------------------------*/

              }
            break;
          case 0x01:
              {
                /** 0011 0001 0bit 0001		btclr	%1, $%a0			*/
#line 412 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0011 0001 0bit 0001		btclr	%1, $%a0			*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("btclr	%1, $%a0");
#line 412 "rl78-decode.opc"
                ID(branch_cond_clear); DC(pc+IMMS(1)+3); SR(A); SB(bit); COND(T);

              }
            break;
          case 0x02:
              {
                /** 0011 0001 0bit 0010		bt	%s1, $%a0			*/
#line 404 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0011 0001 0bit 0010		bt	%s1, $%a0			*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("bt	%s1, $%a0");
#line 404 "rl78-decode.opc"
                ID(branch_cond); SM(None, SADDR); SB(bit); DC(pc+IMMS(1)+4); COND(T);

              /*----------------------------------------------------------------------*/

              }
            break;
          case 0x03:
              {
                /** 0011 0001 0bit 0011		bt	%1, $%a0			*/
#line 398 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0011 0001 0bit 0011		bt	%1, $%a0			*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("bt	%1, $%a0");
#line 398 "rl78-decode.opc"
                ID(branch_cond); DC(pc+IMMS(1)+3); SR(A); SB(bit); COND(T);

              }
            break;
          case 0x04:
              {
                /** 0011 0001 0bit 0100		bf	%s1, $%a0			*/
#line 365 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0011 0001 0bit 0100		bf	%s1, $%a0			*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("bf	%s1, $%a0");
#line 365 "rl78-decode.opc"
                ID(branch_cond); SM(None, SADDR); SB(bit); DC(pc+IMMS(1)+4); COND(F);

              /*----------------------------------------------------------------------*/

              }
            break;
          case 0x05:
              {
                /** 0011 0001 0bit 0101		bf	%1, $%a0			*/
#line 359 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0011 0001 0bit 0101		bf	%1, $%a0			*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("bf	%1, $%a0");
#line 359 "rl78-decode.opc"
                ID(branch_cond); DC(pc+IMMS(1)+3); SR(A); SB(bit); COND(F);

              }
            break;
          case 0x07:
              {
                /** 0011 0001 0cnt 0111		shl	%0, %1				*/
#line 1077 "rl78-decode.opc"
                int cnt AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0011 0001 0cnt 0111		shl	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  cnt = 0x%x\n", cnt);
                  }
                SYNTAX("shl	%0, %1");
#line 1077 "rl78-decode.opc"
                ID(shl); DR(C); SC(cnt);

              }
            break;
          case 0x08:
              {
                /** 0011 0001 0cnt 1000		shl	%0, %1				*/
#line 1074 "rl78-decode.opc"
                int cnt AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0011 0001 0cnt 1000		shl	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  cnt = 0x%x\n", cnt);
                  }
                SYNTAX("shl	%0, %1");
#line 1074 "rl78-decode.opc"
                ID(shl); DR(B); SC(cnt);

              }
            break;
          case 0x09:
              {
                /** 0011 0001 0cnt 1001		shl	%0, %1				*/
#line 1071 "rl78-decode.opc"
                int cnt AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0011 0001 0cnt 1001		shl	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  cnt = 0x%x\n", cnt);
                  }
                SYNTAX("shl	%0, %1");
#line 1071 "rl78-decode.opc"
                ID(shl); DR(A); SC(cnt);

              }
            break;
          case 0x0a:
              {
                /** 0011 0001 0cnt 1010		shr	%0, %1				*/
#line 1088 "rl78-decode.opc"
                int cnt AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0011 0001 0cnt 1010		shr	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  cnt = 0x%x\n", cnt);
                  }
                SYNTAX("shr	%0, %1");
#line 1088 "rl78-decode.opc"
                ID(shr); DR(A); SC(cnt);

              }
            break;
          case 0x0b:
              {
                /** 0011 0001 0cnt 1011		sar	%0, %1				*/
#line 1035 "rl78-decode.opc"
                int cnt AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0011 0001 0cnt 1011		sar	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  cnt = 0x%x\n", cnt);
                  }
                SYNTAX("sar	%0, %1");
#line 1035 "rl78-decode.opc"
                ID(sar); DR(A); SC(cnt);

              }
            break;
          case 0x0c:
          case 0x8c:
              {
                /** 0011 0001 wcnt 1100		shlw	%0, %1				*/
#line 1083 "rl78-decode.opc"
                int wcnt AU = (op[1] >> 4) & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0011 0001 wcnt 1100		shlw	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  wcnt = 0x%x\n", wcnt);
                  }
                SYNTAX("shlw	%0, %1");
#line 1083 "rl78-decode.opc"
                ID(shl); W(); DR(BC); SC(wcnt);

              /*----------------------------------------------------------------------*/

              }
            break;
          case 0x0d:
          case 0x8d:
              {
                /** 0011 0001 wcnt 1101		shlw	%0, %1				*/
#line 1080 "rl78-decode.opc"
                int wcnt AU = (op[1] >> 4) & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0011 0001 wcnt 1101		shlw	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  wcnt = 0x%x\n", wcnt);
                  }
                SYNTAX("shlw	%0, %1");
#line 1080 "rl78-decode.opc"
                ID(shl); W(); DR(AX); SC(wcnt);

              }
            break;
          case 0x0e:
          case 0x8e:
              {
                /** 0011 0001 wcnt 1110		shrw	%0, %1				*/
#line 1091 "rl78-decode.opc"
                int wcnt AU = (op[1] >> 4) & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0011 0001 wcnt 1110		shrw	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  wcnt = 0x%x\n", wcnt);
                  }
                SYNTAX("shrw	%0, %1");
#line 1091 "rl78-decode.opc"
                ID(shr); W(); DR(AX); SC(wcnt);

              /*----------------------------------------------------------------------*/

              }
            break;
          case 0x0f:
          case 0x8f:
              {
                /** 0011 0001 wcnt 1111		sarw	%0, %1				*/
#line 1038 "rl78-decode.opc"
                int wcnt AU = (op[1] >> 4) & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0011 0001 wcnt 1111		sarw	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  wcnt = 0x%x\n", wcnt);
                  }
                SYNTAX("sarw	%0, %1");
#line 1038 "rl78-decode.opc"
                ID(sar); W(); DR(AX); SC(wcnt);

              /*----------------------------------------------------------------------*/

              }
            break;
          case 0x80:
              {
                /** 0011 0001 1bit 0000		btclr	%s1, $%a0			*/
#line 415 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0011 0001 1bit 0000		btclr	%s1, $%a0			*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("btclr	%s1, $%a0");
#line 415 "rl78-decode.opc"
                ID(branch_cond_clear); SM(None, SFR); SB(bit); DC(pc+IMMS(1)+4); COND(T);

              }
            break;
          case 0x81:
              {
                /** 0011 0001 1bit 0001		btclr	%e1, $%a0			*/
#line 409 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0011 0001 1bit 0001		btclr	%e1, $%a0			*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("btclr	%e1, $%a0");
#line 409 "rl78-decode.opc"
                ID(branch_cond_clear); DC(pc+IMMS(1)+3); SM(HL,0); SB(bit); COND(T);

              }
            break;
          case 0x82:
              {
                /** 0011 0001 1bit 0010		bt	%s1, $%a0			*/
#line 401 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0011 0001 1bit 0010		bt	%s1, $%a0			*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("bt	%s1, $%a0");
#line 401 "rl78-decode.opc"
                ID(branch_cond); SM(None, SFR); SB(bit); DC(pc+IMMS(1)+4); COND(T);

              }
            break;
          case 0x83:
              {
                /** 0011 0001 1bit 0011		bt	%e1, $%a0			*/
#line 395 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0011 0001 1bit 0011		bt	%e1, $%a0			*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("bt	%e1, $%a0");
#line 395 "rl78-decode.opc"
                ID(branch_cond); DC(pc+IMMS(1)+3); SM(HL,0); SB(bit); COND(T);

              }
            break;
          case 0x84:
              {
                /** 0011 0001 1bit 0100		bf	%s1, $%a0			*/
#line 362 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0011 0001 1bit 0100		bf	%s1, $%a0			*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("bf	%s1, $%a0");
#line 362 "rl78-decode.opc"
                ID(branch_cond); SM(None, SFR); SB(bit); DC(pc+IMMS(1)+4); COND(F);

              }
            break;
          case 0x85:
              {
                /** 0011 0001 1bit 0101		bf	%e1, $%a0			*/
#line 356 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0011 0001 1bit 0101		bf	%e1, $%a0			*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("bf	%e1, $%a0");
#line 356 "rl78-decode.opc"
                ID(branch_cond); DC(pc+IMMS(1)+3); SM(HL,0); SB(bit); COND(F);

              }
            break;
          default: UNSUPPORTED(); break;
        }
      break;
    case 0x33:
    case 0x35:
    case 0x37:
        {
          /** 0011 0ra1			xchw	%0, %1				*/
#line 1241 "rl78-decode.opc"
          int ra AU = (op[0] >> 1) & 0x03;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0011 0ra1			xchw	%0, %1				*/",
                     op[0]);
              printf ("  ra = 0x%x\n", ra);
            }
          SYNTAX("xchw	%0, %1");
#line 1241 "rl78-decode.opc"
          ID(xch); W(); DR(AX); SRW(ra);

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0x38:
        {
          /** 0011 1000			mov	%e0, #%1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0011 1000			mov	%e0, #%1			*/",
                     op[0]);
            }
          SYNTAX("mov	%e0, #%1");
#line 740 "rl78-decode.opc"
          ID(mov); DM(C, IMMU(2)); SC(IMMU(1));

        }
      break;
    case 0x39:
        {
          /** 0011 1001			mov	%e0, #%1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0011 1001			mov	%e0, #%1			*/",
                     op[0]);
            }
          SYNTAX("mov	%e0, #%1");
#line 734 "rl78-decode.opc"
          ID(mov); DM(BC, IMMU(2)); SC(IMMU(1));

        }
      break;
    case 0x3a:
        {
          /** 0011 1010			subc	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0011 1010			subc	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("subc	%0, #%1");
#line 1180 "rl78-decode.opc"
          ID(subc); DM(None, SADDR); SC(IMMU(1)); Fzac;

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0x3b:
        {
          /** 0011 1011			subc	%0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0011 1011			subc	%0, %1				*/",
                     op[0]);
            }
          SYNTAX("subc	%0, %1");
#line 1177 "rl78-decode.opc"
          ID(subc); DR(A); SM(None, SADDR); Fzac;

        }
      break;
    case 0x3c:
        {
          /** 0011 1100			subc	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0011 1100			subc	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("subc	%0, #%1");
#line 1168 "rl78-decode.opc"
          ID(subc); DR(A); SC(IMMU(1)); Fzac;

        }
      break;
    case 0x3d:
        {
          /** 0011 1101			subc	%0, %e1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0011 1101			subc	%0, %e1				*/",
                     op[0]);
            }
          SYNTAX("subc	%0, %e1");
#line 1156 "rl78-decode.opc"
          ID(subc); DR(A); SM(HL, 0); Fzac;

        }
      break;
    case 0x3e:
        {
          /** 0011 1110			subc	%0, %ea1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0011 1110			subc	%0, %ea1			*/",
                     op[0]);
            }
          SYNTAX("subc	%0, %ea1");
#line 1165 "rl78-decode.opc"
          ID(subc); DR(A); SM(HL, IMMU(1)); Fzac;

        }
      break;
    case 0x3f:
        {
          /** 0011 1111			subc	%0, %e!1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0011 1111			subc	%0, %e!1			*/",
                     op[0]);
            }
          SYNTAX("subc	%0, %e!1");
#line 1153 "rl78-decode.opc"
          ID(subc); DR(A); SM(None, IMMU(2)); Fzac;

        }
      break;
    case 0x40:
        {
          /** 0100 0000			cmp	%e!0, #%1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0100 0000			cmp	%e!0, #%1			*/",
                     op[0]);
            }
          SYNTAX("cmp	%e!0, #%1");
#line 482 "rl78-decode.opc"
          ID(cmp); DM(None, IMMU(2)); SC(IMMU(1)); Fzac;

        }
      break;
    case 0x41:
        {
          /** 0100 0001			mov	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0100 0001			mov	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("mov	%0, #%1");
#line 719 "rl78-decode.opc"
          ID(mov); DR(ES); SC(IMMU(1));

        }
      break;
    case 0x42:
        {
          /** 0100 0010			cmpw	%0, %e!1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0100 0010			cmpw	%0, %e!1			*/",
                     op[0]);
            }
          SYNTAX("cmpw	%0, %e!1");
#line 533 "rl78-decode.opc"
          ID(cmp); W(); DR(AX); SM(None, IMMU(2)); Fzac;

        }
      break;
    case 0x43:
    case 0x45:
    case 0x47:
        {
          /** 0100 0ra1			cmpw	%0, %1				*/
#line 542 "rl78-decode.opc"
          int ra AU = (op[0] >> 1) & 0x03;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0100 0ra1			cmpw	%0, %1				*/",
                     op[0]);
              printf ("  ra = 0x%x\n", ra);
            }
          SYNTAX("cmpw	%0, %1");
#line 542 "rl78-decode.opc"
          ID(cmp); W(); DR(AX); SRW(ra); Fzac;

        }
      break;
    case 0x44:
        {
          /** 0100 0100			cmpw	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0100 0100			cmpw	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("cmpw	%0, #%1");
#line 539 "rl78-decode.opc"
          ID(cmp); W(); DR(AX); SC(IMMU(2)); Fzac;

        }
      break;
    case 0x46:
        {
          /** 0100 0110			cmpw	%0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0100 0110			cmpw	%0, %1				*/",
                     op[0]);
            }
          SYNTAX("cmpw	%0, %1");
#line 545 "rl78-decode.opc"
          ID(cmp); W(); DR(AX); SM(None, SADDR); Fzac;

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0x48:
        {
          /** 0100 1000			mov	%e0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0100 1000			mov	%e0, %1				*/",
                     op[0]);
            }
          SYNTAX("mov	%e0, %1");
#line 737 "rl78-decode.opc"
          ID(mov); DM(BC, IMMU(2)); SR(A);

        }
      break;
    case 0x49:
        {
          /** 0100 1001			mov	%0, %e1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0100 1001			mov	%0, %e1				*/",
                     op[0]);
            }
          SYNTAX("mov	%0, %e1");
#line 683 "rl78-decode.opc"
          ID(mov); DR(A); SM(BC, IMMU(2));

        }
      break;
    case 0x4a:
        {
          /** 0100 1010			cmp	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0100 1010			cmp	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("cmp	%0, #%1");
#line 485 "rl78-decode.opc"
          ID(cmp); DM(None, SADDR); SC(IMMU(1)); Fzac;

        }
      break;
    case 0x4b:
        {
          /** 0100 1011			cmp	%0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0100 1011			cmp	%0, %1				*/",
                     op[0]);
            }
          SYNTAX("cmp	%0, %1");
#line 512 "rl78-decode.opc"
          ID(cmp); DR(A); SM(None, SADDR); Fzac;

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0x4c:
        {
          /** 0100 1100			cmp	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0100 1100			cmp	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("cmp	%0, #%1");
#line 503 "rl78-decode.opc"
          ID(cmp); DR(A); SC(IMMU(1)); Fzac;

        }
      break;
    case 0x4d:
        {
          /** 0100 1101			cmp	%0, %e1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0100 1101			cmp	%0, %e1				*/",
                     op[0]);
            }
          SYNTAX("cmp	%0, %e1");
#line 491 "rl78-decode.opc"
          ID(cmp); DR(A); SM(HL, 0); Fzac;

        }
      break;
    case 0x4e:
        {
          /** 0100 1110			cmp	%0, %ea1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0100 1110			cmp	%0, %ea1			*/",
                     op[0]);
            }
          SYNTAX("cmp	%0, %ea1");
#line 500 "rl78-decode.opc"
          ID(cmp); DR(A); SM(HL, IMMU(1)); Fzac;

        }
      break;
    case 0x4f:
        {
          /** 0100 1111			cmp	%0, %e!1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0100 1111			cmp	%0, %e!1			*/",
                     op[0]);
            }
          SYNTAX("cmp	%0, %e!1");
#line 488 "rl78-decode.opc"
          ID(cmp); DR(A); SM(None, IMMU(2)); Fzac;

        }
      break;
    case 0x50:
    case 0x51:
    case 0x52:
    case 0x53:
    case 0x54:
    case 0x55:
    case 0x56:
    case 0x57:
        {
          /** 0101 0reg			mov	%0, #%1				*/
#line 671 "rl78-decode.opc"
          int reg AU = op[0] & 0x07;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0101 0reg			mov	%0, #%1				*/",
                     op[0]);
              printf ("  reg = 0x%x\n", reg);
            }
          SYNTAX("mov	%0, #%1");
#line 671 "rl78-decode.opc"
          ID(mov); DRB(reg); SC(IMMU(1));

        }
      break;
    case 0x58:
        {
          /** 0101 1000			movw	%e0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0101 1000			movw	%e0, %1				*/",
                     op[0]);
            }
          SYNTAX("movw	%e0, %1");
#line 873 "rl78-decode.opc"
          ID(mov); W(); DM(B, IMMU(2)); SR(AX);

        }
      break;
    case 0x59:
        {
          /** 0101 1001			movw	%0, %e1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0101 1001			movw	%0, %e1				*/",
                     op[0]);
            }
          SYNTAX("movw	%0, %e1");
#line 864 "rl78-decode.opc"
          ID(mov); W(); DR(AX); SM(B, IMMU(2));

        }
      break;
    case 0x5a:
        {
          /** 0101 1010	       		and	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0101 1010	       		and	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("and	%0, #%1");
#line 314 "rl78-decode.opc"
          ID(and); DM(None, SADDR); SC(IMMU(1)); Fz;

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0x5b:
        {
          /** 0101 1011	       		and	%0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0101 1011	       		and	%0, %1				*/",
                     op[0]);
            }
          SYNTAX("and	%0, %1");
#line 311 "rl78-decode.opc"
          ID(and); DR(A); SM(None, SADDR); Fz;

        }
      break;
    case 0x5c:
        {
          /** 0101 1100	       		and	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0101 1100	       		and	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("and	%0, #%1");
#line 302 "rl78-decode.opc"
          ID(and); DR(A); SC(IMMU(1)); Fz;

        }
      break;
    case 0x5d:
        {
          /** 0101 1101			and	%0, %e1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0101 1101			and	%0, %e1			*/",
                     op[0]);
            }
          SYNTAX("and	%0, %e1");
#line 290 "rl78-decode.opc"
          ID(and); DR(A); SM(HL, 0); Fz;

        }
      break;
    case 0x5e:
        {
          /** 0101 1110			and	%0, %ea1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0101 1110			and	%0, %ea1			*/",
                     op[0]);
            }
          SYNTAX("and	%0, %ea1");
#line 296 "rl78-decode.opc"
          ID(and); DR(A); SM(HL, IMMU(1)); Fz;

        }
      break;
    case 0x5f:
        {
          /** 0101 1111			and	%0, %e!1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0101 1111			and	%0, %e!1			*/",
                     op[0]);
            }
          SYNTAX("and	%0, %e!1");
#line 287 "rl78-decode.opc"
          ID(and); DR(A); SM(None, IMMU(2)); Fz;

        }
      break;
    case 0x60:
    case 0x62:
    case 0x63:
    case 0x64:
    case 0x65:
    case 0x66:
    case 0x67:
        {
          /** 0110 0rba			mov	%0, %1				*/
#line 674 "rl78-decode.opc"
          int rba AU = op[0] & 0x07;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0110 0rba			mov	%0, %1				*/",
                     op[0]);
              printf ("  rba = 0x%x\n", rba);
            }
          SYNTAX("mov	%0, %1");
#line 674 "rl78-decode.opc"
          ID(mov); DR(A); SRB(rba);

        }
      break;
    case 0x61:
        GETBYTE ();
        switch (op[1] & 0xff)
        {
          case 0x00:
          case 0x01:
          case 0x02:
          case 0x03:
          case 0x04:
          case 0x05:
          case 0x06:
          case 0x07:
              {
                /** 0110 0001 0000 0reg		add	%0, %1				*/
#line 227 "rl78-decode.opc"
                int reg AU = op[1] & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 0000 0reg		add	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  reg = 0x%x\n", reg);
                  }
                SYNTAX("add	%0, %1");
#line 227 "rl78-decode.opc"
                ID(add); DRB(reg); SR(A); Fzac;

              }
            break;
          case 0x08:
          case 0x0a:
          case 0x0b:
          case 0x0c:
          case 0x0d:
          case 0x0e:
          case 0x0f:
              {
                /** 0110 0001 0000 1rba		add	%0, %1				*/
#line 221 "rl78-decode.opc"
                int rba AU = op[1] & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 0000 1rba		add	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  rba = 0x%x\n", rba);
                  }
                SYNTAX("add	%0, %1");
#line 221 "rl78-decode.opc"
                ID(add); DR(A); SRB(rba); Fzac;

              }
            break;
          case 0x09:
              {
                /** 0110 0001 0000 1001		addw	%0, %ea1			*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 0000 1001		addw	%0, %ea1			*/",
                           op[0], op[1]);
                  }
                SYNTAX("addw	%0, %ea1");
#line 270 "rl78-decode.opc"
                ID(add); W(); DR(AX); SM(HL, IMMU(1)); Fzac;

              }
            break;
          case 0x10:
          case 0x11:
          case 0x12:
          case 0x13:
          case 0x14:
          case 0x15:
          case 0x16:
          case 0x17:
              {
                /** 0110 0001 0001 0reg		addc	%0, %1				*/
#line 256 "rl78-decode.opc"
                int reg AU = op[1] & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 0001 0reg		addc	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  reg = 0x%x\n", reg);
                  }
                SYNTAX("addc	%0, %1");
#line 256 "rl78-decode.opc"
                ID(addc); DRB(reg); SR(A); Fzac;

              }
            break;
          case 0x18:
          case 0x1a:
          case 0x1b:
          case 0x1c:
          case 0x1d:
          case 0x1e:
          case 0x1f:
              {
                /** 0110 0001 0001 1rba		addc	%0, %1				*/
#line 253 "rl78-decode.opc"
                int rba AU = op[1] & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 0001 1rba		addc	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  rba = 0x%x\n", rba);
                  }
                SYNTAX("addc	%0, %1");
#line 253 "rl78-decode.opc"
                ID(addc); DR(A); SRB(rba); Fzac;

              }
            break;
          case 0x20:
          case 0x21:
          case 0x22:
          case 0x23:
          case 0x24:
          case 0x25:
          case 0x26:
          case 0x27:
              {
                /** 0110 0001 0010 0reg		sub	%0, %1				*/
#line 1145 "rl78-decode.opc"
                int reg AU = op[1] & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 0010 0reg		sub	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  reg = 0x%x\n", reg);
                  }
                SYNTAX("sub	%0, %1");
#line 1145 "rl78-decode.opc"
                ID(sub); DRB(reg); SR(A); Fzac;

              }
            break;
          case 0x28:
          case 0x2a:
          case 0x2b:
          case 0x2c:
          case 0x2d:
          case 0x2e:
          case 0x2f:
              {
                /** 0110 0001 0010 1rba		sub	%0, %1				*/
#line 1139 "rl78-decode.opc"
                int rba AU = op[1] & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 0010 1rba		sub	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  rba = 0x%x\n", rba);
                  }
                SYNTAX("sub	%0, %1");
#line 1139 "rl78-decode.opc"
                ID(sub); DR(A); SRB(rba); Fzac;

              }
            break;
          case 0x29:
              {
                /** 0110 0001 0010 1001		subw	%0, %ea1			*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 0010 1001		subw	%0, %ea1			*/",
                           op[0], op[1]);
                  }
                SYNTAX("subw	%0, %ea1");
#line 1188 "rl78-decode.opc"
                ID(sub); W(); DR(AX); SM(HL, IMMU(1)); Fzac;

              }
            break;
          case 0x30:
          case 0x31:
          case 0x32:
          case 0x33:
          case 0x34:
          case 0x35:
          case 0x36:
          case 0x37:
              {
                /** 0110 0001 0011 0reg		subc	%0, %1				*/
#line 1174 "rl78-decode.opc"
                int reg AU = op[1] & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 0011 0reg		subc	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  reg = 0x%x\n", reg);
                  }
                SYNTAX("subc	%0, %1");
#line 1174 "rl78-decode.opc"
                ID(subc); DRB(reg); SR(A); Fzac;

              }
            break;
          case 0x38:
          case 0x3a:
          case 0x3b:
          case 0x3c:
          case 0x3d:
          case 0x3e:
          case 0x3f:
              {
                /** 0110 0001 0011 1rba		subc	%0, %1				*/
#line 1171 "rl78-decode.opc"
                int rba AU = op[1] & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 0011 1rba		subc	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  rba = 0x%x\n", rba);
                  }
                SYNTAX("subc	%0, %1");
#line 1171 "rl78-decode.opc"
                ID(subc); DR(A); SRB(rba); Fzac;

              }
            break;
          case 0x40:
          case 0x41:
          case 0x42:
          case 0x43:
          case 0x44:
          case 0x45:
          case 0x46:
          case 0x47:
              {
                /** 0110 0001 0100 0reg		cmp	%0, %1				*/
#line 509 "rl78-decode.opc"
                int reg AU = op[1] & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 0100 0reg		cmp	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  reg = 0x%x\n", reg);
                  }
                SYNTAX("cmp	%0, %1");
#line 509 "rl78-decode.opc"
                ID(cmp); DRB(reg); SR(A); Fzac;

              }
            break;
          case 0x48:
          case 0x4a:
          case 0x4b:
          case 0x4c:
          case 0x4d:
          case 0x4e:
          case 0x4f:
              {
                /** 0110 0001 0100 1rba		cmp	%0, %1				*/
#line 506 "rl78-decode.opc"
                int rba AU = op[1] & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 0100 1rba		cmp	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  rba = 0x%x\n", rba);
                  }
                SYNTAX("cmp	%0, %1");
#line 506 "rl78-decode.opc"
                ID(cmp); DR(A); SRB(rba); Fzac;

              }
            break;
          case 0x49:
              {
                /** 0110 0001 0100 1001		cmpw	%0, %ea1			*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 0100 1001		cmpw	%0, %ea1			*/",
                           op[0], op[1]);
                  }
                SYNTAX("cmpw	%0, %ea1");
#line 536 "rl78-decode.opc"
                ID(cmp); W(); DR(AX); SM(HL, IMMU(1)); Fzac;

              }
            break;
          case 0x50:
          case 0x51:
          case 0x52:
          case 0x53:
          case 0x54:
          case 0x55:
          case 0x56:
          case 0x57:
              {
                /** 0110 0001 0101 0reg		and	%0, %1				*/
#line 308 "rl78-decode.opc"
                int reg AU = op[1] & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 0101 0reg		and	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  reg = 0x%x\n", reg);
                  }
                SYNTAX("and	%0, %1");
#line 308 "rl78-decode.opc"
                ID(and); DRB(reg); SR(A); Fz;

              }
            break;
          case 0x58:
          case 0x5a:
          case 0x5b:
          case 0x5c:
          case 0x5d:
          case 0x5e:
          case 0x5f:
              {
                /** 0110 0001 0101 1rba		and	%0, %1				*/
#line 305 "rl78-decode.opc"
                int rba AU = op[1] & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 0101 1rba		and	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  rba = 0x%x\n", rba);
                  }
                SYNTAX("and	%0, %1");
#line 305 "rl78-decode.opc"
                ID(and); DR(A); SRB(rba); Fz;

              }
            break;
          case 0x59:
              {
                /** 0110 0001 0101 1001		inc	%ea0				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 0101 1001		inc	%ea0				*/",
                           op[0], op[1]);
                  }
                SYNTAX("inc	%ea0");
#line 586 "rl78-decode.opc"
                ID(add); DM(HL, IMMU(1)); SC(1); Fza;

              }
            break;
          case 0x60:
          case 0x61:
          case 0x62:
          case 0x63:
          case 0x64:
          case 0x65:
          case 0x66:
          case 0x67:
              {
                /** 0110 0001 0110 0reg		or	%0, %1				*/
#line 963 "rl78-decode.opc"
                int reg AU = op[1] & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 0110 0reg		or	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  reg = 0x%x\n", reg);
                  }
                SYNTAX("or	%0, %1");
#line 963 "rl78-decode.opc"
                ID(or); DRB(reg); SR(A); Fz;

              }
            break;
          case 0x68:
          case 0x6a:
          case 0x6b:
          case 0x6c:
          case 0x6d:
          case 0x6e:
          case 0x6f:
              {
                /** 0110 0001 0110 1rba		or	%0, %1				*/
#line 960 "rl78-decode.opc"
                int rba AU = op[1] & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 0110 1rba		or	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  rba = 0x%x\n", rba);
                  }
                SYNTAX("or	%0, %1");
#line 960 "rl78-decode.opc"
                ID(or); DR(A); SRB(rba); Fz;

              }
            break;
          case 0x69:
              {
                /** 0110 0001 0110 1001		dec	%ea0				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 0110 1001		dec	%ea0				*/",
                           op[0], op[1]);
                  }
                SYNTAX("dec	%ea0");
#line 553 "rl78-decode.opc"
                ID(sub); DM(HL, IMMU(1)); SC(1); Fza;

              }
            break;
          case 0x70:
          case 0x71:
          case 0x72:
          case 0x73:
          case 0x74:
          case 0x75:
          case 0x76:
          case 0x77:
              {
                /** 0110 0001 0111 0reg		xor	%0, %1				*/
#line 1267 "rl78-decode.opc"
                int reg AU = op[1] & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 0111 0reg		xor	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  reg = 0x%x\n", reg);
                  }
                SYNTAX("xor	%0, %1");
#line 1267 "rl78-decode.opc"
                ID(xor); DRB(reg); SR(A); Fz;

              }
            break;
          case 0x78:
          case 0x7a:
          case 0x7b:
          case 0x7c:
          case 0x7d:
          case 0x7e:
          case 0x7f:
              {
                /** 0110 0001 0111 1rba		xor	%0, %1				*/
#line 1264 "rl78-decode.opc"
                int rba AU = op[1] & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 0111 1rba		xor	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  rba = 0x%x\n", rba);
                  }
                SYNTAX("xor	%0, %1");
#line 1264 "rl78-decode.opc"
                ID(xor); DR(A); SRB(rba); Fz;

              }
            break;
          case 0x79:
              {
                /** 0110 0001 0111 1001		incw	%ea0				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 0111 1001		incw	%ea0				*/",
                           op[0], op[1]);
                  }
                SYNTAX("incw	%ea0");
#line 600 "rl78-decode.opc"
                ID(add); W(); DM(HL, IMMU(1)); SC(1);

              }
            break;
          case 0x80:
          case 0x81:
              {
                /** 0110 0001 1000 000		add	%0, %e1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1000 000		add	%0, %e1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("add	%0, %e1");
#line 209 "rl78-decode.opc"
                ID(add); DR(A); SM2(HL, B, 0); Fzac;

              }
            break;
          case 0x82:
              {
                /** 0110 0001 1000 0010		add	%0, %e1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1000 0010		add	%0, %e1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("add	%0, %e1");
#line 215 "rl78-decode.opc"
                ID(add); DR(A); SM2(HL, C, 0); Fzac;

              }
            break;
          case 0x84:
          case 0x85:
          case 0x86:
          case 0x87:
          case 0x94:
          case 0x95:
          case 0x96:
          case 0x97:
          case 0xa4:
          case 0xa5:
          case 0xa6:
          case 0xa7:
          case 0xb4:
          case 0xb5:
          case 0xb6:
          case 0xb7:
          case 0xc4:
          case 0xc5:
          case 0xc6:
          case 0xc7:
          case 0xd4:
          case 0xd5:
          case 0xd6:
          case 0xd7:
          case 0xe4:
          case 0xe5:
          case 0xe6:
          case 0xe7:
          case 0xf4:
          case 0xf5:
          case 0xf6:
          case 0xf7:
              {
                /** 0110 0001 1nnn 01mm		callt	[%x0]				*/
#line 435 "rl78-decode.opc"
                int nnn AU = (op[1] >> 4) & 0x07;
#line 435 "rl78-decode.opc"
                int mm AU = op[1] & 0x03;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1nnn 01mm		callt	[%x0]				*/",
                           op[0], op[1]);
                    printf ("  nnn = 0x%x,", nnn);
                    printf ("  mm = 0x%x\n", mm);
                  }
                SYNTAX("callt	[%x0]");
#line 435 "rl78-decode.opc"
                ID(call); DM(None, 0x80 + mm*16 + nnn*2);

              /*----------------------------------------------------------------------*/

              }
            break;
          case 0x88:
          case 0x8a:
          case 0x8b:
          case 0x8c:
          case 0x8d:
          case 0x8e:
          case 0x8f:
              {
                /** 0110 0001 1000 1reg		xch	%0, %1				*/
#line 1226 "rl78-decode.opc"
                int reg AU = op[1] & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1000 1reg		xch	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  reg = 0x%x\n", reg);
                  }
                SYNTAX("xch	%0, %1");
#line 1226 "rl78-decode.opc"
                /* Note: DECW uses reg == X, so this must follow DECW */
                ID(xch); DR(A); SRB(reg);

              }
            break;
          case 0x89:
              {
                /** 0110 0001 1000 1001		decw	%ea0				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1000 1001		decw	%ea0				*/",
                           op[0], op[1]);
                  }
                SYNTAX("decw	%ea0");
#line 567 "rl78-decode.opc"
                ID(sub); W(); DM(HL, IMMU(1)); SC(1);

              }
            break;
          case 0x90:
              {
                /** 0110 0001 1001 0000		addc	%0, %e1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1001 0000		addc	%0, %e1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("addc	%0, %e1");
#line 241 "rl78-decode.opc"
                ID(addc); DR(A); SM2(HL, B, 0); Fzac;

              }
            break;
          case 0x92:
              {
                /** 0110 0001 1001 0010		addc	%0, %e1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1001 0010		addc	%0, %e1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("addc	%0, %e1");
#line 244 "rl78-decode.opc"
                ID(addc); DR(A); SM2(HL, C, 0); Fzac;

              }
            break;
          case 0xa0:
          case 0xa1:
              {
                /** 0110 0001 1010 000		sub	%0, %e1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1010 000		sub	%0, %e1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("sub	%0, %e1");
#line 1127 "rl78-decode.opc"
                ID(sub); DR(A); SM2(HL, B, 0); Fzac;

              }
            break;
          case 0xa2:
              {
                /** 0110 0001 1010 0010		sub	%0, %e1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1010 0010		sub	%0, %e1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("sub	%0, %e1");
#line 1133 "rl78-decode.opc"
                ID(sub); DR(A); SM2(HL, C, 0); Fzac;

              }
            break;
          case 0xa8:
              {
                /** 0110 0001 1010 1000	       	xch	%0, %1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1010 1000	       	xch	%0, %1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("xch	%0, %1");
#line 1230 "rl78-decode.opc"
                ID(xch); DR(A); SM(None, SADDR);

              }
            break;
          case 0xa9:
              {
                /** 0110 0001 1010 1001		xch	%0, %e1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1010 1001		xch	%0, %e1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("xch	%0, %e1");
#line 1223 "rl78-decode.opc"
                ID(xch); DR(A); SM2(HL, C, 0);

              }
            break;
          case 0xaa:
              {
                /** 0110 0001 1010 1010		xch	%0, %e!1			*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1010 1010		xch	%0, %e!1			*/",
                           op[0], op[1]);
                  }
                SYNTAX("xch	%0, %e!1");
#line 1205 "rl78-decode.opc"
                ID(xch); DR(A); SM(None, IMMU(2));

              }
            break;
          case 0xab:
              {
                /** 0110 0001 1010 1011	       	xch	%0, %s1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1010 1011	       	xch	%0, %s1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("xch	%0, %s1");
#line 1233 "rl78-decode.opc"
                ID(xch); DR(A); SM(None, SFR);

              }
            break;
          case 0xac:
              {
                /** 0110 0001 1010 1100		xch	%0, %e1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1010 1100		xch	%0, %e1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("xch	%0, %e1");
#line 1214 "rl78-decode.opc"
                ID(xch); DR(A); SM(HL, 0);

              }
            break;
          case 0xad:
              {
                /** 0110 0001 1010 1101		xch	%0, %ea1			*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1010 1101		xch	%0, %ea1			*/",
                           op[0], op[1]);
                  }
                SYNTAX("xch	%0, %ea1");
#line 1220 "rl78-decode.opc"
                ID(xch); DR(A); SM(HL, IMMU(1));

              }
            break;
          case 0xae:
              {
                /** 0110 0001 1010 1110		xch	%0, %e1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1010 1110		xch	%0, %e1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("xch	%0, %e1");
#line 1208 "rl78-decode.opc"
                ID(xch); DR(A); SM(DE, 0);

              }
            break;
          case 0xaf:
              {
                /** 0110 0001 1010 1111		xch	%0, %ea1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1010 1111		xch	%0, %ea1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("xch	%0, %ea1");
#line 1211 "rl78-decode.opc"
                ID(xch); DR(A); SM(DE, IMMU(1));

              }
            break;
          case 0xb0:
              {
                /** 0110 0001 1011 0000		subc	%0, %e1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1011 0000		subc	%0, %e1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("subc	%0, %e1");
#line 1159 "rl78-decode.opc"
                ID(subc); DR(A); SM2(HL, B, 0); Fzac;

              }
            break;
          case 0xb2:
              {
                /** 0110 0001 1011 0010		subc	%0, %e1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1011 0010		subc	%0, %e1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("subc	%0, %e1");
#line 1162 "rl78-decode.opc"
                ID(subc); DR(A); SM2(HL, C, 0); Fzac;

              }
            break;
          case 0xb8:
              {
                /** 0110 0001 1011 1000		mov	%0, %1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1011 1000		mov	%0, %1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("mov	%0, %1");
#line 725 "rl78-decode.opc"
                ID(mov); DR(ES); SM(None, SADDR);

              }
            break;
          case 0xb9:
              {
                /** 0110 0001 1011 1001		xch	%0, %e1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1011 1001		xch	%0, %e1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("xch	%0, %e1");
#line 1217 "rl78-decode.opc"
                ID(xch); DR(A); SM2(HL, B, 0);

              }
            break;
          case 0xc0:
              {
                /** 0110 0001 1100 0000		cmp	%0, %e1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1100 0000		cmp	%0, %e1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("cmp	%0, %e1");
#line 494 "rl78-decode.opc"
                ID(cmp); DR(A); SM2(HL, B, 0); Fzac;

              }
            break;
          case 0xc2:
              {
                /** 0110 0001 1100 0010		cmp	%0, %e1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1100 0010		cmp	%0, %e1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("cmp	%0, %e1");
#line 497 "rl78-decode.opc"
                ID(cmp); DR(A); SM2(HL, C, 0); Fzac;

              }
            break;
          case 0xc3:
              {
                /** 0110 0001 1100 0011		bh	$%a0				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1100 0011		bh	$%a0				*/",
                           op[0], op[1]);
                  }
                SYNTAX("bh	$%a0");
#line 342 "rl78-decode.opc"
                ID(branch_cond); DC(pc+IMMS(1)+3); SR(None); COND(H);

              }
            break;
          case 0xc8:
              {
                /** 0110 0001 1100 1000		sk%c1					*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1100 1000		sk%c1					*/",
                           op[0], op[1]);
                  }
                SYNTAX("sk%c1");
#line 1096 "rl78-decode.opc"
                ID(skip); COND(C);

              }
            break;
          case 0xc9:
              {
                /** 0110 0001 1100 1001		mov	%0, %e1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1100 1001		mov	%0, %e1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("mov	%0, %e1");
#line 662 "rl78-decode.opc"
                ID(mov); DR(A); SM2(HL, B, 0);

              }
            break;
          case 0xca:
          case 0xda:
          case 0xea:
          case 0xfa:
              {
                /** 0110 0001 11rg 1010		call	%0				*/
#line 432 "rl78-decode.opc"
                int rg AU = (op[1] >> 4) & 0x03;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 11rg 1010		call	%0				*/",
                           op[0], op[1]);
                    printf ("  rg = 0x%x\n", rg);
                  }
                SYNTAX("call	%0");
#line 432 "rl78-decode.opc"
                ID(call); DRW(rg);

              }
            break;
          case 0xcb:
              {
                /** 0110 0001 1100 1011		br	ax				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1100 1011		br	ax				*/",
                           op[0], op[1]);
                  }
                SYNTAX("br	ax");
#line 382 "rl78-decode.opc"
                ID(branch); DR(AX);

              /*----------------------------------------------------------------------*/

              }
            break;
          case 0xcc:
              {
                /** 0110 0001 1100 1100		brk					*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1100 1100		brk					*/",
                           op[0], op[1]);
                  }
                SYNTAX("brk");
#line 390 "rl78-decode.opc"
                ID(break);

              /*----------------------------------------------------------------------*/

              }
            break;
          case 0xcd:
              {
                /** 0110 0001 1100 1101		pop	%s0				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1100 1101		pop	%s0				*/",
                           op[0], op[1]);
                  }
                SYNTAX("pop	%s0");
#line 991 "rl78-decode.opc"
                ID(mov); W(); DR(PSW); SPOP();

              /*----------------------------------------------------------------------*/

              }
            break;
          case 0xce:
              {
                /** 0110 0001 1100 1110		movs	%ea0, %1			*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1100 1110		movs	%ea0, %1			*/",
                           op[0], op[1]);
                  }
                SYNTAX("movs	%ea0, %1");
#line 813 "rl78-decode.opc"
                ID(mov); DM(HL, IMMU(1)); SR(X); Fzc;

              /*----------------------------------------------------------------------*/

              }
            break;
          case 0xcf:
          case 0xdf:
          case 0xef:
          case 0xff:
              {
                /** 0110 0001 11rb 1111		sel	rb%1				*/
#line 1043 "rl78-decode.opc"
                int rb AU = (op[1] >> 4) & 0x03;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 11rb 1111		sel	rb%1				*/",
                           op[0], op[1]);
                    printf ("  rb = 0x%x\n", rb);
                  }
                SYNTAX("sel	rb%1");
#line 1043 "rl78-decode.opc"
                ID(sel); SC(rb);

              /*----------------------------------------------------------------------*/

              }
            break;
          case 0xd0:
              {
                /** 0110 0001 1101 0000		and	%0, %e1			*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1101 0000		and	%0, %e1			*/",
                           op[0], op[1]);
                  }
                SYNTAX("and	%0, %e1");
#line 293 "rl78-decode.opc"
                ID(and); DR(A); SM2(HL, B, 0); Fz;

              }
            break;
          case 0xd2:
              {
                /** 0110 0001 1101 0010		and	%0, %e1			*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1101 0010		and	%0, %e1			*/",
                           op[0], op[1]);
                  }
                SYNTAX("and	%0, %e1");
#line 299 "rl78-decode.opc"
                ID(and); DR(A); SM2(HL, C, 0); Fz;

              }
            break;
          case 0xd3:
              {
                /** 0110 0001 1101 0011		bnh	$%a0				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1101 0011		bnh	$%a0				*/",
                           op[0], op[1]);
                  }
                SYNTAX("bnh	$%a0");
#line 345 "rl78-decode.opc"
                ID(branch_cond); DC(pc+IMMS(1)+3); SR(None); COND(NH);

              }
            break;
          case 0xd8:
              {
                /** 0110 0001 1101 1000		sk%c1					*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1101 1000		sk%c1					*/",
                           op[0], op[1]);
                  }
                SYNTAX("sk%c1");
#line 1102 "rl78-decode.opc"
                ID(skip); COND(NC);

              }
            break;
          case 0xd9:
              {
                /** 0110 0001 1101 1001		mov	%e0, %1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1101 1001		mov	%e0, %1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("mov	%e0, %1");
#line 629 "rl78-decode.opc"
                ID(mov); DM2(HL, B, 0); SR(A);

              }
            break;
          case 0xdb:
              {
                /** 0110 0001 1101 1011		ror	%0, %1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1101 1011		ror	%0, %1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("ror	%0, %1");
#line 1024 "rl78-decode.opc"
                ID(ror); DR(A); SC(1);

              }
            break;
          case 0xdc:
              {
                /** 0110 0001 1101 1100		rolc	%0, %1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1101 1100		rolc	%0, %1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("rolc	%0, %1");
#line 1018 "rl78-decode.opc"
                ID(rolc); DR(A); SC(1);

              }
            break;
          case 0xdd:
              {
                /** 0110 0001 1101 1101		push	%s1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1101 1101		push	%s1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("push	%s1");
#line 999 "rl78-decode.opc"
                ID(mov); W(); DPUSH(); SR(PSW);

              /*----------------------------------------------------------------------*/

              }
            break;
          case 0xde:
              {
                /** 0110 0001 1101 1110		cmps	%0, %ea1			*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1101 1110		cmps	%0, %ea1			*/",
                           op[0], op[1]);
                  }
                SYNTAX("cmps	%0, %ea1");
#line 528 "rl78-decode.opc"
                ID(cmp); DR(X); SM(HL, IMMU(1)); Fzac;

              /*----------------------------------------------------------------------*/

              }
            break;
          case 0xe0:
              {
                /** 0110 0001 1110 0000		or	%0, %e1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1110 0000		or	%0, %e1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("or	%0, %e1");
#line 948 "rl78-decode.opc"
                ID(or); DR(A); SM2(HL, B, 0); Fz;

              }
            break;
          case 0xe2:
              {
                /** 0110 0001 1110 0010		or	%0, %e1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1110 0010		or	%0, %e1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("or	%0, %e1");
#line 954 "rl78-decode.opc"
                ID(or); DR(A); SM2(HL, C, 0); Fz;

              }
            break;
          case 0xe3:
              {
                /** 0110 0001 1110 0011		sk%c1					*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1110 0011		sk%c1					*/",
                           op[0], op[1]);
                  }
                SYNTAX("sk%c1");
#line 1099 "rl78-decode.opc"
                ID(skip); COND(H);

              }
            break;
          case 0xe8:
              {
                /** 0110 0001 1110 1000		sk%c1					*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1110 1000		sk%c1					*/",
                           op[0], op[1]);
                  }
                SYNTAX("sk%c1");
#line 1111 "rl78-decode.opc"
                ID(skip); COND(Z);

              /*----------------------------------------------------------------------*/

              }
            break;
          case 0xe9:
              {
                /** 0110 0001 1110 1001		mov	%0, %e1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1110 1001		mov	%0, %e1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("mov	%0, %e1");
#line 665 "rl78-decode.opc"
                ID(mov); DR(A); SM2(HL, C, 0);

              }
            break;
          case 0xeb:
              {
                /** 0110 0001 1110 1011		rol	%0, %1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1110 1011		rol	%0, %1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("rol	%0, %1");
#line 1015 "rl78-decode.opc"
                ID(rol); DR(A); SC(1);

              }
            break;
          case 0xec:
              {
                /** 0110 0001 1110 1100		retb					*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1110 1100		retb					*/",
                           op[0], op[1]);
                  }
                SYNTAX("retb");
#line 1010 "rl78-decode.opc"
                ID(reti);

              /*----------------------------------------------------------------------*/

              }
            break;
          case 0xed:
              {
                /** 0110 0001 1110 1101		halt					*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1110 1101		halt					*/",
                           op[0], op[1]);
                  }
                SYNTAX("halt");
#line 578 "rl78-decode.opc"
                ID(halt);

              /*----------------------------------------------------------------------*/

              }
            break;
          case 0xee:
          case 0xfe:
              {
                /** 0110 0001 111r 1110		rolwc	%0, %1				*/
#line 1021 "rl78-decode.opc"
                int r AU = (op[1] >> 4) & 0x01;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 111r 1110		rolwc	%0, %1				*/",
                           op[0], op[1]);
                    printf ("  r = 0x%x\n", r);
                  }
                SYNTAX("rolwc	%0, %1");
#line 1021 "rl78-decode.opc"
                ID(rolc); W(); DRW(r); SC(1);

              }
            break;
          case 0xf0:
              {
                /** 0110 0001 1111 0000		xor	%0, %e1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1111 0000		xor	%0, %e1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("xor	%0, %e1");
#line 1252 "rl78-decode.opc"
                ID(xor); DR(A); SM2(HL, B, 0); Fz;

              }
            break;
          case 0xf2:
              {
                /** 0110 0001 1111 0010		xor	%0, %e1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1111 0010		xor	%0, %e1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("xor	%0, %e1");
#line 1258 "rl78-decode.opc"
                ID(xor); DR(A); SM2(HL, C, 0); Fz;

              }
            break;
          case 0xf3:
              {
                /** 0110 0001 1111 0011		sk%c1					*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1111 0011		sk%c1					*/",
                           op[0], op[1]);
                  }
                SYNTAX("sk%c1");
#line 1105 "rl78-decode.opc"
                ID(skip); COND(NH);

              }
            break;
          case 0xf8:
              {
                /** 0110 0001 1111 1000		sk%c1					*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1111 1000		sk%c1					*/",
                           op[0], op[1]);
                  }
                SYNTAX("sk%c1");
#line 1108 "rl78-decode.opc"
                ID(skip); COND(NZ);

              }
            break;
          case 0xf9:
              {
                /** 0110 0001 1111 1001		mov	%e0, %1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1111 1001		mov	%e0, %1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("mov	%e0, %1");
#line 638 "rl78-decode.opc"
                ID(mov); DM2(HL, C, 0); SR(A);

              }
            break;
          case 0xfb:
              {
                /** 0110 0001 1111 1011		rorc	%0, %1				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1111 1011		rorc	%0, %1				*/",
                           op[0], op[1]);
                  }
                SYNTAX("rorc	%0, %1");
#line 1027 "rl78-decode.opc"
                ID(rorc); DR(A); SC(1);

              /*----------------------------------------------------------------------*/

              /* Note that the branch insns need to be listed before the shift
                 ones, as "shift count of zero" means "branch insn" */

              }
            break;
          case 0xfc:
              {
                /** 0110 0001 1111 1100		reti					*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1111 1100		reti					*/",
                           op[0], op[1]);
                  }
                SYNTAX("reti");
#line 1007 "rl78-decode.opc"
                ID(reti);

              }
            break;
          case 0xfd:
              {
                /** 0110 0001 1111 1101	stop						*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 1111 1101	stop						*/",
                           op[0], op[1]);
                  }
                SYNTAX("stop");
#line 1116 "rl78-decode.opc"
                ID(stop);

              /*----------------------------------------------------------------------*/

              }
            break;
          default: UNSUPPORTED(); break;
        }
      break;
    case 0x68:
        {
          /** 0110 1000			movw	%e0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0110 1000			movw	%e0, %1				*/",
                     op[0]);
            }
          SYNTAX("movw	%e0, %1");
#line 876 "rl78-decode.opc"
          ID(mov); W(); DM(C, IMMU(2)); SR(AX);

        }
      break;
    case 0x69:
        {
          /** 0110 1001			movw	%0, %e1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0110 1001			movw	%0, %e1				*/",
                     op[0]);
            }
          SYNTAX("movw	%0, %e1");
#line 867 "rl78-decode.opc"
          ID(mov); W(); DR(AX); SM(C, IMMU(2));

        }
      break;
    case 0x6a:
        {
          /** 0110 1010	       		or	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0110 1010	       		or	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("or	%0, #%1");
#line 969 "rl78-decode.opc"
          ID(or); DM(None, SADDR); SC(IMMU(1)); Fz;

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0x6b:
        {
          /** 0110 1011	       		or	%0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0110 1011	       		or	%0, %1				*/",
                     op[0]);
            }
          SYNTAX("or	%0, %1");
#line 966 "rl78-decode.opc"
          ID(or); DR(A); SM(None, SADDR); Fz;

        }
      break;
    case 0x6c:
        {
          /** 0110 1100	       		or	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0110 1100	       		or	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("or	%0, #%1");
#line 957 "rl78-decode.opc"
          ID(or); DR(A); SC(IMMU(1)); Fz;

        }
      break;
    case 0x6d:
        {
          /** 0110 1101			or	%0, %e1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0110 1101			or	%0, %e1				*/",
                     op[0]);
            }
          SYNTAX("or	%0, %e1");
#line 945 "rl78-decode.opc"
          ID(or); DR(A); SM(HL, 0); Fz;

        }
      break;
    case 0x6e:
        {
          /** 0110 1110			or	%0, %ea1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0110 1110			or	%0, %ea1			*/",
                     op[0]);
            }
          SYNTAX("or	%0, %ea1");
#line 951 "rl78-decode.opc"
          ID(or); DR(A); SM(HL, IMMU(1)); Fz;

        }
      break;
    case 0x6f:
        {
          /** 0110 1111			or	%0, %e!1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0110 1111			or	%0, %e!1			*/",
                     op[0]);
            }
          SYNTAX("or	%0, %e!1");
#line 942 "rl78-decode.opc"
          ID(or); DR(A); SM(None, IMMU(2)); Fz;

        }
      break;
    case 0x70:
    case 0x72:
    case 0x73:
    case 0x74:
    case 0x75:
    case 0x76:
    case 0x77:
        {
          /** 0111 0rba			mov	%0, %1				*/
#line 698 "rl78-decode.opc"
          int rba AU = op[0] & 0x07;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0111 0rba			mov	%0, %1				*/",
                     op[0]);
              printf ("  rba = 0x%x\n", rba);
            }
          SYNTAX("mov	%0, %1");
#line 698 "rl78-decode.opc"
          ID(mov); DRB(rba); SR(A);

        }
      break;
    case 0x71:
        GETBYTE ();
        switch (op[1] & 0xff)
        {
          case 0x00:
          case 0x10:
          case 0x20:
          case 0x30:
          case 0x40:
          case 0x50:
          case 0x60:
          case 0x70:
              {
                /** 0111 0001 0bit 0000		set1	%e!0				*/
#line 1048 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 0bit 0000		set1	%e!0				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("set1	%e!0");
#line 1048 "rl78-decode.opc"
                ID(mov); DM(None, IMMU(2)); DB(bit); SC(1);

              }
            break;
          case 0x01:
          case 0x11:
          case 0x21:
          case 0x31:
          case 0x41:
          case 0x51:
          case 0x61:
          case 0x71:
              {
                /** 0111 0001 0bit 0001		mov1	%0, cy				*/
#line 805 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 0bit 0001		mov1	%0, cy				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("mov1	%0, cy");
#line 805 "rl78-decode.opc"
                ID(mov); DM(None, SADDR); DB(bit); SCY();

              }
            break;
          case 0x02:
          case 0x12:
          case 0x22:
          case 0x32:
          case 0x42:
          case 0x52:
          case 0x62:
          case 0x72:
              {
                /** 0111 0001 0bit 0010		set1	%0				*/
#line 1066 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 0bit 0010		set1	%0				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("set1	%0");
#line 1066 "rl78-decode.opc"
                ID(mov); DM(None, SADDR); DB(bit); SC(1);

              /*----------------------------------------------------------------------*/

              }
            break;
          case 0x03:
          case 0x13:
          case 0x23:
          case 0x33:
          case 0x43:
          case 0x53:
          case 0x63:
          case 0x73:
              {
                /** 0111 0001 0bit 0011		clr1	%0				*/
#line 458 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 0bit 0011		clr1	%0				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("clr1	%0");
#line 458 "rl78-decode.opc"
                ID(mov); DM(None, SADDR); DB(bit); SC(0);

              /*----------------------------------------------------------------------*/

              }
            break;
          case 0x04:
          case 0x14:
          case 0x24:
          case 0x34:
          case 0x44:
          case 0x54:
          case 0x64:
          case 0x74:
              {
                /** 0111 0001 0bit 0100		mov1	cy, %1				*/
#line 799 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 0bit 0100		mov1	cy, %1				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("mov1	cy, %1");
#line 799 "rl78-decode.opc"
                ID(mov); DCY(); SM(None, SADDR); SB(bit);

              }
            break;
          case 0x05:
          case 0x15:
          case 0x25:
          case 0x35:
          case 0x45:
          case 0x55:
          case 0x65:
          case 0x75:
              {
                /** 0111 0001 0bit 0101		and1	cy, %s1				*/
#line 328 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 0bit 0101		and1	cy, %s1				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("and1	cy, %s1");
#line 328 "rl78-decode.opc"
                ID(and); DCY(); SM(None, SADDR); SB(bit);

              /*----------------------------------------------------------------------*/

              /* Note that the branch insns need to be listed before the shift
                 ones, as "shift count of zero" means "branch insn" */

              }
            break;
          case 0x06:
          case 0x16:
          case 0x26:
          case 0x36:
          case 0x46:
          case 0x56:
          case 0x66:
          case 0x76:
              {
                /** 0111 0001 0bit 0110		or1	cy, %s1				*/
#line 983 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 0bit 0110		or1	cy, %s1				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("or1	cy, %s1");
#line 983 "rl78-decode.opc"
                ID(or); DCY(); SM(None, SADDR); SB(bit);

              /*----------------------------------------------------------------------*/

              }
            break;
          case 0x07:
          case 0x17:
          case 0x27:
          case 0x37:
          case 0x47:
          case 0x57:
          case 0x67:
          case 0x77:
              {
                /** 0111 0001 0bit 0111		xor1	cy, %s1				*/
#line 1287 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 0bit 0111		xor1	cy, %s1				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("xor1	cy, %s1");
#line 1287 "rl78-decode.opc"
                ID(xor); DCY(); SM(None, SADDR); SB(bit);

              /*----------------------------------------------------------------------*/

              }
            break;
          case 0x08:
          case 0x18:
          case 0x28:
          case 0x38:
          case 0x48:
          case 0x58:
          case 0x68:
          case 0x78:
              {
                /** 0111 0001 0bit 1000		clr1	%e!0				*/
#line 440 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 0bit 1000		clr1	%e!0				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("clr1	%e!0");
#line 440 "rl78-decode.opc"
                ID(mov); DM(None, IMMU(2)); DB(bit); SC(0);

              }
            break;
          case 0x09:
          case 0x19:
          case 0x29:
          case 0x39:
          case 0x49:
          case 0x59:
          case 0x69:
          case 0x79:
              {
                /** 0111 0001 0bit 1001		mov1	%s0, cy				*/
#line 808 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 0bit 1001		mov1	%s0, cy				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("mov1	%s0, cy");
#line 808 "rl78-decode.opc"
                ID(mov); DM(None, SFR); DB(bit); SCY();

              /*----------------------------------------------------------------------*/

              }
            break;
          case 0x0a:
          case 0x1a:
          case 0x2a:
          case 0x3a:
          case 0x4a:
          case 0x5a:
          case 0x6a:
          case 0x7a:
              {
                /** 0111 0001 0bit 1010		set1	%s0				*/
#line 1060 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 0bit 1010		set1	%s0				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("set1	%s0");
#line 1060 "rl78-decode.opc"
                op0 = SFR;
                ID(mov); DM(None, op0); DB(bit); SC(1);
                if (op0 == RL78_SFR_PSW && bit == 7)
                  rl78->syntax = "ei";

              }
            break;
          case 0x0b:
          case 0x1b:
          case 0x2b:
          case 0x3b:
          case 0x4b:
          case 0x5b:
          case 0x6b:
          case 0x7b:
              {
                /** 0111 0001 0bit 1011		clr1	%s0				*/
#line 452 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 0bit 1011		clr1	%s0				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("clr1	%s0");
#line 452 "rl78-decode.opc"
                op0 = SFR;
                ID(mov); DM(None, op0); DB(bit); SC(0);
                if (op0 == RL78_SFR_PSW && bit == 7)
                  rl78->syntax = "di";

              }
            break;
          case 0x0c:
          case 0x1c:
          case 0x2c:
          case 0x3c:
          case 0x4c:
          case 0x5c:
          case 0x6c:
          case 0x7c:
              {
                /** 0111 0001 0bit 1100		mov1	cy, %s1				*/
#line 802 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 0bit 1100		mov1	cy, %s1				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("mov1	cy, %s1");
#line 802 "rl78-decode.opc"
                ID(mov); DCY(); SM(None, SFR); SB(bit);

              }
            break;
          case 0x0d:
          case 0x1d:
          case 0x2d:
          case 0x3d:
          case 0x4d:
          case 0x5d:
          case 0x6d:
          case 0x7d:
              {
                /** 0111 0001 0bit 1101		and1	cy, %s1				*/
#line 325 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 0bit 1101		and1	cy, %s1				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("and1	cy, %s1");
#line 325 "rl78-decode.opc"
                ID(and); DCY(); SM(None, SFR); SB(bit);

              }
            break;
          case 0x0e:
          case 0x1e:
          case 0x2e:
          case 0x3e:
          case 0x4e:
          case 0x5e:
          case 0x6e:
          case 0x7e:
              {
                /** 0111 0001 0bit 1110		or1	cy, %s1				*/
#line 980 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 0bit 1110		or1	cy, %s1				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("or1	cy, %s1");
#line 980 "rl78-decode.opc"
                ID(or); DCY(); SM(None, SFR); SB(bit);

              }
            break;
          case 0x0f:
          case 0x1f:
          case 0x2f:
          case 0x3f:
          case 0x4f:
          case 0x5f:
          case 0x6f:
          case 0x7f:
              {
                /** 0111 0001 0bit 1111		xor1	cy, %s1				*/
#line 1284 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 0bit 1111		xor1	cy, %s1				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("xor1	cy, %s1");
#line 1284 "rl78-decode.opc"
                ID(xor); DCY(); SM(None, SFR); SB(bit);

              }
            break;
          case 0x80:
              {
                /** 0111 0001 1000 0000		set1	cy				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 1000 0000		set1	cy				*/",
                           op[0], op[1]);
                  }
                SYNTAX("set1	cy");
#line 1057 "rl78-decode.opc"
                ID(mov); DCY(); SC(1);

              }
            break;
          case 0x81:
          case 0x91:
          case 0xa1:
          case 0xb1:
          case 0xc1:
          case 0xd1:
          case 0xe1:
          case 0xf1:
              {
                /** 0111 0001 1bit 0001		mov1	%e0, cy				*/
#line 787 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 1bit 0001		mov1	%e0, cy				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("mov1	%e0, cy");
#line 787 "rl78-decode.opc"
                ID(mov); DM(HL, 0); DB(bit); SCY();

              }
            break;
          case 0x82:
          case 0x92:
          case 0xa2:
          case 0xb2:
          case 0xc2:
          case 0xd2:
          case 0xe2:
          case 0xf2:
              {
                /** 0111 0001 1bit 0010		set1	%e0				*/
#line 1051 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 1bit 0010		set1	%e0				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("set1	%e0");
#line 1051 "rl78-decode.opc"
                ID(mov); DM(HL, 0); DB(bit); SC(1);

              }
            break;
          case 0x83:
          case 0x93:
          case 0xa3:
          case 0xb3:
          case 0xc3:
          case 0xd3:
          case 0xe3:
          case 0xf3:
              {
                /** 0111 0001 1bit 0011		clr1	%e0				*/
#line 443 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 1bit 0011		clr1	%e0				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("clr1	%e0");
#line 443 "rl78-decode.opc"
                ID(mov); DM(HL, 0); DB(bit); SC(0);

              }
            break;
          case 0x84:
          case 0x94:
          case 0xa4:
          case 0xb4:
          case 0xc4:
          case 0xd4:
          case 0xe4:
          case 0xf4:
              {
                /** 0111 0001 1bit 0100		mov1	cy, %e1				*/
#line 793 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 1bit 0100		mov1	cy, %e1				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("mov1	cy, %e1");
#line 793 "rl78-decode.opc"
                ID(mov); DCY(); SM(HL, 0); SB(bit);

              }
            break;
          case 0x85:
          case 0x95:
          case 0xa5:
          case 0xb5:
          case 0xc5:
          case 0xd5:
          case 0xe5:
          case 0xf5:
              {
                /** 0111 0001 1bit 0101		and1	cy, %e1			*/
#line 319 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 1bit 0101		and1	cy, %e1			*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("and1	cy, %e1");
#line 319 "rl78-decode.opc"
                ID(and); DCY(); SM(HL, 0); SB(bit);

              }
            break;
          case 0x86:
          case 0x96:
          case 0xa6:
          case 0xb6:
          case 0xc6:
          case 0xd6:
          case 0xe6:
          case 0xf6:
              {
                /** 0111 0001 1bit 0110		or1	cy, %e1				*/
#line 974 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 1bit 0110		or1	cy, %e1				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("or1	cy, %e1");
#line 974 "rl78-decode.opc"
                ID(or); DCY(); SM(HL, 0); SB(bit);

              }
            break;
          case 0x87:
          case 0x97:
          case 0xa7:
          case 0xb7:
          case 0xc7:
          case 0xd7:
          case 0xe7:
          case 0xf7:
              {
                /** 0111 0001 1bit 0111		xor1	cy, %e1				*/
#line 1278 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 1bit 0111		xor1	cy, %e1				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("xor1	cy, %e1");
#line 1278 "rl78-decode.opc"
                ID(xor); DCY(); SM(HL, 0); SB(bit);

              }
            break;
          case 0x88:
              {
                /** 0111 0001 1000 1000		clr1	cy				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 1000 1000		clr1	cy				*/",
                           op[0], op[1]);
                  }
                SYNTAX("clr1	cy");
#line 449 "rl78-decode.opc"
                ID(mov); DCY(); SC(0);

              }
            break;
          case 0x89:
          case 0x99:
          case 0xa9:
          case 0xb9:
          case 0xc9:
          case 0xd9:
          case 0xe9:
          case 0xf9:
              {
                /** 0111 0001 1bit 1001		mov1	%e0, cy				*/
#line 790 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 1bit 1001		mov1	%e0, cy				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("mov1	%e0, cy");
#line 790 "rl78-decode.opc"
                ID(mov); DR(A); DB(bit); SCY();

              }
            break;
          case 0x8a:
          case 0x9a:
          case 0xaa:
          case 0xba:
          case 0xca:
          case 0xda:
          case 0xea:
          case 0xfa:
              {
                /** 0111 0001 1bit 1010		set1	%0				*/
#line 1054 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 1bit 1010		set1	%0				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("set1	%0");
#line 1054 "rl78-decode.opc"
                ID(mov); DR(A); DB(bit); SC(1);

              }
            break;
          case 0x8b:
          case 0x9b:
          case 0xab:
          case 0xbb:
          case 0xcb:
          case 0xdb:
          case 0xeb:
          case 0xfb:
              {
                /** 0111 0001 1bit 1011		clr1	%0				*/
#line 446 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 1bit 1011		clr1	%0				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("clr1	%0");
#line 446 "rl78-decode.opc"
                ID(mov); DR(A); DB(bit); SC(0);

              }
            break;
          case 0x8c:
          case 0x9c:
          case 0xac:
          case 0xbc:
          case 0xcc:
          case 0xdc:
          case 0xec:
          case 0xfc:
              {
                /** 0111 0001 1bit 1100		mov1	cy, %e1				*/
#line 796 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 1bit 1100		mov1	cy, %e1				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("mov1	cy, %e1");
#line 796 "rl78-decode.opc"
                ID(mov); DCY(); SR(A); SB(bit);

              }
            break;
          case 0x8d:
          case 0x9d:
          case 0xad:
          case 0xbd:
          case 0xcd:
          case 0xdd:
          case 0xed:
          case 0xfd:
              {
                /** 0111 0001 1bit 1101		and1	cy, %1				*/
#line 322 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 1bit 1101		and1	cy, %1				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("and1	cy, %1");
#line 322 "rl78-decode.opc"
                ID(and); DCY(); SR(A); SB(bit);

              }
            break;
          case 0x8e:
          case 0x9e:
          case 0xae:
          case 0xbe:
          case 0xce:
          case 0xde:
          case 0xee:
          case 0xfe:
              {
                /** 0111 0001 1bit 1110		or1	cy, %1				*/
#line 977 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 1bit 1110		or1	cy, %1				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("or1	cy, %1");
#line 977 "rl78-decode.opc"
                ID(or); DCY(); SR(A); SB(bit);

              }
            break;
          case 0x8f:
          case 0x9f:
          case 0xaf:
          case 0xbf:
          case 0xcf:
          case 0xdf:
          case 0xef:
          case 0xff:
              {
                /** 0111 0001 1bit 1111		xor1	cy, %1				*/
#line 1281 "rl78-decode.opc"
                int bit AU = (op[1] >> 4) & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 1bit 1111		xor1	cy, %1				*/",
                           op[0], op[1]);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("xor1	cy, %1");
#line 1281 "rl78-decode.opc"
                ID(xor); DCY(); SR(A); SB(bit);

              }
            break;
          case 0xc0:
              {
                /** 0111 0001 1100 0000		not1	cy				*/
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0001 1100 0000		not1	cy				*/",
                           op[0], op[1]);
                  }
                SYNTAX("not1	cy");
#line 918 "rl78-decode.opc"
                ID(xor); DCY(); SC(1);

              /*----------------------------------------------------------------------*/

              }
            break;
          default: UNSUPPORTED(); break;
        }
      break;
    case 0x78:
        {
          /** 0111 1000			movw	%e0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0111 1000			movw	%e0, %1				*/",
                     op[0]);
            }
          SYNTAX("movw	%e0, %1");
#line 879 "rl78-decode.opc"
          ID(mov); W(); DM(BC, IMMU(2)); SR(AX);

        }
      break;
    case 0x79:
        {
          /** 0111 1001			movw	%0, %e1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0111 1001			movw	%0, %e1				*/",
                     op[0]);
            }
          SYNTAX("movw	%0, %e1");
#line 870 "rl78-decode.opc"
          ID(mov); W(); DR(AX); SM(BC, IMMU(2));

        }
      break;
    case 0x7a:
        {
          /** 0111 1010	       		xor	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0111 1010	       		xor	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("xor	%0, #%1");
#line 1273 "rl78-decode.opc"
          ID(xor); DM(None, SADDR); SC(IMMU(1)); Fz;

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0x7b:
        {
          /** 0111 1011	       		xor	%0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0111 1011	       		xor	%0, %1				*/",
                     op[0]);
            }
          SYNTAX("xor	%0, %1");
#line 1270 "rl78-decode.opc"
          ID(xor); DR(A); SM(None, SADDR); Fz;

        }
      break;
    case 0x7c:
        {
          /** 0111 1100	       		xor	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0111 1100	       		xor	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("xor	%0, #%1");
#line 1261 "rl78-decode.opc"
          ID(xor); DR(A); SC(IMMU(1)); Fz;

        }
      break;
    case 0x7d:
        {
          /** 0111 1101			xor	%0, %e1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0111 1101			xor	%0, %e1				*/",
                     op[0]);
            }
          SYNTAX("xor	%0, %e1");
#line 1249 "rl78-decode.opc"
          ID(xor); DR(A); SM(HL, 0); Fz;

        }
      break;
    case 0x7e:
        {
          /** 0111 1110			xor	%0, %ea1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0111 1110			xor	%0, %ea1			*/",
                     op[0]);
            }
          SYNTAX("xor	%0, %ea1");
#line 1255 "rl78-decode.opc"
          ID(xor); DR(A); SM(HL, IMMU(1)); Fz;

        }
      break;
    case 0x7f:
        {
          /** 0111 1111			xor	%0, %e!1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0111 1111			xor	%0, %e!1			*/",
                     op[0]);
            }
          SYNTAX("xor	%0, %e!1");
#line 1246 "rl78-decode.opc"
          ID(xor); DR(A); SM(None, IMMU(2)); Fz;

        }
      break;
    case 0x80:
    case 0x81:
    case 0x82:
    case 0x83:
    case 0x84:
    case 0x85:
    case 0x86:
    case 0x87:
        {
          /** 1000 0reg			inc	%0				*/
#line 589 "rl78-decode.opc"
          int reg AU = op[0] & 0x07;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1000 0reg			inc	%0				*/",
                     op[0]);
              printf ("  reg = 0x%x\n", reg);
            }
          SYNTAX("inc	%0");
#line 589 "rl78-decode.opc"
          ID(add); DRB(reg); SC(1); Fza;

        }
      break;
    case 0x88:
        {
          /** 1000 1000			mov	%0, %ea1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1000 1000			mov	%0, %ea1			*/",
                     op[0]);
            }
          SYNTAX("mov	%0, %ea1");
#line 668 "rl78-decode.opc"
          ID(mov); DR(A); SM(SP, IMMU(1));

        }
      break;
    case 0x89:
        {
          /** 1000 1001			mov	%0, %e1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1000 1001			mov	%0, %e1				*/",
                     op[0]);
            }
          SYNTAX("mov	%0, %e1");
#line 650 "rl78-decode.opc"
          ID(mov); DR(A); SM(DE, 0);

        }
      break;
    case 0x8a:
        {
          /** 1000 1010			mov	%0, %ea1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1000 1010			mov	%0, %ea1			*/",
                     op[0]);
            }
          SYNTAX("mov	%0, %ea1");
#line 653 "rl78-decode.opc"
          ID(mov); DR(A); SM(DE, IMMU(1));

        }
      break;
    case 0x8b:
        {
          /** 1000 1011			mov	%0, %e1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1000 1011			mov	%0, %e1				*/",
                     op[0]);
            }
          SYNTAX("mov	%0, %e1");
#line 656 "rl78-decode.opc"
          ID(mov); DR(A); SM(HL, 0);

        }
      break;
    case 0x8c:
        {
          /** 1000 1100			mov	%0, %ea1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1000 1100			mov	%0, %ea1			*/",
                     op[0]);
            }
          SYNTAX("mov	%0, %ea1");
#line 659 "rl78-decode.opc"
          ID(mov); DR(A); SM(HL, IMMU(1));

        }
      break;
    case 0x8d:
        {
          /** 1000 1101			mov	%0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1000 1101			mov	%0, %1				*/",
                     op[0]);
            }
          SYNTAX("mov	%0, %1");
#line 692 "rl78-decode.opc"
          ID(mov); DR(A); SM(None, SADDR);

        }
      break;
    case 0x8e:
        {
          /** 1000 1110			mov	%0, %s1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1000 1110			mov	%0, %s1				*/",
                     op[0]);
            }
          SYNTAX("mov	%0, %s1");
#line 689 "rl78-decode.opc"
          ID(mov); DR(A); SM(None, SFR);

        }
      break;
    case 0x8f:
        {
          /** 1000 1111			mov	%0, %e!1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1000 1111			mov	%0, %e!1			*/",
                     op[0]);
            }
          SYNTAX("mov	%0, %e!1");
#line 647 "rl78-decode.opc"
          ID(mov); DR(A); SM(None, IMMU(2));

        }
      break;
    case 0x90:
    case 0x91:
    case 0x92:
    case 0x93:
    case 0x94:
    case 0x95:
    case 0x96:
    case 0x97:
        {
          /** 1001 0reg			dec	%0				*/
#line 556 "rl78-decode.opc"
          int reg AU = op[0] & 0x07;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1001 0reg			dec	%0				*/",
                     op[0]);
              printf ("  reg = 0x%x\n", reg);
            }
          SYNTAX("dec	%0");
#line 556 "rl78-decode.opc"
          ID(sub); DRB(reg); SC(1); Fza;

        }
      break;
    case 0x98:
        {
          /** 1001 1000			mov	%a0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1001 1000			mov	%a0, %1				*/",
                     op[0]);
            }
          SYNTAX("mov	%a0, %1");
#line 644 "rl78-decode.opc"
          ID(mov); DM(SP, IMMU(1)); SR(A);

        }
      break;
    case 0x99:
        {
          /** 1001 1001			mov	%e0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1001 1001			mov	%e0, %1				*/",
                     op[0]);
            }
          SYNTAX("mov	%e0, %1");
#line 617 "rl78-decode.opc"
          ID(mov); DM(DE, 0); SR(A);

        }
      break;
    case 0x9a:
        {
          /** 1001 1010			mov	%ea0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1001 1010			mov	%ea0, %1				*/",
                     op[0]);
            }
          SYNTAX("mov	%ea0, %1");
#line 623 "rl78-decode.opc"
          ID(mov); DM(DE, IMMU(1)); SR(A);

        }
      break;
    case 0x9b:
        {
          /** 1001 1011			mov	%e0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1001 1011			mov	%e0, %1				*/",
                     op[0]);
            }
          SYNTAX("mov	%e0, %1");
#line 626 "rl78-decode.opc"
          ID(mov); DM(HL, 0); SR(A);

        }
      break;
    case 0x9c:
        {
          /** 1001 1100			mov	%ea0, %1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1001 1100			mov	%ea0, %1			*/",
                     op[0]);
            }
          SYNTAX("mov	%ea0, %1");
#line 635 "rl78-decode.opc"
          ID(mov); DM(HL, IMMU(1)); SR(A);

        }
      break;
    case 0x9d:
        {
          /** 1001 1101			mov	%0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1001 1101			mov	%0, %1				*/",
                     op[0]);
            }
          SYNTAX("mov	%0, %1");
#line 749 "rl78-decode.opc"
          ID(mov); DM(None, SADDR); SR(A);

        }
      break;
    case 0x9e:
        {
          /** 1001 1110			mov	%s0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1001 1110			mov	%s0, %1				*/",
                     op[0]);
            }
          SYNTAX("mov	%s0, %1");
#line 782 "rl78-decode.opc"
          ID(mov); DM(None, SFR); SR(A);

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0x9f:
        {
          /** 1001 1111			mov	%e!0, %1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1001 1111			mov	%e!0, %1			*/",
                     op[0]);
            }
          SYNTAX("mov	%e!0, %1");
#line 614 "rl78-decode.opc"
          ID(mov); DM(None, IMMU(2)); SR(A);

        }
      break;
    case 0xa0:
        {
          /** 1010 0000			inc	%e!0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1010 0000			inc	%e!0				*/",
                     op[0]);
            }
          SYNTAX("inc	%e!0");
#line 583 "rl78-decode.opc"
          ID(add); DM(None, IMMU(2)); SC(1); Fza;

        }
      break;
    case 0xa1:
    case 0xa3:
    case 0xa5:
    case 0xa7:
        {
          /** 1010 0rg1			incw	%0				*/
#line 603 "rl78-decode.opc"
          int rg AU = (op[0] >> 1) & 0x03;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1010 0rg1			incw	%0				*/",
                     op[0]);
              printf ("  rg = 0x%x\n", rg);
            }
          SYNTAX("incw	%0");
#line 603 "rl78-decode.opc"
          ID(add); W(); DRW(rg); SC(1);

        }
      break;
    case 0xa2:
        {
          /** 1010 0010			incw	%e!0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1010 0010			incw	%e!0				*/",
                     op[0]);
            }
          SYNTAX("incw	%e!0");
#line 597 "rl78-decode.opc"
          ID(add); W(); DM(None, IMMU(2)); SC(1);

        }
      break;
    case 0xa4:
        {
          /** 1010 0100			inc	%0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1010 0100			inc	%0				*/",
                     op[0]);
            }
          SYNTAX("inc	%0");
#line 592 "rl78-decode.opc"
          ID(add); DM(None, SADDR); SC(1); Fza;

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0xa6:
        {
          /** 1010 0110			incw	%0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1010 0110			incw	%0				*/",
                     op[0]);
            }
          SYNTAX("incw	%0");
#line 606 "rl78-decode.opc"
          ID(add); W(); DM(None, SADDR); SC(1);

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0xa8:
        {
          /** 1010 1000			movw	%0, %a1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1010 1000			movw	%0, %a1				*/",
                     op[0]);
            }
          SYNTAX("movw	%0, %a1");
#line 852 "rl78-decode.opc"
          ID(mov); W(); DR(AX); SM(SP, IMMU(1));

        }
      break;
    case 0xa9:
        {
          /** 1010 1001			movw	%0, %e1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1010 1001			movw	%0, %e1				*/",
                     op[0]);
            }
          SYNTAX("movw	%0, %e1");
#line 840 "rl78-decode.opc"
          ID(mov); W(); DR(AX); SM(DE, 0);

        }
      break;
    case 0xaa:
        {
          /** 1010 1010			movw	%0, %ea1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1010 1010			movw	%0, %ea1				*/",
                     op[0]);
            }
          SYNTAX("movw	%0, %ea1");
#line 843 "rl78-decode.opc"
          ID(mov); W(); DR(AX); SM(DE, IMMU(1));

        }
      break;
    case 0xab:
        {
          /** 1010 1011			movw	%0, %e1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1010 1011			movw	%0, %e1				*/",
                     op[0]);
            }
          SYNTAX("movw	%0, %e1");
#line 846 "rl78-decode.opc"
          ID(mov); W(); DR(AX); SM(HL, 0);

        }
      break;
    case 0xac:
        {
          /** 1010 1100			movw	%0, %ea1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1010 1100			movw	%0, %ea1			*/",
                     op[0]);
            }
          SYNTAX("movw	%0, %ea1");
#line 849 "rl78-decode.opc"
          ID(mov); W(); DR(AX); SM(HL, IMMU(1));

        }
      break;
    case 0xad:
        {
          /** 1010 1101			movw	%0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1010 1101			movw	%0, %1				*/",
                     op[0]);
            }
          SYNTAX("movw	%0, %1");
#line 882 "rl78-decode.opc"
          ID(mov); W(); DR(AX); SM(None, SADDR);

        }
      break;
    case 0xae:
        {
          /** 1010 1110			movw	%0, %s1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1010 1110			movw	%0, %s1				*/",
                     op[0]);
            }
          SYNTAX("movw	%0, %s1");
#line 885 "rl78-decode.opc"
          ID(mov); W(); DR(AX); SM(None, SFR);

        }
      break;
    case 0xaf:
        {
          /** 1010 1111			movw	%0, %e!1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1010 1111			movw	%0, %e!1			*/",
                     op[0]);
            }
          SYNTAX("movw	%0, %e!1");
#line 836 "rl78-decode.opc"
          ID(mov); W(); DR(AX); SM(None, IMMU(2));


        }
      break;
    case 0xb0:
        {
          /** 1011 0000			dec	%e!0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1011 0000			dec	%e!0				*/",
                     op[0]);
            }
          SYNTAX("dec	%e!0");
#line 550 "rl78-decode.opc"
          ID(sub); DM(None, IMMU(2)); SC(1); Fza;

        }
      break;
    case 0xb1:
    case 0xb3:
    case 0xb5:
    case 0xb7:
        {
          /** 1011 0rg1 			decw	%0				*/
#line 570 "rl78-decode.opc"
          int rg AU = (op[0] >> 1) & 0x03;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1011 0rg1 			decw	%0				*/",
                     op[0]);
              printf ("  rg = 0x%x\n", rg);
            }
          SYNTAX("decw	%0");
#line 570 "rl78-decode.opc"
          ID(sub); W(); DRW(rg); SC(1);

        }
      break;
    case 0xb2:
        {
          /** 1011 0010			decw	%e!0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1011 0010			decw	%e!0				*/",
                     op[0]);
            }
          SYNTAX("decw	%e!0");
#line 564 "rl78-decode.opc"
          ID(sub); W(); DM(None, IMMU(2)); SC(1);

        }
      break;
    case 0xb4:
        {
          /** 1011 0100			dec	%0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1011 0100			dec	%0				*/",
                     op[0]);
            }
          SYNTAX("dec	%0");
#line 559 "rl78-decode.opc"
          ID(sub); DM(None, SADDR); SC(1); Fza;

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0xb6:
        {
          /** 1011 0110			decw	%0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1011 0110			decw	%0				*/",
                     op[0]);
            }
          SYNTAX("decw	%0");
#line 573 "rl78-decode.opc"
          ID(sub); W(); DM(None, SADDR); SC(1);

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0xb8:
        {
          /** 1011 1000			movw	%a0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1011 1000			movw	%a0, %1				*/",
                     op[0]);
            }
          SYNTAX("movw	%a0, %1");
#line 833 "rl78-decode.opc"
          ID(mov); W(); DM(SP, IMMU(1)); SR(AX);

        }
      break;
    case 0xb9:
        {
          /** 1011 1001			movw	%e0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1011 1001			movw	%e0, %1				*/",
                     op[0]);
            }
          SYNTAX("movw	%e0, %1");
#line 821 "rl78-decode.opc"
          ID(mov); W(); DM(DE, 0); SR(AX);

        }
      break;
    case 0xba:
        {
          /** 1011 1010			movw	%ea0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1011 1010			movw	%ea0, %1				*/",
                     op[0]);
            }
          SYNTAX("movw	%ea0, %1");
#line 824 "rl78-decode.opc"
          ID(mov); W(); DM(DE, IMMU(1)); SR(AX);

        }
      break;
    case 0xbb:
        {
          /** 1011 1011			movw	%e0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1011 1011			movw	%e0, %1				*/",
                     op[0]);
            }
          SYNTAX("movw	%e0, %1");
#line 827 "rl78-decode.opc"
          ID(mov); W(); DM(HL, 0); SR(AX);

        }
      break;
    case 0xbc:
        {
          /** 1011 1100			movw	%ea0, %1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1011 1100			movw	%ea0, %1			*/",
                     op[0]);
            }
          SYNTAX("movw	%ea0, %1");
#line 830 "rl78-decode.opc"
          ID(mov); W(); DM(HL, IMMU(1)); SR(AX);

        }
      break;
    case 0xbd:
        {
          /** 1011 1101			movw	%0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1011 1101			movw	%0, %1				*/",
                     op[0]);
            }
          SYNTAX("movw	%0, %1");
#line 897 "rl78-decode.opc"
          ID(mov); W(); DM(None, SADDR); SR(AX);

        }
      break;
    case 0xbe:
        {
          /** 1011 1110			movw	%s0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1011 1110			movw	%s0, %1				*/",
                     op[0]);
            }
          SYNTAX("movw	%s0, %1");
#line 903 "rl78-decode.opc"
          ID(mov); W(); DM(None, SFR); SR(AX);

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0xbf:
        {
          /** 1011 1111			movw	%e!0, %1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1011 1111			movw	%e!0, %1			*/",
                     op[0]);
            }
          SYNTAX("movw	%e!0, %1");
#line 818 "rl78-decode.opc"
          ID(mov); W(); DM(None, IMMU(2)); SR(AX);

        }
      break;
    case 0xc0:
    case 0xc2:
    case 0xc4:
    case 0xc6:
        {
          /** 1100 0rg0			pop	%0				*/
#line 988 "rl78-decode.opc"
          int rg AU = (op[0] >> 1) & 0x03;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1100 0rg0			pop	%0				*/",
                     op[0]);
              printf ("  rg = 0x%x\n", rg);
            }
          SYNTAX("pop	%0");
#line 988 "rl78-decode.opc"
          ID(mov); W(); DRW(rg); SPOP();

        }
      break;
    case 0xc1:
    case 0xc3:
    case 0xc5:
    case 0xc7:
        {
          /** 1100 0rg1			push	%1				*/
#line 996 "rl78-decode.opc"
          int rg AU = (op[0] >> 1) & 0x03;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1100 0rg1			push	%1				*/",
                     op[0]);
              printf ("  rg = 0x%x\n", rg);
            }
          SYNTAX("push	%1");
#line 996 "rl78-decode.opc"
          ID(mov); W(); DPUSH(); SRW(rg);

        }
      break;
    case 0xc8:
        {
          /** 1100 1000			mov	%a0, #%1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1100 1000			mov	%a0, #%1			*/",
                     op[0]);
            }
          SYNTAX("mov	%a0, #%1");
#line 641 "rl78-decode.opc"
          ID(mov); DM(SP, IMMU(1)); SC(IMMU(1));

        }
      break;
    case 0xc9:
        {
          /** 1100 1001			movw	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1100 1001			movw	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("movw	%0, #%1");
#line 894 "rl78-decode.opc"
          ID(mov); W(); DM(None, SADDR); SC(IMMU(2));

        }
      break;
    case 0xca:
        {
          /** 1100 1010			mov	%ea0, #%1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1100 1010			mov	%ea0, #%1			*/",
                     op[0]);
            }
          SYNTAX("mov	%ea0, #%1");
#line 620 "rl78-decode.opc"
          ID(mov); DM(DE, IMMU(1)); SC(IMMU(1));

        }
      break;
    case 0xcb:
        {
          /** 1100 1011			movw	%s0, #%1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1100 1011			movw	%s0, #%1			*/",
                     op[0]);
            }
          SYNTAX("movw	%s0, #%1");
#line 900 "rl78-decode.opc"
          ID(mov); W(); DM(None, SFR); SC(IMMU(2));

        }
      break;
    case 0xcc:
        {
          /** 1100 1100			mov	%ea0, #%1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1100 1100			mov	%ea0, #%1			*/",
                     op[0]);
            }
          SYNTAX("mov	%ea0, #%1");
#line 632 "rl78-decode.opc"
          ID(mov); DM(HL, IMMU(1)); SC(IMMU(1));

        }
      break;
    case 0xcd:
        {
          /** 1100 1101			mov	%0, #%1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1100 1101			mov	%0, #%1				*/",
                     op[0]);
            }
          SYNTAX("mov	%0, #%1");
#line 746 "rl78-decode.opc"
          ID(mov); DM(None, SADDR); SC(IMMU(1));

        }
      break;
    case 0xce:
        {
          /** 1100 1110			mov	%s0, #%1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1100 1110			mov	%s0, #%1			*/",
                     op[0]);
            }
          SYNTAX("mov	%s0, #%1");
#line 752 "rl78-decode.opc"
          op0 = SFR;
          op1 = IMMU(1);
          ID(mov); DM(None, op0); SC(op1);
          if (op0 == 0xffffb && isa == RL78_ISA_G14)
            switch (op1)
              {
              case 0x01:
        	rl78->syntax = "mulhu"; ID(mulhu);
        	break;
              case 0x02:
        	rl78->syntax = "mulh"; ID(mulh);
        	break;
              case 0x03:
        	rl78->syntax = "divhu"; ID(divhu);
        	break;
              case 0x04:
        	rl78->syntax = "divwu <old-encoding>"; ID(divwu);
        	break;
              case 0x05:
        	rl78->syntax = "machu"; ID(machu);
        	break;
              case 0x06:
        	rl78->syntax = "mach"; ID(mach);
        	break;
              case 0x0b:
        	rl78->syntax = "divwu"; ID(divwu);
        	break;
              }

        }
      break;
    case 0xcf:
        {
          /** 1100 1111			mov	%e!0, #%1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1100 1111			mov	%e!0, #%1			*/",
                     op[0]);
            }
          SYNTAX("mov	%e!0, #%1");
#line 611 "rl78-decode.opc"
          ID(mov); DM(None, IMMU(2)); SC(IMMU(1));

        }
      break;
    case 0xd0:
    case 0xd1:
    case 0xd2:
    case 0xd3:
        {
          /** 1101 00rg			cmp0	%0				*/
#line 520 "rl78-decode.opc"
          int rg AU = op[0] & 0x03;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1101 00rg			cmp0	%0				*/",
                     op[0]);
              printf ("  rg = 0x%x\n", rg);
            }
          SYNTAX("cmp0	%0");
#line 520 "rl78-decode.opc"
          ID(cmp); DRB(rg); SC(0); Fzac;

        }
      break;
    case 0xd4:
        {
          /** 1101 0100			cmp0	%0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1101 0100			cmp0	%0				*/",
                     op[0]);
            }
          SYNTAX("cmp0	%0");
#line 523 "rl78-decode.opc"
          ID(cmp); DM(None, SADDR); SC(0); Fzac;

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0xd5:
        {
          /** 1101 0101			cmp0	%e!0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1101 0101			cmp0	%e!0				*/",
                     op[0]);
            }
          SYNTAX("cmp0	%e!0");
#line 517 "rl78-decode.opc"
          ID(cmp); DM(None, IMMU(2)); SC(0); Fzac;

        }
      break;
    case 0xd6:
        {
          /** 1101 0110			mulu	x				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1101 0110			mulu	x				*/",
                     op[0]);
            }
          SYNTAX("mulu	x");
#line 908 "rl78-decode.opc"
          ID(mulu);

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0xd7:
        {
          /** 1101 0111			ret					*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1101 0111			ret					*/",
                     op[0]);
            }
          SYNTAX("ret");
#line 1004 "rl78-decode.opc"
          ID(ret);

        }
      break;
    case 0xd8:
        {
          /** 1101 1000			mov	%0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1101 1000			mov	%0, %1				*/",
                     op[0]);
            }
          SYNTAX("mov	%0, %1");
#line 713 "rl78-decode.opc"
          ID(mov); DR(X); SM(None, SADDR);

        }
      break;
    case 0xd9:
        {
          /** 1101 1001			mov	%0, %e!1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1101 1001			mov	%0, %e!1			*/",
                     op[0]);
            }
          SYNTAX("mov	%0, %e!1");
#line 710 "rl78-decode.opc"
          ID(mov); DR(X); SM(None, IMMU(2));

        }
      break;
    case 0xda:
    case 0xea:
    case 0xfa:
        {
          /** 11ra 1010			movw	%0, %1				*/
#line 891 "rl78-decode.opc"
          int ra AU = (op[0] >> 4) & 0x03;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 11ra 1010			movw	%0, %1				*/",
                     op[0]);
              printf ("  ra = 0x%x\n", ra);
            }
          SYNTAX("movw	%0, %1");
#line 891 "rl78-decode.opc"
          ID(mov); W(); DRW(ra); SM(None, SADDR);

        }
      break;
    case 0xdb:
    case 0xeb:
    case 0xfb:
        {
          /** 11ra 1011			movw	%0, %es!1			*/
#line 888 "rl78-decode.opc"
          int ra AU = (op[0] >> 4) & 0x03;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 11ra 1011			movw	%0, %es!1			*/",
                     op[0]);
              printf ("  ra = 0x%x\n", ra);
            }
          SYNTAX("movw	%0, %es!1");
#line 888 "rl78-decode.opc"
          ID(mov); W(); DRW(ra); SM(None, IMMU(2));

        }
      break;
    case 0xdc:
        {
          /** 1101 1100			bc	$%a0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1101 1100			bc	$%a0				*/",
                     op[0]);
            }
          SYNTAX("bc	$%a0");
#line 336 "rl78-decode.opc"
          ID(branch_cond); DC(pc+IMMS(1)+2); SR(None); COND(C);

        }
      break;
    case 0xdd:
        {
          /** 1101 1101			bz	$%a0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1101 1101			bz	$%a0				*/",
                     op[0]);
            }
          SYNTAX("bz	$%a0");
#line 348 "rl78-decode.opc"
          ID(branch_cond); DC(pc+IMMS(1)+2); SR(None); COND(Z);

        }
      break;
    case 0xde:
        {
          /** 1101 1110			bnc	$%a0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1101 1110			bnc	$%a0				*/",
                     op[0]);
            }
          SYNTAX("bnc	$%a0");
#line 339 "rl78-decode.opc"
          ID(branch_cond); DC(pc+IMMS(1)+2); SR(None); COND(NC);

        }
      break;
    case 0xdf:
        {
          /** 1101 1111			bnz	$%a0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1101 1111			bnz	$%a0				*/",
                     op[0]);
            }
          SYNTAX("bnz	$%a0");
#line 351 "rl78-decode.opc"
          ID(branch_cond); DC(pc+IMMS(1)+2); SR(None); COND(NZ);

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0xe0:
    case 0xe1:
    case 0xe2:
    case 0xe3:
        {
          /** 1110 00rg			oneb	%0				*/
#line 926 "rl78-decode.opc"
          int rg AU = op[0] & 0x03;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1110 00rg			oneb	%0				*/",
                     op[0]);
              printf ("  rg = 0x%x\n", rg);
            }
          SYNTAX("oneb	%0");
#line 926 "rl78-decode.opc"
          ID(mov); DRB(rg); SC(1);

        }
      break;
    case 0xe4:
        {
          /** 1110 0100			oneb	%0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1110 0100			oneb	%0				*/",
                     op[0]);
            }
          SYNTAX("oneb	%0");
#line 929 "rl78-decode.opc"
          ID(mov); DM(None, SADDR); SC(1);

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0xe5:
        {
          /** 1110 0101			oneb	%e!0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1110 0101			oneb	%e!0				*/",
                     op[0]);
            }
          SYNTAX("oneb	%e!0");
#line 923 "rl78-decode.opc"
          ID(mov); DM(None, IMMU(2)); SC(1);

        }
      break;
    case 0xe6:
        {
          /** 1110 0110			onew	%0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1110 0110			onew	%0				*/",
                     op[0]);
            }
          SYNTAX("onew	%0");
#line 934 "rl78-decode.opc"
          ID(mov); DR(AX); SC(1);

        }
      break;
    case 0xe7:
        {
          /** 1110 0111			onew	%0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1110 0111			onew	%0				*/",
                     op[0]);
            }
          SYNTAX("onew	%0");
#line 937 "rl78-decode.opc"
          ID(mov); DR(BC); SC(1);

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0xe8:
        {
          /** 1110 1000			mov	%0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1110 1000			mov	%0, %1				*/",
                     op[0]);
            }
          SYNTAX("mov	%0, %1");
#line 701 "rl78-decode.opc"
          ID(mov); DR(B); SM(None, SADDR);

        }
      break;
    case 0xe9:
        {
          /** 1110 1001			mov	%0, %e!1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1110 1001			mov	%0, %e!1			*/",
                     op[0]);
            }
          SYNTAX("mov	%0, %e!1");
#line 695 "rl78-decode.opc"
          ID(mov); DR(B); SM(None, IMMU(2));

        }
      break;
    case 0xec:
        {
          /** 1110 1100			br	!%!a0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1110 1100			br	!%!a0				*/",
                     op[0]);
            }
          SYNTAX("br	!%!a0");
#line 370 "rl78-decode.opc"
          ID(branch); DC(IMMU(3));

        }
      break;
    case 0xed:
        {
          /** 1110 1101			br	%!a0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1110 1101			br	%!a0				*/",
                     op[0]);
            }
          SYNTAX("br	%!a0");
#line 373 "rl78-decode.opc"
          ID(branch); DC(IMMU(2));

        }
      break;
    case 0xee:
        {
          /** 1110 1110			br	$%!a0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1110 1110			br	$%!a0				*/",
                     op[0]);
            }
          SYNTAX("br	$%!a0");
#line 376 "rl78-decode.opc"
          ID(branch); DC(pc+IMMS(2)+3);

        }
      break;
    case 0xef:
        {
          /** 1110 1111			br	$%a0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1110 1111			br	$%a0				*/",
                     op[0]);
            }
          SYNTAX("br	$%a0");
#line 379 "rl78-decode.opc"
          ID(branch); DC(pc+IMMS(1)+2);

        }
      break;
    case 0xf0:
    case 0xf1:
    case 0xf2:
    case 0xf3:
        {
          /** 1111 00rg			clrb	%0				*/
#line 466 "rl78-decode.opc"
          int rg AU = op[0] & 0x03;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1111 00rg			clrb	%0				*/",
                     op[0]);
              printf ("  rg = 0x%x\n", rg);
            }
          SYNTAX("clrb	%0");
#line 466 "rl78-decode.opc"
          ID(mov); DRB(rg); SC(0);

        }
      break;
    case 0xf4:
        {
          /** 1111 0100			clrb	%0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1111 0100			clrb	%0				*/",
                     op[0]);
            }
          SYNTAX("clrb	%0");
#line 469 "rl78-decode.opc"
          ID(mov); DM(None, SADDR); SC(0);

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0xf5:
        {
          /** 1111 0101			clrb	%e!0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1111 0101			clrb	%e!0				*/",
                     op[0]);
            }
          SYNTAX("clrb	%e!0");
#line 463 "rl78-decode.opc"
          ID(mov); DM(None, IMMU(2)); SC(0);

        }
      break;
    case 0xf6:
        {
          /** 1111 0110			clrw	%0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1111 0110			clrw	%0				*/",
                     op[0]);
            }
          SYNTAX("clrw	%0");
#line 474 "rl78-decode.opc"
          ID(mov); DR(AX); SC(0);

        }
      break;
    case 0xf7:
        {
          /** 1111 0111			clrw	%0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1111 0111			clrw	%0				*/",
                     op[0]);
            }
          SYNTAX("clrw	%0");
#line 477 "rl78-decode.opc"
          ID(mov); DR(BC); SC(0);

        /*----------------------------------------------------------------------*/

        }
      break;
    case 0xf8:
        {
          /** 1111 1000			mov	%0, %1				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1111 1000			mov	%0, %1				*/",
                     op[0]);
            }
          SYNTAX("mov	%0, %1");
#line 707 "rl78-decode.opc"
          ID(mov); DR(C); SM(None, SADDR);

        }
      break;
    case 0xf9:
        {
          /** 1111 1001			mov	%0, %e!1			*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1111 1001			mov	%0, %e!1			*/",
                     op[0]);
            }
          SYNTAX("mov	%0, %e!1");
#line 704 "rl78-decode.opc"
          ID(mov); DR(C); SM(None, IMMU(2));

        }
      break;
    case 0xfc:
        {
          /** 1111 1100			call	!%!a0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1111 1100			call	!%!a0				*/",
                     op[0]);
            }
          SYNTAX("call	!%!a0");
#line 423 "rl78-decode.opc"
          ID(call); DC(IMMU(3));

        }
      break;
    case 0xfd:
        {
          /** 1111 1101			call	%!a0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1111 1101			call	%!a0				*/",
                     op[0]);
            }
          SYNTAX("call	%!a0");
#line 426 "rl78-decode.opc"
          ID(call); DC(IMMU(2));

        }
      break;
    case 0xfe:
        {
          /** 1111 1110			call	$%!a0				*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1111 1110			call	$%!a0				*/",
                     op[0]);
            }
          SYNTAX("call	$%!a0");
#line 429 "rl78-decode.opc"
          ID(call); DC(pc+IMMS(2)+3);

        }
      break;
    case 0xff:
        {
          /** 1111 1111			brk1					*/
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 1111 1111			brk1					*/",
                     op[0]);
            }
          SYNTAX("brk1");
#line 387 "rl78-decode.opc"
          ID(break);

        }
      break;
  }
#line 1292 "rl78-decode.opc"

  return rl78->n_bytes;
}
