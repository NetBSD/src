/*	$NetBSD: ieee_handler.c,v 1.11.4.1 2002/06/23 17:39:05 jdolecek Exp $	*/

/* 
 * IEEE floating point support for NS32081 and NS32381 fpus.
 * Copyright (c) 1995 Ian Dall
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * IAN DALL ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION.
 * IAN DALL DISCLAIMS ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */
/* 
 *	File:	ieee_handler.c
 *	Author:	Ian Dall
 *	Date:	November 1995
 *
 *	Entry points for the ieee exception handling package.
 *      decodes floating point instructions.
 *
 * HISTORY
 * 23-Apr-96  Ian Dall (Ian.Dall@dsto.defence.gov.au)
 *	Advance pc past trapping instruction always.
 *
 * 13-Apr-96  Matthias Pfaller <leo@dachau.marco.de>
 *	Fix up test for denormalized result in canonical_to_size().
 *
 * 13-Apr-96  Ian Dall (Ian.Dall@dsto.defence.gov.au)
 *	Clean up types in get_operand().
 *
 * 13-Apr-96 Matthias Pfaller <leo@dachau.marco.de>
 *      Remove redundant status test code from fetch_data(). Fix
 *      register relative and PC relative address decoding.
 *
 * 02-Apr-96  Ian Dall (Ian.Dall@dsto.defence.gov.au)
 *	Make zero floats get canonicalized to doubles properly.
 *
 * 02-Apr-96  Ian Dall (Ian.Dall@dsto.defence.gov.au)
 *	Notice that default_trap_handle() has a side effect of changing
 *	state->FSR.
 *
 * 02-Apr-96  Matthias Pfaller <leo@dachau.marco.de>
 *	Add NetBSD kernel support.
 *
 * 14-Dec-95  Ian Dall (Ian.Dall@dsto.defence.gov.au)
 *	First release.
 * */

#include <sys/types.h>
#include "ieee_internal.h"

#if defined(KERNEL) || defined(_KERNEL)
# ifdef MACH
#  include <setjmp.h>
#  define setjmp _setjmp
#  define longjmp _longjmp
#  define AT	vaddr_t
# elif defined(__NetBSD__)	/* MACH */
#  include <machine/param.h>
#  include <sys/systm.h>
#  define longjmp(x, y) longjmp(&x)
#  define setjmp(x) setjmp(&x)
#  define AT	void *
# endif
# define get_dword(addr) ({long _t; COPYIN((addr), (vaddr_t) &_t, sizeof(long)); _t;})
#else /* KERNEL */
# include <stddef.h>
# include <machine/param.h>
# include <setjmp.h>
# define AT	vaddr_t
# ifdef DEBUG
# include <stdio.h>
# endif

# define copyin(u,k,n) ({vaddr_t _u = (u), _k = (k); memcpy((char *) _k, (char *) _u,n);0;})
# define copyout(k,u,n) ({vaddr_t _u = (u), _k = (k); memcpy((char *) _u, (char *) _k,n);0;})
# define get_dword(addr) (* (unsigned int *) addr)

#ifdef MACH
/* Mach only defines this if KERNEL, NetBSD defines it always */
# define ns532_round_page(addr) (((addr) + NBPG - 1) & ~(NBPG - 1))
#endif


static void get_fstate(state *state) {
  asm("sfsr %0" : "=g" (state->FSR));
  asm("movl f0, %0" : "=m" (state->LREG(0)));
  asm("movl f1, %0" : "=m" (state->LREG(1)));
  asm("movl f2, %0" : "=m" (state->LREG(2)));
  asm("movl f3, %0" : "=m" (state->LREG(3)));
  asm("movl f4, %0" : "=m" (state->LREG(4)));
  asm("movl f5, %0" : "=m" (state->LREG(5)));
  asm("movl f6, %0" : "=m" (state->LREG(6)));
  asm("movl f7, %0" : "=m" (state->LREG(7)));
}

