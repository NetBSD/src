/*
 * Simulator for the Hitachi H8/300 architecture.
 *
 * Written by Steve Chamberlain of Cygnus Support. sac@cygnus.com
 *
 * This file is part of H8/300 sim
 *
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * Cygnus offers the following for use in the public domain.  Cygnus makes no
 * warranty with regard to the software or its performance and the user
 * accepts the software "AS IS" with all faults.
 *
 * CYGNUS DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO THIS
 * SOFTWARE INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "config.h"

#include <stdio.h>
#include <signal.h>
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include "ansidecl.h"
#include "bfd.h"
#include "gdb/callback.h"
#include "gdb/remote-sim.h"
#include "gdb/sim-h8300.h"

#ifndef SIGTRAP
# define SIGTRAP 5
#endif

int debug;

host_callback *sim_callback;

static SIM_OPEN_KIND sim_kind;
static char *myname;

/* FIXME: Needs to live in header file.
   This header should also include the things in remote-sim.h.
   One could move this to remote-sim.h but this function isn't needed
   by gdb.  */
void sim_set_simcache_size PARAMS ((int));

#define X(op, size)  op * 4 + size

#define SP (h8300hmode ? SL : SW)
#define SB 0
#define SW 1
#define SL 2
#define OP_REG 1
#define OP_DEC 2
#define OP_DISP 3
#define OP_INC 4
#define OP_PCREL 5
#define OP_MEM 6
#define OP_CCR 7
#define OP_IMM 8
#define OP_ABS 10
#define OP_EXR 11
#define h8_opcodes ops
#define DEFINE_TABLE
#include "opcode/h8300.h"

#include "inst.h"

/* The rate at which to call the host's poll_quit callback.  */

#define POLL_QUIT_INTERVAL 0x80000

#define LOW_BYTE(x) ((x) & 0xff)
#define HIGH_BYTE(x) (((x) >> 8) & 0xff)
#define P(X,Y) ((X << 8) | Y)

#define BUILDSR()   cpu.ccr = (I << 7) | (UI << 6)| (H<<5) | (U<<4) | \
                              (N << 3) | (Z << 2) | (V<<1) | C;

#define BUILDEXR()	    \
  if (h8300smode) cpu.exr = (trace<<7) | intMask;

#define GETSR()		    \
  c = (cpu.ccr >> 0) & 1;\
  v = (cpu.ccr >> 1) & 1;\
  nz = !((cpu.ccr >> 2) & 1);\
  n = (cpu.ccr >> 3) & 1;\
  u = (cpu.ccr >> 4) & 1;\
  h = (cpu.ccr >> 5) & 1;\
  ui = ((cpu.ccr >> 6) & 1);\
  intMaskBit = (cpu.ccr >> 7) & 1;

#define GETEXR()	    \
  if (h8300smode) { \
    trace = (cpu.exr >> 7) & 1;\
    intMask = cpu.exr & 7; }

#ifdef __CHAR_IS_SIGNED__
#define SEXTCHAR(x) ((char) (x))
#endif

#ifndef SEXTCHAR
#define SEXTCHAR(x) ((x & 0x80) ? (x | ~0xff): x & 0xff)
#endif

#define UEXTCHAR(x) ((x) & 0xff)
#define UEXTSHORT(x) ((x) & 0xffff)
#define SEXTSHORT(x) ((short) (x))

static cpu_state_type cpu;

int h8300hmode = 0;
int h8300smode = 0;

static int memory_size;

static int
get_now ()
{
  return time (0);	/* WinXX HAS UNIX like 'time', so why not using it? */
}

static int
now_persec ()
{
  return 1;
}

static int
bitfrom (x)
{
  switch (x & SIZE)
    {
    case L_8:
      return SB;
    case L_16:
      return SW;
    case L_32:
      return SL;
    case L_P:
      return h8300hmode ? SL : SW;
    }
}

static unsigned int
lvalue (x, rn)
{
  switch (x / 4)
    {
    case OP_DISP:
      if (rn == 8)
	{
	  return X (OP_IMM, SP);
	}
      return X (OP_REG, SP);

    case OP_MEM:
      return X (OP_MEM, SP);

    default:
      abort (); /* ?? May be something more usefull? */
    }
}

static unsigned int
decode (addr, data, dst)
     int addr;
     unsigned char *data;
     decoded_inst *dst;

{
  int rs = 0;
  int rd = 0;
  int rdisp = 0;
  int abs = 0;
  int bit = 0;
  int plen = 0;
  struct h8_opcode *q;
  int size = 0;

  dst->dst.type = -1;
  dst->src.type = -1;

  /* Find the exact opcode/arg combo.  */
  for (q = h8_opcodes; q->name; q++)
    {
      op_type *nib = q->data.nib;
      unsigned int len = 0;

      while (1)
	{
	  op_type looking_for = *nib;
	  int thisnib = data[len >> 1];

	  thisnib = (len & 1) ? (thisnib & 0xf) : ((thisnib >> 4) & 0xf);

	  if (looking_for < 16 && looking_for >= 0)
	    {
	      if (looking_for != thisnib)
		goto fail;
	    }
	  else
	    {
	      if ((int) looking_for & (int) B31)
		{
		  if (!(((int) thisnib & 0x8) != 0))
		    goto fail;

		  looking_for = (op_type) ((int) looking_for & ~(int) B31);
		  thisnib &= 0x7;
		}

	      if ((int) looking_for & (int) B30)
		{
		  if (!(((int) thisnib & 0x8) == 0))
		    goto fail;

		  looking_for = (op_type) ((int) looking_for & ~(int) B30);
		}

	      if (looking_for & DBIT)
		{
		  /* Exclude adds/subs by looking at bit 0 and 2, and
                     make sure the operand size, either w or l,
                     matches by looking at bit 1.  */
		  if ((looking_for & 7) != (thisnib & 7))
		    goto fail;

		  abs = (thisnib & 0x8) ? 2 : 1;
		}
	      else if (looking_for & (REG | IND | INC | DEC))
		{
		  if (looking_for & REG)
		    {
		      /* Can work out size from the register.  */
		      size = bitfrom (looking_for);
		    }
		  if (looking_for & SRC)
		    rs = thisnib;
		  else
		    rd = thisnib;
		}
	      else if (looking_for & L_16)
		{
		  abs = (data[len >> 1]) * 256 + data[(len + 2) >> 1];
		  plen = 16;
		  if (looking_for & (PCREL | DISP))
		    {
		      abs = (short) (abs);
		    }
		}
	      else if (looking_for & ABSJMP)
		{
		  abs = (data[1] << 16) | (data[2] << 8) | (data[3]);
		}
	      else if (looking_for & MEMIND)
		{
		  abs = data[1];
		}
	      else if (looking_for & L_32)
		{
		  int i = len >> 1;

		  abs = (data[i] << 24)
		    | (data[i + 1] << 16)
		    | (data[i + 2] << 8)
		    | (data[i + 3]);

		  plen = 32;
		}
	      else if (looking_for & L_24)
		{
		  int i = len >> 1;

		  abs = (data[i] << 16) | (data[i + 1] << 8) | (data[i + 2]);
		  plen = 24;
		}
	      else if (looking_for & IGNORE)
		{
		  ;
		}
	      else if (looking_for & DISPREG)
		{
		  rdisp = thisnib & 0x7;
		}
	      else if (looking_for & KBIT)
		{
		  switch (thisnib)
		    {
		    case 9:
		      abs = 4;
		      break;
		    case 8:
		      abs = 2;
		      break;
		    case 0:
		      abs = 1;
		      break;
		    default:
		      goto fail;
		    }
		}
	      else if (looking_for & L_8)
		{
		  plen = 8;

		  if (looking_for & PCREL)
		    {
		      abs = SEXTCHAR (data[len >> 1]);
		    }
		  else if (looking_for & ABS8MEM)
		    {
		      plen = 8;
		      abs = h8300hmode ? ~0xff0000ff : ~0xffff00ff;
		      abs |= data[len >> 1] & 0xff;
		    }
		  else
		    {
		      abs = data[len >> 1] & 0xff;
		    }
		}
	      else if (looking_for & L_3)
		{
		  plen = 3;

		  bit = thisnib;
		}
	      else if (looking_for == E)
		{
		  dst->op = q;

		  /* Fill in the args.  */
		  {
		    op_type *args = q->args.nib;
		    int hadone = 0;

		    while (*args != E)
		      {
			int x = *args;
			int rn = (x & DST) ? rd : rs;
			ea_type *p;

			if (x & DST)
			  p = &(dst->dst);
			else
			  p = &(dst->src);

			if (x & L_3)
			  {
			    p->type = X (OP_IMM, size);
			    p->literal = bit;
			  }
			else if (x & (IMM | KBIT | DBIT))
			  {
			    p->type = X (OP_IMM, size);
			    p->literal = abs;
			  }
			else if (x & REG)
			  {
			    /* Reset the size.
			       Some ops (like mul) have two sizes.  */

			    size = bitfrom (x);
			    p->type = X (OP_REG, size);
			    p->reg = rn;
			  }
			else if (x & INC)
			  {
			    p->type = X (OP_INC, size);
			    p->reg = rn & 0x7;
			  }
			else if (x & DEC)
			  {
			    p->type = X (OP_DEC, size);
			    p->reg = rn & 0x7;
			  }
			else if (x & IND)
			  {
			    p->type = X (OP_DISP, size);
			    p->reg = rn & 0x7;
			    p->literal = 0;
			  }
			else if (x & (ABS | ABSJMP | ABS8MEM))
			  {
			    p->type = X (OP_DISP, size);
			    p->literal = abs;
			    p->reg = 8;
			  }
			else if (x & MEMIND)
			  {
			    p->type = X (OP_MEM, size);
			    p->literal = abs;
			  }
			else if (x & PCREL)
			  {
			    p->type = X (OP_PCREL, size);
			    p->literal = abs + addr + 2;
			    if (x & L_16)
			      p->literal += 2;
			  }
			else if (x & ABSJMP)
			  {
			    p->type = X (OP_IMM, SP);
			    p->literal = abs;
			  }
			else if (x & DISP)
			  {
			    p->type = X (OP_DISP, size);
			    p->literal = abs;
			    p->reg = rdisp & 0x7;
			  }
			else if (x & CCR)
			  {
			    p->type = OP_CCR;
			  }
			else if (x & EXR)
			  {
			    p->type = OP_EXR;
			  }
			else
			  printf ("Hmmmm %x", x);

			args++;
		      }
		  }

		  /* But a jmp or a jsr gets automagically lvalued,
		     since we branch to their address not their
		     contents.  */
		  if (q->how == O (O_JSR, SB)
		      || q->how == O (O_JMP, SB))
		    {
		      dst->src.type = lvalue (dst->src.type, dst->src.reg);
		    }

		  if (dst->dst.type == -1)
		    dst->dst = dst->src;

		  dst->opcode = q->how;
		  dst->cycles = q->time;

		  /* And a jsr to 0xc4 is turned into a magic trap.  */

		  if (dst->opcode == O (O_JSR, SB))
		    {
		      if (dst->src.literal == 0xc4)
			{
			  dst->opcode = O (O_SYSCALL, SB);
			}
		    }

		  dst->next_pc = addr + len / 2;
		  return;
		}
	      else
		printf ("Don't understand %x \n", looking_for);
	    }

	  len++;
	  nib++;
	}

    fail:
      ;
    }

  /* Fell off the end.  */
  dst->opcode = O (O_ILL, SB);
}

