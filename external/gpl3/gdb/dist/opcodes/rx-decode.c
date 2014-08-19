#line 1 "rx-decode.opc"
/* -*- c -*- */
/* Copyright 2012 Free Software Foundation, Inc.
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
#include "opcode/rx.h"

#define RX_OPCODE_BIG_ENDIAN 0

typedef struct
{
  RX_Opcode_Decoded * rx;
  int (* getbyte)(void *);
  void * ptr;
  unsigned char * op;
} LocalData;

static int trace = 0;

#define BSIZE 0
#define WSIZE 1
#define LSIZE 2

/* These are for when the upper bits are "don't care" or "undefined".  */
static int bwl[] =
{
  RX_Byte,
  RX_Word,
  RX_Long
};

static int sbwl[] =
{
  RX_SByte,
  RX_SWord,
  RX_Long
};

static int ubwl[] =
{
  RX_UByte,
  RX_UWord,
  RX_Long
};

static int memex[] =
{
  RX_SByte,
  RX_SWord,
  RX_Long,
  RX_UWord
};

#define ID(x) rx->id = RXO_##x
#define OP(n,t,r,a) (rx->op[n].type = t, \
		     rx->op[n].reg = r,	     \
		     rx->op[n].addend = a )
#define OPs(n,t,r,a,s) (OP (n,t,r,a), \
			rx->op[n].size = s )

/* This is for the BWL and BW bitfields.  */
static int SCALE[] = { 1, 2, 4 };
/* This is for the prefix size enum.  */
static int PSCALE[] = { 4, 1, 1, 1, 2, 2, 2, 3, 4 };

static int flagmap[] = {0, 1, 2, 3, 0, 0, 0, 0,
		       16, 17, 0, 0, 0, 0, 0, 0 };

static int dsp3map[] = { 8, 9, 10, 3, 4, 5, 6, 7 };

/*
 *C	a constant (immediate) c
 *R	A register
 *I	Register indirect, no offset
 *Is	Register indirect, with offset
 *D	standard displacement: type (r,[r],dsp8,dsp16 code), register, BWL code
 *P	standard displacement: type (r,[r]), reg, assumes UByte
 *Pm	memex displacement: type (r,[r]), reg, memex code
 *cc	condition code.  */

#define DC(c)       OP (0, RX_Operand_Immediate, 0, c)
#define DR(r)       OP (0, RX_Operand_Register,  r, 0)
#define DI(r,a)     OP (0, RX_Operand_Indirect,  r, a)
#define DIs(r,a,s)  OP (0, RX_Operand_Indirect,  r, (a) * SCALE[s])
#define DD(t,r,s)   rx_disp (0, t, r, bwl[s], ld);
#define DF(r)       OP (0, RX_Operand_Flag,  flagmap[r], 0)

#define SC(i)       OP (1, RX_Operand_Immediate, 0, i)
#define SR(r)       OP (1, RX_Operand_Register,  r, 0)
#define SRR(r)      OP (1, RX_Operand_TwoReg,  r, 0)
#define SI(r,a)     OP (1, RX_Operand_Indirect,  r, a)
#define SIs(r,a,s)  OP (1, RX_Operand_Indirect,  r, (a) * SCALE[s])
#define SD(t,r,s)   rx_disp (1, t, r, bwl[s], ld);
#define SP(t,r)     rx_disp (1, t, r, (t!=3) ? RX_UByte : RX_Long, ld); P(t, 1);
#define SPm(t,r,m)  rx_disp (1, t, r, memex[m], ld); rx->op[1].size = memex[m];
#define Scc(cc)     OP (1, RX_Operand_Condition,  cc, 0)

#define S2C(i)      OP (2, RX_Operand_Immediate, 0, i)
#define S2R(r)      OP (2, RX_Operand_Register,  r, 0)
#define S2I(r,a)    OP (2, RX_Operand_Indirect,  r, a)
#define S2Is(r,a,s) OP (2, RX_Operand_Indirect,  r, (a) * SCALE[s])
#define S2D(t,r,s)  rx_disp (2, t, r, bwl[s], ld);
#define S2P(t,r)    rx_disp (2, t, r, (t!=3) ? RX_UByte : RX_Long, ld); P(t, 2);
#define S2Pm(t,r,m) rx_disp (2, t, r, memex[m], ld); rx->op[2].size = memex[m];
#define S2cc(cc)    OP (2, RX_Operand_Condition,  cc, 0)

#define BWL(sz)     rx->op[0].size = rx->op[1].size = rx->op[2].size = rx->size = bwl[sz]
#define sBWL(sz)    rx->op[0].size = rx->op[1].size = rx->op[2].size = rx->size = sbwl[sz]
#define uBWL(sz)    rx->op[0].size = rx->op[1].size = rx->op[2].size = rx->size = ubwl[sz]
#define P(t, n)	    rx->op[n].size = (t!=3) ? RX_UByte : RX_Long;

#define F(f) store_flags(rx, f)

#define AU ATTRIBUTE_UNUSED
#define GETBYTE() (ld->op [ld->rx->n_bytes++] = ld->getbyte (ld->ptr))

#define SYNTAX(x) rx->syntax = x

#define UNSUPPORTED() \
  rx->syntax = "*unknown*"

#define IMM(sf)   immediate (sf, 0, ld)
#define IMMex(sf) immediate (sf, 1, ld)

static int
immediate (int sfield, int ex, LocalData * ld)
{
  unsigned long i = 0, j;

  switch (sfield)
    {
#define B ((unsigned long) GETBYTE())
    case 0:
#if RX_OPCODE_BIG_ENDIAN
      i  = B;
      if (ex && (i & 0x80))
	i -= 0x100;
      i <<= 24;
      i |= B << 16;
      i |= B << 8;
      i |= B;
#else
      i = B;
      i |= B << 8;
      i |= B << 16;
      j = B;
      if (ex && (j & 0x80))
	j -= 0x100;
      i |= j << 24;
#endif
      break;
    case 3:
#if RX_OPCODE_BIG_ENDIAN
      i  = B << 16;
      i |= B << 8;
      i |= B;
#else
      i  = B;
      i |= B << 8;
      i |= B << 16;
#endif
      if (ex && (i & 0x800000))
	i -= 0x1000000;
      break;
    case 2:
#if RX_OPCODE_BIG_ENDIAN
      i |= B << 8;
      i |= B;
#else
      i |= B;
      i |= B << 8;
#endif
      if (ex && (i & 0x8000))
	i -= 0x10000;
      break;
    case 1:
      i |= B;
      if (ex && (i & 0x80))
	i -= 0x100;
      break;
    default:
      abort();
    }
  return i;
}

static void
rx_disp (int n, int type, int reg, int size, LocalData * ld)
{
  int disp;

  ld->rx->op[n].reg = reg;
  switch (type)
    {
    case 3:
      ld->rx->op[n].type = RX_Operand_Register;
      break;
    case 0:
      ld->rx->op[n].type = RX_Operand_Indirect;
      ld->rx->op[n].addend = 0;
      break;
    case 1:
      ld->rx->op[n].type = RX_Operand_Indirect;
      disp = GETBYTE ();
      ld->rx->op[n].addend = disp * PSCALE[size];
      break;
    case 2:
      ld->rx->op[n].type = RX_Operand_Indirect;
      disp = GETBYTE ();
#if RX_OPCODE_BIG_ENDIAN
      disp = disp * 256 + GETBYTE ();
#else
      disp = disp + GETBYTE () * 256;
#endif
      ld->rx->op[n].addend = disp * PSCALE[size];
      break;
    default:
      abort ();
    }
}

#define xO 8
#define xS 4
#define xZ 2
#define xC 1

#define F_____ 
#define F___ZC rx->flags_0 = rx->flags_s = xZ|xC;
#define F__SZ_ rx->flags_0 = rx->flags_s = xS|xZ;
#define F__SZC rx->flags_0 = rx->flags_s = xS|xZ|xC;
#define F_0SZC rx->flags_0 = xO|xS|xZ|xC; rx->flags_s = xS|xZ|xC;
#define F_O___ rx->flags_0 = rx->flags_s = xO;
#define F_OS__ rx->flags_0 = rx->flags_s = xO|xS;
#define F_OSZ_ rx->flags_0 = rx->flags_s = xO|xS|xZ;
#define F_OSZC rx->flags_0 = rx->flags_s = xO|xS|xZ|xC;

int
rx_decode_opcode (unsigned long pc AU,
		  RX_Opcode_Decoded * rx,
		  int (* getbyte)(void *),
		  void * ptr)
{
  LocalData lds, * ld = &lds;
  unsigned char op[20] = {0};

  lds.rx = rx;
  lds.getbyte = getbyte;
  lds.ptr = ptr;
  lds.op = op;

  memset (rx, 0, sizeof (*rx));
  BWL(LSIZE);


/*----------------------------------------------------------------------*/
/* MOV									*/

  GETBYTE ();
  switch (op[0] & 0xff)
  {
    case 0x00:
        {
          /** 0000 0000			brk */
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0000 0000			brk */",
                     op[0]);
            }
          SYNTAX("brk");
#line 984 "rx-decode.opc"
          ID(brk);
        
        }
      break;
    case 0x01:
        {
          /** 0000 0001			dbt */
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0000 0001			dbt */",
                     op[0]);
            }
          SYNTAX("dbt");
#line 987 "rx-decode.opc"
          ID(dbt);
        
        }
      break;
    case 0x02:
        {
          /** 0000 0010			rts */
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0000 0010			rts */",
                     op[0]);
            }
          SYNTAX("rts");
#line 765 "rx-decode.opc"
          ID(rts);
        
        /*----------------------------------------------------------------------*/
        /* NOP								*/
        
        }
      break;
    case 0x03:
        {
          /** 0000 0011			nop */
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0000 0011			nop */",
                     op[0]);
            }
          SYNTAX("nop");
#line 771 "rx-decode.opc"
          ID(nop);
        
        /*----------------------------------------------------------------------*/
        /* STRING FUNCTIONS							*/
        
        }
      break;
    case 0x04:
        {
          /** 0000 0100			bra.a	%a0 */
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0000 0100			bra.a	%a0 */",
                     op[0]);
            }
          SYNTAX("bra.a	%a0");
#line 743 "rx-decode.opc"
          ID(branch); DC(pc + IMMex(3));
        
        }
      break;
    case 0x05:
        {
          /** 0000 0101			bsr.a	%a0 */
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0000 0101			bsr.a	%a0 */",
                     op[0]);
            }
          SYNTAX("bsr.a	%a0");
#line 759 "rx-decode.opc"
          ID(jsr); DC(pc + IMMex(3));
        
        }
      break;
    case 0x06:
        GETBYTE ();
        switch (op[1] & 0xff)
        {
          case 0x00:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_1:
                    {
                      /** 0000 0110 mx00 00ss rsrc rdst			sub	%2%S2, %1 */
#line 539 "rx-decode.opc"
                      int mx AU = (op[1] >> 6) & 0x03;
#line 539 "rx-decode.opc"
                      int ss AU = op[1] & 0x03;
#line 539 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 539 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 0000 0110 mx00 00ss rsrc rdst			sub	%2%S2, %1 */",
                                 op[0], op[1], op[2]);
                          printf ("  mx = 0x%x,", mx);
                          printf ("  ss = 0x%x,", ss);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("sub	%2%S2, %1");
#line 539 "rx-decode.opc"
                      ID(sub); S2Pm(ss, rsrc, mx); SR(rdst); DR(rdst); F_OSZC;
                    
                    }
                  break;
              }
            break;
          case 0x01:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_1;
                  break;
              }
            break;
          case 0x02:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_1;
                  break;
              }
            break;
          case 0x03:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_1;
                  break;
              }
            break;
          case 0x04:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_2:
                    {
                      /** 0000 0110 mx00 01ss rsrc rdst		cmp	%2%S2, %1 */
#line 527 "rx-decode.opc"
                      int mx AU = (op[1] >> 6) & 0x03;
#line 527 "rx-decode.opc"
                      int ss AU = op[1] & 0x03;
#line 527 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 527 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 0000 0110 mx00 01ss rsrc rdst		cmp	%2%S2, %1 */",
                                 op[0], op[1], op[2]);
                          printf ("  mx = 0x%x,", mx);
                          printf ("  ss = 0x%x,", ss);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("cmp	%2%S2, %1");