static void set_fstate(state *state) {
  /* DON'T tell gcc we are clobbering fp registers, else it will
   * save and restore some, undoing our changes!
   */
  asm("lfsr %0":: "g" (state->FSR));
  asm("movl %0, f0":: "m" (state->LREG(0)));
  asm("movl %0, f1":: "m" (state->LREG(1)));
  asm("movl %0, f2":: "m" (state->LREG(2)));
  asm("movl %0, f3":: "m" (state->LREG(3)));
  asm("movl %0, f4":: "m" (state->LREG(4)));
  asm("movl %0, f5":: "m" (state->LREG(5)));
  asm("movl %0, f6":: "m" (state->LREG(6)));
  asm("movl %0, f7":: "m" (state->LREG(7)));
}


int ieee_sig(int sig, int code, struct sigcontext *scp)
{
  int ret;
  vaddr_t orig_pc;
  state state;
  get_fstate(&state);
  state.scp = scp;
  orig_pc = state.PC;
  DP(1, "sig = 0x%x, code = 0x%x, pc = 0x%x, fsr = 0x%x\n", sig, code, state.PC, state.FSR);
  ret = ieee_handle_exception(&state);
  DP(1, " pc incremented by %ld\n", state.PC - orig_pc);
  set_fstate(&state);
  return ret;
}
#endif /* KERNEL */

int ieee_handler_debug = 0;

const union t_conv infty =
    {d_bits: { sign: 0, exp: 0x7ff, mantissa: 0, mantissa2: 0}};

const union t_conv snan =
    {d_bits: { sign: 0, exp: 0x7ff, mantissa: 0x40000, mantissa2: 0}};

const union t_conv qnan =
    {d_bits: { sign: 0, exp: 0x7ff, mantissa: 0x80000, mantissa2: 0}};

#define COPYIN(U,K,N) ({if (copyin((AT)U, (AT)K, N) != 0) longjmp(copyin_buffer.copyfail, 1);0;})

/* Adressing modes.  */
#define Adrmod_index_byte 0x1c
#define Adrmod_index_word 0x1d
#define Adrmod_index_doubleword 0x1e
#define Adrmod_index_quadword 0x1f

/* Is MODE an indexed addressing mode?  */
#define Adrmod_is_index(mode) \
  (mode == Adrmod_index_byte \
   || mode == Adrmod_index_word \
   || mode == Adrmod_index_doubleword \
   || mode == Adrmod_index_quadword)


/* Extract a bit field from a stream. Assume nbits less that
 * or equal to 8. Also assume little endian. Assume no alignment
 * restrictions on short.
 */
static inline int bit_extract(char *buffer, int offset, int nbits)
{
  int start_byte = offset/8;
  int end_byte = (offset + nbits - 1)/8;
  return (((start_byte == end_byte? buffer[start_byte]:
	    *(short *)(buffer + start_byte)) >> offset%8)
	  & ((1 << nbits) - 1));
}

#ifdef DEBUG
static int invalid_float(union t_conv *p, int size)
{
  int val;
  if ( size == sizeof (float) )
    val = (p->f_bits.exp == 0xff
	   || (p->f_bits.exp == 0 && p->f_bits.mantissa != 0));
  else if ( size == sizeof (double) )
    val = (p->d_bits.exp == 0x7ff
	   || (p->d_bits.exp == 0 && (p->d_bits.mantissa != 0 || p->d_bits.mantissa2 != 0)));
  else
    val = 1;
  return (val);
}

static void print_op(struct operand *op)
{
  if (op->type == op_type_float) {
    if (invalid_float(&op->data, op->size)) {
      if (op->size == sizeof(double)) {
	printf("s: %d, e: %x, m1: %x, m2: %x", op->data.d_bits.sign, op->data.d_bits.exp, op->data.d_bits.mantissa, op->data.d_bits.mantissa2);
      }
      else {
	printf("s: %d, e: %x, m1: %x", op->data.f_bits.sign, op->data.f_bits.exp, op->data.f_bits.mantissa);
      }
    }
    else {
      if (op->size == sizeof(double))
	printf("%25.17e", op->data.d);
      else
	printf("%18.10e", op->data.f);
    }
  }
  else
    switch(op->size) {
    case 1:
      printf("%d", op->data.c);
      break;
    case 2:
      printf("%d", op->data.s);
      break;
    case 4:
      printf("%d", op->data.i);
      break;
    }
}
#endif