static void
compile (pc)
{
  int idx;

  /* Find the next cache entry to use.  */
  idx = cpu.cache_top + 1;
  cpu.compiles++;
  if (idx >= cpu.csize)
    {
      idx = 1;
    }
  cpu.cache_top = idx;

  /* Throw away its old meaning.  */
  cpu.cache_idx[cpu.cache[idx].oldpc] = 0;

  /* Set to new address.  */
  cpu.cache[idx].oldpc = pc;

  /* Fill in instruction info.  */
  decode (pc, cpu.memory + pc, cpu.cache + idx);

  /* Point to new cache entry.  */
  cpu.cache_idx[pc] = idx;
}


static unsigned char *breg[18];
static unsigned short *wreg[18];
static unsigned int *lreg[18];

#define GET_B_REG(x) *(breg[x])
#define SET_B_REG(x,y) (*(breg[x])) = (y)
#define GET_W_REG(x) *(wreg[x])
#define SET_W_REG(x,y) (*(wreg[x])) = (y)

#define GET_L_REG(x) *(lreg[x])
#define SET_L_REG(x,y) (*(lreg[x])) = (y)

#define GET_MEMORY_L(x) \
  (x < memory_size \
   ? ((cpu.memory[x+0] << 24) | (cpu.memory[x+1] << 16) \
      | (cpu.memory[x+2] << 8) | cpu.memory[x+3]) \
   : ((cpu.eightbit[(x+0) & 0xff] << 24) | (cpu.eightbit[(x+1) & 0xff] << 16) \
      | (cpu.eightbit[(x+2) & 0xff] << 8) | cpu.eightbit[(x+3) & 0xff]))

#define GET_MEMORY_W(x) \
  (x < memory_size \
   ? ((cpu.memory[x+0] << 8) | (cpu.memory[x+1] << 0)) \
   : ((cpu.eightbit[(x+0) & 0xff] << 8) | (cpu.eightbit[(x+1) & 0xff] << 0)))


#define GET_MEMORY_B(x) \
  (x < memory_size ? (cpu.memory[x]) : (cpu.eightbit[x & 0xff]))

#define SET_MEMORY_L(x,y)  \
{  register unsigned char *_p; register int __y = y; \
   _p = (x < memory_size ? cpu.memory+x : cpu.eightbit + (x & 0xff)); \
   _p[0] = (__y)>>24; _p[1] = (__y)>>16; \
   _p[2] = (__y)>>8; _p[3] = (__y)>>0;}

#define SET_MEMORY_W(x,y) \
{  register unsigned char *_p; register int __y = y; \
   _p = (x < memory_size ? cpu.memory+x : cpu.eightbit + (x & 0xff)); \
   _p[0] = (__y)>>8; _p[1] =(__y);}

#define SET_MEMORY_B(x,y) \
  (x < memory_size ? (cpu.memory[(x)] = y) : (cpu.eightbit[x & 0xff] = y))

int
fetch (arg, n)
     ea_type *arg;
{
  int rn = arg->reg;
  int abs = arg->literal;
  int r;
  int t;

  switch (arg->type)
    {
    case X (OP_REG, SB):
      return GET_B_REG (rn);
    case X (OP_REG, SW):
      return GET_W_REG (rn);
    case X (OP_REG, SL):
      return GET_L_REG (rn);
    case X (OP_IMM, SB):
    case X (OP_IMM, SW):
    case X (OP_IMM, SL):
      return abs;
    case X (OP_DEC, SB):
      abort ();

    case X (OP_INC, SB):
      t = GET_L_REG (rn);
      t &= cpu.mask;
      r = GET_MEMORY_B (t);
      t++;
      t = t & cpu.mask;
      SET_L_REG (rn, t);
      return r;
      break;
    case X (OP_INC, SW):
      t = GET_L_REG (rn);
      t &= cpu.mask;
      r = GET_MEMORY_W (t);
      t += 2;
      t = t & cpu.mask;
      SET_L_REG (rn, t);
      return r;
    case X (OP_INC, SL):
      t = GET_L_REG (rn);
      t &= cpu.mask;
      r = GET_MEMORY_L (t);

      t += 4;
      t = t & cpu.mask;
      SET_L_REG (rn, t);
      return r;

    case X (OP_DISP, SB):
      t = GET_L_REG (rn) + abs;
      t &= cpu.mask;
      return GET_MEMORY_B (t);

    case X (OP_DISP, SW):
      t = GET_L_REG (rn) + abs;
      t &= cpu.mask;
      return GET_MEMORY_W (t);

    case X (OP_DISP, SL):
      t = GET_L_REG (rn) + abs;
      t &= cpu.mask;
      return GET_MEMORY_L (t);

    case X (OP_MEM, SL):
      t = GET_MEMORY_L (abs);
      t &= cpu.mask;
      return t;

    case X (OP_MEM, SW):
      t = GET_MEMORY_W (abs);
      t &= cpu.mask;
      return t;

    default:
      abort (); /* ?? May be something more usefull? */

    }
}