#line 527 "rx-decode.opc"
                      ID(sub); S2Pm(ss, rsrc, mx); SR(rdst); F_OSZC;
                    
                    /*----------------------------------------------------------------------*/
                    /* SUB									*/
                    
                    }
                  break;
              }
            break;
          case 0x05:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_2;
                  break;
              }
            break;
          case 0x06:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_2;
                  break;
              }
            break;
          case 0x07:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_2;
                  break;
              }
            break;
          case 0x08:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_3:
                    {
                      /** 0000 0110 mx00 10ss rsrc rdst	add	%1%S1, %0 */
#line 503 "rx-decode.opc"
                      int mx AU = (op[1] >> 6) & 0x03;
#line 503 "rx-decode.opc"
                      int ss AU = op[1] & 0x03;
#line 503 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 503 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 0000 0110 mx00 10ss rsrc rdst	add	%1%S1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  mx = 0x%x,", mx);
                          printf ("  ss = 0x%x,", ss);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("add	%1%S1, %0");
#line 503 "rx-decode.opc"
                      ID(add); SPm(ss, rsrc, mx); DR(rdst); F_OSZC;
                    
                    }
                  break;
              }
            break;
          case 0x09:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_3;
                  break;
              }
            break;
          case 0x0a:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_3;
                  break;
              }
            break;
          case 0x0b:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_3;
                  break;
              }
            break;
          case 0x0c:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_4:
                    {
                      /** 0000 0110 mx00 11ss rsrc rdst	mul	%1%S1, %0 */
#line 608 "rx-decode.opc"
                      int mx AU = (op[1] >> 6) & 0x03;
#line 608 "rx-decode.opc"
                      int ss AU = op[1] & 0x03;
#line 608 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 608 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 0000 0110 mx00 11ss rsrc rdst	mul	%1%S1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  mx = 0x%x,", mx);
                          printf ("  ss = 0x%x,", ss);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("mul	%1%S1, %0");
#line 608 "rx-decode.opc"
                      ID(mul); SPm(ss, rsrc, mx); DR(rdst); F_____;
                    
                    }
                  break;
              }
            break;
          case 0x0d:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_4;
                  break;
              }
            break;
          case 0x0e:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_4;
                  break;
              }
            break;
          case 0x0f:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_4;
                  break;
              }
            break;
          case 0x10:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_5:
                    {
                      /** 0000 0110 mx01 00ss rsrc rdst	and	%1%S1, %0 */
#line 416 "rx-decode.opc"
                      int mx AU = (op[1] >> 6) & 0x03;
#line 416 "rx-decode.opc"
                      int ss AU = op[1] & 0x03;
#line 416 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 416 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 0000 0110 mx01 00ss rsrc rdst	and	%1%S1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  mx = 0x%x,", mx);
                          printf ("  ss = 0x%x,", ss);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("and	%1%S1, %0");
#line 416 "rx-decode.opc"
                      ID(and); SPm(ss, rsrc, mx); DR(rdst); F__SZ_;
                    
                    }
                  break;
              }
            break;
          case 0x11:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_5;
                  break;
              }
            break;
          case 0x12:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_5;
                  break;
              }
            break;
          case 0x13:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_5;
                  break;
              }
            break;
          case 0x14:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_6:
                    {
                      /** 0000 0110 mx01 01ss rsrc rdst			or	%1%S1, %0 */
#line 434 "rx-decode.opc"
                      int mx AU = (op[1] >> 6) & 0x03;
#line 434 "rx-decode.opc"
                      int ss AU = op[1] & 0x03;
#line 434 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 434 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 0000 0110 mx01 01ss rsrc rdst			or	%1%S1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  mx = 0x%x,", mx);
                          printf ("  ss = 0x%x,", ss);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("or	%1%S1, %0");
#line 434 "rx-decode.opc"
                      ID(or); SPm(ss, rsrc, mx); DR(rdst); F__SZ_;
                    
                    }
                  break;
              }
            break;
          case 0x15:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_6;
                  break;
              }
            break;
          case 0x16:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_6;
                  break;
              }
            break;
          case 0x17:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_6;
                  break;
              }
            break;
          case 0x20:
              GETBYTE ();
              switch (op[2] & 0xff)
              {
                case 0x00:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        op_semantics_7:
                          {
                            /** 0000 0110 mx10 00sp 0000 0000 rsrc rdst	sbb	%1%S1, %0 */
#line 552 "rx-decode.opc"
                            int mx AU = (op[1] >> 6) & 0x03;
#line 552 "rx-decode.opc"
                            int sp AU = op[1] & 0x03;
#line 552 "rx-decode.opc"
                            int rsrc AU = (op[3] >> 4) & 0x0f;
#line 552 "rx-decode.opc"
                            int rdst AU = op[3] & 0x0f;
                            if (trace)
                              {
                                printf ("\033[33m%s\033[0m  %02x %02x %02x %02x\n",
                                       "/** 0000 0110 mx10 00sp 0000 0000 rsrc rdst	sbb	%1%S1, %0 */",
                                       op[0], op[1], op[2], op[3]);
                                printf ("  mx = 0x%x,", mx);
                                printf ("  sp = 0x%x,", sp);
                                printf ("  rsrc = 0x%x,", rsrc);
                                printf ("  rdst = 0x%x\n", rdst);
                              }
                            SYNTAX("sbb	%1%S1, %0");
#line 552 "rx-decode.opc"
                            ID(sbb); SPm(sp, rsrc, mx); DR(rdst); F_OSZC;
                          
                          /*----------------------------------------------------------------------*/
                          /* ABS									*/
                          
                          }
                        break;
                    }
                  break;
                case 0x04:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        op_semantics_8:
                          {
                            /** 0000 0110 mx10 00ss 0000 0100 rsrc rdst	max	%1%S1, %0 */
#line 581 "rx-decode.opc"
                            int mx AU = (op[1] >> 6) & 0x03;
#line 581 "rx-decode.opc"
                            int ss AU = op[1] & 0x03;
#line 581 "rx-decode.opc"
                            int rsrc AU = (op[3] >> 4) & 0x0f;
#line 581 "rx-decode.opc"
                            int rdst AU = op[3] & 0x0f;
                            if (trace)
                              {
                                printf ("\033[33m%s\033[0m  %02x %02x %02x %02x\n",
                                       "/** 0000 0110 mx10 00ss 0000 0100 rsrc rdst	max	%1%S1, %0 */",
                                       op[0], op[1], op[2], op[3]);
                                printf ("  mx = 0x%x,", mx);
                                printf ("  ss = 0x%x,", ss);
                                printf ("  rsrc = 0x%x,", rsrc);
                                printf ("  rdst = 0x%x\n", rdst);
                              }
                            SYNTAX("max	%1%S1, %0");
#line 581 "rx-decode.opc"
                            ID(max); SPm(ss, rsrc, mx); DR(rdst);
                          
                          /*----------------------------------------------------------------------*/
                          /* MIN									*/
                          
                          }
                        break;
                    }
                  break;
                case 0x05:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        op_semantics_9:
                          {
                            /** 0000 0110 mx10 00ss 0000 0101 rsrc rdst	min	%1%S1, %0 */
#line 593 "rx-decode.opc"
                            int mx AU = (op[1] >> 6) & 0x03;
#line 593 "rx-decode.opc"
                            int ss AU = op[1] & 0x03;
#line 593 "rx-decode.opc"
                            int rsrc AU = (op[3] >> 4) & 0x0f;
#line 593 "rx-decode.opc"
                            int rdst AU = op[3] & 0x0f;
                            if (trace)
                              {
                                printf ("\033[33m%s\033[0m  %02x %02x %02x %02x\n",
                                       "/** 0000 0110 mx10 00ss 0000 0101 rsrc rdst	min	%1%S1, %0 */",
                                       op[0], op[1], op[2], op[3]);
                                printf ("  mx = 0x%x,", mx);
                                printf ("  ss = 0x%x,", ss);
                                printf ("  rsrc = 0x%x,", rsrc);
                                printf ("  rdst = 0x%x\n", rdst);
                              }
                            SYNTAX("min	%1%S1, %0");
#line 593 "rx-decode.opc"
                            ID(min); SPm(ss, rsrc, mx); DR(rdst);
                          
                          /*----------------------------------------------------------------------*/
                          /* MUL									*/
                          
                          }
                        break;
                    }
                  break;
                case 0x06:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        op_semantics_10:
                          {
                            /** 0000 0110 mx10 00ss 0000 0110 rsrc rdst	emul	%1%S1, %0 */
#line 623 "rx-decode.opc"
                            int mx AU = (op[1] >> 6) & 0x03;
#line 623 "rx-decode.opc"
                            int ss AU = op[1] & 0x03;
#line 623 "rx-decode.opc"
                            int rsrc AU = (op[3] >> 4) & 0x0f;
#line 623 "rx-decode.opc"
                            int rdst AU = op[3] & 0x0f;
                            if (trace)
                              {
                                printf ("\033[33m%s\033[0m  %02x %02x %02x %02x\n",
                                       "/** 0000 0110 mx10 00ss 0000 0110 rsrc rdst	emul	%1%S1, %0 */",
                                       op[0], op[1], op[2], op[3]);
                                printf ("  mx = 0x%x,", mx);
                                printf ("  ss = 0x%x,", ss);
                                printf ("  rsrc = 0x%x,", rsrc);
                                printf ("  rdst = 0x%x\n", rdst);
                              }
                            SYNTAX("emul	%1%S1, %0");
#line 623 "rx-decode.opc"
                            ID(emul); SPm(ss, rsrc, mx); DR(rdst);
                          
                          /*----------------------------------------------------------------------*/
                          /* EMULU									*/
                          
                          }
                        break;
                    }
                  break;
                case 0x07:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        op_semantics_11:
                          {
                            /** 0000 0110 mx10 00ss 0000 0111 rsrc rdst	emulu	%1%S1, %0 */
#line 635 "rx-decode.opc"
                            int mx AU = (op[1] >> 6) & 0x03;
#line 635 "rx-decode.opc"
                            int ss AU = op[1] & 0x03;
#line 635 "rx-decode.opc"
                            int rsrc AU = (op[3] >> 4) & 0x0f;
#line 635 "rx-decode.opc"
                            int rdst AU = op[3] & 0x0f;
                            if (trace)
                              {
                                printf ("\033[33m%s\033[0m  %02x %02x %02x %02x\n",
                                       "/** 0000 0110 mx10 00ss 0000 0111 rsrc rdst	emulu	%1%S1, %0 */",
                                       op[0], op[1], op[2], op[3]);
                                printf ("  mx = 0x%x,", mx);
                                printf ("  ss = 0x%x,", ss);
                                printf ("  rsrc = 0x%x,", rsrc);
                                printf ("  rdst = 0x%x\n", rdst);
                              }
                            SYNTAX("emulu	%1%S1, %0");
#line 635 "rx-decode.opc"
                            ID(emulu); SPm(ss, rsrc, mx); DR(rdst);
                          
                          /*----------------------------------------------------------------------*/
                          /* DIV									*/
                          
                          }
                        break;
                    }
                  break;
                case 0x08:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        op_semantics_12:
                          {
                            /** 0000 0110 mx10 00ss 0000 1000 rsrc rdst	div	%1%S1, %0 */
#line 647 "rx-decode.opc"
                            int mx AU = (op[1] >> 6) & 0x03;
#line 647 "rx-decode.opc"
                            int ss AU = op[1] & 0x03;
#line 647 "rx-decode.opc"
                            int rsrc AU = (op[3] >> 4) & 0x0f;
#line 647 "rx-decode.opc"
                            int rdst AU = op[3] & 0x0f;
                            if (trace)
                              {
                                printf ("\033[33m%s\033[0m  %02x %02x %02x %02x\n",
                                       "/** 0000 0110 mx10 00ss 0000 1000 rsrc rdst	div	%1%S1, %0 */",
                                       op[0], op[1], op[2], op[3]);
                                printf ("  mx = 0x%x,", mx);
                                printf ("  ss = 0x%x,", ss);
                                printf ("  rsrc = 0x%x,", rsrc);
                                printf ("  rdst = 0x%x\n", rdst);
                              }
                            SYNTAX("div	%1%S1, %0");
#line 647 "rx-decode.opc"
                            ID(div); SPm(ss, rsrc, mx); DR(rdst); F_O___;
                          
                          /*----------------------------------------------------------------------*/
                          /* DIVU									*/
                          
                          }
                        break;
                    }
                  break;
                case 0x09:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        op_semantics_13:
                          {
                            /** 0000 0110 mx10 00ss 0000 1001 rsrc rdst	divu	%1%S1, %0 */
#line 659 "rx-decode.opc"
                            int mx AU = (op[1] >> 6) & 0x03;
#line 659 "rx-decode.opc"
                            int ss AU = op[1] & 0x03;
#line 659 "rx-decode.opc"
                            int rsrc AU = (op[3] >> 4) & 0x0f;
#line 659 "rx-decode.opc"
                            int rdst AU = op[3] & 0x0f;
                            if (trace)
                              {
                                printf ("\033[33m%s\033[0m  %02x %02x %02x %02x\n",
                                       "/** 0000 0110 mx10 00ss 0000 1001 rsrc rdst	divu	%1%S1, %0 */",
                                       op[0], op[1], op[2], op[3]);
                                printf ("  mx = 0x%x,", mx);
                                printf ("  ss = 0x%x,", ss);
                                printf ("  rsrc = 0x%x,", rsrc);
                                printf ("  rdst = 0x%x\n", rdst);
                              }
                            SYNTAX("divu	%1%S1, %0");
#line 659 "rx-decode.opc"
                            ID(divu); SPm(ss, rsrc, mx); DR(rdst); F_O___;
                          
                          /*----------------------------------------------------------------------*/
                          /* SHIFT								*/
                          
                          }
                        break;
                    }
                  break;
                case 0x0c:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        op_semantics_14:
                          {
                            /** 0000 0110 mx10 00ss 0000 1100 rsrc rdst	tst	%1%S1, %2 */
#line 470 "rx-decode.opc"
                            int mx AU = (op[1] >> 6) & 0x03;
#line 470 "rx-decode.opc"
                            int ss AU = op[1] & 0x03;
#line 470 "rx-decode.opc"
                            int rsrc AU = (op[3] >> 4) & 0x0f;
#line 470 "rx-decode.opc"
                            int rdst AU = op[3] & 0x0f;
                            if (trace)
                              {
                                printf ("\033[33m%s\033[0m  %02x %02x %02x %02x\n",
                                       "/** 0000 0110 mx10 00ss 0000 1100 rsrc rdst	tst	%1%S1, %2 */",
                                       op[0], op[1], op[2], op[3]);
                                printf ("  mx = 0x%x,", mx);
                                printf ("  ss = 0x%x,", ss);
                                printf ("  rsrc = 0x%x,", rsrc);
                                printf ("  rdst = 0x%x\n", rdst);
                              }
                            SYNTAX("tst	%1%S1, %2");
#line 470 "rx-decode.opc"
                            ID(and); SPm(ss, rsrc, mx); S2R(rdst); F__SZ_;
                          
                          /*----------------------------------------------------------------------*/
                          /* NEG									*/
                          
                          }
                        break;
                    }
                  break;
                case 0x0d:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        op_semantics_15:
                          {
                            /** 0000 0110 mx10 00ss 0000 1101 rsrc rdst	xor	%1%S1, %0 */
#line 449 "rx-decode.opc"
                            int mx AU = (op[1] >> 6) & 0x03;
#line 449 "rx-decode.opc"
                            int ss AU = op[1] & 0x03;
#line 449 "rx-decode.opc"
                            int rsrc AU = (op[3] >> 4) & 0x0f;
#line 449 "rx-decode.opc"
                            int rdst AU = op[3] & 0x0f;
                            if (trace)
                              {
                                printf ("\033[33m%s\033[0m  %02x %02x %02x %02x\n",
                                       "/** 0000 0110 mx10 00ss 0000 1101 rsrc rdst	xor	%1%S1, %0 */",
                                       op[0], op[1], op[2], op[3]);
                                printf ("  mx = 0x%x,", mx);
                                printf ("  ss = 0x%x,", ss);
                                printf ("  rsrc = 0x%x,", rsrc);
                                printf ("  rdst = 0x%x\n", rdst);
                              }
                            SYNTAX("xor	%1%S1, %0");
#line 449 "rx-decode.opc"
                            ID(xor); SPm(ss, rsrc, mx); DR(rdst); F__SZ_;
                          
                          /*----------------------------------------------------------------------*/
                          /* NOT									*/
                          
                          }
                        break;
                    }
                  break;
                case 0x10:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        op_semantics_16:
                          {
                            /** 0000 0110 mx10 00ss 0001 0000 rsrc rdst	xchg	%1%S1, %0 */
#line 383 "rx-decode.opc"
                            int mx AU = (op[1] >> 6) & 0x03;
#line 383 "rx-decode.opc"
                            int ss AU = op[1] & 0x03;
#line 383 "rx-decode.opc"
                            int rsrc AU = (op[3] >> 4) & 0x0f;
#line 383 "rx-decode.opc"
                            int rdst AU = op[3] & 0x0f;
                            if (trace)
                              {
                                printf ("\033[33m%s\033[0m  %02x %02x %02x %02x\n",
                                       "/** 0000 0110 mx10 00ss 0001 0000 rsrc rdst	xchg	%1%S1, %0 */",
                                       op[0], op[1], op[2], op[3]);
                                printf ("  mx = 0x%x,", mx);
                                printf ("  ss = 0x%x,", ss);
                                printf ("  rsrc = 0x%x,", rsrc);
                                printf ("  rdst = 0x%x\n", rdst);
                              }
                            SYNTAX("xchg	%1%S1, %0");
#line 383 "rx-decode.opc"
                            ID(xchg); DR(rdst); SPm(ss, rsrc, mx);
                          
                          /*----------------------------------------------------------------------*/
                          /* STZ/STNZ								*/
                          
                          }
                        break;
                    }
                  break;
                case 0x11:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        op_semantics_17:
                          {
                            /** 0000 0110 mx10 00sd 0001 0001 rsrc rdst	itof	%1%S1, %0 */
#line 888 "rx-decode.opc"
                            int mx AU = (op[1] >> 6) & 0x03;
#line 888 "rx-decode.opc"
                            int sd AU = op[1] & 0x03;
#line 888 "rx-decode.opc"
                            int rsrc AU = (op[3] >> 4) & 0x0f;
#line 888 "rx-decode.opc"
                            int rdst AU = op[3] & 0x0f;
                            if (trace)
                              {
                                printf ("\033[33m%s\033[0m  %02x %02x %02x %02x\n",
                                       "/** 0000 0110 mx10 00sd 0001 0001 rsrc rdst	itof	%1%S1, %0 */",
                                       op[0], op[1], op[2], op[3]);
                                printf ("  mx = 0x%x,", mx);
                                printf ("  sd = 0x%x,", sd);
                                printf ("  rsrc = 0x%x,", rsrc);
                                printf ("  rdst = 0x%x\n", rdst);
                              }
                            SYNTAX("itof	%1%S1, %0");
#line 888 "rx-decode.opc"
                            ID(itof); DR (rdst); SPm(sd, rsrc, mx); F__SZ_;
                          
                          /*----------------------------------------------------------------------*/
                          /* BIT OPS								*/
                          
                          }
                        break;
                    }
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0x21:
              GETBYTE ();
              switch (op[2] & 0xff)
              {
                case 0x00:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_7;
                        break;
                    }
                  break;
                case 0x04:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_8;
                        break;
                    }
                  break;
                case 0x05:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_9;
                        break;
                    }
                  break;
                case 0x06:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_10;
                        break;
                    }
                  break;
                case 0x07:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_11;
                        break;
                    }
                  break;
                case 0x08:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_12;
                        break;
                    }
                  break;
                case 0x09:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_13;
                        break;
                    }
                  break;
                case 0x0c:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_14;
                        break;
                    }
                  break;
                case 0x0d:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_15;
                        break;
                    }
                  break;
                case 0x10:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_16;
                        break;
                    }
                  break;
                case 0x11:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_17;
                        break;
                    }
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0x22:
              GETBYTE ();
              switch (op[2] & 0xff)
              {
                case 0x00:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_7;
                        break;
                    }
                  break;
                case 0x04:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_8;
                        break;
                    }
                  break;
                case 0x05:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_9;
                        break;
                    }
                  break;
                case 0x06:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_10;
                        break;
                    }
                  break;
                case 0x07:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_11;
                        break;
                    }
                  break;
                case 0x08:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_12;
                        break;
                    }
                  break;
                case 0x09:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_13;
                        break;
                    }
                  break;
                case 0x0c:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_14;
                        break;
                    }
                  break;
                case 0x0d:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_15;
                        break;
                    }
                  break;
                case 0x10:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_16;
                        break;
                    }
                  break;
                case 0x11:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_17;
                        break;
                    }
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0x23:
              GETBYTE ();
              switch (op[2] & 0xff)
              {
                case 0x00:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_7;
                        break;
                    }
                  break;
                case 0x04:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_8;
                        break;
                    }
                  break;
                case 0x05:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_9;
                        break;
                    }
                  break;
                case 0x06:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_10;
                        break;
                    }
                  break;
                case 0x07:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_11;
                        break;
                    }
                  break;
                case 0x08:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_12;
                        break;
                    }
                  break;
                case 0x09:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_13;
                        break;
                    }
                  break;
                case 0x0c:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_14;
                        break;
                    }
                  break;
                case 0x0d:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_15;
                        break;
                    }
                  break;
                case 0x10:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_16;
                        break;
                    }
                  break;
                case 0x11:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_17;
                        break;
                    }
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0x40:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_1;
                  break;
              }
            break;
          case 0x41:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_1;
                  break;
              }
            break;
          case 0x42:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_1;
                  break;
              }
            break;
          case 0x43:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_1;
                  break;
              }
            break;
          case 0x44:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_2;
                  break;
              }
            break;
          case 0x45:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_2;
                  break;
              }
            break;
          case 0x46:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_2;
                  break;
              }
            break;
          case 0x47:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_2;
                  break;
              }
            break;
          case 0x48:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_3;
                  break;
              }
            break;
          case 0x49:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_3;
                  break;
              }
            break;
          case 0x4a:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_3;
                  break;
              }
            break;
          case 0x4b:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_3;
                  break;
              }
            break;
          case 0x4c:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_4;
                  break;
              }
            break;
          case 0x4d:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_4;
                  break;
              }
            break;
          case 0x4e:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_4;
                  break;
              }
            break;
          case 0x4f:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_4;
                  break;
              }
            break;
          case 0x50:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_5;
                  break;
              }
            break;
          case 0x51:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_5;
                  break;
              }
            break;
          case 0x52:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_5;
                  break;
              }
            break;
          case 0x53:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_5;
                  break;
              }
            break;
          case 0x54:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_6;
                  break;
              }
            break;
          case 0x55:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_6;
                  break;
              }
            break;
          case 0x56:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_6;
                  break;
              }
            break;
          case 0x57:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_6;
                  break;
              }
            break;
          case 0x60:
              GETBYTE ();
              switch (op[2] & 0xff)
              {
                case 0x00:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_7;
                        break;
                    }
                  break;
                case 0x04:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_8;
                        break;
                    }
                  break;
                case 0x05:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_9;
                        break;
                    }
                  break;
                case 0x06:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_10;
                        break;
                    }
                  break;
                case 0x07:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_11;
                        break;
                    }
                  break;
                case 0x08:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_12;
                        break;
                    }
                  break;
                case 0x09:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_13;
                        break;
                    }
                  break;
                case 0x0c:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_14;
                        break;
                    }
                  break;
                case 0x0d:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_15;
                        break;
                    }
                  break;
                case 0x10:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_16;
                        break;
                    }
                  break;
                case 0x11:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_17;
                        break;
                    }
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0x61:
              GETBYTE ();
              switch (op[2] & 0xff)
              {
                case 0x00:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_7;
                        break;
                    }
                  break;
                case 0x04:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_8;
                        break;
                    }
                  break;
                case 0x05:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_9;
                        break;
                    }
                  break;
                case 0x06:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_10;
                        break;
                    }
                  break;
                case 0x07:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_11;
                        break;
                    }
                  break;
                case 0x08:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_12;
                        break;
                    }
                  break;
                case 0x09:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_13;
                        break;
                    }
                  break;
                case 0x0c:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_14;
                        break;
                    }
                  break;
                case 0x0d:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_15;
                        break;
                    }
                  break;
                case 0x10:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_16;
                        break;
                    }
                  break;
                case 0x11:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_17;
                        break;
                    }
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0x62:
              GETBYTE ();
              switch (op[2] & 0xff)
              {
                case 0x00:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_7;
                        break;
                    }
                  break;
                case 0x04:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_8;
                        break;
                    }
                  break;
                case 0x05:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_9;
                        break;
                    }
                  break;
                case 0x06:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_10;
                        break;
                    }
                  break;
                case 0x07:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_11;
                        break;
                    }
                  break;
                case 0x08:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_12;
                        break;
                    }
                  break;
                case 0x09:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_13;
                        break;
                    }
                  break;
                case 0x0c:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_14;
                        break;
                    }
                  break;
                case 0x0d:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_15;
                        break;
                    }
                  break;
                case 0x10:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_16;
                        break;
                    }
                  break;
                case 0x11:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_17;
                        break;
                    }
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0x63:
              GETBYTE ();
              switch (op[2] & 0xff)
              {
                case 0x00:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_7;
                        break;
                    }
                  break;
                case 0x04:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_8;
                        break;
                    }
                  break;
                case 0x05:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_9;
                        break;
                    }
                  break;
                case 0x06:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_10;
                        break;
                    }
                  break;
                case 0x07:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_11;
                        break;
                    }
                  break;
                case 0x08:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_12;
                        break;
                    }
                  break;
                case 0x09:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_13;
                        break;
                    }
                  break;
                case 0x0c:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_14;
                        break;
                    }
                  break;
                case 0x0d:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_15;
                        break;
                    }
                  break;
                case 0x10:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_16;
                        break;
                    }
                  break;
                case 0x11:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_17;
                        break;
                    }
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0x80:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_1;
                  break;
              }
            break;
          case 0x81:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_1;
                  break;
              }
            break;
          case 0x82:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_1;
                  break;
              }
            break;
          case 0x83:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_1;
                  break;
              }
            break;
          case 0x84:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_2;
                  break;
              }
            break;
          case 0x85:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_2;
                  break;
              }
            break;
          case 0x86:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_2;
                  break;
              }
            break;
          case 0x87:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_2;
                  break;
              }
            break;
          case 0x88:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_3;
                  break;
              }
            break;
          case 0x89:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_3;
                  break;
              }
            break;
          case 0x8a:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_3;
                  break;
              }
            break;
          case 0x8b:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_3;
                  break;
              }
            break;
          case 0x8c:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_4;
                  break;
              }
            break;
          case 0x8d:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_4;
                  break;
              }
            break;
          case 0x8e:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_4;
                  break;
              }
            break;
          case 0x8f:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_4;
                  break;
              }
            break;
          case 0x90:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_5;
                  break;
              }
            break;
          case 0x91:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_5;
                  break;
              }
            break;
          case 0x92:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_5;
                  break;
              }
            break;
          case 0x93:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_5;
                  break;
              }
            break;
          case 0x94:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_6;
                  break;
              }
            break;
          case 0x95:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_6;
                  break;
              }
            break;
          case 0x96:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_6;
                  break;
              }
            break;
          case 0x97:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_6;
                  break;
              }
            break;
          case 0xa0:
              GETBYTE ();
              switch (op[2] & 0xff)
              {
                case 0x00:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_7;
                        break;
                    }
                  break;
                case 0x02:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        op_semantics_18:
                          {
                            /** 0000 0110 1010 00ss 0000 0010 rsrc rdst	adc	%1%S1, %0 */
#line 491 "rx-decode.opc"
                            int ss AU = op[1] & 0x03;
#line 491 "rx-decode.opc"
                            int rsrc AU = (op[3] >> 4) & 0x0f;
#line 491 "rx-decode.opc"
                            int rdst AU = op[3] & 0x0f;
                            if (trace)
                              {
                                printf ("\033[33m%s\033[0m  %02x %02x %02x %02x\n",
                                       "/** 0000 0110 1010 00ss 0000 0010 rsrc rdst	adc	%1%S1, %0 */",
                                       op[0], op[1], op[2], op[3]);
                                printf ("  ss = 0x%x,", ss);
                                printf ("  rsrc = 0x%x,", rsrc);
                                printf ("  rdst = 0x%x\n", rdst);
                              }
                            SYNTAX("adc	%1%S1, %0");
#line 491 "rx-decode.opc"
                            ID(adc); SPm(ss, rsrc, 2); DR(rdst); F_OSZC;
                          
                          /*----------------------------------------------------------------------*/
                          /* ADD									*/
                          
                          }
                        break;
                    }
                  break;
                case 0x04:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_8;
                        break;
                    }
                  break;
                case 0x05:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_9;
                        break;
                    }
                  break;
                case 0x06:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_10;
                        break;
                    }
                  break;
                case 0x07:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_11;
                        break;
                    }
                  break;
                case 0x08:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_12;
                        break;
                    }
                  break;
                case 0x09:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_13;
                        break;
                    }
                  break;
                case 0x0c:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_14;
                        break;
                    }
                  break;
                case 0x0d:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_15;
                        break;
                    }
                  break;
                case 0x10:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_16;
                        break;
                    }
                  break;
                case 0x11:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_17;
                        break;
                    }
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0xa1:
              GETBYTE ();
              switch (op[2] & 0xff)
              {
                case 0x00:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_7;
                        break;
                    }
                  break;
                case 0x02:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_18;
                        break;
                    }
                  break;
                case 0x04:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_8;
                        break;
                    }
                  break;
                case 0x05:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_9;
                        break;
                    }
                  break;
                case 0x06:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_10;
                        break;
                    }
                  break;
                case 0x07:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_11;
                        break;
                    }
                  break;
                case 0x08:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_12;
                        break;
                    }
                  break;
                case 0x09:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_13;
                        break;
                    }
                  break;
                case 0x0c:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_14;
                        break;
                    }
                  break;
                case 0x0d:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_15;
                        break;
                    }
                  break;
                case 0x10:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_16;
                        break;
                    }
                  break;
                case 0x11:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_17;
                        break;
                    }
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0xa2:
              GETBYTE ();
              switch (op[2] & 0xff)
              {
                case 0x00:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_7;
                        break;
                    }
                  break;
                case 0x02:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_18;
                        break;
                    }
                  break;
                case 0x04:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_8;
                        break;
                    }
                  break;
                case 0x05:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_9;
                        break;
                    }
                  break;
                case 0x06:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_10;
                        break;
                    }
                  break;
                case 0x07:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_11;
                        break;
                    }
                  break;
                case 0x08:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_12;
                        break;
                    }
                  break;
                case 0x09:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_13;
                        break;
                    }
                  break;
                case 0x0c:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_14;
                        break;
                    }
                  break;
                case 0x0d:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_15;
                        break;
                    }
                  break;
                case 0x10:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_16;
                        break;
                    }
                  break;
                case 0x11:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_17;
                        break;
                    }
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0xa3:
              GETBYTE ();
              switch (op[2] & 0xff)
              {
                case 0x00:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_7;
                        break;
                    }
                  break;
                case 0x02:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_18;
                        break;
                    }
                  break;
                case 0x04:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_8;
                        break;
                    }
                  break;
                case 0x05:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_9;
                        break;
                    }
                  break;
                case 0x06:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_10;
                        break;
                    }
                  break;
                case 0x07:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_11;
                        break;
                    }
                  break;
                case 0x08:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_12;
                        break;
                    }
                  break;
                case 0x09:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_13;
                        break;
                    }
                  break;
                case 0x0c:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_14;
                        break;
                    }
                  break;
                case 0x0d:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_15;
                        break;
                    }
                  break;
                case 0x10:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_16;
                        break;
                    }
                  break;
                case 0x11:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_17;
                        break;
                    }
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0xc0:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_1;
                  break;
              }
            break;
          case 0xc1:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_1;
                  break;
              }
            break;
          case 0xc2:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_1;
                  break;
              }
            break;
          case 0xc3:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_1;
                  break;
              }
            break;
          case 0xc4:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_2;
                  break;
              }
            break;
          case 0xc5:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_2;
                  break;
              }
            break;
          case 0xc6:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_2;
                  break;
              }
            break;
          case 0xc7:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_2;
                  break;
              }
            break;
          case 0xc8:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_3;
                  break;
              }
            break;
          case 0xc9:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_3;
                  break;
              }
            break;
          case 0xca:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_3;
                  break;
              }
            break;
          case 0xcb:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_3;
                  break;
              }
            break;
          case 0xcc:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_4;
                  break;
              }
            break;
          case 0xcd:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_4;
                  break;
              }
            break;
          case 0xce:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_4;
                  break;
              }
            break;
          case 0xcf:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_4;
                  break;
              }
            break;
          case 0xd0:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_5;
                  break;
              }
            break;
          case 0xd1:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_5;
                  break;
              }
            break;
          case 0xd2:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_5;
                  break;
              }
            break;
          case 0xd3:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_5;
                  break;
              }
            break;
          case 0xd4:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_6;
                  break;
              }
            break;
          case 0xd5:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_6;
                  break;
              }
            break;
          case 0xd6:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_6;
                  break;
              }
            break;
          case 0xd7:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_6;
                  break;
              }
            break;
          case 0xe0:
              GETBYTE ();
              switch (op[2] & 0xff)
              {
                case 0x00:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_7;
                        break;
                    }
                  break;
                case 0x04:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_8;
                        break;
                    }
                  break;
                case 0x05:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_9;
                        break;
                    }
                  break;
                case 0x06:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_10;
                        break;
                    }
                  break;
                case 0x07:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_11;
                        break;
                    }
                  break;
                case 0x08:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_12;
                        break;
                    }
                  break;
                case 0x09:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_13;
                        break;
                    }
                  break;
                case 0x0c:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_14;
                        break;
                    }
                  break;
                case 0x0d:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_15;
                        break;
                    }
                  break;
                case 0x10:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_16;
                        break;
                    }
                  break;
                case 0x11:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_17;
                        break;
                    }
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0xe1:
              GETBYTE ();
              switch (op[2] & 0xff)
              {
                case 0x00:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_7;
                        break;
                    }
                  break;
                case 0x04:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_8;
                        break;
                    }
                  break;
                case 0x05:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_9;
                        break;
                    }
                  break;
                case 0x06:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_10;
                        break;
                    }
                  break;
                case 0x07:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_11;
                        break;
                    }
                  break;
                case 0x08:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_12;
                        break;
                    }
                  break;
                case 0x09:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_13;
                        break;
                    }
                  break;
                case 0x0c:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_14;
                        break;
                    }
                  break;
                case 0x0d:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_15;
                        break;
                    }
                  break;
                case 0x10:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_16;
                        break;
                    }
                  break;
                case 0x11:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_17;
                        break;
                    }
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0xe2:
              GETBYTE ();
              switch (op[2] & 0xff)
              {
                case 0x00:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_7;
                        break;
                    }
                  break;
                case 0x04:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_8;
                        break;
                    }
                  break;
                case 0x05:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_9;
                        break;
                    }
                  break;
                case 0x06:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_10;
                        break;
                    }
                  break;
                case 0x07:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_11;
                        break;
                    }
                  break;
                case 0x08:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_12;
                        break;
                    }
                  break;
                case 0x09:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_13;
                        break;
                    }
                  break;
                case 0x0c:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_14;
                        break;
                    }
                  break;
                case 0x0d:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_15;
                        break;
                    }
                  break;
                case 0x10:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_16;
                        break;
                    }
                  break;
                case 0x11:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_17;
                        break;
                    }
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0xe3:
              GETBYTE ();
              switch (op[2] & 0xff)
              {
                case 0x00:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_7;
                        break;
                    }
                  break;
                case 0x04:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_8;
                        break;
                    }
                  break;
                case 0x05:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_9;
                        break;
                    }
                  break;
                case 0x06:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_10;
                        break;
                    }
                  break;
                case 0x07:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_11;
                        break;
                    }
                  break;
                case 0x08:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_12;
                        break;
                    }
                  break;
                case 0x09:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_13;
                        break;
                    }
                  break;
                case 0x0c:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_14;
                        break;
                    }
                  break;
                case 0x0d:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_15;
                        break;
                    }
                  break;
                case 0x10:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_16;
                        break;
                    }
                  break;
                case 0x11:
                    GETBYTE ();
                    switch (op[3] & 0x00)
                    {
                      case 0x00:
                        goto op_semantics_17;
                        break;
                    }
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          default: UNSUPPORTED(); break;
        }
      break;
    case 0x08:
    case 0x09:
    case 0x0a:
    case 0x0b:
    case 0x0c:
    case 0x0d:
    case 0x0e:
    case 0x0f:
        {
          /** 0000 1dsp			bra.s	%a0 */
#line 734 "rx-decode.opc"
          int dsp AU = op[0] & 0x07;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0000 1dsp			bra.s	%a0 */",
                     op[0]);
              printf ("  dsp = 0x%x\n", dsp);
            }
          SYNTAX("bra.s	%a0");