static const int regoffsets[] = {REGOFFSET(0), REGOFFSET(1), REGOFFSET(2),
				REGOFFSET(3), REGOFFSET(4), REGOFFSET(5),
				REGOFFSET(6), REGOFFSET(7)};

static inline void read_reg(int regno, struct operand *op, state *state) {
  vaddr_t addr = REGBASE(state) + regoffsets[regno];
  switch(op->size) {
  case 1: *(char *) &op->data = *(char *) addr; break;
  case 2: *(short *) &op->data = *(short *) addr; break;
  default: *(int *) &op->data = *(int *) addr; break;
  }
  op->where.tag = op_where_register;
  op->where.addr = (vaddr_t) &op->data;
}

static inline vaddr_t reg_addr(int regno, state *state) {
  return REGBASE(state) + regoffsets[regno];
}


static void read_freg(int regno, struct operand *op, state *state) {
  vaddr_t addr;
  if (op->size == sizeof(float)) {
    static const int offsets[] = {
      FREGOFFSET(0), FREGOFFSET(1), FREGOFFSET(2), FREGOFFSET(3),
      FREGOFFSET(4), FREGOFFSET(5), FREGOFFSET(6), FREGOFFSET(7)};
    addr = FREGBASE(state) + offsets[regno];
    *(float *) &op->data = *(float *) addr;
  }
  else {
    static const int offsets[] = {
      LREGOFFSET(0), LREGOFFSET(1), LREGOFFSET(2), LREGOFFSET(3),
      LREGOFFSET(4), LREGOFFSET(5), LREGOFFSET(6), LREGOFFSET(7)};
    addr = LREGBASE(state) + offsets[regno];
    *(double *) &op->data = *(double *) addr;
  }
  op->where.tag = op_where_register;
  op->where.addr = addr;
}


static int canonical_to_size(struct operand *op) {
  int ret = FPC_TT_NONE;
  union t_conv t = op->data;
  if(op->type == op_type_float) {
    if (op->size == sizeof(float)) {
      unsigned long mantissa;
      int exp = t.d_bits.exp + EXP_FBIAS - EXP_DBIAS;
      op->data.f_bits.sign = t.d_bits.sign;
      op->data.f_bits.exp = (exp < 0)? 0: ((exp > 0xff)? 0xff: exp);
      if(exp >= 0xff && t.d_bits.exp < 0x7ff) {
	/* Overflow. need +- infinity.
	 * Allow for trap on overflow.
	 */
	ret = FPC_TT_OVFL;
	mantissa = 0;
      }
      else {
	mantissa = t.d_bits.mantissa << 3;
	mantissa |= (t.d_bits.mantissa2 >> 29) & 7;
	if(exp <= 0 && (mantissa != 0 || t.d_bits.exp != 0)) {
	  /* Denormalize */
	  mantissa |= 0x800000;
	  mantissa >>= -exp;
	  ret = FPC_TT_UNDFL;
	}
	if (mantissa == 0 && ISNAN(t))
	  mantissa = 0x400000;	/* Signaling NaN */
      }
      op->data.f_bits.mantissa = mantissa;
    }
  }
  else {
    switch (op->size) {
    case sizeof(char):
      op->data.c = t.i;
      break;
    case sizeof(short):
      op->data.s = t.i;
      break;
    }
  }
  return ret;
}