static void
store (arg, n)
     ea_type *arg;
     int n;
{
  int rn = arg->reg;
  int abs = arg->literal;
  int t;

  switch (arg->type)
    {
    case X (OP_REG, SB):
      SET_B_REG (rn, n);
      break;
    case X (OP_REG, SW):
      SET_W_REG (rn, n);
      break;
    case X (OP_REG, SL):
      SET_L_REG (rn, n);
      break;

    case X (OP_DEC, SB):
      t = GET_L_REG (rn) - 1;
      t &= cpu.mask;
      SET_L_REG (rn, t);
      SET_MEMORY_B (t, n);

      break;
    case X (OP_DEC, SW):
      t = (GET_L_REG (rn) - 2) & cpu.mask;
      SET_L_REG (rn, t);
      SET_MEMORY_W (t, n);
      break;

    case X (OP_DEC, SL):
      t = (GET_L_REG (rn) - 4) & cpu.mask;
      SET_L_REG (rn, t);
      SET_MEMORY_L (t, n);
      break;

    case X (OP_DISP, SB):
      t = GET_L_REG (rn) + abs;
      t &= cpu.mask;
      SET_MEMORY_B (t, n);
      break;

    case X (OP_DISP, SW):
      t = GET_L_REG (rn) + abs;
      t &= cpu.mask;
      SET_MEMORY_W (t, n);
      break;

    case X (OP_DISP, SL):
      t = GET_L_REG (rn) + abs;
      t &= cpu.mask;
      SET_MEMORY_L (t, n);
      break;
    default:
      abort ();
    }
}


static union
{
  short int i;
  struct
    {
      char low;
      char high;
    }
  u;
}

littleendian;

static void
init_pointers ()
{
  static int init;

  if (!init)
    {
      int i;

      init = 1;
      littleendian.i = 1;

      if (h8300smode)
	memory_size = H8300S_MSIZE;
      else if (h8300hmode)
	memory_size = H8300H_MSIZE;
      else
	memory_size = H8300_MSIZE;
      cpu.memory = (unsigned char *) calloc (sizeof (char), memory_size);
      cpu.cache_idx = (unsigned short *) calloc (sizeof (short), memory_size);
      cpu.eightbit = (unsigned char *) calloc (sizeof (char), 256);

      /* `msize' must be a power of two.  */
      if ((memory_size & (memory_size - 1)) != 0)
	abort ();
      cpu.mask = memory_size - 1;

      for (i = 0; i < 9; i++)
	{
	  cpu.regs[i] = 0;
	}

      for (i = 0; i < 8; i++)
	{
	  unsigned char *p = (unsigned char *) (cpu.regs + i);
	  unsigned char *e = (unsigned char *) (cpu.regs + i + 1);
	  unsigned short *q = (unsigned short *) (cpu.regs + i);
	  unsigned short *u = (unsigned short *) (cpu.regs + i + 1);
	  cpu.regs[i] = 0x00112233;
	  while (p < e)
	    {
	      if (*p == 0x22)
		{
		  breg[i] = p;
		}
	      if (*p == 0x33)
		{
		  breg[i + 8] = p;
		}
	      p++;
	    }
	  while (q < u)
	    {
	      if (*q == 0x2233)
		{
		  wreg[i] = q;
		}
	      if (*q == 0x0011)
		{
		  wreg[i + 8] = q;
		}
	      q++;
	    }
	  cpu.regs[i] = 0;
	  lreg[i] = &cpu.regs[i];
	}

      lreg[8] = &cpu.regs[8];

      /* Initialize the seg registers.  */
      if (!cpu.cache)
	sim_set_simcache_size (CSIZE);
    }
}

static void
control_c (sig, code, scp, addr)
     int sig;
     int code;
     char *scp;
     char *addr;
{
  cpu.state = SIM_STATE_STOPPED;
  cpu.exception = SIGINT;
}

#define C (c != 0)
#define Z (nz == 0)
#define V (v != 0)
#define N (n != 0)
#define U (u != 0)
#define H (h != 0)
#define UI (ui != 0)
#define I (intMaskBit != 0)

static int
mop (code, bsize, sign)
     decoded_inst *code;
     int bsize;
     int sign;
{
  int multiplier;
  int multiplicand;
  int result;
  int n, nz;

  if (sign)
    {
      multiplicand =
	bsize ? SEXTCHAR (GET_W_REG (code->dst.reg)) :
	SEXTSHORT (GET_W_REG (code->dst.reg));
      multiplier =
	bsize ? SEXTCHAR (GET_B_REG (code->src.reg)) :
	SEXTSHORT (GET_W_REG (code->src.reg));
    }
  else
    {
      multiplicand = bsize ? UEXTCHAR (GET_W_REG (code->dst.reg)) :
	UEXTSHORT (GET_W_REG (code->dst.reg));
      multiplier =
	bsize ? UEXTCHAR (GET_B_REG (code->src.reg)) :
	UEXTSHORT (GET_W_REG (code->src.reg));

    }
  result = multiplier * multiplicand;

  if (sign)
    {
      n = result & (bsize ? 0x8000 : 0x80000000);
      nz = result & (bsize ? 0xffff : 0xffffffff);
    }
  if (bsize)
    {
      SET_W_REG (code->dst.reg, result);
    }
  else
    {
      SET_L_REG (code->dst.reg, result);
    }
#if 0
  return ((n == 1) << 1) | (nz == 1);
#endif
}

#define ONOT(name, how) \
case O (name, SB):				\
{						\
  int t;					\
  int hm = 0x80;				\
  rd = GET_B_REG (code->src.reg);		\
  how; 						\
  goto shift8;					\
} 						\
case O (name, SW):				\
{ 						\
  int t;					\
  int hm = 0x8000;				\
  rd = GET_W_REG (code->src.reg); 		\
  how; 						\
  goto shift16;					\
} 						\
case O (name, SL):				\
{						\
  int t;					\
  int hm = 0x80000000; 				\
  rd = GET_L_REG (code->src.reg);		\
  how; 						\
  goto shift32;					\
}

#define OSHIFTS(name, how1, how2) \
case O (name, SB):				\
{						\
  int t;					\
  int hm = 0x80;				\
  rd = GET_B_REG (code->src.reg);		\
  if ((GET_MEMORY_B (pc + 1) & 0x40) == 0)	\
    {						\
      how1;					\
    }						\
  else						\
    {						\
      how2;					\
    }						\
  goto shift8;					\
} 						\
case O (name, SW):				\
{ 						\
  int t;					\
  int hm = 0x8000;				\
  rd = GET_W_REG (code->src.reg); 		\
  if ((GET_MEMORY_B (pc + 1) & 0x40) == 0)	\
    {						\
      how1;					\
    }						\
  else						\
    {						\
      how2;					\
    }						\
  goto shift16;					\
} 						\
case O (name, SL):				\
{						\
  int t;					\
  int hm = 0x80000000; 				\
  rd = GET_L_REG (code->src.reg);		\
  if ((GET_MEMORY_B (pc + 1) & 0x40) == 0)	\
    {						\
      how1;					\
    }						\
  else						\
    {						\
      how2;					\
    }						\
  goto shift32;					\
}

#define OBITOP(name,f, s, op) 			\
case O (name, SB):				\
{						\
  int m;					\
  int b; 					\
  if (f) ea = fetch (&code->dst);		\
  m=1<< fetch (&code->src);			\
  op;						\
  if (s) store (&code->dst,ea); goto next;	\
}