#line 734 "rx-decode.opc"
          ID(branch); DC(pc + dsp3map[dsp]);
        
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
    case 0x18:
    case 0x19:
    case 0x1a:
    case 0x1b:
    case 0x1c:
    case 0x1d:
    case 0x1e:
    case 0x1f:
        {
          /** 0001 n dsp			b%1.s	%a0 */
#line 724 "rx-decode.opc"
          int n AU = (op[0] >> 3) & 0x01;
#line 724 "rx-decode.opc"
          int dsp AU = op[0] & 0x07;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0001 n dsp			b%1.s	%a0 */",
                     op[0]);
              printf ("  n = 0x%x,", n);
              printf ("  dsp = 0x%x\n", dsp);
            }
          SYNTAX("b%1.s	%a0");
#line 724 "rx-decode.opc"
          ID(branch); Scc(n); DC(pc + dsp3map[dsp]);
        
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
    case 0x28:
    case 0x29:
    case 0x2a:
    case 0x2b:
    case 0x2c:
    case 0x2d:
    case 0x2f:
        {
          /** 0010 cond			b%1.b	%a0 */
#line 727 "rx-decode.opc"
          int cond AU = op[0] & 0x0f;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0010 cond			b%1.b	%a0 */",
                     op[0]);
              printf ("  cond = 0x%x\n", cond);
            }
          SYNTAX("b%1.b	%a0");
#line 727 "rx-decode.opc"
          ID(branch); Scc(cond); DC(pc + IMMex (1));
        
        }
      break;
    case 0x2e:
        {
          /** 0010 1110			bra.b	%a0 */
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0010 1110			bra.b	%a0 */",
                     op[0]);
            }
          SYNTAX("bra.b	%a0");
#line 737 "rx-decode.opc"
          ID(branch); DC(pc + IMMex(1));
        
        }
      break;
    case 0x38:
        {
          /** 0011 1000			bra.w	%a0 */
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0011 1000			bra.w	%a0 */",
                     op[0]);
            }
          SYNTAX("bra.w	%a0");
#line 740 "rx-decode.opc"
          ID(branch); DC(pc + IMMex(2));
        
        }
      break;
    case 0x39:
        {
          /** 0011 1001			bsr.w	%a0 */
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0011 1001			bsr.w	%a0 */",
                     op[0]);
            }
          SYNTAX("bsr.w	%a0");
#line 756 "rx-decode.opc"
          ID(jsr); DC(pc + IMMex(2));
        
        }
      break;
    case 0x3a:
    case 0x3b:
        {
          /** 0011 101c			b%1.w	%a0 */
#line 730 "rx-decode.opc"
          int c AU = op[0] & 0x01;
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0011 101c			b%1.w	%a0 */",
                     op[0]);
              printf ("  c = 0x%x\n", c);
            }
          SYNTAX("b%1.w	%a0");
#line 730 "rx-decode.opc"
          ID(branch); Scc(c); DC(pc + IMMex (2));
        
        
        }
      break;
    case 0x3c:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            op_semantics_19:
              {
                /** 0011 11sz d dst sppp		mov%s	#%1, %0 */
#line 304 "rx-decode.opc"
                int sz AU = op[0] & 0x03;
#line 304 "rx-decode.opc"
                int d AU = (op[1] >> 7) & 0x01;
#line 304 "rx-decode.opc"
                int dst AU = (op[1] >> 4) & 0x07;
#line 304 "rx-decode.opc"
                int sppp AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0011 11sz d dst sppp		mov%s	#%1, %0 */",
                           op[0], op[1]);
                    printf ("  sz = 0x%x,", sz);
                    printf ("  d = 0x%x,", d);
                    printf ("  dst = 0x%x,", dst);
                    printf ("  sppp = 0x%x\n", sppp);
                  }
                SYNTAX("mov%s	#%1, %0");
#line 304 "rx-decode.opc"
                ID(mov); sBWL (sz); DIs(dst, d*16+sppp, sz); SC(IMM(1)); F_____;
              
              }
            break;
        }
      break;
    case 0x3d:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_19;
            break;
        }
      break;
    case 0x3e:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_19;
            break;
        }
      break;
    case 0x3f:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
              {
                /** 0011 1111 rega regb		rtsd	#%1, %2-%0 */
#line 401 "rx-decode.opc"
                int rega AU = (op[1] >> 4) & 0x0f;
#line 401 "rx-decode.opc"
                int regb AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0011 1111 rega regb		rtsd	#%1, %2-%0 */",
                           op[0], op[1]);
                    printf ("  rega = 0x%x,", rega);
                    printf ("  regb = 0x%x\n", regb);
                  }
                SYNTAX("rtsd	#%1, %2-%0");
#line 401 "rx-decode.opc"
                ID(rtsd); SC(IMM(1) * 4); S2R(rega); DR(regb);
              
              /*----------------------------------------------------------------------*/
              /* AND									*/
              
              }
            break;
        }
      break;
    case 0x40:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            op_semantics_20:
              {
                /** 0100 00ss rsrc rdst			sub	%2%S2, %1 */
#line 536 "rx-decode.opc"
                int ss AU = op[0] & 0x03;
#line 536 "rx-decode.opc"
                int rsrc AU = (op[1] >> 4) & 0x0f;
#line 536 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0100 00ss rsrc rdst			sub	%2%S2, %1 */",
                           op[0], op[1]);
                    printf ("  ss = 0x%x,", ss);
                    printf ("  rsrc = 0x%x,", rsrc);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("sub	%2%S2, %1");
#line 536 "rx-decode.opc"
                ID(sub); S2P(ss, rsrc); SR(rdst); DR(rdst); F_OSZC;
              
              }
            break;
        }
      break;
    case 0x41:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_20;
            break;
        }
      break;
    case 0x42:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_20;
            break;
        }
      break;
    case 0x43:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_20;
            break;
        }
      break;
    case 0x44:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            op_semantics_21:
              {
                /** 0100 01ss rsrc rdst		cmp	%2%S2, %1 */
#line 524 "rx-decode.opc"
                int ss AU = op[0] & 0x03;
#line 524 "rx-decode.opc"
                int rsrc AU = (op[1] >> 4) & 0x0f;
#line 524 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0100 01ss rsrc rdst		cmp	%2%S2, %1 */",
                           op[0], op[1]);
                    printf ("  ss = 0x%x,", ss);
                    printf ("  rsrc = 0x%x,", rsrc);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("cmp	%2%S2, %1");
#line 524 "rx-decode.opc"
                ID(sub); S2P(ss, rsrc); SR(rdst); F_OSZC;
              
              }
            break;
        }
      break;
    case 0x45:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_21;
            break;
        }
      break;
    case 0x46:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_21;
            break;
        }
      break;
    case 0x47:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_21;
            break;
        }
      break;
    case 0x48:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            op_semantics_22:
              {
                /** 0100 10ss rsrc rdst			add	%1%S1, %0 */
#line 500 "rx-decode.opc"
                int ss AU = op[0] & 0x03;
#line 500 "rx-decode.opc"
                int rsrc AU = (op[1] >> 4) & 0x0f;
#line 500 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0100 10ss rsrc rdst			add	%1%S1, %0 */",
                           op[0], op[1]);
                    printf ("  ss = 0x%x,", ss);
                    printf ("  rsrc = 0x%x,", rsrc);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("add	%1%S1, %0");
#line 500 "rx-decode.opc"
                ID(add); SP(ss, rsrc); DR(rdst); F_OSZC;
              
              }
            break;
        }
      break;
    case 0x49:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_22;
            break;
        }
      break;
    case 0x4a:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_22;
            break;
        }
      break;
    case 0x4b:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_22;
            break;
        }
      break;
    case 0x4c:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            op_semantics_23:
              {
                /** 0100 11ss rsrc rdst			mul	%1%S1, %0 */
#line 605 "rx-decode.opc"
                int ss AU = op[0] & 0x03;
#line 605 "rx-decode.opc"
                int rsrc AU = (op[1] >> 4) & 0x0f;
#line 605 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0100 11ss rsrc rdst			mul	%1%S1, %0 */",
                           op[0], op[1]);
                    printf ("  ss = 0x%x,", ss);
                    printf ("  rsrc = 0x%x,", rsrc);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("mul	%1%S1, %0");
#line 605 "rx-decode.opc"
                ID(mul); SP(ss, rsrc); DR(rdst); F_____;
              
              }
            break;
        }
      break;
    case 0x4d:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_23;
            break;
        }
      break;
    case 0x4e:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_23;
            break;
        }
      break;
    case 0x4f:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_23;
            break;
        }
      break;
    case 0x50:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            op_semantics_24:
              {
                /** 0101 00ss rsrc rdst			and	%1%S1, %0 */
#line 413 "rx-decode.opc"
                int ss AU = op[0] & 0x03;
#line 413 "rx-decode.opc"
                int rsrc AU = (op[1] >> 4) & 0x0f;
#line 413 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0101 00ss rsrc rdst			and	%1%S1, %0 */",
                           op[0], op[1]);
                    printf ("  ss = 0x%x,", ss);
                    printf ("  rsrc = 0x%x,", rsrc);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("and	%1%S1, %0");
#line 413 "rx-decode.opc"
                ID(and); SP(ss, rsrc); DR(rdst); F__SZ_;
              
              }
            break;
        }
      break;
    case 0x51:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_24;
            break;
        }
      break;
    case 0x52:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_24;
            break;
        }
      break;
    case 0x53:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_24;
            break;
        }
      break;
    case 0x54:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            op_semantics_25:
              {
                /** 0101 01ss rsrc rdst			or	%1%S1, %0 */
#line 431 "rx-decode.opc"
                int ss AU = op[0] & 0x03;
#line 431 "rx-decode.opc"
                int rsrc AU = (op[1] >> 4) & 0x0f;
#line 431 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0101 01ss rsrc rdst			or	%1%S1, %0 */",
                           op[0], op[1]);
                    printf ("  ss = 0x%x,", ss);
                    printf ("  rsrc = 0x%x,", rsrc);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("or	%1%S1, %0");
#line 431 "rx-decode.opc"
                ID(or); SP(ss, rsrc); DR(rdst); F__SZ_;
              
              }
            break;
        }
      break;
    case 0x55:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_25;
            break;
        }
      break;
    case 0x56:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_25;
            break;
        }
      break;
    case 0x57:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_25;
            break;
        }
      break;
    case 0x58:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            op_semantics_26:
              {
                /** 0101 1 s ss rsrc rdst	movu%s	%1, %0 */
#line 352 "rx-decode.opc"
                int s AU = (op[0] >> 2) & 0x01;
#line 352 "rx-decode.opc"
                int ss AU = op[0] & 0x03;
#line 352 "rx-decode.opc"
                int rsrc AU = (op[1] >> 4) & 0x0f;
#line 352 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0101 1 s ss rsrc rdst	movu%s	%1, %0 */",
                           op[0], op[1]);
                    printf ("  s = 0x%x,", s);
                    printf ("  ss = 0x%x,", ss);
                    printf ("  rsrc = 0x%x,", rsrc);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("movu%s	%1, %0");
#line 352 "rx-decode.opc"
                ID(mov); uBWL(s); SD(ss, rsrc, s); DR(rdst); F_____;
              
              }
            break;
        }
      break;
    case 0x59:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_26;
            break;
        }
      break;
    case 0x5a:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_26;
            break;
        }
      break;
    case 0x5b:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_26;
            break;
        }
      break;
    case 0x5c:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_26;
            break;
        }
      break;
    case 0x5d:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_26;
            break;
        }
      break;
    case 0x5e:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_26;
            break;
        }
      break;
    case 0x5f:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_26;
            break;
        }
      break;
    case 0x60:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
              {
                /** 0110 0000 immm rdst			sub	#%2, %0 */
#line 533 "rx-decode.opc"
                int immm AU = (op[1] >> 4) & 0x0f;
#line 533 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0000 immm rdst			sub	#%2, %0 */",
                           op[0], op[1]);
                    printf ("  immm = 0x%x,", immm);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("sub	#%2, %0");
#line 533 "rx-decode.opc"
                ID(sub); S2C(immm); SR(rdst); DR(rdst); F_OSZC;
              
              }
            break;
        }
      break;
    case 0x61:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
              {
                /** 0110 0001 immm rdst			cmp	#%2, %1 */
#line 515 "rx-decode.opc"
                int immm AU = (op[1] >> 4) & 0x0f;
#line 515 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0001 immm rdst			cmp	#%2, %1 */",
                           op[0], op[1]);
                    printf ("  immm = 0x%x,", immm);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("cmp	#%2, %1");
#line 515 "rx-decode.opc"
                ID(sub); S2C(immm); SR(rdst); F_OSZC;
              
              }
            break;
        }
      break;
    case 0x62:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
              {
                /** 0110 0010 immm rdst			add	#%1, %0 */
#line 497 "rx-decode.opc"
                int immm AU = (op[1] >> 4) & 0x0f;
#line 497 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0010 immm rdst			add	#%1, %0 */",
                           op[0], op[1]);
                    printf ("  immm = 0x%x,", immm);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("add	#%1, %0");
#line 497 "rx-decode.opc"
                ID(add); SC(immm); DR(rdst); F_OSZC;
              
              }
            break;
        }
      break;
    case 0x63:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
              {
                /** 0110 0011 immm rdst			mul	#%1, %0 */
#line 599 "rx-decode.opc"
                int immm AU = (op[1] >> 4) & 0x0f;
#line 599 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0011 immm rdst			mul	#%1, %0 */",
                           op[0], op[1]);
                    printf ("  immm = 0x%x,", immm);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("mul	#%1, %0");
#line 599 "rx-decode.opc"
                ID(mul); DR(rdst); SC(immm); F_____;
              
              }
            break;
        }
      break;
    case 0x64:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
              {
                /** 0110 0100 immm rdst			and	#%1, %0 */
#line 407 "rx-decode.opc"
                int immm AU = (op[1] >> 4) & 0x0f;
#line 407 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0100 immm rdst			and	#%1, %0 */",
                           op[0], op[1]);
                    printf ("  immm = 0x%x,", immm);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("and	#%1, %0");
#line 407 "rx-decode.opc"
                ID(and); SC(immm); DR(rdst); F__SZ_;
              
              }
            break;
        }
      break;
    case 0x65:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
              {
                /** 0110 0101 immm rdst			or	#%1, %0 */
#line 425 "rx-decode.opc"
                int immm AU = (op[1] >> 4) & 0x0f;
#line 425 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0101 immm rdst			or	#%1, %0 */",
                           op[0], op[1]);
                    printf ("  immm = 0x%x,", immm);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("or	#%1, %0");
#line 425 "rx-decode.opc"
                ID(or); SC(immm); DR(rdst); F__SZ_;
              
              }
            break;
        }
      break;
    case 0x66:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
              {
                /** 0110 0110 immm rdst		mov%s	#%1, %0 */
#line 301 "rx-decode.opc"
                int immm AU = (op[1] >> 4) & 0x0f;
#line 301 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 0110 immm rdst		mov%s	#%1, %0 */",
                           op[0], op[1]);
                    printf ("  immm = 0x%x,", immm);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("mov%s	#%1, %0");
#line 301 "rx-decode.opc"
                ID(mov); DR(rdst); SC(immm); F_____;
              
              }
            break;
        }
      break;
    case 0x67:
        {
          /** 0110 0111			rtsd	#%1 */
          if (trace)
            {
              printf ("\033[33m%s\033[0m  %02x\n",
                     "/** 0110 0111			rtsd	#%1 */",
                     op[0]);
            }
          SYNTAX("rtsd	#%1");
#line 398 "rx-decode.opc"
          ID(rtsd); SC(IMM(1) * 4);
        
        }
      break;
    case 0x68:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            op_semantics_27:
              {
                /** 0110 100i mmmm rdst			shlr	#%2, %0 */
#line 685 "rx-decode.opc"
                int i AU = op[0] & 0x01;
#line 685 "rx-decode.opc"
                int mmmm AU = (op[1] >> 4) & 0x0f;
#line 685 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 100i mmmm rdst			shlr	#%2, %0 */",
                           op[0], op[1]);
                    printf ("  i = 0x%x,", i);
                    printf ("  mmmm = 0x%x,", mmmm);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("shlr	#%2, %0");
#line 685 "rx-decode.opc"
                ID(shlr); S2C(i*16+mmmm); SR(rdst); DR(rdst); F__SZC;
              
              }
            break;
        }
      break;
    case 0x69:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_27;
            break;
        }
      break;
    case 0x6a:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            op_semantics_28:
              {
                /** 0110 101i mmmm rdst			shar	#%2, %0 */
#line 675 "rx-decode.opc"
                int i AU = op[0] & 0x01;
#line 675 "rx-decode.opc"
                int mmmm AU = (op[1] >> 4) & 0x0f;
#line 675 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 101i mmmm rdst			shar	#%2, %0 */",
                           op[0], op[1]);
                    printf ("  i = 0x%x,", i);
                    printf ("  mmmm = 0x%x,", mmmm);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("shar	#%2, %0");