static void canonicalise_op(struct operand *op) {
  union t_conv t = op->data;
  if(op->type == op_type_float) {
    if (op->size == sizeof(float)) {
      op->data.d_bits.sign = t.f_bits.sign;
      op->data.d_bits.exp = (t.f_bits.exp == 0xff)? 0x7ff: t.f_bits.exp + EXP_DBIAS - EXP_FBIAS;
      op->data.d_bits.mantissa = t.f_bits.mantissa >> 3;
      op->data.d_bits.mantissa2 = (t.f_bits.mantissa & 7) << 29;
      if (t.f_bits.exp == 0) {
	if (t.f_bits.mantissa == 0) {
	  /* Float is zero */
	  op->data.d_bits.exp = 0;
	}
	else {
	  /* Was a subnormal float. Subnormal floats can always be converted
	   * to normal doubles.
	   */
	  int norm;
	  op->data.d_bits.exp = 0;
	  norm = ieee_normalize(&(op->data));
	  op->data.d_bits.exp = EXP_DBIAS - EXP_FBIAS + norm;
	}
      }
    }
  }
  else {
    switch (op->size) {
    case sizeof(char):
      op->data.i = t.c;
      break;
    case sizeof(short):
      op->data.i = t.s;
      break;
    }
  }
}

#define MAX_LEN 21		/* The longest an instruction can be */

static struct {
  vaddr_t base;
  char *max;
  char buf[MAX_LEN];
#if defined(_KERNEL) && defined(__NetBSD__) && !defined(MACH)
  label_t copyfail;
#else
  jmp_buf copyfail;
#endif
} copyin_buffer;

static void store_result(struct operand *op)
{
  if (op->where.tag == op_where_memory) {
    /* copyout */
    if (copyout((AT) &op->data, (AT)op->where.addr, op->size) != 0) {
      longjmp (copyin_buffer.copyfail, 1);
    }
  }
  else {
    /* Register */
    memcpy((char *)op->where.addr, (char *) &op->data, op->size);
  }
}

#define FETCH_DATA(addr) ((addr) < copyin_buffer.max? 1: fetch_data(addr))
#define BUF_TO_UADDR(addr) ((addr) - copyin_buffer.buf + copyin_buffer.base)

#define FETCH_CHUNK 8		/* Size of chunks to copyin */

/* Fetch data from user space so as to validate buffer up to addr.
 * We do some read ahead (to minimise the number of copyins) but
 * we never read ahead past a page boundary incase that would not
 * be mapped.
 */
static int fetch_data (char *addr) {
  vaddr_t u_addr = copyin_buffer.base + copyin_buffer.max - copyin_buffer.buf;
  vaddr_t u_end_addr = copyin_buffer.base + addr - copyin_buffer.buf;
  vaddr_t k_addr = (vaddr_t) copyin_buffer.max;
  int n;
  int n_page = ns532_round_page(u_end_addr) - copyin_buffer.base + copyin_buffer.buf - copyin_buffer.max;
  int n_max = MAX_LEN + copyin_buffer.buf - copyin_buffer.max;
  int n_min = addr - copyin_buffer.max + 1;
  n = MIN(n_max, MAX(n_min, MIN(FETCH_CHUNK, n_page)));
  COPYIN(u_addr, k_addr, n);
  DP(2, "fetch_data: addr = 0x%p, from 0x%lx, to 0x%lx, n = %d\n", addr,
     (long)u_addr, (long)k_addr, n);
  copyin_buffer.max += n;
  return 1;
}

static int get_displacement (char **buffer)
{
  int disp = 0;
  char *buf = *buffer;
  FETCH_DATA(buf);
  switch (*buf & 0xc0)
    {
    case 0x00:
    case 0x40:
      disp = *buf++ & 0x7f;
      disp = (disp & 0x40) ? disp | 0xffffff80: disp;
      break;
    case 0x80:
      FETCH_DATA(buf + 1);
      disp = *buf++ & 0x3f;
      disp = (disp & 0x20) ? disp | 0xffffffc0: disp;
      disp = (disp << 8) | (0xff & *buf++);
      break;
    case 0xc0:
      FETCH_DATA(buf + 3);
      disp = *buf++ & 0x3f;
      disp = (disp & 0x20) ? disp | 0xffffffc0: disp;
      disp = (disp << 8) | (0xff & *buf++);
      disp = (disp << 8) | (0xff & *buf++);
      disp = (disp << 8) | (0xff & *buf++);
      break;
    }
  *buffer = buf;
  DP(1, "disp = %d\n", disp);
  return disp;
}