int
sim_stop (sd)
     SIM_DESC sd;
{
  cpu.state = SIM_STATE_STOPPED;
  cpu.exception = SIGINT;
  return 1;
}

#define R0_REGNUM	0
#define R1_REGNUM	1
#define R2_REGNUM	2
#define R3_REGNUM	3
#define R4_REGNUM	4
#define R5_REGNUM	5
#define R6_REGNUM	6
#define R7_REGNUM	7

#define SP_REGNUM       R7_REGNUM	/* Contains address of top of stack */
#define FP_REGNUM       R6_REGNUM	/* Contains address of executing
					 * stack frame */

#define CCR_REGNUM      8	/* Contains processor status */
#define PC_REGNUM       9	/* Contains program counter */

#define CYCLE_REGNUM    10

#define EXR_REGNUM	11
#define INST_REGNUM     12
#define TICK_REGNUM     13

void
sim_resume (sd, step, siggnal)
     SIM_DESC sd;
{
  static int init1;
  int cycles = 0;
  int insts = 0;
  int tick_start = get_now ();
  void (*prev) ();
  int poll_count = 0;
  int res;
  int tmp;
  int rd;
  int ea;
  int bit;
  int pc;
  int c, nz, v, n, u, h, ui, intMaskBit;
  int trace, intMask;
  int oldmask;
  init_pointers ();

  prev = signal (SIGINT, control_c);

  if (step)
    {
      cpu.state = SIM_STATE_STOPPED;
      cpu.exception = SIGTRAP;
    }
  else
    {
      cpu.state = SIM_STATE_RUNNING;
      cpu.exception = 0;
    }

  pc = cpu.pc;

  /* The PC should never be odd.  */
  if (pc & 0x1)
    abort ();

  GETSR ();
  GETEXR ();

  oldmask = cpu.mask;
  if (!h8300hmode)
    cpu.mask = 0xffff;
  do
    {
      int cidx;
      decoded_inst *code;

    top:
      cidx = cpu.cache_idx[pc];
      code = cpu.cache + cidx;


#define ALUOP(STORE, NAME, HOW) \
    case O (NAME,SB):  HOW; if (STORE)goto alu8;else goto just_flags_alu8;  \
    case O (NAME, SW): HOW; if (STORE)goto alu16;else goto just_flags_alu16; \
    case O (NAME,SL):  HOW; if (STORE)goto alu32;else goto just_flags_alu32;


#define LOGOP(NAME, HOW) \
    case O (NAME,SB): HOW; goto log8;\
    case O (NAME, SW): HOW; goto log16;\
    case O (NAME,SL): HOW; goto log32;



#if ADEBUG
      if (debug)
	{
	  printf ("%x %d %s\n", pc, code->opcode,
		  code->op ? code->op->name : "**");
	}
      cpu.stats[code->opcode]++;

#endif

      if (code->opcode)
	{
	  cycles += code->cycles;
	  insts++;
	}

      switch (code->opcode)
	{
	case 0:
	  /*
	   * This opcode is a fake for when we get to an
	   * instruction which hasnt been compiled
	   */
	  compile (pc);
	  goto top;
	  break;


	case O (O_SUBX, SB):
	  rd = fetch (&code->dst);
	  ea = fetch (&code->src);
	  ea = -(ea + C);
	  res = rd + ea;
	  goto alu8;

	case O (O_ADDX, SB):
	  rd = fetch (&code->dst);
	  ea = fetch (&code->src);
	  ea = C + ea;
	  res = rd + ea;
	  goto alu8;

#define EA    ea = fetch (&code->src);
#define RD_EA ea = fetch (&code->src); rd = fetch (&code->dst);

	  ALUOP (1, O_SUB, RD_EA;
		 ea = -ea;
		 res = rd + ea);
	  ALUOP (1, O_NEG, EA;
		 ea = -ea;
		 rd = 0;
		 res = rd + ea);

	case O (O_ADD, SB):
	  rd = GET_B_REG (code->dst.reg);
	  ea = fetch (&code->src);
	  res = rd + ea;
	  goto alu8;
	case O (O_ADD, SW):
	  rd = GET_W_REG (code->dst.reg);
	  ea = fetch (&code->src);
	  res = rd + ea;
	  goto alu16;
	case O (O_ADD, SL):
	  rd = GET_L_REG (code->dst.reg);
	  ea = fetch (&code->src);
	  res = rd + ea;
	  goto alu32;


	  LOGOP (O_AND, RD_EA;
		 res = rd & ea);

	  LOGOP (O_OR, RD_EA;
		 res = rd | ea);

	  LOGOP (O_XOR, RD_EA;
		 res = rd ^ ea);


	case O (O_MOV_TO_MEM, SB):
	  res = GET_B_REG (code->src.reg);
	  goto log8;
	case O (O_MOV_TO_MEM, SW):
	  res = GET_W_REG (code->src.reg);
	  goto log16;
	case O (O_MOV_TO_MEM, SL):
	  res = GET_L_REG (code->src.reg);
	  goto log32;


	case O (O_MOV_TO_REG, SB):
	  res = fetch (&code->src);
	  SET_B_REG (code->dst.reg, res);
	  goto just_flags_log8;
	case O (O_MOV_TO_REG, SW):
	  res = fetch (&code->src);
	  SET_W_REG (code->dst.reg, res);
	  goto just_flags_log16;
	case O (O_MOV_TO_REG, SL):
	  res = fetch (&code->src);
	  SET_L_REG (code->dst.reg, res);
	  goto just_flags_log32;

	case O (O_EEPMOV, SB):
	case O (O_EEPMOV, SW):
	  if (h8300hmode||h8300smode)
	    {
	      register unsigned char *_src,*_dst;
	      unsigned int count = (code->opcode == O(O_EEPMOV, SW))?cpu.regs[R4_REGNUM]&0xffff:
		cpu.regs[R4_REGNUM]&0xff;

	      _src = cpu.regs[R5_REGNUM] < memory_size ? cpu.memory+cpu.regs[R5_REGNUM] :
		cpu.eightbit + (cpu.regs[R5_REGNUM] & 0xff);
	      if ((_src+count)>=(cpu.memory+memory_size))
		{
		  if ((_src+count)>=(cpu.eightbit+0x100))
		    goto illegal;
		}
	      _dst = cpu.regs[R6_REGNUM] < memory_size ? cpu.memory+cpu.regs[R6_REGNUM] :
	           				       	      cpu.eightbit + (cpu.regs[R6_REGNUM] & 0xff);
	      if ((_dst+count)>=(cpu.memory+memory_size))
		{
		  if ((_dst+count)>=(cpu.eightbit+0x100))
		    goto illegal;
		}
	      memcpy(_dst,_src,count);

	      cpu.regs[R5_REGNUM]+=count;
	      cpu.regs[R6_REGNUM]+=count;
	      cpu.regs[R4_REGNUM]&=(code->opcode == O(O_EEPMOV, SW))?(~0xffff):(~0xff);
	      cycles += 2*count;
	      goto next;
	    }
	  goto illegal;

	case O (O_ADDS, SL):
	  SET_L_REG (code->dst.reg,
		     GET_L_REG (code->dst.reg)
		     + code->src.literal);

	  goto next;

	case O (O_SUBS, SL):
	  SET_L_REG (code->dst.reg,
		     GET_L_REG (code->dst.reg)
		     - code->src.literal);
	  goto next;

	case O (O_CMP, SB):
	  rd = fetch (&code->dst);
	  ea = fetch (&code->src);
	  ea = -ea;
	  res = rd + ea;
	  goto just_flags_alu8;

	case O (O_CMP, SW):
	  rd = fetch (&code->dst);
	  ea = fetch (&code->src);
	  ea = -ea;
	  res = rd + ea;
	  goto just_flags_alu16;

	case O (O_CMP, SL):
	  rd = fetch (&code->dst);
	  ea = fetch (&code->src);
	  ea = -ea;
	  res = rd + ea;
	  goto just_flags_alu32;


	case O (O_DEC, SB):
	  rd = GET_B_REG (code->src.reg);
	  ea = -1;
	  res = rd + ea;
	  SET_B_REG (code->src.reg, res);
	  goto just_flags_inc8;

	case O (O_DEC, SW):
	  rd = GET_W_REG (code->dst.reg);
	  ea = -code->src.literal;
	  res = rd + ea;
	  SET_W_REG (code->dst.reg, res);
	  goto just_flags_inc16;

	case O (O_DEC, SL):
	  rd = GET_L_REG (code->dst.reg);
	  ea = -code->src.literal;
	  res = rd + ea;
	  SET_L_REG (code->dst.reg, res);
	  goto just_flags_inc32;


	case O (O_INC, SB):
	  rd = GET_B_REG (code->src.reg);
	  ea = 1;
	  res = rd + ea;
	  SET_B_REG (code->src.reg, res);
	  goto just_flags_inc8;

	case O (O_INC, SW):
	  rd = GET_W_REG (code->dst.reg);
	  ea = code->src.literal;
	  res = rd + ea;
	  SET_W_REG (code->dst.reg, res);
	  goto just_flags_inc16;

	case O (O_INC, SL):
	  rd = GET_L_REG (code->dst.reg);
	  ea = code->src.literal;
	  res = rd + ea;
	  SET_L_REG (code->dst.reg, res);
	  goto just_flags_inc32;

#define GET_CCR(x) BUILDSR();x = cpu.ccr
#define GET_EXR(x) BUILDEXR ();x = cpu.exr

	case O (O_LDC, SB):
	case O (O_LDC, SW):
	  res = fetch (&code->src);
	  goto setc;
	case O (O_STC, SB):
	case O (O_STC, SW):
	  if (code->src.type == OP_CCR)
	    {
	      GET_CCR (res);
	    }
	  else if (code->src.type == OP_EXR && h8300smode)
	    {
	      GET_EXR (res);
	    }
	  else
	    goto illegal;
	  store (&code->dst, res);
	  goto next;

	case O (O_ANDC, SB):
	  if (code->dst.type == OP_CCR)
	    {
	      GET_CCR (rd);
	    }
	  else if (code->dst.type == OP_EXR && h8300smode)
	    {
	      GET_EXR (rd);
	    }
	  else
	    goto illegal;
	  ea = code->src.literal;
	  res = rd & ea;
	  goto setc;

	case O (O_ORC, SB):
	  if (code->dst.type == OP_CCR)
	    {
	      GET_CCR (rd);
	    }
	  else if (code->dst.type == OP_EXR && h8300smode)
	    {
	      GET_EXR (rd);
	    }
	  else
	    goto illegal;
	  ea = code->src.literal;
	  res = rd | ea;
	  goto setc;

	case O (O_XORC, SB):
	  if (code->dst.type == OP_CCR)
	    {
	      GET_CCR (rd);
	    }
	  else if (code->dst.type == OP_EXR && h8300smode)
	    {
	      GET_EXR (rd);
	    }
	  else
	    goto illegal;
	  ea = code->src.literal;
	  res = rd ^ ea;
	  goto setc;


	case O (O_BRA, SB):
	  if (1)
	    goto condtrue;
	  goto next;

	case O (O_BRN, SB):
	  if (0)
	    goto condtrue;
	  goto next;

	case O (O_BHI, SB):
	  if ((C || Z) == 0)
	    goto condtrue;
	  goto next;


	case O (O_BLS, SB):
	  if ((C || Z))
	    goto condtrue;
	  goto next;

	case O (O_BCS, SB):
	  if ((C == 1))
	    goto condtrue;
	  goto next;

	case O (O_BCC, SB):
	  if ((C == 0))
	    goto condtrue;
	  goto next;

	case O (O_BEQ, SB):
	  if (Z)
	    goto condtrue;
	  goto next;
	case O (O_BGT, SB):
	  if (((Z || (N ^ V)) == 0))
	    goto condtrue;
	  goto next;


	case O (O_BLE, SB):
	  if (((Z || (N ^ V)) == 1))
	    goto condtrue;
	  goto next;

	case O (O_BGE, SB):
	  if ((N ^ V) == 0)
	    goto condtrue;
	  goto next;
	case O (O_BLT, SB):
	  if ((N ^ V))
	    goto condtrue;
	  goto next;
	case O (O_BMI, SB):
	  if ((N))
	    goto condtrue;
	  goto next;
	case O (O_BNE, SB):
	  if ((Z == 0))
	    goto condtrue;
	  goto next;

	case O (O_BPL, SB):
	  if (N == 0)
	    goto condtrue;
	  goto next;
	case O (O_BVC, SB):
	  if ((V == 0))
	    goto condtrue;
	  goto next;
	case O (O_BVS, SB):
	  if ((V == 1))
	    goto condtrue;
	  goto next;

	case O (O_SYSCALL, SB):
	  {
	    char c = cpu.regs[2];
	    sim_callback->write_stdout (sim_callback, &c, 1);
	  }
	  goto next;

	  ONOT (O_NOT, rd = ~rd; v = 0;);
	  OSHIFTS (O_SHLL,
		   c = rd & hm; v = 0; rd <<= 1,
		   c = rd & (hm >> 1); v = 0; rd <<= 2);
	  OSHIFTS (O_SHLR,
		   c = rd & 1; v = 0; rd = (unsigned int) rd >> 1,
		   c = rd & 2; v = 0; rd = (unsigned int) rd >> 2);
	  OSHIFTS (O_SHAL,
		   c = rd & hm; v = (rd & hm) != ((rd & (hm >> 1)) << 1); rd <<= 1,
		   c = rd & (hm >> 1); v = (rd & (hm >> 1)) != ((rd & (hm >> 2)) << 2); rd <<= 2);
	  OSHIFTS (O_SHAR,
		   t = rd & hm; c = rd & 1; v = 0; rd >>= 1; rd |= t,
		   t = rd & hm; c = rd & 2; v = 0; rd >>= 2; rd |= t | t >> 1);
	  OSHIFTS (O_ROTL,
		   c = rd & hm; v = 0; rd <<= 1; rd |= C,
		   c = rd & hm; v = 0; rd <<= 1; rd |= C; c = rd & hm; rd <<= 1; rd |= C);
	  OSHIFTS (O_ROTR,
		   c = rd & 1; v = 0; rd = (unsigned int) rd >> 1; if (c) rd |= hm,
		   c = rd & 1; v = 0; rd = (unsigned int) rd >> 1; if (c) rd |= hm; c = rd & 1; rd = (unsigned int) rd >> 1; if (c) rd |= hm);
	  OSHIFTS (O_ROTXL,
		   t = rd & hm; rd <<= 1; rd |= C; c = t; v = 0,
		   t = rd & hm; rd <<= 1; rd |= C; c = t; v = 0; t = rd & hm; rd <<= 1; rd |= C; c = t);
	  OSHIFTS (O_ROTXR,
		   t = rd & 1; rd = (unsigned int) rd >> 1; if (C) rd |= hm; c = t; v = 0,
		   t = rd & 1; rd = (unsigned int) rd >> 1; if (C) rd |= hm; c = t; v = 0; t = rd & 1; rd = (unsigned int) rd >> 1; if (C) rd |= hm; c = t);

	case O (O_JMP, SB):
	  {
	    pc = fetch (&code->src);
	    goto end;

	  }

	case O (O_JSR, SB):
	  {
	    int tmp;
	    pc = fetch (&code->src);
	  call:
	    tmp = cpu.regs[7];

	    if (h8300hmode)
	      {
		tmp -= 4;
		SET_MEMORY_L (tmp, code->next_pc);
	      }
	    else
	      {
		tmp -= 2;
		SET_MEMORY_W (tmp, code->next_pc);
	      }
	    cpu.regs[7] = tmp;

	    goto end;
	  }
	case O (O_BSR, SB):
	  pc = code->src.literal;
	  goto call;

	case O (O_RTS, SN):
	  {
	    int tmp;

	    tmp = cpu.regs[7];

	    if (h8300hmode)
	      {
		pc = GET_MEMORY_L (tmp);
		tmp += 4;
	      }
	    else
	      {
		pc = GET_MEMORY_W (tmp);
		tmp += 2;
	      }

	    cpu.regs[7] = tmp;
	    goto end;
	  }

	case O (O_ILL, SB):
	  cpu.state = SIM_STATE_STOPPED;
	  cpu.exception = SIGILL;
	  goto end;
	case O (O_SLEEP, SN):
	  /* FIXME: Doesn't this break for breakpoints when r0
	     contains just the right (er, wrong) value?  */
	  cpu.state = SIM_STATE_STOPPED;
	  /* The format of r0 is defined by target newlib.  Expand
             the macros here instead of looking for .../sys/wait.h.  */
#define SIM_WIFEXITED(v) (((v) & 0xff) == 0)
#define SIM_WIFSIGNALED(v) (((v) & 0x7f) > 0 && (((v) & 0x7f) < 0x7f))
  	  if (! SIM_WIFEXITED (cpu.regs[0]) && SIM_WIFSIGNALED (cpu.regs[0]))
	    cpu.exception = SIGILL;
	  else
	    cpu.exception = SIGTRAP;
	  goto end;
	case O (O_BPT, SN):
	  cpu.state = SIM_STATE_STOPPED;
	  cpu.exception = SIGTRAP;
	  goto end;

	  OBITOP (O_BNOT, 1, 1, ea ^= m);
	  OBITOP (O_BTST, 1, 0, nz = ea & m);
	  OBITOP (O_BCLR, 1, 1, ea &= ~m);
	  OBITOP (O_BSET, 1, 1, ea |= m);
	  OBITOP (O_BLD, 1, 0, c = ea & m);
	  OBITOP (O_BILD, 1, 0, c = !(ea & m));
	  OBITOP (O_BST, 1, 1, ea &= ~m;
		  if (C) ea |= m);
	  OBITOP (O_BIST, 1, 1, ea &= ~m;
		  if (!C) ea |= m);
	  OBITOP (O_BAND, 1, 0, c = (ea & m) && C);
	  OBITOP (O_BIAND, 1, 0, c = !(ea & m) && C);
	  OBITOP (O_BOR, 1, 0, c = (ea & m) || C);
	  OBITOP (O_BIOR, 1, 0, c = !(ea & m) || C);
	  OBITOP (O_BXOR, 1, 0, c = (ea & m) != C);
	  OBITOP (O_BIXOR, 1, 0, c = !(ea & m) != C);

#define MOP(bsize, signed)			\
  mop (code, bsize, signed);			\
  goto next;

	case O (O_MULS, SB):
	  MOP (1, 1);
	  break;
	case O (O_MULS, SW):
	  MOP (0, 1);
	  break;
	case O (O_MULU, SB):
	  MOP (1, 0);
	  break;
	case O (O_MULU, SW):
	  MOP (0, 0);
	  break;

	case O (O_TAS, SB):
	  if (!h8300smode || code->src.type != X (OP_REG, SL))
	    goto illegal;
	  switch (code->src.reg)
	    {
	    case R0_REGNUM:
	    case R1_REGNUM:
	    case R4_REGNUM:
	    case R5_REGNUM:
	      break;
	    default:
	      goto illegal;
	    }
	  res = fetch (&code->src);
	  store (&code->src,res|0x80);
	  goto just_flags_log8;

	case O (O_DIVU, SB):
	  {
	    rd = GET_W_REG (code->dst.reg);
	    ea = GET_B_REG (code->src.reg);
	    if (ea)
	      {
		tmp = (unsigned) rd % ea;
		rd = (unsigned) rd / ea;
	      }
	    SET_W_REG (code->dst.reg, (rd & 0xff) | (tmp << 8));
	    n = ea & 0x80;
	    nz = ea & 0xff;

	    goto next;
	  }
	case O (O_DIVU, SW):
	  {
	    rd = GET_L_REG (code->dst.reg);
	    ea = GET_W_REG (code->src.reg);
	    n = ea & 0x8000;
	    nz = ea & 0xffff;
	    if (ea)
	      {
		tmp = (unsigned) rd % ea;
		rd = (unsigned) rd / ea;
	      }
	    SET_L_REG (code->dst.reg, (rd & 0xffff) | (tmp << 16));
	    goto next;
	  }

	case O (O_DIVS, SB):
	  {

	    rd = SEXTSHORT (GET_W_REG (code->dst.reg));
	    ea = SEXTCHAR (GET_B_REG (code->src.reg));
	    if (ea)
	      {
		tmp = (int) rd % (int) ea;
		rd = (int) rd / (int) ea;
		n = rd & 0x8000;
		nz = 1;
	      }
	    else
	      nz = 0;
	    SET_W_REG (code->dst.reg, (rd & 0xff) | (tmp << 8));
	    goto next;
	  }
	case O (O_DIVS, SW):
	  {
	    rd = GET_L_REG (code->dst.reg);
	    ea = SEXTSHORT (GET_W_REG (code->src.reg));
	    if (ea)
	      {
		tmp = (int) rd % (int) ea;
		rd = (int) rd / (int) ea;
		n = rd & 0x80000000;
		nz = 1;
	      }
	    else
	      nz = 0;
	    SET_L_REG (code->dst.reg, (rd & 0xffff) | (tmp << 16));
	    goto next;
	  }
	case O (O_EXTS, SW):
	  rd = GET_B_REG (code->src.reg + 8) & 0xff; /* Yes, src, not dst.  */
	  ea = rd & 0x80 ? -256 : 0;
	  res = rd + ea;
	  goto log16;
	case O (O_EXTS, SL):
	  rd = GET_W_REG (code->src.reg) & 0xffff;
	  ea = rd & 0x8000 ? -65536 : 0;
	  res = rd + ea;
	  goto log32;
	case O (O_EXTU, SW):
	  rd = GET_B_REG (code->src.reg + 8) & 0xff;
	  ea = 0;
	  res = rd + ea;
	  goto log16;
	case O (O_EXTU, SL):
	  rd = GET_W_REG (code->src.reg) & 0xffff;
	  ea = 0;
	  res = rd + ea;
	  goto log32;

	case O (O_NOP, SN):
	  goto next;

	case O (O_STM, SL):
	  {
	    int nregs, firstreg, i;

	    nregs = GET_MEMORY_B (pc + 1);
	    nregs >>= 4;
	    nregs &= 0xf;
	    firstreg = GET_MEMORY_B (pc + 3);
	    firstreg &= 0xf;
	    for (i = firstreg; i <= firstreg + nregs; i++)
	      {
		cpu.regs[7] -= 4;
		SET_MEMORY_L (cpu.regs[7], cpu.regs[i]);
	      }
	  }
	  goto next;

	case O (O_LDM, SL):
	  {
	    int nregs, firstreg, i;

	    nregs = GET_MEMORY_B (pc + 1);
	    nregs >>= 4;
	    nregs &= 0xf;
	    firstreg = GET_MEMORY_B (pc + 3);
	    firstreg &= 0xf;
	    for (i = firstreg; i >= firstreg - nregs; i--)
	      {
		cpu.regs[i] = GET_MEMORY_L (cpu.regs[7]);
		cpu.regs[7] += 4;
	      }
	  }
	  goto next;

	default:
        illegal:
	  cpu.state = SIM_STATE_STOPPED;
	  cpu.exception = SIGILL;
	  goto end;

	}
      abort ();

    setc:
      if (code->dst.type == OP_CCR)
	{
	  cpu.ccr = res;
	  GETSR ();
	}
      else if (code->dst.type == OP_EXR && h8300smode)
	{
	  cpu.exr = res;
	  GETEXR ();
	}
      else
	goto illegal;

      goto next;

    condtrue:
      /* When a branch works */
      pc = code->src.literal;
      goto end;

      /* Set the cond codes from res */
    bitop:

      /* Set the flags after an 8 bit inc/dec operation */
    just_flags_inc8:
      n = res & 0x80;
      nz = res & 0xff;
      v = (rd & 0x7f) == 0x7f;
      goto next;


      /* Set the flags after an 16 bit inc/dec operation */
    just_flags_inc16:
      n = res & 0x8000;
      nz = res & 0xffff;
      v = (rd & 0x7fff) == 0x7fff;
      goto next;


      /* Set the flags after an 32 bit inc/dec operation */
    just_flags_inc32:
      n = res & 0x80000000;
      nz = res & 0xffffffff;
      v = (rd & 0x7fffffff) == 0x7fffffff;
      goto next;


    shift8:
      /* Set flags after an 8 bit shift op, carry,overflow set in insn */
      n = (rd & 0x80);
      nz = rd & 0xff;
      SET_B_REG (code->src.reg, rd);
      goto next;

    shift16:
      /* Set flags after an 16 bit shift op, carry,overflow set in insn */
      n = (rd & 0x8000);
      nz = rd & 0xffff;
      SET_W_REG (code->src.reg, rd);
      goto next;

    shift32:
      /* Set flags after an 32 bit shift op, carry,overflow set in insn */
      n = (rd & 0x80000000);
      nz = rd & 0xffffffff;
      SET_L_REG (code->src.reg, rd);
      goto next;

    log32:
      store (&code->dst, res);
    just_flags_log32:
      /* flags after a 32bit logical operation */
      n = res & 0x80000000;
      nz = res & 0xffffffff;
      v = 0;
      goto next;

    log16:
      store (&code->dst, res);
    just_flags_log16:
      /* flags after a 16bit logical operation */
      n = res & 0x8000;
      nz = res & 0xffff;
      v = 0;
      goto next;


    log8:
      store (&code->dst, res);
    just_flags_log8:
      n = res & 0x80;
      nz = res & 0xff;
      v = 0;
      goto next;

    alu8:
      SET_B_REG (code->dst.reg, res);
    just_flags_alu8:
      n = res & 0x80;
      nz = res & 0xff;
      c = (res & 0x100);
      switch (code->opcode / 4)
	{
	case O_ADD:
	  v = ((rd & 0x80) == (ea & 0x80)
	       && (rd & 0x80) != (res & 0x80));
	  break;
	case O_SUB:
	case O_CMP:
	  v = ((rd & 0x80) != (-ea & 0x80)
	       && (rd & 0x80) != (res & 0x80));
	  break;
	case O_NEG:
	  v = (rd == 0x80);
	  break;
	}
      goto next;

    alu16:
      SET_W_REG (code->dst.reg, res);
    just_flags_alu16:
      n = res & 0x8000;
      nz = res & 0xffff;
      c = (res & 0x10000);
      switch (code->opcode / 4)
	{
	case O_ADD:
	  v = ((rd & 0x8000) == (ea & 0x8000)
	       && (rd & 0x8000) != (res & 0x8000));
	  break;
	case O_SUB:
	case O_CMP:
	  v = ((rd & 0x8000) != (-ea & 0x8000)
	       && (rd & 0x8000) != (res & 0x8000));
	  break;
	case O_NEG:
	  v = (rd == 0x8000);
	  break;
	}
      goto next;

    alu32:
      SET_L_REG (code->dst.reg, res);
    just_flags_alu32:
      n = res & 0x80000000;
      nz = res & 0xffffffff;
      switch (code->opcode / 4)
	{
	case O_ADD:
	  v = ((rd & 0x80000000) == (ea & 0x80000000)
	       && (rd & 0x80000000) != (res & 0x80000000));
	  c = ((unsigned) res < (unsigned) rd) || ((unsigned) res < (unsigned) ea);
	  break;
	case O_SUB:
	case O_CMP:
	  v = ((rd & 0x80000000) != (-ea & 0x80000000)
	       && (rd & 0x80000000) != (res & 0x80000000));
	  c = (unsigned) rd < (unsigned) -ea;
	  break;
	case O_NEG:
	  v = (rd == 0x80000000);
	  c = res != 0;
	  break;
	}
      goto next;

    next:;
      pc = code->next_pc;

    end:
      ;
#if 0
      if (cpu.regs[8])
	abort ();
#endif

      if (--poll_count < 0)
	{
	  poll_count = POLL_QUIT_INTERVAL;
	  if ((*sim_callback->poll_quit) != NULL
	      && (*sim_callback->poll_quit) (sim_callback))
	    sim_stop (sd);
	}

    }
  while (cpu.state == SIM_STATE_RUNNING);
  cpu.ticks += get_now () - tick_start;
  cpu.cycles += cycles;
  cpu.insts += insts;

  cpu.pc = pc;
  BUILDSR ();
  BUILDEXR ();
  cpu.mask = oldmask;
  signal (SIGINT, prev);
}