#line 675 "rx-decode.opc"
                ID(shar); S2C(i*16+mmmm); SR(rdst); DR(rdst); F_0SZC;
              
              }
            break;
        }
      break;
    case 0x6b:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_28;
            break;
        }
      break;
    case 0x6c:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            op_semantics_29:
              {
                /** 0110 110i mmmm rdst			shll	#%2, %0 */
#line 665 "rx-decode.opc"
                int i AU = op[0] & 0x01;
#line 665 "rx-decode.opc"
                int mmmm AU = (op[1] >> 4) & 0x0f;
#line 665 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 110i mmmm rdst			shll	#%2, %0 */",
                           op[0], op[1]);
                    printf ("  i = 0x%x,", i);
                    printf ("  mmmm = 0x%x,", mmmm);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("shll	#%2, %0");
#line 665 "rx-decode.opc"
                ID(shll); S2C(i*16+mmmm); SR(rdst); DR(rdst); F_OSZC;
              
              }
            break;
        }
      break;
    case 0x6d:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_29;
            break;
        }
      break;
    case 0x6e:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
              {
                /** 0110 1110 dsta dstb		pushm	%1-%2 */
#line 365 "rx-decode.opc"
                int dsta AU = (op[1] >> 4) & 0x0f;
#line 365 "rx-decode.opc"
                int dstb AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 1110 dsta dstb		pushm	%1-%2 */",
                           op[0], op[1]);
                    printf ("  dsta = 0x%x,", dsta);
                    printf ("  dstb = 0x%x\n", dstb);
                  }
                SYNTAX("pushm	%1-%2");
#line 365 "rx-decode.opc"
                ID(pushm); SR(dsta); S2R(dstb); F_____;
                
              }
            break;
        }
      break;
    case 0x6f:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
              {
                /** 0110 1111 dsta dstb		popm	%1-%2 */
#line 362 "rx-decode.opc"
                int dsta AU = (op[1] >> 4) & 0x0f;
#line 362 "rx-decode.opc"
                int dstb AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0110 1111 dsta dstb		popm	%1-%2 */",
                           op[0], op[1]);
                    printf ("  dsta = 0x%x,", dsta);
                    printf ("  dstb = 0x%x\n", dstb);
                  }
                SYNTAX("popm	%1-%2");
#line 362 "rx-decode.opc"
                ID(popm); SR(dsta); S2R(dstb); F_____;
              
              }
            break;
        }
      break;
    case 0x70:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            op_semantics_30:
              {
                /** 0111 00im rsrc rdst			add	#%1, %2, %0 */
#line 506 "rx-decode.opc"
                int im AU = op[0] & 0x03;
#line 506 "rx-decode.opc"
                int rsrc AU = (op[1] >> 4) & 0x0f;
#line 506 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 00im rsrc rdst			add	#%1, %2, %0 */",
                           op[0], op[1]);
                    printf ("  im = 0x%x,", im);
                    printf ("  rsrc = 0x%x,", rsrc);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("add	#%1, %2, %0");
#line 506 "rx-decode.opc"
                ID(add); SC(IMMex(im)); S2R(rsrc); DR(rdst); F_OSZC;
              
              }
            break;
        }
      break;
    case 0x71:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_30;
            break;
        }
      break;
    case 0x72:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_30;
            break;
        }
      break;
    case 0x73:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_30;
            break;
        }
      break;
    case 0x74:
        GETBYTE ();
        switch (op[1] & 0xf0)
        {
          case 0x00:
            op_semantics_31:
              {
                /** 0111 01im 0000 rsrc		cmp	#%2, %1%S1 */
#line 518 "rx-decode.opc"
                int im AU = op[0] & 0x03;
#line 518 "rx-decode.opc"
                int rsrc AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 01im 0000 rsrc		cmp	#%2, %1%S1 */",
                           op[0], op[1]);
                    printf ("  im = 0x%x,", im);
                    printf ("  rsrc = 0x%x\n", rsrc);
                  }
                SYNTAX("cmp	#%2, %1%S1");
#line 518 "rx-decode.opc"
                ID(sub); SR(rsrc); S2C(IMMex(im)); F_OSZC;
              
              }
            break;
          case 0x10:
            op_semantics_32:
              {
                /** 0111 01im 0001rdst			mul	#%1, %0 */
#line 602 "rx-decode.opc"
                int im AU = op[0] & 0x03;
#line 602 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 01im 0001rdst			mul	#%1, %0 */",
                           op[0], op[1]);
                    printf ("  im = 0x%x,", im);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("mul	#%1, %0");
#line 602 "rx-decode.opc"
                ID(mul); DR(rdst); SC(IMMex(im)); F_____;
              
              }
            break;
          case 0x20:
            op_semantics_33:
              {
                /** 0111 01im 0010 rdst			and	#%1, %0 */
#line 410 "rx-decode.opc"
                int im AU = op[0] & 0x03;
#line 410 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 01im 0010 rdst			and	#%1, %0 */",
                           op[0], op[1]);
                    printf ("  im = 0x%x,", im);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("and	#%1, %0");
#line 410 "rx-decode.opc"
                ID(and); SC(IMMex(im)); DR(rdst); F__SZ_;
              
              }
            break;
          case 0x30:
            op_semantics_34:
              {
                /** 0111 01im 0011 rdst			or	#%1, %0 */
#line 428 "rx-decode.opc"
                int im AU = op[0] & 0x03;
#line 428 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 01im 0011 rdst			or	#%1, %0 */",
                           op[0], op[1]);
                    printf ("  im = 0x%x,", im);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("or	#%1, %0");
#line 428 "rx-decode.opc"
                ID(or); SC(IMMex(im)); DR(rdst); F__SZ_;
              
              }
            break;
          default: UNSUPPORTED(); break;
        }
      break;
    case 0x75:
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
          case 0x08:
          case 0x09:
          case 0x0a:
          case 0x0b:
          case 0x0c:
          case 0x0d:
          case 0x0e:
          case 0x0f:
            goto op_semantics_31;
            break;
          case 0x10:
          case 0x11:
          case 0x12:
          case 0x13:
          case 0x14:
          case 0x15:
          case 0x16:
          case 0x17:
          case 0x18:
          case 0x19:
          case 0x1a:
          case 0x1b:
          case 0x1c:
          case 0x1d:
          case 0x1e:
          case 0x1f:
            goto op_semantics_32;
            break;
          case 0x20:
          case 0x21:
          case 0x22:
          case 0x23:
          case 0x24:
          case 0x25:
          case 0x26:
          case 0x27:
          case 0x28:
          case 0x29:
          case 0x2a:
          case 0x2b:
          case 0x2c:
          case 0x2d:
          case 0x2e:
          case 0x2f:
            goto op_semantics_33;
            break;
          case 0x30:
          case 0x31:
          case 0x32:
          case 0x33:
          case 0x34:
          case 0x35:
          case 0x36:
          case 0x37:
          case 0x38:
          case 0x39:
          case 0x3a:
          case 0x3b:
          case 0x3c:
          case 0x3d:
          case 0x3e:
          case 0x3f:
            goto op_semantics_34;
            break;
          case 0x40:
          case 0x41:
          case 0x42:
          case 0x43:
          case 0x44:
          case 0x45:
          case 0x46:
          case 0x47:
          case 0x48:
          case 0x49:
          case 0x4a:
          case 0x4b:
          case 0x4c:
          case 0x4d:
          case 0x4e:
          case 0x4f:
              {
                /** 0111 0101 0100 rdst		mov%s	#%1, %0 */
#line 282 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0101 0100 rdst		mov%s	#%1, %0 */",
                           op[0], op[1]);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("mov%s	#%1, %0");
#line 282 "rx-decode.opc"
                ID(mov); DR(rdst); SC(IMM (1)); F_____;
              
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
          case 0x58:
          case 0x59:
          case 0x5a:
          case 0x5b:
          case 0x5c:
          case 0x5d:
          case 0x5e:
          case 0x5f:
              {
                /** 0111 0101 0101 rsrc			cmp	#%2, %1 */
#line 521 "rx-decode.opc"
                int rsrc AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0101 0101 rsrc			cmp	#%2, %1 */",
                           op[0], op[1]);
                    printf ("  rsrc = 0x%x\n", rsrc);
                  }
                SYNTAX("cmp	#%2, %1");
#line 521 "rx-decode.opc"
                ID(sub); SR(rsrc); S2C(IMM(1)); F_OSZC;
              
              }
            break;
          case 0x60:
              {
                /** 0111 0101 0110 0000		int #%1 */
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 0101 0110 0000		int #%1 */",
                           op[0], op[1]);
                  }
                SYNTAX("int #%1");
#line 990 "rx-decode.opc"
                ID(int); SC(IMM(1));
              
              }
            break;
          case 0x70:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                    {
                      /** 0111 0101 0111 0000 0000 immm	mvtipl	#%1 */
#line 957 "rx-decode.opc"
                      int immm AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 0111 0101 0111 0000 0000 immm	mvtipl	#%1 */",
                                 op[0], op[1], op[2]);
                          printf ("  immm = 0x%x\n", immm);
                        }
                      SYNTAX("mvtipl	#%1");
#line 957 "rx-decode.opc"
                      ID(mvtipl); SC(immm);
                    
                    }
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          default: UNSUPPORTED(); break;
        }
      break;
    case 0x76:
        GETBYTE ();
        switch (op[1] & 0xf0)
        {
          case 0x00:
            goto op_semantics_31;
            break;
          case 0x10:
            goto op_semantics_32;
            break;
          case 0x20:
            goto op_semantics_33;
            break;
          case 0x30:
            goto op_semantics_34;
            break;
          default: UNSUPPORTED(); break;
        }
      break;
    case 0x77:
        GETBYTE ();
        switch (op[1] & 0xf0)
        {
          case 0x00:
            goto op_semantics_31;
            break;
          case 0x10:
            goto op_semantics_32;
            break;
          case 0x20:
            goto op_semantics_33;
            break;
          case 0x30:
            goto op_semantics_34;
            break;
          default: UNSUPPORTED(); break;
        }
      break;
    case 0x78:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            op_semantics_35:
              {
                /** 0111 100b ittt rdst			bset	#%1, %0 */
#line 902 "rx-decode.opc"
                int b AU = op[0] & 0x01;
#line 902 "rx-decode.opc"
                int ittt AU = (op[1] >> 4) & 0x0f;
#line 902 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 100b ittt rdst			bset	#%1, %0 */",
                           op[0], op[1]);
                    printf ("  b = 0x%x,", b);
                    printf ("  ittt = 0x%x,", ittt);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("bset	#%1, %0");
#line 902 "rx-decode.opc"
                ID(bset); BWL(LSIZE); SC(b*16+ittt); DR(rdst); F_____;
              
              
              }
            break;
        }
      break;
    case 0x79:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_35;
            break;
        }
      break;
    case 0x7a:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            op_semantics_36:
              {
                /** 0111 101b ittt rdst			bclr	#%1, %0 */
#line 914 "rx-decode.opc"
                int b AU = op[0] & 0x01;
#line 914 "rx-decode.opc"
                int ittt AU = (op[1] >> 4) & 0x0f;
#line 914 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 101b ittt rdst			bclr	#%1, %0 */",
                           op[0], op[1]);
                    printf ("  b = 0x%x,", b);
                    printf ("  ittt = 0x%x,", ittt);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("bclr	#%1, %0");
#line 914 "rx-decode.opc"
                ID(bclr); BWL(LSIZE); SC(b*16+ittt); DR(rdst); F_____;
              
              
              }
            break;
        }
      break;
    case 0x7b:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_36;
            break;
        }
      break;
    case 0x7c:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            op_semantics_37:
              {
                /** 0111 110b ittt rdst			btst	#%2, %1 */
#line 926 "rx-decode.opc"
                int b AU = op[0] & 0x01;
#line 926 "rx-decode.opc"
                int ittt AU = (op[1] >> 4) & 0x0f;
#line 926 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 110b ittt rdst			btst	#%2, %1 */",
                           op[0], op[1]);
                    printf ("  b = 0x%x,", b);
                    printf ("  ittt = 0x%x,", ittt);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("btst	#%2, %1");
#line 926 "rx-decode.opc"
                ID(btst); BWL(LSIZE); S2C(b*16+ittt); SR(rdst); F___ZC;
              
              
              }
            break;
        }
      break;
    case 0x7d:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_37;
            break;
        }
      break;
    case 0x7e:
        GETBYTE ();
        switch (op[1] & 0xf0)
        {
          case 0x00:
              {
                /** 0111 1110 0000 rdst			not	%0 */
#line 455 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1110 0000 rdst			not	%0 */",
                           op[0], op[1]);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("not	%0");
#line 455 "rx-decode.opc"
                ID(xor); DR(rdst); SR(rdst); S2C(~0); F__SZ_;
              
              }
            break;
          case 0x10:
              {
                /** 0111 1110 0001 rdst			neg	%0 */
#line 476 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1110 0001 rdst			neg	%0 */",
                           op[0], op[1]);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("neg	%0");
#line 476 "rx-decode.opc"
                ID(sub); DR(rdst); SC(0); S2R(rdst); F_OSZC;
              
              }
            break;
          case 0x20:
              {
                /** 0111 1110 0010 rdst			abs	%0 */
#line 558 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1110 0010 rdst			abs	%0 */",
                           op[0], op[1]);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("abs	%0");
#line 558 "rx-decode.opc"
                ID(abs); DR(rdst); SR(rdst); F_OSZ_;
              
              }
            break;
          case 0x30:
              {
                /** 0111 1110 0011 rdst		sat	%0 */
#line 840 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1110 0011 rdst		sat	%0 */",
                           op[0], op[1]);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("sat	%0");
#line 840 "rx-decode.opc"
                ID(sat); DR (rdst);
              
              }
            break;
          case 0x40:
              {
                /** 0111 1110 0100 rdst			rorc	%0 */
#line 700 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1110 0100 rdst			rorc	%0 */",
                           op[0], op[1]);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("rorc	%0");
#line 700 "rx-decode.opc"
                ID(rorc); DR(rdst); F__SZC;
              
              }
            break;
          case 0x50:
              {
                /** 0111 1110 0101 rdst			rolc	%0 */
#line 697 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1110 0101 rdst			rolc	%0 */",
                           op[0], op[1]);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("rolc	%0");
#line 697 "rx-decode.opc"
                ID(rolc); DR(rdst); F__SZC;
              
              }
            break;
          case 0x80:
          case 0x90:
          case 0xa0:
              {
                /** 0111 1110 10sz rsrc		push%s	%1 */
#line 371 "rx-decode.opc"
                int sz AU = (op[1] >> 4) & 0x03;
#line 371 "rx-decode.opc"
                int rsrc AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1110 10sz rsrc		push%s	%1 */",
                           op[0], op[1]);
                    printf ("  sz = 0x%x,", sz);
                    printf ("  rsrc = 0x%x\n", rsrc);
                  }
                SYNTAX("push%s	%1");
#line 371 "rx-decode.opc"
                ID(mov); BWL(sz); OP(0, RX_Operand_Predec, 0, 0); SR(rsrc); F_____;
              
              }
            break;
          case 0xb0:
              {
                /** 0111 1110 1011 rdst		pop	%0 */
#line 368 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1110 1011 rdst		pop	%0 */",
                           op[0], op[1]);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("pop	%0");
#line 368 "rx-decode.opc"
                ID(mov); OP(1, RX_Operand_Postinc, 0, 0); DR(rdst); F_____;
                
              }
            break;
          case 0xc0:
          case 0xd0:
              {
                /** 0111 1110 110 crsrc			pushc	%1 */
#line 963 "rx-decode.opc"
                int crsrc AU = op[1] & 0x1f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1110 110 crsrc			pushc	%1 */",
                           op[0], op[1]);
                    printf ("  crsrc = 0x%x\n", crsrc);
                  }
                SYNTAX("pushc	%1");
#line 963 "rx-decode.opc"
                ID(mov); OP(0, RX_Operand_Predec, 0, 0); SR(crsrc + 16);
              
              }
            break;
          case 0xe0:
          case 0xf0:
              {
                /** 0111 1110 111 crdst			popc	%0 */
#line 960 "rx-decode.opc"
                int crdst AU = op[1] & 0x1f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1110 111 crdst			popc	%0 */",
                           op[0], op[1]);
                    printf ("  crdst = 0x%x\n", crdst);
                  }
                SYNTAX("popc	%0");
#line 960 "rx-decode.opc"
                ID(mov); OP(1, RX_Operand_Postinc, 0, 0); DR(crdst + 16);
              
              }
            break;
          default: UNSUPPORTED(); break;
        }
      break;
    case 0x7f:
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
          case 0x08:
          case 0x09:
          case 0x0a:
          case 0x0b:
          case 0x0c:
          case 0x0d:
          case 0x0e:
          case 0x0f:
              {
                /** 0111 1111 0000 rsrc		jmp	%0 */
#line 750 "rx-decode.opc"
                int rsrc AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1111 0000 rsrc		jmp	%0 */",
                           op[0], op[1]);
                    printf ("  rsrc = 0x%x\n", rsrc);
                  }
                SYNTAX("jmp	%0");
#line 750 "rx-decode.opc"
                ID(branch); DR(rsrc);
              
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
          case 0x18:
          case 0x19:
          case 0x1a:
          case 0x1b:
          case 0x1c:
          case 0x1d:
          case 0x1e:
          case 0x1f:
              {
                /** 0111 1111 0001 rsrc		jsr	%0 */
#line 753 "rx-decode.opc"
                int rsrc AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1111 0001 rsrc		jsr	%0 */",
                           op[0], op[1]);
                    printf ("  rsrc = 0x%x\n", rsrc);
                  }
                SYNTAX("jsr	%0");
#line 753 "rx-decode.opc"
                ID(jsr); DR(rsrc);
              
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
          case 0x48:
          case 0x49:
          case 0x4a:
          case 0x4b:
          case 0x4c:
          case 0x4d:
          case 0x4e:
          case 0x4f:
              {
                /** 0111 1111 0100 rsrc		bra.l	%0 */
#line 746 "rx-decode.opc"
                int rsrc AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1111 0100 rsrc		bra.l	%0 */",
                           op[0], op[1]);
                    printf ("  rsrc = 0x%x\n", rsrc);
                  }
                SYNTAX("bra.l	%0");
#line 746 "rx-decode.opc"
                ID(branchrel); DR(rsrc);
              
              
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
          case 0x58:
          case 0x59:
          case 0x5a:
          case 0x5b:
          case 0x5c:
          case 0x5d:
          case 0x5e:
          case 0x5f:
              {
                /** 0111 1111 0101 rsrc		bsr.l	%0 */
#line 762 "rx-decode.opc"
                int rsrc AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1111 0101 rsrc		bsr.l	%0 */",
                           op[0], op[1]);
                    printf ("  rsrc = 0x%x\n", rsrc);
                  }
                SYNTAX("bsr.l	%0");
#line 762 "rx-decode.opc"
                ID(jsrrel); DR(rsrc);
              
              }
            break;
          case 0x80:
          case 0x81:
          case 0x82:
              {
                /** 0111 1111 1000 00sz		suntil%s */
#line 786 "rx-decode.opc"
                int sz AU = op[1] & 0x03;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1111 1000 00sz		suntil%s */",
                           op[0], op[1]);
                    printf ("  sz = 0x%x\n", sz);
                  }
                SYNTAX("suntil%s");
#line 786 "rx-decode.opc"
                ID(suntil); BWL(sz); F___ZC;
              
              }
            break;
          case 0x83:
              {
                /** 0111 1111 1000 0011		scmpu */
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1111 1000 0011		scmpu */",
                           op[0], op[1]);
                  }
                SYNTAX("scmpu");
#line 777 "rx-decode.opc"
                ID(scmpu); F___ZC;
              
              }
            break;
          case 0x84:
          case 0x85:
          case 0x86:
              {
                /** 0111 1111 1000 01sz		swhile%s */
#line 789 "rx-decode.opc"
                int sz AU = op[1] & 0x03;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1111 1000 01sz		swhile%s */",
                           op[0], op[1]);
                    printf ("  sz = 0x%x\n", sz);
                  }
                SYNTAX("swhile%s");
#line 789 "rx-decode.opc"
                ID(swhile); BWL(sz); F___ZC;
              
              }
            break;
          case 0x87:
              {
                /** 0111 1111 1000 0111		smovu */
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1111 1000 0111		smovu */",
                           op[0], op[1]);
                  }
                SYNTAX("smovu");
#line 780 "rx-decode.opc"
                ID(smovu);
              
              }
            break;
          case 0x88:
          case 0x89:
          case 0x8a:
              {
                /** 0111 1111 1000 10sz		sstr%s */
#line 795 "rx-decode.opc"
                int sz AU = op[1] & 0x03;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1111 1000 10sz		sstr%s */",
                           op[0], op[1]);
                    printf ("  sz = 0x%x\n", sz);
                  }
                SYNTAX("sstr%s");
#line 795 "rx-decode.opc"
                ID(sstr); BWL(sz);
              
              /*----------------------------------------------------------------------*/
              /* RMPA									*/
              
              }
            break;
          case 0x8b:
              {
                /** 0111 1111 1000 1011		smovb */
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1111 1000 1011		smovb */",
                           op[0], op[1]);
                  }
                SYNTAX("smovb");
#line 783 "rx-decode.opc"
                ID(smovb);
              
              }
            break;
          case 0x8c:
          case 0x8d:
          case 0x8e:
              {
                /** 0111 1111 1000 11sz		rmpa%s */
#line 801 "rx-decode.opc"
                int sz AU = op[1] & 0x03;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1111 1000 11sz		rmpa%s */",
                           op[0], op[1]);
                    printf ("  sz = 0x%x\n", sz);
                  }
                SYNTAX("rmpa%s");
#line 801 "rx-decode.opc"
                ID(rmpa); BWL(sz); F_OS__;
              
              /*----------------------------------------------------------------------*/
              /* HI/LO stuff								*/
              
              }
            break;
          case 0x8f:
              {
                /** 0111 1111 1000 1111		smovf */
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1111 1000 1111		smovf */",
                           op[0], op[1]);
                  }
                SYNTAX("smovf");