static int get_operand(char **buf, unsigned char gen, unsigned char index, struct operand *op, state *state)
{
  int ret = FPC_TT_NONE;
  vaddr_t addr = 0;
  int disp1, disp2;
  DP(1,"gen = 0x%x\n", gen);

  if (op->type == op_type_float && (gen & ~7) == 0) {
    read_freg(gen, op, state);
    return ret;
  }
  switch (gen) {
  case 0: case 1: case 2: case 3:
  case 4: case 5: case 6: case 7:
    if (op->class != op_class_addr) {
      read_reg(gen, op, state);
      return ret;
    }
    else
      addr = reg_addr(gen, state);
    break;
  case 0x8: case 0x9: case 0xa: case 0xb:
  case 0xc: case 0xd: case 0xe: case 0xf:
    /* Register relative disp(R0 -- R7) */
    /* rn out of state, then get data out of res_addr */
    disp1 = get_displacement (buf);
    addr =  (disp1 + *(unsigned int *)reg_addr(gen & 7, state));
    break;
  case 0x10:
  case 0x11:
  case 0x12:
    /* Memory relative disp2(disp1(FP, SP, SB)) */
    disp1 = get_displacement (buf);
    disp2 = get_displacement (buf);
    addr =  (disp1
	     + (vaddr_t) (gen == 0x10? state->FP: (gen == 0x11? state->SP:
						       state->SB)));
    addr =  disp2 + get_dword(addr);
    break;
  case 0x14:
    /* Immediate */
    if (op->class == op_class_read) {
      char *p;
      int i;
      FETCH_DATA(*buf + op->size - 1);
      for(p = (char *)&op->data, i = op->size; i--; p++) {
	*p = *(*buf + i);
      }
      op->where.tag = op_where_immediate;
      op->where.addr = 0;
      *buf += op->size;
      return ret;
    }
    else {
      return FPC_TT_ILL;
    }
  case 0x15:
    /* Absolute @disp */
    disp1 = get_displacement (buf);
    addr = disp1;
    break;
  case 0x16:
    /* External EXT(disp1) + disp2 (Mod table stuff) */
    disp1 = get_displacement (buf);
    disp2 = get_displacement (buf);
    return FPC_TT_ILL;			/* Unsupported */
  case 0x17:
    /* Top of stack tos */
    addr = state->SP;
    switch (op->class) {
    case op_class_read:
      state->SP += op->size;
      break;
    case op_class_write:
      state->SP -= op->size;
      addr =  state->SP;
      break;
    default: ;
      /* Keep gcc quiet */
    }
    break;
  case 0x18:
    /* Memory space disp(FP) */
    disp1 = get_displacement (buf);
    addr =  (disp1 + (vaddr_t) state->FP);
    break;
  case 0x19:
    /* Memory space disp(SP) */
    disp1 = get_displacement (buf);
    addr =  (disp1 + (vaddr_t) state->SP);
    break;
  case 0x1a:
    /* Memory space disp(SB) */
    disp1 = get_displacement (buf);
    addr =  (disp1 + (vaddr_t) state->SB);
    break;
  case 0x1b:
    /* Memory space disp(PC) */
    disp1 = get_displacement (buf);
    addr = disp1 + (vaddr_t) state->PC;
    break;
  case 0x1c:
  case 0x1d:
  case 0x1e:
  case 0x1f:
    /* Scaled index basemode[R0 -- R7:B,W,D,Q] */
    {
      enum op_class save = op->class;
      op->class = op_class_addr;
      if ((ret = get_operand(buf, index >> 3, 0, op, state)) != FPC_TT_NONE)
	return ret;
      addr = op->where.addr + (* (int *) reg_addr(index & 7, state)) * (1 << (gen & 3));
      op->class = save;
      break;
    }
  }
  if (op->class == op_class_read || op->class == op_class_rmw) {
    COPYIN(addr, (vaddr_t)&op->data, op->size);
  }
  op->where.tag = op_where_memory;
  op->where.addr = addr;
  return ret;
}