int
sim_trace (sd)
     SIM_DESC sd;
{
  /* FIXME: Unfinished.  */
  abort ();
}

int
sim_write (sd, addr, buffer, size)
     SIM_DESC sd;
     SIM_ADDR addr;
     unsigned char *buffer;
     int size;
{
  int i;

  init_pointers ();
  if (addr < 0)
    return 0;
  for (i = 0; i < size; i++)
    {
      if (addr < memory_size)
	{
	  cpu.memory[addr + i] = buffer[i];
	  cpu.cache_idx[addr + i] = 0;
	}
      else
	cpu.eightbit[(addr + i) & 0xff] = buffer[i];
    }
  return size;
}

int
sim_read (sd, addr, buffer, size)
     SIM_DESC sd;
     SIM_ADDR addr;
     unsigned char *buffer;
     int size;
{
  init_pointers ();
  if (addr < 0)
    return 0;
  if (addr < memory_size)
    memcpy (buffer, cpu.memory + addr, size);
  else
    memcpy (buffer, cpu.eightbit + (addr & 0xff), size);
  return size;
}


int
sim_store_register (sd, rn, value, length)
     SIM_DESC sd;
     int rn;
     unsigned char *value;
     int length;
{
  int longval;
  int shortval;
  int intval;
  longval = (value[0] << 24) | (value[1] << 16) | (value[2] << 8) | value[3];
  shortval = (value[0] << 8) | (value[1]);
  intval = h8300hmode ? longval : shortval;

  init_pointers ();
  switch (rn)
    {
    case PC_REGNUM:
      cpu.pc = intval;
      break;
    default:
      abort ();
    case R0_REGNUM:
    case R1_REGNUM:
    case R2_REGNUM:
    case R3_REGNUM:
    case R4_REGNUM:
    case R5_REGNUM:
    case R6_REGNUM:
    case R7_REGNUM:
      cpu.regs[rn] = intval;
      break;
    case CCR_REGNUM:
      cpu.ccr = intval;
      break;
    case EXR_REGNUM:
      cpu.exr = intval;
      break;
    case CYCLE_REGNUM:
      cpu.cycles = longval;
      break;

    case INST_REGNUM:
      cpu.insts = longval;
      break;

    case TICK_REGNUM:
      cpu.ticks = longval;
      break;
    }
  return -1;
}