#line 792 "rx-decode.opc"
                ID(smovf);
              
              }
            break;
          case 0x93:
              {
                /** 0111 1111 1001 0011		satr */
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1111 1001 0011		satr */",
                           op[0], op[1]);
                  }
                SYNTAX("satr");
#line 843 "rx-decode.opc"
                ID(satr);
              
              /*----------------------------------------------------------------------*/
              /* FLOAT								*/
              
              }
            break;
          case 0x94:
              {
                /** 0111 1111 1001 0100		rtfi */
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1111 1001 0100		rtfi */",
                           op[0], op[1]);
                  }
                SYNTAX("rtfi");
#line 978 "rx-decode.opc"
                ID(rtfi);
              
              }
            break;
          case 0x95:
              {
                /** 0111 1111 1001 0101		rte */
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1111 1001 0101		rte */",
                           op[0], op[1]);
                  }
                SYNTAX("rte");
#line 981 "rx-decode.opc"
                ID(rte);
              
              }
            break;
          case 0x96:
              {
                /** 0111 1111 1001 0110		wait */
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1111 1001 0110		wait */",
                           op[0], op[1]);
                  }
                SYNTAX("wait");
#line 993 "rx-decode.opc"
                ID(wait);
              
              /*----------------------------------------------------------------------*/
              /* SCcnd								*/
              
              }
            break;
          case 0xa0:
          case 0xa1:
          case 0xa2:
          case 0xa3:
          case 0xa4:
          case 0xa5:
          case 0xa6:
          case 0xa7:
          case 0xa8:
          case 0xa9:
          case 0xaa:
          case 0xab:
          case 0xac:
          case 0xad:
          case 0xae:
          case 0xaf:
              {
                /** 0111 1111 1010 rdst			setpsw	%0 */
#line 954 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1111 1010 rdst			setpsw	%0 */",
                           op[0], op[1]);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("setpsw	%0");
#line 954 "rx-decode.opc"
                ID(setpsw); DF(rdst);
              
              }
            break;
          case 0xb0:
          case 0xb1:
          case 0xb2:
          case 0xb3:
          case 0xb4:
          case 0xb5:
          case 0xb6:
          case 0xb7:
          case 0xb8:
          case 0xb9:
          case 0xba:
          case 0xbb:
          case 0xbc:
          case 0xbd:
          case 0xbe:
          case 0xbf:
              {
                /** 0111 1111 1011 rdst			clrpsw	%0 */
#line 951 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 0111 1111 1011 rdst			clrpsw	%0 */",
                           op[0], op[1]);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("clrpsw	%0");
#line 951 "rx-decode.opc"
                ID(clrpsw); DF(rdst);
              
              }
            break;
          default: UNSUPPORTED(); break;
        }
      break;
    case 0x80:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            op_semantics_38:
              {
                /** 10sz 0dsp a dst b src	mov%s	%1, %0 */
#line 329 "rx-decode.opc"
                int sz AU = (op[0] >> 4) & 0x03;
#line 329 "rx-decode.opc"
                int dsp AU = op[0] & 0x07;
#line 329 "rx-decode.opc"
                int a AU = (op[1] >> 7) & 0x01;
#line 329 "rx-decode.opc"
                int dst AU = (op[1] >> 4) & 0x07;
#line 329 "rx-decode.opc"
                int b AU = (op[1] >> 3) & 0x01;
#line 329 "rx-decode.opc"
                int src AU = op[1] & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 10sz 0dsp a dst b src	mov%s	%1, %0 */",
                           op[0], op[1]);
                    printf ("  sz = 0x%x,", sz);
                    printf ("  dsp = 0x%x,", dsp);
                    printf ("  a = 0x%x,", a);
                    printf ("  dst = 0x%x,", dst);
                    printf ("  b = 0x%x,", b);
                    printf ("  src = 0x%x\n", src);
                  }
                SYNTAX("mov%s	%1, %0");
#line 329 "rx-decode.opc"
                ID(mov); sBWL(sz); DIs(dst, dsp*4+a*2+b, sz); SR(src); F_____;
              
              }
            break;
        }
      break;
    case 0x81:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_38;
            break;
        }
      break;
    case 0x82:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_38;
            break;
        }
      break;
    case 0x83:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_38;
            break;
        }
      break;
    case 0x84:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_38;
            break;
        }
      break;
    case 0x85:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_38;
            break;
        }
      break;
    case 0x86:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_38;
            break;
        }
      break;
    case 0x87:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_38;
            break;
        }
      break;
    case 0x88:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            op_semantics_39:
              {
                /** 10sz 1dsp a src b dst	mov%s	%1, %0 */
#line 326 "rx-decode.opc"
                int sz AU = (op[0] >> 4) & 0x03;
#line 326 "rx-decode.opc"
                int dsp AU = op[0] & 0x07;
#line 326 "rx-decode.opc"
                int a AU = (op[1] >> 7) & 0x01;
#line 326 "rx-decode.opc"
                int src AU = (op[1] >> 4) & 0x07;
#line 326 "rx-decode.opc"
                int b AU = (op[1] >> 3) & 0x01;
#line 326 "rx-decode.opc"
                int dst AU = op[1] & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 10sz 1dsp a src b dst	mov%s	%1, %0 */",
                           op[0], op[1]);
                    printf ("  sz = 0x%x,", sz);
                    printf ("  dsp = 0x%x,", dsp);
                    printf ("  a = 0x%x,", a);
                    printf ("  src = 0x%x,", src);
                    printf ("  b = 0x%x,", b);
                    printf ("  dst = 0x%x\n", dst);
                  }
                SYNTAX("mov%s	%1, %0");
#line 326 "rx-decode.opc"
                ID(mov); sBWL(sz); DR(dst); SIs(src, dsp*4+a*2+b, sz); F_____;
              
              }
            break;
        }
      break;
    case 0x89:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_39;
            break;
        }
      break;
    case 0x8a:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_39;
            break;
        }
      break;
    case 0x8b:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_39;
            break;
        }
      break;
    case 0x8c:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_39;
            break;
        }
      break;
    case 0x8d:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_39;
            break;
        }
      break;
    case 0x8e:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_39;
            break;
        }
      break;
    case 0x8f:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_39;
            break;
        }
      break;
    case 0x90:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_38;
            break;
        }
      break;
    case 0x91:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_38;
            break;
        }
      break;
    case 0x92:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_38;
            break;
        }
      break;
    case 0x93:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_38;
            break;
        }
      break;
    case 0x94:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_38;
            break;
        }
      break;
    case 0x95:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_38;
            break;
        }
      break;
    case 0x96:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_38;
            break;
        }
      break;
    case 0x97:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_38;
            break;
        }
      break;
    case 0x98:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_39;
            break;
        }
      break;
    case 0x99:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_39;
            break;
        }
      break;
    case 0x9a:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_39;
            break;
        }
      break;
    case 0x9b:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_39;
            break;
        }
      break;
    case 0x9c:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_39;
            break;
        }
      break;
    case 0x9d:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_39;
            break;
        }
      break;
    case 0x9e:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_39;
            break;
        }
      break;
    case 0x9f:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_39;
            break;
        }
      break;
    case 0xa0:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_38;
            break;
        }
      break;
    case 0xa1:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_38;
            break;
        }
      break;
    case 0xa2:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_38;
            break;
        }
      break;
    case 0xa3:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_38;
            break;
        }
      break;
    case 0xa4:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_38;
            break;
        }
      break;
    case 0xa5:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_38;
            break;
        }
      break;
    case 0xa6:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_38;
            break;
        }
      break;
    case 0xa7:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_38;
            break;
        }
      break;
    case 0xa8:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_39;
            break;
        }
      break;
    case 0xa9:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_39;
            break;
        }
      break;
    case 0xaa:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_39;
            break;
        }
      break;
    case 0xab:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_39;
            break;
        }
      break;
    case 0xac:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_39;
            break;
        }
      break;
    case 0xad:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_39;
            break;
        }
      break;
    case 0xae:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_39;
            break;
        }
      break;
    case 0xaf:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_39;
            break;
        }
      break;
    case 0xb0:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            op_semantics_40:
              {
                /** 1011 w dsp a src b dst	movu%s	%1, %0 */
#line 349 "rx-decode.opc"
                int w AU = (op[0] >> 3) & 0x01;
#line 349 "rx-decode.opc"
                int dsp AU = op[0] & 0x07;
#line 349 "rx-decode.opc"
                int a AU = (op[1] >> 7) & 0x01;
#line 349 "rx-decode.opc"
                int src AU = (op[1] >> 4) & 0x07;
#line 349 "rx-decode.opc"
                int b AU = (op[1] >> 3) & 0x01;
#line 349 "rx-decode.opc"
                int dst AU = op[1] & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 1011 w dsp a src b dst	movu%s	%1, %0 */",
                           op[0], op[1]);
                    printf ("  w = 0x%x,", w);
                    printf ("  dsp = 0x%x,", dsp);
                    printf ("  a = 0x%x,", a);
                    printf ("  src = 0x%x,", src);
                    printf ("  b = 0x%x,", b);
                    printf ("  dst = 0x%x\n", dst);
                  }
                SYNTAX("movu%s	%1, %0");
#line 349 "rx-decode.opc"
                ID(mov); uBWL(w); DR(dst); SIs(src, dsp*4+a*2+b, w); F_____;
              
              }
            break;
        }
      break;
    case 0xb1:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_40;
            break;
        }
      break;
    case 0xb2:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_40;
            break;
        }
      break;
    case 0xb3:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_40;
            break;
        }
      break;
    case 0xb4:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_40;
            break;
        }
      break;
    case 0xb5:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_40;
            break;
        }
      break;
    case 0xb6:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_40;
            break;
        }
      break;
    case 0xb7:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_40;
            break;
        }
      break;
    case 0xb8:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_40;
            break;
        }
      break;
    case 0xb9:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_40;
            break;
        }
      break;
    case 0xba:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_40;
            break;
        }
      break;
    case 0xbb:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_40;
            break;
        }
      break;
    case 0xbc:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_40;
            break;
        }
      break;
    case 0xbd:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_40;
            break;
        }
      break;
    case 0xbe:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_40;
            break;
        }
      break;
    case 0xbf:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_40;
            break;
        }
      break;
    case 0xc0:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            op_semantics_41:
              {
                /** 11sz sd ss rsrc rdst	mov%s	%1, %0 */
#line 307 "rx-decode.opc"
                int sz AU = (op[0] >> 4) & 0x03;
#line 307 "rx-decode.opc"
                int sd AU = (op[0] >> 2) & 0x03;
#line 307 "rx-decode.opc"
                int ss AU = op[0] & 0x03;
#line 307 "rx-decode.opc"
                int rsrc AU = (op[1] >> 4) & 0x0f;
#line 307 "rx-decode.opc"
                int rdst AU = op[1] & 0x0f;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 11sz sd ss rsrc rdst	mov%s	%1, %0 */",
                           op[0], op[1]);
                    printf ("  sz = 0x%x,", sz);
                    printf ("  sd = 0x%x,", sd);
                    printf ("  ss = 0x%x,", ss);
                    printf ("  rsrc = 0x%x,", rsrc);
                    printf ("  rdst = 0x%x\n", rdst);
                  }
                SYNTAX("mov%s	%1, %0");
#line 307 "rx-decode.opc"
                if (sd == 3 && ss == 3 && sz == 2 && rsrc == 0 && rdst == 0)
                  {
                    ID(nop2);
                    rx->syntax = "nop";
                  }
                else
                  {
                    ID(mov); sBWL(sz); F_____;
                    if ((ss == 3) && (sd != 3))
              	{
              	  SD(ss, rdst, sz); DD(sd, rsrc, sz);
              	}
                    else
              	{
              	  SD(ss, rsrc, sz); DD(sd, rdst, sz);
              	}
                  }
              
              }
            break;
        }
      break;
    case 0xc1:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xc2:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xc3:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xc4:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xc5:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xc6:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xc7:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xc8:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xc9:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xca:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xcb:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xcc:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xcd:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xce:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xcf:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xd0:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xd1:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xd2:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xd3:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xd4:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xd5:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xd6:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xd7:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xd8:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xd9:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xda:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xdb:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xdc:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xdd:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xde:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xdf:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xe0:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xe1:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xe2:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xe3:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xe4:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xe5:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xe6:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xe7:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xe8:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xe9:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xea:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xeb:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xec:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xed:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xee:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xef:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_41;
            break;
        }
      break;
    case 0xf0:
        GETBYTE ();
        switch (op[1] & 0x08)
        {
          case 0x00:
            op_semantics_42:
              {
                /** 1111 00sd rdst 0bit			bset	#%1, %0%S0 */
#line 894 "rx-decode.opc"
                int sd AU = op[0] & 0x03;
#line 894 "rx-decode.opc"
                int rdst AU = (op[1] >> 4) & 0x0f;
#line 894 "rx-decode.opc"
                int bit AU = op[1] & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 1111 00sd rdst 0bit			bset	#%1, %0%S0 */",
                           op[0], op[1]);
                    printf ("  sd = 0x%x,", sd);
                    printf ("  rdst = 0x%x,", rdst);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("bset	#%1, %0%S0");
#line 894 "rx-decode.opc"
                ID(bset); BWL(BSIZE); SC(bit); DD(sd, rdst, BSIZE); F_____;
              
              }
            break;
          case 0x08:
            op_semantics_43:
              {
                /** 1111 00sd rdst 1bit			bclr	#%1, %0%S0 */
#line 906 "rx-decode.opc"
                int sd AU = op[0] & 0x03;
#line 906 "rx-decode.opc"
                int rdst AU = (op[1] >> 4) & 0x0f;
#line 906 "rx-decode.opc"
                int bit AU = op[1] & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 1111 00sd rdst 1bit			bclr	#%1, %0%S0 */",
                           op[0], op[1]);
                    printf ("  sd = 0x%x,", sd);
                    printf ("  rdst = 0x%x,", rdst);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("bclr	#%1, %0%S0");
#line 906 "rx-decode.opc"
                ID(bclr); BWL(BSIZE); SC(bit); DD(sd, rdst, BSIZE); F_____;
              
              }
            break;
        }
      break;
    case 0xf1:
        GETBYTE ();
        switch (op[1] & 0x08)
        {
          case 0x00:
            goto op_semantics_42;
            break;
          case 0x08:
            goto op_semantics_43;
            break;
        }
      break;
    case 0xf2:
        GETBYTE ();
        switch (op[1] & 0x08)
        {
          case 0x00:
            goto op_semantics_42;
            break;
          case 0x08:
            goto op_semantics_43;
            break;
        }
      break;
    case 0xf3:
        GETBYTE ();
        switch (op[1] & 0x08)
        {
          case 0x00:
            goto op_semantics_42;
            break;
          case 0x08:
            goto op_semantics_43;
            break;
        }
      break;
    case 0xf4:
        GETBYTE ();
        switch (op[1] & 0x0c)
        {
          case 0x00:
          case 0x04:
            op_semantics_44:
              {
                /** 1111 01sd rdst 0bit			btst	#%2, %1%S1 */
#line 918 "rx-decode.opc"
                int sd AU = op[0] & 0x03;
#line 918 "rx-decode.opc"
                int rdst AU = (op[1] >> 4) & 0x0f;
#line 918 "rx-decode.opc"
                int bit AU = op[1] & 0x07;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 1111 01sd rdst 0bit			btst	#%2, %1%S1 */",
                           op[0], op[1]);
                    printf ("  sd = 0x%x,", sd);
                    printf ("  rdst = 0x%x,", rdst);
                    printf ("  bit = 0x%x\n", bit);
                  }
                SYNTAX("btst	#%2, %1%S1");
#line 918 "rx-decode.opc"
                ID(btst); BWL(BSIZE); S2C(bit); SD(sd, rdst, BSIZE); F___ZC;
              
              }
            break;
          case 0x08:
            op_semantics_45:
              {
                /** 1111 01ss rsrc 10sz		push%s	%1 */
#line 374 "rx-decode.opc"
                int ss AU = op[0] & 0x03;
#line 374 "rx-decode.opc"
                int rsrc AU = (op[1] >> 4) & 0x0f;
#line 374 "rx-decode.opc"
                int sz AU = op[1] & 0x03;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 1111 01ss rsrc 10sz		push%s	%1 */",
                           op[0], op[1]);
                    printf ("  ss = 0x%x,", ss);
                    printf ("  rsrc = 0x%x,", rsrc);
                    printf ("  sz = 0x%x\n", sz);
                  }
                SYNTAX("push%s	%1");
#line 374 "rx-decode.opc"
                ID(mov); BWL(sz); OP(0, RX_Operand_Predec, 0, 0); SD(ss, rsrc, sz); F_____;
              
              /*----------------------------------------------------------------------*/
              /* XCHG									*/
              
              }
            break;
          default: UNSUPPORTED(); break;
        }
      break;
    case 0xf5:
        GETBYTE ();
        switch (op[1] & 0x0c)
        {
          case 0x00:
          case 0x04:
            goto op_semantics_44;
            break;
          case 0x08:
            goto op_semantics_45;
            break;
          default: UNSUPPORTED(); break;
        }
      break;
    case 0xf6:
        GETBYTE ();
        switch (op[1] & 0x0c)
        {
          case 0x00:
          case 0x04:
            goto op_semantics_44;
            break;
          case 0x08:
            goto op_semantics_45;
            break;
          default: UNSUPPORTED(); break;
        }
      break;
    case 0xf7:
        GETBYTE ();
        switch (op[1] & 0x0c)
        {
          case 0x00:
          case 0x04:
            goto op_semantics_44;
            break;
          case 0x08:
            goto op_semantics_45;
            break;
          default: UNSUPPORTED(); break;
        }
      break;
    case 0xf8:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            op_semantics_46:
              {
                /** 1111 10sd rdst im sz	mov%s	#%1, %0 */
#line 285 "rx-decode.opc"
                int sd AU = op[0] & 0x03;
#line 285 "rx-decode.opc"
                int rdst AU = (op[1] >> 4) & 0x0f;
#line 285 "rx-decode.opc"
                int im AU = (op[1] >> 2) & 0x03;
#line 285 "rx-decode.opc"
                int sz AU = op[1] & 0x03;
                if (trace)
                  {
                    printf ("\033[33m%s\033[0m  %02x %02x\n",
                           "/** 1111 10sd rdst im sz	mov%s	#%1, %0 */",
                           op[0], op[1]);
                    printf ("  sd = 0x%x,", sd);
                    printf ("  rdst = 0x%x,", rdst);
                    printf ("  im = 0x%x,", im);
                    printf ("  sz = 0x%x\n", sz);
                  }
                SYNTAX("mov%s	#%1, %0");
#line 285 "rx-decode.opc"
                ID(mov); DD(sd, rdst, sz);
                if ((im == 1 && sz == 0)
                    || (im == 2 && sz == 1)
                    || (im == 0 && sz == 2))
                  {
                    BWL (sz);
                    SC(IMM(im));
                  }
                else
                  {
                    sBWL (sz);
                    SC(IMMex(im));
                  }
                 F_____;
              
              }
            break;
        }
      break;
    case 0xf9:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_46;
            break;
        }
      break;
    case 0xfa:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_46;
            break;
        }
      break;
    case 0xfb:
        GETBYTE ();
        switch (op[1] & 0x00)
        {
          case 0x00:
            goto op_semantics_46;
            break;
        }
      break;
    case 0xfc:
        GETBYTE ();
        switch (op[1] & 0xff)
        {
          case 0x03:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                    {
                      /** 1111 1100 0000 0011 rsrc rdst	sbb	%1, %0 */
#line 548 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 548 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 0000 0011 rsrc rdst	sbb	%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("sbb	%1, %0");
#line 548 "rx-decode.opc"
                      ID(sbb); SR (rsrc); DR(rdst); F_OSZC;
                    
                      /* FIXME: only supports .L */
                    }
                  break;
              }
            break;
          case 0x07:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                    {
                      /** 1111 1100 0000 0111 rsrc rdst	neg	%2, %0 */
#line 479 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 479 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 0000 0111 rsrc rdst	neg	%2, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("neg	%2, %0");
#line 479 "rx-decode.opc"
                      ID(sub); DR(rdst); SC(0); S2R(rsrc); F_OSZC;
                    
                    /*----------------------------------------------------------------------*/
                    /* ADC									*/
                    
                    }
                  break;
              }
            break;
          case 0x0b:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                    {
                      /** 1111 1100 0000 1011 rsrc rdst	adc	%1, %0 */
#line 488 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 488 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 0000 1011 rsrc rdst	adc	%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("adc	%1, %0");
#line 488 "rx-decode.opc"
                      ID(adc); SR(rsrc); DR(rdst); F_OSZC;
                    
                    }
                  break;
              }
            break;
          case 0x0f:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                    {
                      /** 1111 1100 0000 1111 rsrc rdst	abs	%1, %0 */
#line 561 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 561 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 0000 1111 rsrc rdst	abs	%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("abs	%1, %0");
#line 561 "rx-decode.opc"
                      ID(abs); DR(rdst); SR(rsrc); F_OSZ_;
                    
                    /*----------------------------------------------------------------------*/
                    /* MAX									*/
                    
                    }
                  break;
              }
            break;
          case 0x10:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_47:
                    {
                      /** 1111 1100 0001 00ss rsrc rdst	max	%1%S1, %0 */
#line 570 "rx-decode.opc"
                      int ss AU = op[1] & 0x03;
#line 570 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 570 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 0001 00ss rsrc rdst	max	%1%S1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  ss = 0x%x,", ss);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("max	%1%S1, %0");
#line 570 "rx-decode.opc"
                      if (ss == 3 && rsrc == 0 && rdst == 0)
                        {
                          ID(nop3);
                          rx->syntax = "nop";
                        }
                      else
                        {
                          ID(max); SP(ss, rsrc); DR(rdst);
                        }
                    
                    }
                  break;
              }
            break;
          case 0x11:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_47;
                  break;
              }
            break;
          case 0x12:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_47;
                  break;
              }
            break;
          case 0x13:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_47;
                  break;
              }
            break;
          case 0x14:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_48:
                    {
                      /** 1111 1100 0001 01ss rsrc rdst	min	%1%S1, %0 */
#line 590 "rx-decode.opc"
                      int ss AU = op[1] & 0x03;
#line 590 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 590 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 0001 01ss rsrc rdst	min	%1%S1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  ss = 0x%x,", ss);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("min	%1%S1, %0");
#line 590 "rx-decode.opc"
                      ID(min); SP(ss, rsrc); DR(rdst);
                    
                    }
                  break;
              }
            break;
          case 0x15:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_48;
                  break;
              }
            break;
          case 0x16:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_48;
                  break;
              }
            break;
          case 0x17:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_48;
                  break;
              }
            break;
          case 0x18:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_49:
                    {
                      /** 1111 1100 0001 10ss rsrc rdst	emul	%1%S1, %0 */
#line 620 "rx-decode.opc"
                      int ss AU = op[1] & 0x03;
#line 620 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 620 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 0001 10ss rsrc rdst	emul	%1%S1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  ss = 0x%x,", ss);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("emul	%1%S1, %0");
#line 620 "rx-decode.opc"
                      ID(emul); SP(ss, rsrc); DR(rdst);
                    
                    }
                  break;
              }
            break;
          case 0x19:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_49;
                  break;
              }
            break;
          case 0x1a:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_49;
                  break;
              }
            break;
          case 0x1b:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_49;
                  break;
              }
            break;
          case 0x1c:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_50:
                    {
                      /** 1111 1100 0001 11ss rsrc rdst	emulu	%1%S1, %0 */
#line 632 "rx-decode.opc"
                      int ss AU = op[1] & 0x03;
#line 632 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 632 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 0001 11ss rsrc rdst	emulu	%1%S1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  ss = 0x%x,", ss);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("emulu	%1%S1, %0");
#line 632 "rx-decode.opc"
                      ID(emulu); SP(ss, rsrc); DR(rdst);
                    
                    }
                  break;
              }
            break;
          case 0x1d:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_50;
                  break;
              }
            break;
          case 0x1e:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_50;
                  break;
              }
            break;
          case 0x1f:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_50;
                  break;
              }
            break;
          case 0x20:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_51:
                    {
                      /** 1111 1100 0010 00ss rsrc rdst	div	%1%S1, %0 */
#line 644 "rx-decode.opc"
                      int ss AU = op[1] & 0x03;
#line 644 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 644 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 0010 00ss rsrc rdst	div	%1%S1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  ss = 0x%x,", ss);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("div	%1%S1, %0");
#line 644 "rx-decode.opc"
                      ID(div); SP(ss, rsrc); DR(rdst); F_O___;
                    
                    }
                  break;
              }
            break;
          case 0x21:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_51;
                  break;
              }
            break;
          case 0x22:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_51;
                  break;
              }
            break;
          case 0x23:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_51;
                  break;
              }
            break;
          case 0x24:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_52:
                    {
                      /** 1111 1100 0010 01ss rsrc rdst	divu	%1%S1, %0 */
#line 656 "rx-decode.opc"
                      int ss AU = op[1] & 0x03;
#line 656 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 656 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 0010 01ss rsrc rdst	divu	%1%S1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  ss = 0x%x,", ss);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("divu	%1%S1, %0");
#line 656 "rx-decode.opc"
                      ID(divu); SP(ss, rsrc); DR(rdst); F_O___;
                    
                    }
                  break;
              }
            break;
          case 0x25:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_52;
                  break;
              }
            break;
          case 0x26:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_52;
                  break;
              }
            break;
          case 0x27:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_52;
                  break;
              }
            break;
          case 0x30:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_53:
                    {
                      /** 1111 1100 0011 00ss rsrc rdst	tst	%1%S1, %2 */
#line 467 "rx-decode.opc"
                      int ss AU = op[1] & 0x03;
#line 467 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 467 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 0011 00ss rsrc rdst	tst	%1%S1, %2 */",
                                 op[0], op[1], op[2]);
                          printf ("  ss = 0x%x,", ss);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("tst	%1%S1, %2");