static int default_trap_handle(struct operand *op1, struct operand *op2, struct operand *f0_op, int xopcode, state *state)
{
  int user_trap = FPC_TT_NONE;
  unsigned int fsr = state->FSR;
  int   trap_type = fsr & FPC_TT;

  DP(2, "trap type %d\n", trap_type);
  {
    int s = GET_SET_FSR(fsr & FPC_RM); /* Same rounding mode as at exception */

    switch (trap_type)
      {
      case FPC_TT_UNDFL:	/* Underflow */
	user_trap = ieee_undfl(op1, op2, f0_op, xopcode, state);
	break;
      case FPC_TT_OVFL:		/* Overflow */
	user_trap = ieee_ovfl(op1, op2, f0_op, xopcode, state);
	break;
      case FPC_TT_DIV0:		/* Divide by zero */
	user_trap = ieee_dze(op1, op2, f0_op, xopcode, state);
	break;
      case FPC_TT_ILL:		/* Illegal instruction */
	/* Illegal instruction. Cause a SIGILL ? */
	user_trap = FPC_TT_ILL;
	break;
      case FPC_TT_INVOP:	/* Invalid operation */
	user_trap = ieee_invop(op1, op2, f0_op, xopcode, state);
	break;
      case FPC_TT_INEXACT:	/* Inexact result */
	/* Nothing to be done */
	/* Flags in hardware. We can't get here unless FPC_IEF is set */
	user_trap = FPC_TT_INEXACT;
	break;
      case FPC_TT_NONE:
      default:
	DP(0,"ieee_handler: fpu trap type none\n");
      }
    SET_FSR(s);
  }
  return user_trap;
}