int
sim_fetch_register (sd, rn, buf, length)
     SIM_DESC sd;
     int rn;
     unsigned char *buf;
     int length;
{
  int v;
  int longreg = 0;

  init_pointers ();

  if (!h8300smode && rn >=EXR_REGNUM)
    rn++;
  switch (rn)
    {
    default:
      abort ();
    case CCR_REGNUM:
      v = cpu.ccr;
      break;
    case EXR_REGNUM:
      v = cpu.exr;
      break;
    case PC_REGNUM:
      v = cpu.pc;
      break;
    case R0_REGNUM:
    case R1_REGNUM:
    case R2_REGNUM:
    case R3_REGNUM:
    case R4_REGNUM:
    case R5_REGNUM:
    case R6_REGNUM:
    case R7_REGNUM:
      v = cpu.regs[rn];
      break;
    case CYCLE_REGNUM:
      v = cpu.cycles;
      longreg = 1;
      break;
    case TICK_REGNUM:
      v = cpu.ticks;
      longreg = 1;
      break;
    case INST_REGNUM:
      v = cpu.insts;
      longreg = 1;
      break;
    }
  if (h8300hmode || longreg)
    {
      buf[0] = v >> 24;
      buf[1] = v >> 16;
      buf[2] = v >> 8;
      buf[3] = v >> 0;
    }
  else
    {
      buf[0] = v >> 8;
      buf[1] = v;
    }
  return -1;
}