#line 467 "rx-decode.opc"
                      ID(and); SP(ss, rsrc); S2R(rdst); F__SZ_;
                    
                    }
                  break;
              }
            break;
          case 0x31:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_53;
                  break;
              }
            break;
          case 0x32:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_53;
                  break;
              }
            break;
          case 0x33:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_53;
                  break;
              }
            break;
          case 0x34:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_54:
                    {
                      /** 1111 1100 0011 01ss rsrc rdst	xor	%1%S1, %0 */
#line 446 "rx-decode.opc"
                      int ss AU = op[1] & 0x03;
#line 446 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 446 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 0011 01ss rsrc rdst	xor	%1%S1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  ss = 0x%x,", ss);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("xor	%1%S1, %0");
#line 446 "rx-decode.opc"
                      ID(xor); SP(ss, rsrc); DR(rdst); F__SZ_;
                    
                    }
                  break;
              }
            break;
          case 0x35:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_54;
                  break;
              }
            break;
          case 0x36:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_54;
                  break;
              }
            break;
          case 0x37:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_54;
                  break;
              }
            break;
          case 0x3b:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                    {
                      /** 1111 1100 0011 1011 rsrc rdst	not	%1, %0 */
#line 458 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 458 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 0011 1011 rsrc rdst	not	%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("not	%1, %0");
#line 458 "rx-decode.opc"
                      ID(xor); DR(rdst); SR(rsrc); S2C(~0); F__SZ_;
                    
                    /*----------------------------------------------------------------------*/
                    /* TST									*/
                    
                    }
                  break;
              }
            break;
          case 0x40:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_55:
                    {
                      /** 1111 1100 0100 00ss rsrc rdst	xchg	%1%S1, %0 */
#line 380 "rx-decode.opc"
                      int ss AU = op[1] & 0x03;
#line 380 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 380 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 0100 00ss rsrc rdst	xchg	%1%S1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  ss = 0x%x,", ss);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("xchg	%1%S1, %0");
#line 380 "rx-decode.opc"
                      ID(xchg); DR(rdst); SP(ss, rsrc);
                    
                    }
                  break;
              }
            break;
          case 0x41:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_55;
                  break;
              }
            break;
          case 0x42:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_55;
                  break;
              }
            break;
          case 0x43:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_55;
                  break;
              }
            break;
          case 0x44:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_56:
                    {
                      /** 1111 1100 0100 01sd rsrc rdst	itof	%1%S1, %0 */
#line 885 "rx-decode.opc"
                      int sd AU = op[1] & 0x03;
#line 885 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 885 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 0100 01sd rsrc rdst	itof	%1%S1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  sd = 0x%x,", sd);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("itof	%1%S1, %0");
#line 885 "rx-decode.opc"
                      ID(itof); DR (rdst); SP(sd, rsrc); F__SZ_;
                    
                    }
                  break;
              }
            break;
          case 0x45:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_56;
                  break;
              }
            break;
          case 0x46:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_56;
                  break;
              }
            break;
          case 0x47:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_56;
                  break;
              }
            break;
          case 0x60:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_57:
                    {
                      /** 1111 1100 0110 00sd rdst rsrc	bset	%1, %0%S0 */
#line 897 "rx-decode.opc"
                      int sd AU = op[1] & 0x03;
#line 897 "rx-decode.opc"
                      int rdst AU = (op[2] >> 4) & 0x0f;
#line 897 "rx-decode.opc"
                      int rsrc AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 0110 00sd rdst rsrc	bset	%1, %0%S0 */",
                                 op[0], op[1], op[2]);
                          printf ("  sd = 0x%x,", sd);
                          printf ("  rdst = 0x%x,", rdst);
                          printf ("  rsrc = 0x%x\n", rsrc);
                        }
                      SYNTAX("bset	%1, %0%S0");
#line 897 "rx-decode.opc"
                      ID(bset); BWL(BSIZE); SR(rsrc); DD(sd, rdst, BSIZE); F_____;
                      if (sd == 3) /* bset reg,reg */
                        BWL(LSIZE);
                    
                    }
                  break;
              }
            break;
          case 0x61:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_57;
                  break;
              }
            break;
          case 0x62:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_57;
                  break;
              }
            break;
          case 0x63:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_57;
                  break;
              }
            break;
          case 0x64:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_58:
                    {
                      /** 1111 1100 0110 01sd rdst rsrc	bclr	%1, %0%S0 */
#line 909 "rx-decode.opc"
                      int sd AU = op[1] & 0x03;
#line 909 "rx-decode.opc"
                      int rdst AU = (op[2] >> 4) & 0x0f;
#line 909 "rx-decode.opc"
                      int rsrc AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 0110 01sd rdst rsrc	bclr	%1, %0%S0 */",
                                 op[0], op[1], op[2]);
                          printf ("  sd = 0x%x,", sd);
                          printf ("  rdst = 0x%x,", rdst);
                          printf ("  rsrc = 0x%x\n", rsrc);
                        }
                      SYNTAX("bclr	%1, %0%S0");
#line 909 "rx-decode.opc"
                      ID(bclr); BWL(BSIZE); SR(rsrc); DD(sd, rdst, BSIZE); F_____;
                      if (sd == 3) /* bset reg,reg */
                        BWL(LSIZE);
                    
                    }
                  break;
              }
            break;
          case 0x65:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_58;
                  break;
              }
            break;
          case 0x66:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_58;
                  break;
              }
            break;
          case 0x67:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_58;
                  break;
              }
            break;
          case 0x68:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_59:
                    {
                      /** 1111 1100 0110 10sd rdst rsrc	btst	%2, %1%S1 */
#line 921 "rx-decode.opc"
                      int sd AU = op[1] & 0x03;
#line 921 "rx-decode.opc"
                      int rdst AU = (op[2] >> 4) & 0x0f;
#line 921 "rx-decode.opc"
                      int rsrc AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 0110 10sd rdst rsrc	btst	%2, %1%S1 */",
                                 op[0], op[1], op[2]);
                          printf ("  sd = 0x%x,", sd);
                          printf ("  rdst = 0x%x,", rdst);
                          printf ("  rsrc = 0x%x\n", rsrc);
                        }
                      SYNTAX("btst	%2, %1%S1");
#line 921 "rx-decode.opc"
                      ID(btst); BWL(BSIZE); S2R(rsrc); SD(sd, rdst, BSIZE); F___ZC;
                      if (sd == 3) /* bset reg,reg */
                        BWL(LSIZE);
                    
                    }
                  break;
              }
            break;
          case 0x69:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_59;
                  break;
              }
            break;
          case 0x6a:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_59;
                  break;
              }
            break;
          case 0x6b:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_59;
                  break;
              }
            break;
          case 0x6c:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_60:
                    {
                      /** 1111 1100 0110 11sd rdst rsrc	bnot	%1, %0%S0 */
#line 933 "rx-decode.opc"
                      int sd AU = op[1] & 0x03;
#line 933 "rx-decode.opc"
                      int rdst AU = (op[2] >> 4) & 0x0f;
#line 933 "rx-decode.opc"
                      int rsrc AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 0110 11sd rdst rsrc	bnot	%1, %0%S0 */",
                                 op[0], op[1], op[2]);
                          printf ("  sd = 0x%x,", sd);
                          printf ("  rdst = 0x%x,", rdst);
                          printf ("  rsrc = 0x%x\n", rsrc);
                        }
                      SYNTAX("bnot	%1, %0%S0");
#line 933 "rx-decode.opc"
                      ID(bnot); BWL(BSIZE); SR(rsrc); DD(sd, rdst, BSIZE);
                      if (sd == 3) /* bset reg,reg */
                        BWL(LSIZE);
                    
                    }
                  break;
              }
            break;
          case 0x6d:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_60;
                  break;
              }
            break;
          case 0x6e:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_60;
                  break;
              }
            break;
          case 0x6f:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_60;
                  break;
              }
            break;
          case 0x80:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_61:
                    {
                      /** 1111 1100 1000 00sd rsrc rdst	fsub	%1%S1, %0 */
#line 864 "rx-decode.opc"
                      int sd AU = op[1] & 0x03;
#line 864 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 864 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 1000 00sd rsrc rdst	fsub	%1%S1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  sd = 0x%x,", sd);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("fsub	%1%S1, %0");
#line 864 "rx-decode.opc"
                      ID(fsub); DR(rdst); SD(sd, rsrc, LSIZE); F__SZ_;
                    
                    }
                  break;
              }
            break;
          case 0x81:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_61;
                  break;
              }
            break;
          case 0x82:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_61;
                  break;
              }
            break;
          case 0x83:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_61;
                  break;
              }
            break;
          case 0x84:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_62:
                    {
                      /** 1111 1100 1000 01sd rsrc rdst	fcmp	%1%S1, %0 */
#line 858 "rx-decode.opc"
                      int sd AU = op[1] & 0x03;
#line 858 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 858 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 1000 01sd rsrc rdst	fcmp	%1%S1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  sd = 0x%x,", sd);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("fcmp	%1%S1, %0");
#line 858 "rx-decode.opc"
                      ID(fcmp); DR(rdst); SD(sd, rsrc, LSIZE); F_OSZ_;
                    
                    }
                  break;
              }
            break;
          case 0x85:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_62;
                  break;
              }
            break;
          case 0x86:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_62;
                  break;
              }
            break;
          case 0x87:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_62;
                  break;
              }
            break;
          case 0x88:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_63:
                    {
                      /** 1111 1100 1000 10sd rsrc rdst	fadd	%1%S1, %0 */
#line 852 "rx-decode.opc"
                      int sd AU = op[1] & 0x03;
#line 852 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 852 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 1000 10sd rsrc rdst	fadd	%1%S1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  sd = 0x%x,", sd);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("fadd	%1%S1, %0");
#line 852 "rx-decode.opc"
                      ID(fadd); DR(rdst); SD(sd, rsrc, LSIZE); F__SZ_;
                    
                    }
                  break;
              }
            break;
          case 0x89:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_63;
                  break;
              }
            break;
          case 0x8a:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_63;
                  break;
              }
            break;
          case 0x8b:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_63;
                  break;
              }
            break;
          case 0x8c:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_64:
                    {
                      /** 1111 1100 1000 11sd rsrc rdst	fmul	%1%S1, %0 */
#line 873 "rx-decode.opc"
                      int sd AU = op[1] & 0x03;
#line 873 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 873 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 1000 11sd rsrc rdst	fmul	%1%S1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  sd = 0x%x,", sd);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("fmul	%1%S1, %0");
#line 873 "rx-decode.opc"
                      ID(fmul); DR(rdst); SD(sd, rsrc, LSIZE); F__SZ_;
                    
                    }
                  break;
              }
            break;
          case 0x8d:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_64;
                  break;
              }
            break;
          case 0x8e:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_64;
                  break;
              }
            break;
          case 0x8f:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_64;
                  break;
              }
            break;
          case 0x90:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_65:
                    {
                      /** 1111 1100 1001 00sd rsrc rdst	fdiv	%1%S1, %0 */
#line 879 "rx-decode.opc"
                      int sd AU = op[1] & 0x03;
#line 879 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 879 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 1001 00sd rsrc rdst	fdiv	%1%S1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  sd = 0x%x,", sd);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("fdiv	%1%S1, %0");
#line 879 "rx-decode.opc"
                      ID(fdiv); DR(rdst); SD(sd, rsrc, LSIZE); F__SZ_;
                    
                    }
                  break;
              }
            break;
          case 0x91:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_65;
                  break;
              }
            break;
          case 0x92:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_65;
                  break;
              }
            break;
          case 0x93:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_65;
                  break;
              }
            break;
          case 0x94:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_66:
                    {
                      /** 1111 1100 1001 01sd rsrc rdst	ftoi	%1%S1, %0 */
#line 867 "rx-decode.opc"
                      int sd AU = op[1] & 0x03;
#line 867 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 867 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 1001 01sd rsrc rdst	ftoi	%1%S1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  sd = 0x%x,", sd);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("ftoi	%1%S1, %0");
#line 867 "rx-decode.opc"
                      ID(ftoi); DR(rdst); SD(sd, rsrc, LSIZE); F__SZ_;
                    
                    }
                  break;
              }
            break;
          case 0x95:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_66;
                  break;
              }
            break;
          case 0x96:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_66;
                  break;
              }
            break;
          case 0x97:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_66;
                  break;
              }
            break;
          case 0x98:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_67:
                    {
                      /** 1111 1100 1001 10sd rsrc rdst	round	%1%S1, %0 */
#line 882 "rx-decode.opc"
                      int sd AU = op[1] & 0x03;
#line 882 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 882 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 1001 10sd rsrc rdst	round	%1%S1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  sd = 0x%x,", sd);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("round	%1%S1, %0");
#line 882 "rx-decode.opc"
                      ID(round); DR(rdst); SD(sd, rsrc, LSIZE); F__SZ_;
                    
                    }
                  break;
              }
            break;
          case 0x99:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_67;
                  break;
              }
            break;
          case 0x9a:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_67;
                  break;
              }
            break;
          case 0x9b:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_67;
                  break;
              }
            break;
          case 0xd0:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_68:
                    {
                      /** 1111 1100 1101 sz sd rdst cond	sc%1%s	%0 */
#line 999 "rx-decode.opc"
                      int sz AU = (op[1] >> 2) & 0x03;
#line 999 "rx-decode.opc"
                      int sd AU = op[1] & 0x03;
#line 999 "rx-decode.opc"
                      int rdst AU = (op[2] >> 4) & 0x0f;
#line 999 "rx-decode.opc"
                      int cond AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 1101 sz sd rdst cond	sc%1%s	%0 */",
                                 op[0], op[1], op[2]);
                          printf ("  sz = 0x%x,", sz);
                          printf ("  sd = 0x%x,", sd);
                          printf ("  rdst = 0x%x,", rdst);
                          printf ("  cond = 0x%x\n", cond);
                        }
                      SYNTAX("sc%1%s	%0");
#line 999 "rx-decode.opc"
                      ID(sccnd); BWL(sz); DD (sd, rdst, sz); Scc(cond);
                    
                    }
                  break;
              }
            break;
          case 0xd1:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_68;
                  break;
              }
            break;
          case 0xd2:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_68;
                  break;
              }
            break;
          case 0xd3:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_68;
                  break;
              }
            break;
          case 0xd4:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_68;
                  break;
              }
            break;
          case 0xd5:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_68;
                  break;
              }
            break;
          case 0xd6:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_68;
                  break;
              }
            break;
          case 0xd7:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_68;
                  break;
              }
            break;
          case 0xd8:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_68;
                  break;
              }
            break;
          case 0xd9:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_68;
                  break;
              }
            break;
          case 0xda:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_68;
                  break;
              }
            break;
          case 0xdb:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_68;
                  break;
              }
            break;
          case 0xe0:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  op_semantics_69:
                    {
                      /** 1111 1100 111bit sd rdst cond	bm%2	#%1, %0%S0 */
#line 942 "rx-decode.opc"
                      int bit AU = (op[1] >> 2) & 0x07;
#line 942 "rx-decode.opc"
                      int sd AU = op[1] & 0x03;
#line 942 "rx-decode.opc"
                      int rdst AU = (op[2] >> 4) & 0x0f;
#line 942 "rx-decode.opc"
                      int cond AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 111bit sd rdst cond	bm%2	#%1, %0%S0 */",
                                 op[0], op[1], op[2]);
                          printf ("  bit = 0x%x,", bit);
                          printf ("  sd = 0x%x,", sd);
                          printf ("  rdst = 0x%x,", rdst);
                          printf ("  cond = 0x%x\n", cond);
                        }
                      SYNTAX("bm%2	#%1, %0%S0");
#line 942 "rx-decode.opc"
                      ID(bmcc); BWL(BSIZE); S2cc(cond); SC(bit); DD(sd, rdst, BSIZE);
                    
                    }
                  break;
                case 0x0f:
                  op_semantics_70:
                    {
                      /** 1111 1100 111bit sd rdst 1111	bnot	#%1, %0%S0 */
#line 930 "rx-decode.opc"
                      int bit AU = (op[1] >> 2) & 0x07;
#line 930 "rx-decode.opc"
                      int sd AU = op[1] & 0x03;
#line 930 "rx-decode.opc"
                      int rdst AU = (op[2] >> 4) & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1100 111bit sd rdst 1111	bnot	#%1, %0%S0 */",
                                 op[0], op[1], op[2]);
                          printf ("  bit = 0x%x,", bit);
                          printf ("  sd = 0x%x,", sd);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("bnot	#%1, %0%S0");
#line 930 "rx-decode.opc"
                      ID(bnot); BWL(BSIZE); SC(bit); DD(sd, rdst, BSIZE);
                    
                    }
                  break;
              }
            break;
          case 0xe1:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xe2:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xe3:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xe4:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xe5:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xe6:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xe7:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xe8:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xe9:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xea:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xeb:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xec:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xed:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xee:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xef:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xf0:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xf1:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xf2:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xf3:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xf4:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xf5:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xf6:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xf7:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xf8:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xf9:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xfa:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xfb:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xfc:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xfd:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xfe:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          case 0xff:
              GETBYTE ();
              switch (op[2] & 0x0f)
              {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                  goto op_semantics_69;
                  break;
                case 0x0f:
                  goto op_semantics_70;
                  break;
              }
            break;
          default: UNSUPPORTED(); break;
        }
      break;
    case 0xfd:
        GETBYTE ();
        switch (op[1] & 0xff)
        {
          case 0x00:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                    {
                      /** 1111 1101 0000 0000 srca srcb	mulhi	%1, %2 */
#line 807 "rx-decode.opc"
                      int srca AU = (op[2] >> 4) & 0x0f;
#line 807 "rx-decode.opc"
                      int srcb AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0000 0000 srca srcb	mulhi	%1, %2 */",
                                 op[0], op[1], op[2]);
                          printf ("  srca = 0x%x,", srca);
                          printf ("  srcb = 0x%x\n", srcb);
                        }
                      SYNTAX("mulhi	%1, %2");
#line 807 "rx-decode.opc"
                      ID(mulhi); SR(srca); S2R(srcb); F_____;
                    
                    }
                  break;
              }
            break;
          case 0x01:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                    {
                      /** 1111 1101 0000 0001 srca srcb	mullo	%1, %2 */
#line 810 "rx-decode.opc"
                      int srca AU = (op[2] >> 4) & 0x0f;
#line 810 "rx-decode.opc"
                      int srcb AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0000 0001 srca srcb	mullo	%1, %2 */",
                                 op[0], op[1], op[2]);
                          printf ("  srca = 0x%x,", srca);
                          printf ("  srcb = 0x%x\n", srcb);
                        }
                      SYNTAX("mullo	%1, %2");