int ieee_handle_exception(state *state)
{
  int fmt, xopcode, ret;
  int fsr, user_trap;
  volatile int ofsr;		/* Volatile for setjmp */
  unsigned char int_type, float_type, opcode, gen1, gen2, index1, index2;
  struct operand op1, op2, f0_op, *res;
  char *buf = copyin_buffer.buf;
  copyin_buffer.base = state->PC;
  /* Save fsr and set fsr to 0 so that floating point operations within
   * the emulation proceed in a known way. */
  ofsr = GET_SET_FSR(0);
  if (setjmp(copyin_buffer.copyfail) != 0) {
    SET_FSR(ofsr);
    return FPC_TT_ILL;
  }
  user_trap = FPC_TT_NONE;
  int_type = 0;
  float_type = 0;
  opcode = 0;
  index1 = 0;
  index2 = 0;
  copyin_buffer.max = buf;
  /* All fp instructions are 24 bits
   * and extensions start after 3 bytes
   */
  FETCH_DATA(buf + 2);

  fsr = state->FSR;

  switch(fmt = bit_extract(buf, 0, 8))
    {
    case 0x3e:			/* Format 9 */
      int_type = bit_extract(buf, 8, 2);
      float_type = bit_extract(buf, 10, 1);
      opcode = bit_extract(buf, 11, 3);
      fmt = fmt9;
      break;
    case 0xbe:			/* Format 11 */
    case 0xfe:			/* Format 12 */
      fmt = (fmt == 0xbe)? fmt11: fmt12;
      float_type = bit_extract(buf, 8, 1);
      opcode = bit_extract(buf, 10, 4);
      break;
    default:
      user_trap = FPC_TT_ILL;
      DP(0, "ieee_handler: format not of a floating point instruction\n");
    }
  xopcode = XOPCODE(fmt, opcode);
  DP(2, "xopcode: 0x%x\n", xopcode);

  gen2 = bit_extract(buf, 14, 5);
  gen1 = bit_extract(buf, 19, 5);
  buf += 3;
  if (Adrmod_is_index(gen1)) {
    FETCH_DATA(buf);
    index1 = *buf++;
  }
  if (Adrmod_is_index(gen2)) {
    FETCH_DATA(buf);
    index2 = *buf++;
  }

  op1.class = op_class_read;
  op2.class = op_class_rmw;
  op1.type = op2.type = op_type_float;
  op1.size = op2.size = float_type? 4: 8;
  res = &op2;
  switch(xopcode) {
  case MOVLF:
    op1.type = op_type_float; op1.size = 8;
    op2.size = 4; op2.class = op_class_write;
    break;
  case MOVFL:
    op1.type = op_type_float; op1.size = 4;
    op2.size = 8; op2.class = op_class_write;
    break;
  case MOVIF:
    op1.type = op_type_int; op1.size = 1 << int_type;
    op2.class = op_class_write;
    break;
  case NEGF:
    op2.class = op_class_write;
    break;
  case ROUNDFI:
  case TRUNCFI:
  case FLOORFI:
    op2.type = op_type_int; op2.size = 1 << int_type;
    break;
  case CMPF:
    op2.class = op_class_read;
    res = 0;
    break;
  case POLYF:
  case DOTF:
    op1.type = op2.type = f0_op.type = op_type_float;
    op1.size = op2.size = f0_op.size = float_type? 4: 8;
    op2.class = op_class_read;
    f0_op.class = op_class_rmw;
    read_freg(0, &f0_op, state);
    canonicalise_op(&f0_op);
    res = &f0_op;
    break;
  }

  if ((ret = get_operand(&buf, gen1, index1, &op1, state)) != FPC_TT_NONE) {
    user_trap = ret;
    DP(0, "get_operand failed\n");
  }

  if ((ret = get_operand(&buf, gen2, index2, &op2, state)) != FPC_TT_NONE) {
    user_trap = ret;
    DP(0, "get_operand failed\n");
  }

#ifdef DEBUG
  if (ieee_handler_debug > 1) {
    printf( "op1 = "); print_op(&op1); printf("\n");
    printf( "op2 = "); print_op(&op2); printf("\n");
  }
#endif

  canonicalise_op(&op1);
  canonicalise_op(&op2);

  if(user_trap == FPC_TT_NONE) {
    user_trap = default_trap_handle(&op1, &op2, &f0_op, xopcode, state);
    fsr = state->FSR;		/* May have been side effected */

    /* user_trap now has traps generated during emulation. Correct ieee
     * results already calculated, but must see whether we need to
     * pass trap on to user.
     *
     * User trap can be set for many reasons. eg dividing a denorm by 0
     * might initially result in an invalid operand trap, but during
     * emulation we set the divide by zero trap.
     */

    if (res) {
      int ret;
      if((ret = canonical_to_size(res)) != FPC_TT_NONE)
	user_trap = ret;
    }

    switch (user_trap) {
    case FPC_TT_UNDFL:
      fsr |= FPC_UF;
      if ((fsr & FPC_UNDE) == 0)
	user_trap = FPC_TT_NONE;
      break;
    case FPC_TT_OVFL:
      fsr |= FPC_OVF;
      if ((fsr & FPC_OVE) == 0)
	user_trap = FPC_TT_NONE;
      break;
    case FPC_TT_DIV0:
      fsr |= FPC_DZF;
      if ((fsr & FPC_DZE) == 0)
	user_trap = FPC_TT_NONE;
      break;
    case FPC_TT_ILL:
      break;
    case FPC_TT_INVOP:
      fsr |= FPC_IVF;
      if ((fsr & FPC_IVE) == 0)
	user_trap = FPC_TT_NONE;
      break;
    case FPC_TT_INEXACT:
      fsr |= FPC_IF;
      if ((fsr & FPC_IEN) == 0)
	user_trap = FPC_TT_NONE;
      break;
    }
  }

  if(user_trap == FPC_TT_NONE) {
    if(res) {
#ifdef DEBUG
      if (ieee_handler_debug > 1) {
	printf( "res = "); print_op(res); printf("\n");
      }
#endif
      store_result(res);
    }
  }
  state->PC = BUF_TO_UADDR(buf);
  SET_FSR(ofsr);
  state->FSR = fsr;
  return user_trap;
}