void
sim_stop_reason (sd, reason, sigrc)
     SIM_DESC sd;
     enum sim_stop *reason;
     int *sigrc;
{
#if 0 /* FIXME: This should work but we can't use it.
	 grep for SLEEP above.  */
  switch (cpu.state)
    {
    case SIM_STATE_EXITED : *reason = sim_exited; break;
    case SIM_STATE_SIGNALLED : *reason = sim_signalled; break;
    case SIM_STATE_STOPPED : *reason = sim_stopped; break;
    default : abort ();
    }
#else
  *reason = sim_stopped;
#endif
  *sigrc = cpu.exception;
}

/* FIXME: Rename to sim_set_mem_size.  */

void
sim_size (n)
     int n;
{
  /* Memory size is fixed.  */
}

void
sim_set_simcache_size (n)
{
  if (cpu.cache)
    free (cpu.cache);
  if (n < 2)
    n = 2;
  cpu.cache = (decoded_inst *) malloc (sizeof (decoded_inst) * n);
  memset (cpu.cache, 0, sizeof (decoded_inst) * n);
  cpu.csize = n;
}


void
sim_info (sd, verbose)
     SIM_DESC sd;
     int verbose;
{
  double timetaken = (double) cpu.ticks / (double) now_persec ();
  double virttime = cpu.cycles / 10.0e6;

  (*sim_callback->printf_filtered) (sim_callback,
				    "\n\n#instructions executed  %10d\n",
				    cpu.insts);
  (*sim_callback->printf_filtered) (sim_callback,
				    "#cycles (v approximate) %10d\n",
				    cpu.cycles);
  (*sim_callback->printf_filtered) (sim_callback,
				    "#real time taken        %10.4f\n",
				    timetaken);
  (*sim_callback->printf_filtered) (sim_callback,
				    "#virtual time taked     %10.4f\n",
				    virttime);
  if (timetaken != 0.0)
    (*sim_callback->printf_filtered) (sim_callback,
				      "#simulation ratio       %10.4f\n",
				      virttime / timetaken);
  (*sim_callback->printf_filtered) (sim_callback,
				    "#compiles               %10d\n",
				    cpu.compiles);
  (*sim_callback->printf_filtered) (sim_callback,
				    "#cache size             %10d\n",
				    cpu.csize);

#ifdef ADEBUG
  /* This to be conditional on `what' (aka `verbose'),
     however it was never passed as non-zero.  */
  if (1)
    {
      int i;
      for (i = 0; i < O_LAST; i++)
	{
	  if (cpu.stats[i])
	    (*sim_callback->printf_filtered) (sim_callback,
					      "%d: %d\n", i, cpu.stats[i]);
	}
    }
#endif
}