#line 810 "rx-decode.opc"
                      ID(mullo); SR(srca); S2R(srcb); F_____;
                    
                    }
                  break;
              }
            break;
          case 0x04:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                    {
                      /** 1111 1101 0000 0100 srca srcb	machi	%1, %2 */
#line 813 "rx-decode.opc"
                      int srca AU = (op[2] >> 4) & 0x0f;
#line 813 "rx-decode.opc"
                      int srcb AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0000 0100 srca srcb	machi	%1, %2 */",
                                 op[0], op[1], op[2]);
                          printf ("  srca = 0x%x,", srca);
                          printf ("  srcb = 0x%x\n", srcb);
                        }
                      SYNTAX("machi	%1, %2");
#line 813 "rx-decode.opc"
                      ID(machi); SR(srca); S2R(srcb); F_____;
                    
                    }
                  break;
              }
            break;
          case 0x05:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                    {
                      /** 1111 1101 0000 0101 srca srcb	maclo	%1, %2 */
#line 816 "rx-decode.opc"
                      int srca AU = (op[2] >> 4) & 0x0f;
#line 816 "rx-decode.opc"
                      int srcb AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0000 0101 srca srcb	maclo	%1, %2 */",
                                 op[0], op[1], op[2]);
                          printf ("  srca = 0x%x,", srca);
                          printf ("  srcb = 0x%x\n", srcb);
                        }
                      SYNTAX("maclo	%1, %2");
#line 816 "rx-decode.opc"
                      ID(maclo); SR(srca); S2R(srcb); F_____;
                    
                    }
                  break;
              }
            break;
          case 0x17:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                    {
                      /** 1111 1101 0001 0111 0000 rsrc	mvtachi	%1 */
#line 819 "rx-decode.opc"
                      int rsrc AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0001 0111 0000 rsrc	mvtachi	%1 */",
                                 op[0], op[1], op[2]);
                          printf ("  rsrc = 0x%x\n", rsrc);
                        }
                      SYNTAX("mvtachi	%1");
#line 819 "rx-decode.opc"
                      ID(mvtachi); SR(rsrc); F_____;
                    
                    }
                  break;
                case 0x10:
                    {
                      /** 1111 1101 0001 0111 0001 rsrc	mvtaclo	%1 */
#line 822 "rx-decode.opc"
                      int rsrc AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0001 0111 0001 rsrc	mvtaclo	%1 */",
                                 op[0], op[1], op[2]);
                          printf ("  rsrc = 0x%x\n", rsrc);
                        }
                      SYNTAX("mvtaclo	%1");
#line 822 "rx-decode.opc"
                      ID(mvtaclo); SR(rsrc); F_____;
                    
                    }
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0x18:
              GETBYTE ();
              switch (op[2] & 0xef)
              {
                case 0x00:
                    {
                      /** 1111 1101 0001 1000 000i 0000	racw	#%1 */
#line 834 "rx-decode.opc"
                      int i AU = (op[2] >> 4) & 0x01;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0001 1000 000i 0000	racw	#%1 */",
                                 op[0], op[1], op[2]);
                          printf ("  i = 0x%x\n", i);
                        }
                      SYNTAX("racw	#%1");
#line 834 "rx-decode.opc"
                      ID(racw); SC(i+1); F_____;
                    
                    /*----------------------------------------------------------------------*/
                    /* SAT									*/
                    
                    }
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0x1f:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                    {
                      /** 1111 1101 0001 1111 0000 rdst	mvfachi	%0 */
#line 825 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0001 1111 0000 rdst	mvfachi	%0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("mvfachi	%0");
#line 825 "rx-decode.opc"
                      ID(mvfachi); DR(rdst); F_____;
                    
                    }
                  break;
                case 0x10:
                    {
                      /** 1111 1101 0001 1111 0001 rdst	mvfaclo	%0 */
#line 831 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0001 1111 0001 rdst	mvfaclo	%0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("mvfaclo	%0");
#line 831 "rx-decode.opc"
                      ID(mvfaclo); DR(rdst); F_____;
                    
                    }
                  break;
                case 0x20:
                    {
                      /** 1111 1101 0001 1111 0010 rdst	mvfacmi	%0 */
#line 828 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0001 1111 0010 rdst	mvfacmi	%0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("mvfacmi	%0");
#line 828 "rx-decode.opc"
                      ID(mvfacmi); DR(rdst); F_____;
                    
                    }
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0x20:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_71:
                    {
                      /** 1111 1101 0010 0p sz rdst rsrc	mov%s	%1, %0 */
#line 341 "rx-decode.opc"
                      int p AU = (op[1] >> 2) & 0x01;
#line 341 "rx-decode.opc"
                      int sz AU = op[1] & 0x03;
#line 341 "rx-decode.opc"
                      int rdst AU = (op[2] >> 4) & 0x0f;
#line 341 "rx-decode.opc"
                      int rsrc AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0010 0p sz rdst rsrc	mov%s	%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  p = 0x%x,", p);
                          printf ("  sz = 0x%x,", sz);
                          printf ("  rdst = 0x%x,", rdst);
                          printf ("  rsrc = 0x%x\n", rsrc);
                        }
                      SYNTAX("mov%s	%1, %0");
#line 341 "rx-decode.opc"
                      ID(mov); sBWL (sz); SR(rsrc); F_____;
                      OP(0, p ? RX_Operand_Predec : RX_Operand_Postinc, rdst, 0);
                    
                    }
                  break;
              }
            break;
          case 0x21:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_71;
                  break;
              }
            break;
          case 0x22:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_71;
                  break;
              }
            break;
          case 0x24:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_71;
                  break;
              }
            break;
          case 0x25:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_71;
                  break;
              }
            break;
          case 0x26:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_71;
                  break;
              }
            break;
          case 0x28:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_72:
                    {
                      /** 1111 1101 0010 1p sz rsrc rdst	mov%s	%1, %0 */
#line 345 "rx-decode.opc"
                      int p AU = (op[1] >> 2) & 0x01;
#line 345 "rx-decode.opc"
                      int sz AU = op[1] & 0x03;
#line 345 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 345 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0010 1p sz rsrc rdst	mov%s	%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  p = 0x%x,", p);
                          printf ("  sz = 0x%x,", sz);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("mov%s	%1, %0");
#line 345 "rx-decode.opc"
                      ID(mov); sBWL (sz); DR(rdst); F_____;
                      OP(1, p ? RX_Operand_Predec : RX_Operand_Postinc, rsrc, 0);
                    
                    }
                  break;
              }
            break;
          case 0x29:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_72;
                  break;
              }
            break;
          case 0x2a:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_72;
                  break;
              }
            break;
          case 0x2c:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_72;
                  break;
              }
            break;
          case 0x2d:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_72;
                  break;
              }
            break;
          case 0x2e:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_72;
                  break;
              }
            break;
          case 0x38:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_73:
                    {
                      /** 1111 1101 0011 1p sz rsrc rdst	movu%s	%1, %0 */
#line 355 "rx-decode.opc"
                      int p AU = (op[1] >> 2) & 0x01;
#line 355 "rx-decode.opc"
                      int sz AU = op[1] & 0x03;
#line 355 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 355 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0011 1p sz rsrc rdst	movu%s	%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  p = 0x%x,", p);
                          printf ("  sz = 0x%x,", sz);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("movu%s	%1, %0");
#line 355 "rx-decode.opc"
                      ID(mov); uBWL (sz); DR(rdst); F_____;
                       OP(1, p ? RX_Operand_Predec : RX_Operand_Postinc, rsrc, 0);
                    
                    /*----------------------------------------------------------------------*/
                    /* PUSH/POP								*/
                    
                    }
                  break;
              }
            break;
          case 0x39:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_73;
                  break;
              }
            break;
          case 0x3a:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_73;
                  break;
              }
            break;
          case 0x3c:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_73;
                  break;
              }
            break;
          case 0x3d:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_73;
                  break;
              }
            break;
          case 0x3e:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_73;
                  break;
              }
            break;
          case 0x60:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                    {
                      /** 1111 1101 0110 0000 rsrc rdst	shlr	%2, %0 */
#line 688 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 688 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0110 0000 rsrc rdst	shlr	%2, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("shlr	%2, %0");
#line 688 "rx-decode.opc"
                      ID(shlr); S2R(rsrc); SR(rdst); DR(rdst); F__SZC;
                    
                    }
                  break;
              }
            break;
          case 0x61:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                    {
                      /** 1111 1101 0110 0001 rsrc rdst	shar	%2, %0 */
#line 678 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 678 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0110 0001 rsrc rdst	shar	%2, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("shar	%2, %0");
#line 678 "rx-decode.opc"
                      ID(shar); S2R(rsrc); SR(rdst); DR(rdst); F_0SZC;
                    
                    }
                  break;
              }
            break;
          case 0x62:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                    {
                      /** 1111 1101 0110 0010 rsrc rdst	shll	%2, %0 */
#line 668 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 668 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0110 0010 rsrc rdst	shll	%2, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("shll	%2, %0");
#line 668 "rx-decode.opc"
                      ID(shll); S2R(rsrc); SR(rdst); DR(rdst); F_OSZC;
                    
                    }
                  break;
              }
            break;
          case 0x64:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                    {
                      /** 1111 1101 0110 0100 rsrc rdst	rotr	%1, %0 */
#line 712 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 712 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0110 0100 rsrc rdst	rotr	%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("rotr	%1, %0");
#line 712 "rx-decode.opc"
                      ID(rotr); SR(rsrc); DR(rdst); F__SZC;
                    
                    }
                  break;
              }
            break;
          case 0x65:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                    {
                      /** 1111 1101 0110 0101 rsrc rdst	revw	%1, %0 */
#line 715 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 715 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0110 0101 rsrc rdst	revw	%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("revw	%1, %0");
#line 715 "rx-decode.opc"
                      ID(revw); SR(rsrc); DR(rdst);
                    
                    }
                  break;
              }
            break;
          case 0x66:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                    {
                      /** 1111 1101 0110 0110 rsrc rdst	rotl	%1, %0 */
#line 706 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 706 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0110 0110 rsrc rdst	rotl	%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("rotl	%1, %0");
#line 706 "rx-decode.opc"
                      ID(rotl); SR(rsrc); DR(rdst); F__SZC;
                    
                    }
                  break;
              }
            break;
          case 0x67:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                    {
                      /** 1111 1101 0110 0111 rsrc rdst	revl	%1, %0 */
#line 718 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 718 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0110 0111 rsrc rdst	revl	%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("revl	%1, %0");
#line 718 "rx-decode.opc"
                      ID(revl); SR(rsrc); DR(rdst);
                    
                    /*----------------------------------------------------------------------*/
                    /* BRANCH								*/
                    
                    }
                  break;
              }
            break;
          case 0x68:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_74:
                    {
                      /** 1111 1101 0110 100c rsrc rdst	mvtc	%1, %0 */
#line 969 "rx-decode.opc"
                      int c AU = op[1] & 0x01;
#line 969 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 969 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0110 100c rsrc rdst	mvtc	%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  c = 0x%x,", c);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("mvtc	%1, %0");
#line 969 "rx-decode.opc"
                      ID(mov); SR(rsrc); DR(c*16+rdst + 16);
                    
                    }
                  break;
              }
            break;
          case 0x69:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_74;
                  break;
              }
            break;
          case 0x6a:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_75:
                    {
                      /** 1111 1101 0110 101s rsrc rdst	mvfc	%1, %0 */
#line 972 "rx-decode.opc"
                      int s AU = op[1] & 0x01;
#line 972 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 972 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0110 101s rsrc rdst	mvfc	%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  s = 0x%x,", s);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("mvfc	%1, %0");
#line 972 "rx-decode.opc"
                      ID(mov); SR((s*16+rsrc) + 16); DR(rdst);
                    
                    /*----------------------------------------------------------------------*/
                    /* INTERRUPTS								*/
                    
                    }
                  break;
              }
            break;
          case 0x6b:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_75;
                  break;
              }
            break;
          case 0x6c:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_76:
                    {
                      /** 1111 1101 0110 110i mmmm rdst	rotr	#%1, %0 */
#line 709 "rx-decode.opc"
                      int i AU = op[1] & 0x01;
#line 709 "rx-decode.opc"
                      int mmmm AU = (op[2] >> 4) & 0x0f;
#line 709 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0110 110i mmmm rdst	rotr	#%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  i = 0x%x,", i);
                          printf ("  mmmm = 0x%x,", mmmm);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("rotr	#%1, %0");
#line 709 "rx-decode.opc"
                      ID(rotr); SC(i*16+mmmm); DR(rdst); F__SZC;
                    
                    }
                  break;
              }
            break;
          case 0x6d:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_76;
                  break;
              }
            break;
          case 0x6e:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_77:
                    {
                      /** 1111 1101 0110 111i mmmm rdst	rotl	#%1, %0 */
#line 703 "rx-decode.opc"
                      int i AU = op[1] & 0x01;
#line 703 "rx-decode.opc"
                      int mmmm AU = (op[2] >> 4) & 0x0f;
#line 703 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0110 111i mmmm rdst	rotl	#%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  i = 0x%x,", i);
                          printf ("  mmmm = 0x%x,", mmmm);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("rotl	#%1, %0");
#line 703 "rx-decode.opc"
                      ID(rotl); SC(i*16+mmmm); DR(rdst); F__SZC;
                    
                    }
                  break;
              }
            break;
          case 0x6f:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_77;
                  break;
              }
            break;
          case 0x70:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x20:
                  op_semantics_78:
                    {
                      /** 1111 1101 0111 im00 0010rdst	adc	#%1, %0 */
#line 485 "rx-decode.opc"
                      int im AU = (op[1] >> 2) & 0x03;
#line 485 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0111 im00 0010rdst	adc	#%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  im = 0x%x,", im);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("adc	#%1, %0");
#line 485 "rx-decode.opc"
                      ID(adc); SC(IMMex(im)); DR(rdst); F_OSZC;
                    
                    }
                  break;
                case 0x40:
                  op_semantics_79:
                    {
                      /** 1111 1101 0111 im00 0100rdst	max	#%1, %0 */
#line 567 "rx-decode.opc"
                      int im AU = (op[1] >> 2) & 0x03;
#line 567 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0111 im00 0100rdst	max	#%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  im = 0x%x,", im);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("max	#%1, %0");
#line 567 "rx-decode.opc"
                      ID(max); DR(rdst); SC(IMMex(im));
                    
                    }
                  break;
                case 0x50:
                  op_semantics_80:
                    {
                      /** 1111 1101 0111 im00 0101rdst	min	#%1, %0 */
#line 587 "rx-decode.opc"
                      int im AU = (op[1] >> 2) & 0x03;
#line 587 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0111 im00 0101rdst	min	#%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  im = 0x%x,", im);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("min	#%1, %0");
#line 587 "rx-decode.opc"
                      ID(min); DR(rdst); SC(IMMex(im));
                    
                    }
                  break;
                case 0x60:
                  op_semantics_81:
                    {
                      /** 1111 1101 0111 im00 0110rdst	emul	#%1, %0 */
#line 617 "rx-decode.opc"
                      int im AU = (op[1] >> 2) & 0x03;
#line 617 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0111 im00 0110rdst	emul	#%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  im = 0x%x,", im);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("emul	#%1, %0");
#line 617 "rx-decode.opc"
                      ID(emul); DR(rdst); SC(IMMex(im));
                    
                    }
                  break;
                case 0x70:
                  op_semantics_82:
                    {
                      /** 1111 1101 0111 im00 0111rdst	emulu	#%1, %0 */
#line 629 "rx-decode.opc"
                      int im AU = (op[1] >> 2) & 0x03;
#line 629 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0111 im00 0111rdst	emulu	#%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  im = 0x%x,", im);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("emulu	#%1, %0");
#line 629 "rx-decode.opc"
                      ID(emulu); DR(rdst); SC(IMMex(im));
                    
                    }
                  break;
                case 0x80:
                  op_semantics_83:
                    {
                      /** 1111 1101 0111 im00 1000rdst	div	#%1, %0 */
#line 641 "rx-decode.opc"
                      int im AU = (op[1] >> 2) & 0x03;
#line 641 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0111 im00 1000rdst	div	#%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  im = 0x%x,", im);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("div	#%1, %0");
#line 641 "rx-decode.opc"
                      ID(div); DR(rdst); SC(IMMex(im)); F_O___;
                    
                    }
                  break;
                case 0x90:
                  op_semantics_84:
                    {
                      /** 1111 1101 0111 im00 1001rdst	divu	#%1, %0 */
#line 653 "rx-decode.opc"
                      int im AU = (op[1] >> 2) & 0x03;
#line 653 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0111 im00 1001rdst	divu	#%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  im = 0x%x,", im);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("divu	#%1, %0");
#line 653 "rx-decode.opc"
                      ID(divu); DR(rdst); SC(IMMex(im)); F_O___;
                    
                    }
                  break;
                case 0xc0:
                  op_semantics_85:
                    {
                      /** 1111 1101 0111 im00 1100rdst	tst	#%1, %2 */
#line 464 "rx-decode.opc"
                      int im AU = (op[1] >> 2) & 0x03;
#line 464 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0111 im00 1100rdst	tst	#%1, %2 */",
                                 op[0], op[1], op[2]);
                          printf ("  im = 0x%x,", im);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("tst	#%1, %2");