/* Indicate whether the cpu is an H8/300 or H8/300H.
   FLAG is non-zero for the H8/300H.  */

void
set_h8300h (h_flag, s_flag)
     int h_flag, s_flag;
{
  /* FIXME: Much of the code in sim_load can be moved to sim_open.
     This function being replaced by a sim_open:ARGV configuration
     option.  */
  h8300hmode = h_flag;
  h8300smode = s_flag;
}

SIM_DESC
sim_open (kind, ptr, abfd, argv)
     SIM_OPEN_KIND kind;
     struct host_callback_struct *ptr;
     struct _bfd *abfd;
     char **argv;
{
  /* FIXME: Much of the code in sim_load can be moved here.  */

  sim_kind = kind;
  myname = argv[0];
  sim_callback = ptr;
  /* Fudge our descriptor.  */
  return (SIM_DESC) 1;
}

void
sim_close (sd, quitting)
     SIM_DESC sd;
     int quitting;
{
  /* Nothing to do.  */
}

/* Called by gdb to load a program into memory.  */

SIM_RC
sim_load (sd, prog, abfd, from_tty)
     SIM_DESC sd;
     char *prog;
     bfd *abfd;
     int from_tty;
{
  bfd *prog_bfd;

  /* FIXME: The code below that sets a specific variant of the H8/300
     being simulated should be moved to sim_open().  */

  /* See if the file is for the H8/300 or H8/300H.  */
  /* ??? This may not be the most efficient way.  The z8k simulator
     does this via a different mechanism (INIT_EXTRA_SYMTAB_INFO).  */
  if (abfd != NULL)
    prog_bfd = abfd;
  else
    prog_bfd = bfd_openr (prog, "coff-h8300");
  if (prog_bfd != NULL)
    {
      /* Set the cpu type.  We ignore failure from bfd_check_format
	 and bfd_openr as sim_load_file checks too.  */
      if (bfd_check_format (prog_bfd, bfd_object))
	{
	  unsigned long mach = bfd_get_mach (prog_bfd);
	  set_h8300h (mach == bfd_mach_h8300h || mach == bfd_mach_h8300s,
		      mach == bfd_mach_h8300s);
	}
    }

  /* If we're using gdb attached to the simulator, then we have to
     reallocate memory for the simulator.

     When gdb first starts, it calls fetch_registers (among other
     functions), which in turn calls init_pointers, which allocates
     simulator memory.

     The problem is when we do that, we don't know whether we're
     debugging an H8/300 or H8/300H program.

     This is the first point at which we can make that determination,
     so we just reallocate memory now; this will also allow us to handle
     switching between H8/300 and H8/300H programs without exiting
     gdb.  */

  if (h8300smode)
    memory_size = H8300S_MSIZE;
  else if (h8300hmode)
    memory_size = H8300H_MSIZE;
  else
    memory_size = H8300_MSIZE;

  if (cpu.memory)
    free (cpu.memory);
  if (cpu.cache_idx)
    free (cpu.cache_idx);
  if (cpu.eightbit)
    free (cpu.eightbit);

  cpu.memory = (unsigned char *) calloc (sizeof (char), memory_size);
  cpu.cache_idx = (unsigned short *) calloc (sizeof (short), memory_size);
  cpu.eightbit = (unsigned char *) calloc (sizeof (char), 256);

  /* `msize' must be a power of two.  */
  if ((memory_size & (memory_size - 1)) != 0)
    abort ();
  cpu.mask = memory_size - 1;

  if (sim_load_file (sd, myname, sim_callback, prog, prog_bfd,
		     sim_kind == SIM_OPEN_DEBUG,
		     0, sim_write)
      == NULL)
    {
      /* Close the bfd if we opened it.  */
      if (abfd == NULL && prog_bfd != NULL)
	bfd_close (prog_bfd);
      return SIM_RC_FAIL;
    }

  /* Close the bfd if we opened it.  */
  if (abfd == NULL && prog_bfd != NULL)
    bfd_close (prog_bfd);
  return SIM_RC_OK;
}

SIM_RC
sim_create_inferior (sd, abfd, argv, env)
     SIM_DESC sd;
     struct _bfd *abfd;
     char **argv;
     char **env;
{
  if (abfd != NULL)
    cpu.pc = bfd_get_start_address (abfd);
  else
    cpu.pc = 0;
  return SIM_RC_OK;
}

void
sim_do_command (sd, cmd)
     SIM_DESC sd;
     char *cmd;
{
  (*sim_callback->printf_filtered) (sim_callback,
				    "This simulator does not accept any commands.\n");
}

void
sim_set_callbacks (ptr)
     struct host_callback_struct *ptr;
{
  sim_callback = ptr;
}