#line 464 "rx-decode.opc"
                      ID(and); SC(IMMex(im)); S2R(rdst); F__SZ_;
                    
                    }
                  break;
                case 0xd0:
                  op_semantics_86:
                    {
                      /** 1111 1101 0111 im00 1101rdst	xor	#%1, %0 */
#line 443 "rx-decode.opc"
                      int im AU = (op[1] >> 2) & 0x03;
#line 443 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0111 im00 1101rdst	xor	#%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  im = 0x%x,", im);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("xor	#%1, %0");
#line 443 "rx-decode.opc"
                      ID(xor); SC(IMMex(im)); DR(rdst); F__SZ_;
                    
                    }
                  break;
                case 0xe0:
                  op_semantics_87:
                    {
                      /** 1111 1101 0111 im00 1110rdst	stz	#%1, %0 */
#line 389 "rx-decode.opc"
                      int im AU = (op[1] >> 2) & 0x03;
#line 389 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0111 im00 1110rdst	stz	#%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  im = 0x%x,", im);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("stz	#%1, %0");
#line 389 "rx-decode.opc"
                      ID(stcc); SC(IMMex(im)); DR(rdst); S2cc(RXC_z);
                    
                    }
                  break;
                case 0xf0:
                  op_semantics_88:
                    {
                      /** 1111 1101 0111 im00 1111rdst	stnz	#%1, %0 */
#line 392 "rx-decode.opc"
                      int im AU = (op[1] >> 2) & 0x03;
#line 392 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0111 im00 1111rdst	stnz	#%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  im = 0x%x,", im);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("stnz	#%1, %0");
#line 392 "rx-decode.opc"
                      ID(stcc); SC(IMMex(im)); DR(rdst); S2cc(RXC_nz);
                    
                    /*----------------------------------------------------------------------*/
                    /* RTSD									*/
                    
                    }
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0x72:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                    {
                      /** 1111 1101 0111 0010 0000 rdst	fsub	#%1, %0 */
#line 861 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0111 0010 0000 rdst	fsub	#%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("fsub	#%1, %0");
#line 861 "rx-decode.opc"
                      ID(fsub); DR(rdst); SC(IMM(0)); F__SZ_;
                    
                    }
                  break;
                case 0x10:
                    {
                      /** 1111 1101 0111 0010 0001 rdst	fcmp	#%1, %0 */
#line 855 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0111 0010 0001 rdst	fcmp	#%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("fcmp	#%1, %0");
#line 855 "rx-decode.opc"
                      ID(fcmp); DR(rdst); SC(IMM(0)); F_OSZ_;
                    
                    }
                  break;
                case 0x20:
                    {
                      /** 1111 1101 0111 0010 0010 rdst	fadd	#%1, %0 */
#line 849 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0111 0010 0010 rdst	fadd	#%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("fadd	#%1, %0");
#line 849 "rx-decode.opc"
                      ID(fadd); DR(rdst); SC(IMM(0)); F__SZ_;
                    
                    }
                  break;
                case 0x30:
                    {
                      /** 1111 1101 0111 0010 0011 rdst	fmul	#%1, %0 */
#line 870 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0111 0010 0011 rdst	fmul	#%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("fmul	#%1, %0");
#line 870 "rx-decode.opc"
                      ID(fmul); DR(rdst); SC(IMM(0)); F__SZ_;
                    
                    }
                  break;
                case 0x40:
                    {
                      /** 1111 1101 0111 0010 0100 rdst	fdiv	#%1, %0 */
#line 876 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0111 0010 0100 rdst	fdiv	#%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("fdiv	#%1, %0");
#line 876 "rx-decode.opc"
                      ID(fdiv); DR(rdst); SC(IMM(0)); F__SZ_;
                    
                    }
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0x73:
              GETBYTE ();
              switch (op[2] & 0xe0)
              {
                case 0x00:
                  op_semantics_89:
                    {
                      /** 1111 1101 0111 im11 000crdst	mvtc	#%1, %0 */
#line 966 "rx-decode.opc"
                      int im AU = (op[1] >> 2) & 0x03;
#line 966 "rx-decode.opc"
                      int crdst AU = op[2] & 0x1f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 0111 im11 000crdst	mvtc	#%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  im = 0x%x,", im);
                          printf ("  crdst = 0x%x\n", crdst);
                        }
                      SYNTAX("mvtc	#%1, %0");
#line 966 "rx-decode.opc"
                      ID(mov); SC(IMMex(im)); DR(crdst + 16);
                    
                    }
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0x74:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x20:
                  goto op_semantics_78;
                  break;
                case 0x40:
                  goto op_semantics_79;
                  break;
                case 0x50:
                  goto op_semantics_80;
                  break;
                case 0x60:
                  goto op_semantics_81;
                  break;
                case 0x70:
                  goto op_semantics_82;
                  break;
                case 0x80:
                  goto op_semantics_83;
                  break;
                case 0x90:
                  goto op_semantics_84;
                  break;
                case 0xc0:
                  goto op_semantics_85;
                  break;
                case 0xd0:
                  goto op_semantics_86;
                  break;
                case 0xe0:
                  goto op_semantics_87;
                  break;
                case 0xf0:
                  goto op_semantics_88;
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0x77:
              GETBYTE ();
              switch (op[2] & 0xe0)
              {
                case 0x00:
                  goto op_semantics_89;
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0x78:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x20:
                  goto op_semantics_78;
                  break;
                case 0x40:
                  goto op_semantics_79;
                  break;
                case 0x50:
                  goto op_semantics_80;
                  break;
                case 0x60:
                  goto op_semantics_81;
                  break;
                case 0x70:
                  goto op_semantics_82;
                  break;
                case 0x80:
                  goto op_semantics_83;
                  break;
                case 0x90:
                  goto op_semantics_84;
                  break;
                case 0xc0:
                  goto op_semantics_85;
                  break;
                case 0xd0:
                  goto op_semantics_86;
                  break;
                case 0xe0:
                  goto op_semantics_87;
                  break;
                case 0xf0:
                  goto op_semantics_88;
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0x7b:
              GETBYTE ();
              switch (op[2] & 0xe0)
              {
                case 0x00:
                  goto op_semantics_89;
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0x7c:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x20:
                  goto op_semantics_78;
                  break;
                case 0x40:
                  goto op_semantics_79;
                  break;
                case 0x50:
                  goto op_semantics_80;
                  break;
                case 0x60:
                  goto op_semantics_81;
                  break;
                case 0x70:
                  goto op_semantics_82;
                  break;
                case 0x80:
                  goto op_semantics_83;
                  break;
                case 0x90:
                  goto op_semantics_84;
                  break;
                case 0xc0:
                  goto op_semantics_85;
                  break;
                case 0xd0:
                  goto op_semantics_86;
                  break;
                case 0xe0:
                  goto op_semantics_87;
                  break;
                case 0xf0:
                  goto op_semantics_88;
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0x7f:
              GETBYTE ();
              switch (op[2] & 0xe0)
              {
                case 0x00:
                  goto op_semantics_89;
                  break;
                default: UNSUPPORTED(); break;
              }
            break;
          case 0x80:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_90:
                    {
                      /** 1111 1101 100immmm rsrc rdst	shlr	#%2, %1, %0 */
#line 691 "rx-decode.opc"
                      int immmm AU = op[1] & 0x1f;
#line 691 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 691 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 100immmm rsrc rdst	shlr	#%2, %1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  immmm = 0x%x,", immmm);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("shlr	#%2, %1, %0");
#line 691 "rx-decode.opc"
                      ID(shlr); S2C(immmm); SR(rsrc); DR(rdst); F__SZC;
                    
                    /*----------------------------------------------------------------------*/
                    /* ROTATE								*/
                    
                    }
                  break;
              }
            break;
          case 0x81:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x82:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x83:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x84:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x85:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x86:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x87:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x88:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x89:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x8a:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x8b:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x8c:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x8d:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x8e:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x8f:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x90:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x91:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x92:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x93:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x94:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x95:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x96:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x97:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x98:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x99:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x9a:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x9b:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x9c:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x9d:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x9e:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0x9f:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_90;
                  break;
              }
            break;
          case 0xa0:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_91:
                    {
                      /** 1111 1101 101immmm rsrc rdst	shar	#%2, %1, %0 */
#line 681 "rx-decode.opc"
                      int immmm AU = op[1] & 0x1f;
#line 681 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 681 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 101immmm rsrc rdst	shar	#%2, %1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  immmm = 0x%x,", immmm);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("shar	#%2, %1, %0");
#line 681 "rx-decode.opc"
                      ID(shar); S2C(immmm); SR(rsrc); DR(rdst); F_0SZC;
                    
                    
                    }
                  break;
              }
            break;
          case 0xa1:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xa2:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xa3:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xa4:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xa5:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xa6:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xa7:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xa8:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xa9:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xaa:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xab:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xac:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xad:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xae:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xaf:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xb0:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xb1:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xb2:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xb3:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xb4:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xb5:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xb6:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xb7:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xb8:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xb9:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xba:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xbb:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xbc:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xbd:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xbe:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xbf:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_91;
                  break;
              }
            break;
          case 0xc0:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_92:
                    {
                      /** 1111 1101 110immmm rsrc rdst	shll	#%2, %1, %0 */
#line 671 "rx-decode.opc"
                      int immmm AU = op[1] & 0x1f;
#line 671 "rx-decode.opc"
                      int rsrc AU = (op[2] >> 4) & 0x0f;
#line 671 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 110immmm rsrc rdst	shll	#%2, %1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  immmm = 0x%x,", immmm);
                          printf ("  rsrc = 0x%x,", rsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("shll	#%2, %1, %0");
#line 671 "rx-decode.opc"
                      ID(shll); S2C(immmm); SR(rsrc); DR(rdst); F_OSZC;
                    
                    
                    }
                  break;
              }
            break;
          case 0xc1:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xc2:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xc3:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xc4:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xc5:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xc6:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xc7:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xc8:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xc9:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xca:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xcb:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xcc:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xcd:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xce:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xcf:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xd0:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xd1:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xd2:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xd3:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xd4:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xd5:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xd6:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xd7:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xd8:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xd9:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xda:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xdb:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xdc:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xdd:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xde:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xdf:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_92;
                  break;
              }
            break;
          case 0xe0:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  op_semantics_93:
                    {
                      /** 1111 1101 111 bittt cond rdst	bm%2	#%1, %0%S0 */
#line 945 "rx-decode.opc"
                      int bittt AU = op[1] & 0x1f;
#line 945 "rx-decode.opc"
                      int cond AU = (op[2] >> 4) & 0x0f;
#line 945 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 111 bittt cond rdst	bm%2	#%1, %0%S0 */",
                                 op[0], op[1], op[2]);
                          printf ("  bittt = 0x%x,", bittt);
                          printf ("  cond = 0x%x,", cond);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("bm%2	#%1, %0%S0");
#line 945 "rx-decode.opc"
                      ID(bmcc); BWL(LSIZE); S2cc(cond); SC(bittt); DR(rdst);
                    
                    /*----------------------------------------------------------------------*/
                    /* CONTROL REGISTERS							*/
                    
                    }
                  break;
                case 0xf0:
                  op_semantics_94:
                    {
                      /** 1111 1101 111bittt 1111 rdst	bnot	#%1, %0 */
#line 938 "rx-decode.opc"
                      int bittt AU = op[1] & 0x1f;
#line 938 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1101 111bittt 1111 rdst	bnot	#%1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  bittt = 0x%x,", bittt);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("bnot	#%1, %0");
#line 938 "rx-decode.opc"
                      ID(bnot); BWL(LSIZE); SC(bittt); DR(rdst);
                    
                    
                    }
                  break;
              }
            break;
          case 0xe1:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xe2:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xe3:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xe4:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xe5:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xe6:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xe7:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xe8:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xe9:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xea:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xeb:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xec:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xed:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xee:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xef:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xf0:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xf1:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xf2:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xf3:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xf4:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xf5:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xf6:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xf7:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xf8:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xf9:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xfa:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xfb:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xfc:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xfd:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xfe:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          case 0xff:
              GETBYTE ();
              switch (op[2] & 0xf0)
              {
                case 0x00:
                case 0x10:
                case 0x20:
                case 0x30:
                case 0x40:
                case 0x50:
                case 0x60:
                case 0x70:
                case 0x80:
                case 0x90:
                case 0xa0:
                case 0xb0:
                case 0xc0:
                case 0xd0:
                case 0xe0:
                  goto op_semantics_93;
                  break;
                case 0xf0:
                  goto op_semantics_94;
                  break;
              }
            break;
          default: UNSUPPORTED(); break;
        }
      break;
    case 0xfe:
        GETBYTE ();
        switch (op[1] & 0xff)
        {
          case 0x00:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_95:
                    {
                      /** 1111 1110 00sz isrc bsrc rdst	mov%s	%0, [%1, %2] */
#line 335 "rx-decode.opc"
                      int sz AU = (op[1] >> 4) & 0x03;
#line 335 "rx-decode.opc"
                      int isrc AU = op[1] & 0x0f;
#line 335 "rx-decode.opc"
                      int bsrc AU = (op[2] >> 4) & 0x0f;
#line 335 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1110 00sz isrc bsrc rdst	mov%s	%0, [%1, %2] */",
                                 op[0], op[1], op[2]);
                          printf ("  sz = 0x%x,", sz);
                          printf ("  isrc = 0x%x,", isrc);
                          printf ("  bsrc = 0x%x,", bsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("mov%s	%0, [%1, %2]");
#line 335 "rx-decode.opc"
                      ID(movbir); sBWL(sz); DR(rdst); SRR(isrc); S2R(bsrc); F_____;
                    
                    }
                  break;
              }
            break;
          case 0x01:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x02:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x03:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x04:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x05:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x06:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x07:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x08:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x09:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x0a:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x0b:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x0c:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x0d:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x0e:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x0f:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x10:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x11:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x12:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x13:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x14:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x15:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x16:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x17:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x18:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x19:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x1a:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x1b:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x1c:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x1d:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x1e:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x1f:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x20:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x21:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x22:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x23:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x24:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x25:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x26:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x27:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x28:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x29:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x2a:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x2b:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x2c:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x2d:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x2e:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x2f:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_95;
                  break;
              }
            break;
          case 0x40:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_96:
                    {
                      /** 1111 1110 01sz isrc bsrc rdst	mov%s	[%1, %2], %0 */
#line 332 "rx-decode.opc"
                      int sz AU = (op[1] >> 4) & 0x03;
#line 332 "rx-decode.opc"
                      int isrc AU = op[1] & 0x0f;
#line 332 "rx-decode.opc"
                      int bsrc AU = (op[2] >> 4) & 0x0f;
#line 332 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1110 01sz isrc bsrc rdst	mov%s	[%1, %2], %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  sz = 0x%x,", sz);
                          printf ("  isrc = 0x%x,", isrc);
                          printf ("  bsrc = 0x%x,", bsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("mov%s	[%1, %2], %0");
#line 332 "rx-decode.opc"
                      ID(movbi); sBWL(sz); DR(rdst); SRR(isrc); S2R(bsrc); F_____;
                    
                    }
                  break;
              }
            break;
          case 0x41:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x42:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x43:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x44:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x45:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x46:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x47:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x48:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x49:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x4a:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x4b:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x4c:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x4d:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x4e:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x4f:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x50:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x51:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x52:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x53:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x54:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x55:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x56:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x57:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x58:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x59:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x5a:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x5b:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x5c:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x5d:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x5e:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x5f:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x60:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x61:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x62:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x63:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x64:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x65:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x66:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x67:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x68:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x69:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x6a:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x6b:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x6c:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x6d:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x6e:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0x6f:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_96;
                  break;
              }
            break;
          case 0xc0:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_97:
                    {
                      /** 1111 1110 11sz isrc bsrc rdst	movu%s	[%1, %2], %0 */
#line 338 "rx-decode.opc"
                      int sz AU = (op[1] >> 4) & 0x03;
#line 338 "rx-decode.opc"
                      int isrc AU = op[1] & 0x0f;
#line 338 "rx-decode.opc"
                      int bsrc AU = (op[2] >> 4) & 0x0f;
#line 338 "rx-decode.opc"
                      int rdst AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1110 11sz isrc bsrc rdst	movu%s	[%1, %2], %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  sz = 0x%x,", sz);
                          printf ("  isrc = 0x%x,", isrc);
                          printf ("  bsrc = 0x%x,", bsrc);
                          printf ("  rdst = 0x%x\n", rdst);
                        }
                      SYNTAX("movu%s	[%1, %2], %0");
#line 338 "rx-decode.opc"
                      ID(movbi); uBWL(sz); DR(rdst); SRR(isrc); S2R(bsrc); F_____;
                    
                    }
                  break;
              }
            break;
          case 0xc1:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xc2:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xc3:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xc4:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xc5:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xc6:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xc7:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xc8:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xc9:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xca:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xcb:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xcc:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xcd:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xce:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xcf:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xd0:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xd1:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xd2:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xd3:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xd4:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xd5:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xd6:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xd7:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xd8:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xd9:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xda:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xdb:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xdc:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xdd:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xde:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xdf:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xe0:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xe1:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xe2:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xe3:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xe4:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xe5:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xe6:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xe7:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xe8:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xe9:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xea:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xeb:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xec:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xed:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xee:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          case 0xef:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_97;
                  break;
              }
            break;
          default: UNSUPPORTED(); break;
        }
      break;
    case 0xff:
        GETBYTE ();
        switch (op[1] & 0xff)
        {
          case 0x00:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_98:
                    {
                      /** 1111 1111 0000 rdst srca srcb	sub	%2, %1, %0 */
#line 542 "rx-decode.opc"
                      int rdst AU = op[1] & 0x0f;
#line 542 "rx-decode.opc"
                      int srca AU = (op[2] >> 4) & 0x0f;
#line 542 "rx-decode.opc"
                      int srcb AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1111 0000 rdst srca srcb	sub	%2, %1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rdst = 0x%x,", rdst);
                          printf ("  srca = 0x%x,", srca);
                          printf ("  srcb = 0x%x\n", srcb);
                        }
                      SYNTAX("sub	%2, %1, %0");
#line 542 "rx-decode.opc"
                      ID(sub); DR(rdst); SR(srcb); S2R(srca); F_OSZC;
                    
                    /*----------------------------------------------------------------------*/
                    /* SBB									*/
                    
                    }
                  break;
              }
            break;
          case 0x01:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_98;
                  break;
              }
            break;
          case 0x02:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_98;
                  break;
              }
            break;
          case 0x03:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_98;
                  break;
              }
            break;
          case 0x04:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_98;
                  break;
              }
            break;
          case 0x05:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_98;
                  break;
              }
            break;
          case 0x06:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_98;
                  break;
              }
            break;
          case 0x07:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_98;
                  break;
              }
            break;
          case 0x08:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_98;
                  break;
              }
            break;
          case 0x09:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_98;
                  break;
              }
            break;
          case 0x0a:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_98;
                  break;
              }
            break;
          case 0x0b:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_98;
                  break;
              }
            break;
          case 0x0c:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_98;
                  break;
              }
            break;
          case 0x0d:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_98;
                  break;
              }
            break;
          case 0x0e:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_98;
                  break;
              }
            break;
          case 0x0f:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_98;
                  break;
              }
            break;
          case 0x20:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_99:
                    {
                      /** 1111 1111 0010 rdst srca srcb	add	%2, %1, %0 */
#line 509 "rx-decode.opc"
                      int rdst AU = op[1] & 0x0f;
#line 509 "rx-decode.opc"
                      int srca AU = (op[2] >> 4) & 0x0f;
#line 509 "rx-decode.opc"
                      int srcb AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1111 0010 rdst srca srcb	add	%2, %1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rdst = 0x%x,", rdst);
                          printf ("  srca = 0x%x,", srca);
                          printf ("  srcb = 0x%x\n", srcb);
                        }
                      SYNTAX("add	%2, %1, %0");
#line 509 "rx-decode.opc"
                      ID(add); DR(rdst); SR(srcb); S2R(srca); F_OSZC;
                    
                    /*----------------------------------------------------------------------*/
                    /* CMP									*/
                    
                    }
                  break;
              }
            break;
          case 0x21:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_99;
                  break;
              }
            break;
          case 0x22:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_99;
                  break;
              }
            break;
          case 0x23:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_99;
                  break;
              }
            break;
          case 0x24:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_99;
                  break;
              }
            break;
          case 0x25:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_99;
                  break;
              }
            break;
          case 0x26:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_99;
                  break;
              }
            break;
          case 0x27:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_99;
                  break;
              }
            break;
          case 0x28:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_99;
                  break;
              }
            break;
          case 0x29:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_99;
                  break;
              }
            break;
          case 0x2a:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_99;
                  break;
              }
            break;
          case 0x2b:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_99;
                  break;
              }
            break;
          case 0x2c:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_99;
                  break;
              }
            break;
          case 0x2d:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_99;
                  break;
              }
            break;
          case 0x2e:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_99;
                  break;
              }
            break;
          case 0x2f:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_99;
                  break;
              }
            break;
          case 0x30:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_100:
                    {
                      /** 1111 1111 0011 rdst srca srcb	mul 	%2, %1, %0 */
#line 611 "rx-decode.opc"
                      int rdst AU = op[1] & 0x0f;
#line 611 "rx-decode.opc"
                      int srca AU = (op[2] >> 4) & 0x0f;
#line 611 "rx-decode.opc"
                      int srcb AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1111 0011 rdst srca srcb	mul 	%2, %1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rdst = 0x%x,", rdst);
                          printf ("  srca = 0x%x,", srca);
                          printf ("  srcb = 0x%x\n", srcb);
                        }
                      SYNTAX("mul 	%2, %1, %0");
#line 611 "rx-decode.opc"
                      ID(mul); DR(rdst); SR(srcb); S2R(srca); F_____;
                    
                    /*----------------------------------------------------------------------*/
                    /* EMUL									*/
                    
                    }
                  break;
              }
            break;
          case 0x31:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_100;
                  break;
              }
            break;
          case 0x32:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_100;
                  break;
              }
            break;
          case 0x33:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_100;
                  break;
              }
            break;
          case 0x34:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_100;
                  break;
              }
            break;
          case 0x35:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_100;
                  break;
              }
            break;
          case 0x36:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_100;
                  break;
              }
            break;
          case 0x37:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_100;
                  break;
              }
            break;
          case 0x38:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_100;
                  break;
              }
            break;
          case 0x39:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_100;
                  break;
              }
            break;
          case 0x3a:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_100;
                  break;
              }
            break;
          case 0x3b:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_100;
                  break;
              }
            break;
          case 0x3c:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_100;
                  break;
              }
            break;
          case 0x3d:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_100;
                  break;
              }
            break;
          case 0x3e:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_100;
                  break;
              }
            break;
          case 0x3f:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_100;
                  break;
              }
            break;
          case 0x40:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_101:
                    {
                      /** 1111 1111 0100 rdst srca srcb	and	%2, %1, %0 */
#line 419 "rx-decode.opc"
                      int rdst AU = op[1] & 0x0f;
#line 419 "rx-decode.opc"
                      int srca AU = (op[2] >> 4) & 0x0f;
#line 419 "rx-decode.opc"
                      int srcb AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1111 0100 rdst srca srcb	and	%2, %1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rdst = 0x%x,", rdst);
                          printf ("  srca = 0x%x,", srca);
                          printf ("  srcb = 0x%x\n", srcb);
                        }
                      SYNTAX("and	%2, %1, %0");
#line 419 "rx-decode.opc"
                      ID(and); DR(rdst); SR(srcb); S2R(srca); F__SZ_;
                    
                    /*----------------------------------------------------------------------*/
                    /* OR									*/
                    
                    }
                  break;
              }
            break;
          case 0x41:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_101;
                  break;
              }
            break;
          case 0x42:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_101;
                  break;
              }
            break;
          case 0x43:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_101;
                  break;
              }
            break;
          case 0x44:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_101;
                  break;
              }
            break;
          case 0x45:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_101;
                  break;
              }
            break;
          case 0x46:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_101;
                  break;
              }
            break;
          case 0x47:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_101;
                  break;
              }
            break;
          case 0x48:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_101;
                  break;
              }
            break;
          case 0x49:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_101;
                  break;
              }
            break;
          case 0x4a:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_101;
                  break;
              }
            break;
          case 0x4b:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_101;
                  break;
              }
            break;
          case 0x4c:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_101;
                  break;
              }
            break;
          case 0x4d:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_101;
                  break;
              }
            break;
          case 0x4e:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_101;
                  break;
              }
            break;
          case 0x4f:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_101;
                  break;
              }
            break;
          case 0x50:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  op_semantics_102:
                    {
                      /** 1111 1111 0101 rdst srca srcb	or	%2, %1, %0 */
#line 437 "rx-decode.opc"
                      int rdst AU = op[1] & 0x0f;
#line 437 "rx-decode.opc"
                      int srca AU = (op[2] >> 4) & 0x0f;
#line 437 "rx-decode.opc"
                      int srcb AU = op[2] & 0x0f;
                      if (trace)
                        {
                          printf ("\033[33m%s\033[0m  %02x %02x %02x\n",
                                 "/** 1111 1111 0101 rdst srca srcb	or	%2, %1, %0 */",
                                 op[0], op[1], op[2]);
                          printf ("  rdst = 0x%x,", rdst);
                          printf ("  srca = 0x%x,", srca);
                          printf ("  srcb = 0x%x\n", srcb);
                        }
                      SYNTAX("or	%2, %1, %0");
#line 437 "rx-decode.opc"
                      ID(or); DR(rdst); SR(srcb); S2R(srca); F__SZ_;
                    
                    /*----------------------------------------------------------------------*/
                    /* XOR									*/
                    
                    }
                  break;
              }
            break;
          case 0x51:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_102;
                  break;
              }
            break;
          case 0x52:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_102;
                  break;
              }
            break;
          case 0x53:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_102;
                  break;
              }
            break;
          case 0x54:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_102;
                  break;
              }
            break;
          case 0x55:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_102;
                  break;
              }
            break;
          case 0x56:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_102;
                  break;
              }
            break;
          case 0x57:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_102;
                  break;
              }
            break;
          case 0x58:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_102;
                  break;
              }
            break;
          case 0x59:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_102;
                  break;
              }
            break;
          case 0x5a:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_102;
                  break;
              }
            break;
          case 0x5b:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_102;
                  break;
              }
            break;
          case 0x5c:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_102;
                  break;
              }
            break;
          case 0x5d:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_102;
                  break;
              }
            break;
          case 0x5e:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_102;
                  break;
              }
            break;
          case 0x5f:
              GETBYTE ();
              switch (op[2] & 0x00)
              {
                case 0x00:
                  goto op_semantics_102;
                  break;
              }
            break;
          default: UNSUPPORTED(); break;
        }
      break;
    default: UNSUPPORTED(); break;
  }
#line 1002 "rx-decode.opc"

  return rx->n_bytes;
}
