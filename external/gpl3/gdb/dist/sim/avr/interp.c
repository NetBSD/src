/* Simulator for Atmel's AVR core.
   Copyright (C) 2009-2013 Free Software Foundation, Inc.
   Written by Tristan Gingold, AdaCore.

   This file is part of GDB, the GNU debugger.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "config.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "bfd.h"
#include "gdb/callback.h"
#include "gdb/signals.h"
#include "libiberty.h"
#include "gdb/remote-sim.h"
#include "dis-asm.h"
#include "sim-utils.h"

/* As AVR is a 8/16 bits processor, define handy types.  */
typedef unsigned short int word;
typedef signed short int sword;
typedef unsigned char byte;
typedef signed char sbyte;

/* Debug flag to display instructions and registers.  */
static int tracing = 0;
static int lock_step = 0;
static int verbose;

/* The only real register.  */
static unsigned int pc;

/* We update a cycle counter.  */
static unsigned int cycles = 0;

/* If true, the pc needs more than 2 bytes.  */
static int avr_pc22;

static struct bfd *cur_bfd;

static enum sim_stop cpu_exception;
static int cpu_signal;

static SIM_OPEN_KIND sim_kind;
static char *myname;
static host_callback *callback;

/* Max size of I space (which is always flash on avr).  */
#define MAX_AVR_FLASH (128 * 1024)
#define PC_MASK (MAX_AVR_FLASH - 1)

/* Mac size of D space.  */
#define MAX_AVR_SRAM (64 * 1024)
#define SRAM_MASK (MAX_AVR_SRAM - 1)

/* D space offset in ELF file.  */
#define SRAM_VADDR 0x800000

/* Simulator specific ports.  */
#define STDIO_PORT	0x52
#define EXIT_PORT	0x4F
#define ABORT_PORT	0x49

/* GDB defined register numbers.  */
#define AVR_SREG_REGNUM  32
#define AVR_SP_REGNUM    33
#define AVR_PC_REGNUM    34

/* Memory mapped registers.  */
#define SREG	0x5F
#define REG_SP	0x5D
#define EIND	0x5C
#define RAMPZ	0x5B

#define REGX 0x1a
#define REGY 0x1c
#define REGZ 0x1e
#define REGZ_LO 0x1e
#define REGZ_HI 0x1f

/* Sreg (status) bits.  */
#define SREG_I 0x80
#define SREG_T 0x40
#define SREG_H 0x20
#define SREG_S 0x10
#define SREG_V 0x08
#define SREG_N 0x04
#define SREG_Z 0x02
#define SREG_C 0x01

/* In order to speed up emulation we use a simple approach:
   a code is associated with each instruction.  The pre-decoding occurs
   usually once when the instruction is first seen.
   This works well because I&D spaces are separated.

   Missing opcodes: sleep, spm, wdr (as they are mmcu dependent).
*/
enum avr_opcode
  {
    /* Opcode not yet decoded.  */
    OP_unknown,
    OP_bad,

    OP_nop,

    OP_rjmp,
    OP_rcall,
    OP_ret,
    OP_reti,

    OP_break,

    OP_brbs,
    OP_brbc,

    OP_bset,
    OP_bclr,

    OP_bld,
    OP_bst,

    OP_sbrc,
    OP_sbrs,

    OP_eor,
    OP_and,
    OP_andi,
    OP_or,
    OP_ori,
    OP_com,
    OP_swap,
    OP_neg,

    OP_out,
    OP_in,
    OP_cbi,
    OP_sbi,

    OP_sbic,
    OP_sbis,

    OP_ldi,
    OP_cpse,
    OP_cp,
    OP_cpi,
    OP_cpc,
    OP_sub,
    OP_sbc,
    OP_sbiw,
    OP_adiw,
    OP_add,
    OP_adc,
    OP_subi,
    OP_sbci,
    OP_inc,
    OP_dec,
    OP_lsr,
    OP_ror,
    OP_asr,

    OP_mul,
    OP_muls,
    OP_mulsu,
    OP_fmul,
    OP_fmuls,
    OP_fmulsu,

    OP_mov,
    OP_movw,

    OP_push,
    OP_pop,

    OP_st_X,
    OP_st_dec_X,
    OP_st_X_inc,
    OP_st_Y_inc,
    OP_st_dec_Y,
    OP_st_Z_inc,
    OP_st_dec_Z,
    OP_std_Y,
    OP_std_Z,
    OP_ldd_Y,
    OP_ldd_Z,
    OP_ld_Z_inc,
    OP_ld_dec_Z,
    OP_ld_Y_inc,
    OP_ld_dec_Y,
    OP_ld_X,
    OP_ld_X_inc,
    OP_ld_dec_X,
    
    OP_lpm,
    OP_lpm_Z,
    OP_lpm_inc_Z,
    OP_elpm,
    OP_elpm_Z,
    OP_elpm_inc_Z,

    OP_ijmp,
    OP_icall,

    OP_eijmp,
    OP_eicall,

    /* 2 words opcodes.  */
#define OP_2words OP_jmp
    OP_jmp,
    OP_call,
    OP_sts,
    OP_lds
  };

struct avr_insn_cell
{
  /* The insn (16 bits).  */
  word op;

  /* Pre-decoding code.  */
  enum avr_opcode code : 8;
  /* One byte of additional information.  */
  byte r;
};

/* I&D memories.  */
static struct avr_insn_cell flash[MAX_AVR_FLASH];
static byte sram[MAX_AVR_SRAM];

void
sim_size (int s)
{
}

/* Sign extend a value.  */
static int sign_ext (word val, int nb_bits)
{
  if (val & (1 << (nb_bits - 1)))
    return val | (-1 << nb_bits);
  return val;
}

/* Insn field extractors.  */

/* Extract xxxx_xxxRx_xxxx_RRRR.  */
static inline byte get_r (word op)
{
  return (op & 0xf) | ((op >> 5) & 0x10);
}

/* Extract xxxx_xxxxx_xxxx_RRRR.  */
static inline byte get_r16 (word op)
{
  return 16 + (op & 0xf);
}

/* Extract xxxx_xxxxx_xxxx_xRRR.  */
static inline byte get_r16_23 (word op)
{
  return 16 + (op & 0x7);
}

/* Extract xxxx_xxxD_DDDD_xxxx.  */
static inline byte get_d (word op)
{
  return (op >> 4) & 0x1f;
}

/* Extract xxxx_xxxx_DDDD_xxxx.  */
static inline byte get_d16 (word op)
{
  return 16 + ((op >> 4) & 0x0f);
}

/* Extract xxxx_xxxx_xDDD_xxxx.  */
static inline byte get_d16_23 (word op)
{
  return 16 + ((op >> 4) & 0x07);
}

/* Extract xxxx_xAAx_xxxx_AAAA.  */
static inline byte get_A (word op)
{
  return (op & 0x0f) | ((op & 0x600) >> 5);
}

/* Extract xxxx_xxxx_AAAA_Axxx.  */
static inline byte get_biA (word op)
{
  return (op >> 3) & 0x1f;
}

/* Extract xxxx_KKKK_xxxx_KKKK.  */
static inline byte get_K (word op)
{
  return (op & 0xf) | ((op & 0xf00) >> 4);
}

/* Extract xxxx_xxKK_KKKK_Kxxx.  */
static inline int get_k (word op)
{
  return sign_ext ((op & 0x3f8) >> 3, 7);
}

/* Extract xxxx_xxxx_xxDD_xxxx.  */
static inline byte get_d24 (word op)
{
  return 24 + ((op >> 3) & 6);
}

/* Extract xxxx_xxxx_KKxx_KKKK.  */
static inline byte get_k6 (word op)
{
  return (op & 0xf) | ((op >> 2) & 0x30);
}
 
/* Extract xxQx_QQxx_xxxx_xQQQ.  */
static inline byte get_q (word op)
{
  return (op & 7) | ((op >> 7) & 0x18)| ((op >> 8) & 0x20);
}

/* Extract xxxx_xxxx_xxxx_xBBB.  */
static inline byte get_b (word op)
{
  return (op & 7);
}

/* AVR is little endian.  */
static inline word
read_word (unsigned int addr)
{
  return sram[addr] | (sram[addr + 1] << 8);
}

static inline void
write_word (unsigned int addr, word w)
{
  sram[addr] = w;
  sram[addr + 1] = w >> 8;
}

static inline word
read_word_post_inc (unsigned int addr)
{
  word v = read_word (addr);
  write_word (addr, v + 1);
  return v;
}

static inline word
read_word_pre_dec (unsigned int addr)
{
  word v = read_word (addr) - 1;
  write_word (addr, v);
  return v;
}

static void
update_flags_logic (byte res)
{
  sram[SREG] &= ~(SREG_S | SREG_V | SREG_N | SREG_Z);
  if (res == 0)
    sram[SREG] |= SREG_Z;
  if (res & 0x80)
    sram[SREG] |= SREG_N | SREG_S;
}

static void
update_flags_add (byte r, byte a, byte b)
{
  byte carry;

  sram[SREG] &= ~(SREG_H | SREG_S | SREG_V | SREG_N | SREG_Z | SREG_C);
  if (r & 0x80)
    sram[SREG] |= SREG_N;
  carry = (a & b) | (a & ~r) | (b & ~r);
  if (carry & 0x08)
    sram[SREG] |= SREG_H;
  if (carry & 0x80)
    sram[SREG] |= SREG_C;
  if (((a & b & ~r) | (~a & ~b & r)) & 0x80)
    sram[SREG] |= SREG_V;
  if (!(sram[SREG] & SREG_N) ^ !(sram[SREG] & SREG_V))
    sram[SREG] |= SREG_S;
  if (r == 0)
    sram[SREG] |= SREG_Z;
}

static void update_flags_sub (byte r, byte a, byte b)
{
  byte carry;

  sram[SREG] &= ~(SREG_H | SREG_S | SREG_V | SREG_N | SREG_Z | SREG_C);
  if (r & 0x80)
    sram[SREG] |= SREG_N;
  carry = (~a & b) | (b & r) | (r & ~a);
  if (carry & 0x08)
    sram[SREG] |= SREG_H;
  if (carry & 0x80)
    sram[SREG] |= SREG_C;
  if (((a & ~b & ~r) | (~a & b & r)) & 0x80)
    sram[SREG] |= SREG_V;
  if (!(sram[SREG] & SREG_N) ^ !(sram[SREG] & SREG_V))
    sram[SREG] |= SREG_S;
  /* Note: Z is not set.  */
}

static enum avr_opcode
decode (unsigned int pc)
{
  word op1 = flash[pc].op;

  switch ((op1 >> 12) & 0x0f)
    {
    case 0x0:
      switch ((op1 >> 10) & 0x3)
        {
        case 0x0:
          switch ((op1 >> 8) & 0x3)
            {
            case 0x0:
              if (op1 == 0)
                return OP_nop;
              break;
            case 0x1:
              return OP_movw;
            case 0x2:
              return OP_muls;
            case 0x3:
              if (op1 & 0x80)
                {
                  if (op1 & 0x08)
                    return OP_fmulsu;
                  else
                    return OP_fmuls;
                }
              else
                {
                  if (op1 & 0x08)
                    return OP_fmul;
                  else
                    return OP_mulsu;
                }
            }
          break;
        case 0x1:
          return OP_cpc;
        case 0x2:
          flash[pc].r = SREG_C;
          return OP_sbc;
        case 0x3:
          flash[pc].r = 0;
          return OP_add;
        }
      break;
    case 0x1:
      switch ((op1 >> 10) & 0x3)
        {
        case 0x0:
          return OP_cpse;
        case 0x1:
          return OP_cp;
        case 0x2:
          flash[pc].r = 0;
          return OP_sub;
        case 0x3:
          flash[pc].r = SREG_C;
          return OP_adc;
        }
      break;
    case 0x2:
      switch ((op1 >> 10) & 0x3)
        {
        case 0x0:
          return OP_and;
        case 0x1:
          return OP_eor;
        case 0x2:
          return OP_or;
        case 0x3:
          return OP_mov;
        }
      break;
    case 0x3:
      return OP_cpi;
    case 0x4:
      return OP_sbci;
    case 0x5:
      return OP_subi;
    case 0x6:
      return OP_ori;
    case 0x7:
      return OP_andi;
    case 0x8:
    case 0xa:
      if (op1 & 0x0200)
        {
          if (op1 & 0x0008)
            {
              flash[pc].r = get_q (op1);
              return OP_std_Y;
            }
          else
            {
              flash[pc].r = get_q (op1);
              return OP_std_Z;
            }
        }
      else
        {
          if (op1 & 0x0008)
            {
              flash[pc].r = get_q (op1);
              return OP_ldd_Y;
            }
          else
            {
              flash[pc].r = get_q (op1);
              return OP_ldd_Z;
            }
        }
      break;
    case 0x9: /* 9xxx */
      switch ((op1 >> 8) & 0xf)
        {
        case 0x0:
        case 0x1:
          switch ((op1 >> 0) & 0xf)
            {
            case 0x0:
              return OP_lds;
            case 0x1:
              return OP_ld_Z_inc;
            case 0x2:
              return OP_ld_dec_Z;
            case 0x4:
              return OP_lpm_Z;
            case 0x5:
              return OP_lpm_inc_Z;
            case 0x6:
              return OP_elpm_Z;
            case 0x7:
              return OP_elpm_inc_Z;
            case 0x9:
              return OP_ld_Y_inc;
            case 0xa:
              return OP_ld_dec_Y;
            case 0xc:
              return OP_ld_X;
            case 0xd:
              return OP_ld_X_inc;
            case 0xe:
              return OP_ld_dec_X;
            case 0xf:
              return OP_pop;
            }
          break;
        case 0x2:
        case 0x3:
          switch ((op1 >> 0) & 0xf)
            {
            case 0x0:
              return OP_sts;
            case 0x1:
              return OP_st_Z_inc;
            case 0x2:
              return OP_st_dec_Z;
            case 0x9:
              return OP_st_Y_inc;
            case 0xa:
              return OP_st_dec_Y;
            case 0xc:
              return OP_st_X;
            case 0xd:
              return OP_st_X_inc;
            case 0xe:
              return OP_st_dec_X;
            case 0xf:
              return OP_push;
            }
          break;
        case 0x4:
        case 0x5:
          switch (op1 & 0xf)
            {
            case 0x0:
              return OP_com;
            case 0x1:
              return OP_neg;
            case 0x2:
              return OP_swap;
            case 0x3:
              return OP_inc;
            case 0x5:
              flash[pc].r = 0x80;
              return OP_asr;
            case 0x6:
              flash[pc].r = 0;
              return OP_lsr;
            case 0x7:
              return OP_ror;
            case 0x8: /* 9[45]x8 */
              switch ((op1 >> 4) & 0x1f)
                {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                  return OP_bset;
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                case 0x0f:
                  return OP_bclr;
                case 0x10:
                  return OP_ret;
                case 0x11:
                  return OP_reti;
                case 0x19:
                  return OP_break;
                case 0x1c:
                  return OP_lpm;
                case 0x1d:
                  return OP_elpm;
                default:
                  break;
                }
              break;
            case 0x9: /* 9[45]x9 */
              switch ((op1 >> 4) & 0x1f)
                {
                case 0x00:
                  return OP_ijmp;
                case 0x01:
                  return OP_eijmp;
                case 0x10:
                  return OP_icall;
                case 0x11:
                  return OP_eicall;
                default:
                  break;
                }
              break;
            case 0xa:
              return OP_dec;
            case 0xc:
            case 0xd:
              flash[pc].r = ((op1 & 0x1f0) >> 3) | (op1 & 1);
              return OP_jmp;
            case 0xe:
            case 0xf:
              flash[pc].r = ((op1 & 0x1f0) >> 3) | (op1 & 1);
              return OP_call;
            }
          break;
        case 0x6:
          return OP_adiw;
        case 0x7:
          return OP_sbiw;
        case 0x8:
          return OP_cbi;
        case 0x9:
          return OP_sbic;
        case 0xa:
          return OP_sbi;
        case 0xb:
          return OP_sbis;
        case 0xc:
        case 0xd:
        case 0xe:
        case 0xf:
          return OP_mul;
        }
      break;
    case 0xb:
      flash[pc].r = get_A (op1);
      if (((op1 >> 11) & 1) == 0)
        return OP_in;
      else
        return OP_out;
    case 0xc:
      return OP_rjmp;
    case 0xd:
      return OP_rcall;
    case 0xe:
      return OP_ldi;
    case 0xf:
      switch ((op1 >> 9) & 7)
        {
        case 0:
        case 1:
          flash[pc].r = 1 << (op1 & 7);
          return OP_brbs;
        case 2:
        case 3:
          flash[pc].r = 1 << (op1 & 7);
          return OP_brbc;
        case 4:
          if ((op1 & 8) == 0)
            {
              flash[pc].r = 1 << (op1 & 7);
              return OP_bld;
            }
          break;
        case 5:
          if ((op1 & 8) == 0)
            {
              flash[pc].r = 1 << (op1 & 7);
              return OP_bst;
            }
          break;
        case 6:
          if ((op1 & 8) == 0)
            {
              flash[pc].r = 1 << (op1 & 7);
              return OP_sbrc;
            }
          break;
        case 7:
          if ((op1 & 8) == 0)
            {
              flash[pc].r = 1 << (op1 & 7);
              return OP_sbrs;
            }
          break;
        }
    }
  sim_cb_eprintf (callback,
                  "Unhandled instruction at pc=0x%x, op=%04x\n", pc * 2, op1);
  return OP_bad;
}

/* Disassemble an instruction.  */

static int
disasm_read_memory (bfd_vma memaddr, bfd_byte *myaddr, unsigned int length,
			struct disassemble_info *info)
{
  int res;

  res = sim_read (NULL, memaddr, myaddr, length);
  if (res != length)
    return -1;
  return 0;
}

/* Memory error support for an opcodes disassembler.  */

static void
disasm_perror_memory (int status, bfd_vma memaddr,
			  struct disassemble_info *info)
{
  if (status != -1)
    /* Can't happen.  */
    info->fprintf_func (info->stream, "Unknown error %d.", status);
  else
    /* Actually, address between memaddr and memaddr + len was
       out of bounds.  */
    info->fprintf_func (info->stream,
			"Address 0x%x is out of bounds.",
			(int) memaddr);
}

static void
disassemble_insn (SIM_DESC sd, SIM_ADDR pc)
{
  struct disassemble_info disasm_info;
  int len;
  int i;

  INIT_DISASSEMBLE_INFO (disasm_info, callback, sim_cb_eprintf);

  disasm_info.arch = bfd_get_arch (cur_bfd);
  disasm_info.mach = bfd_get_mach (cur_bfd);
  disasm_info.endian = BFD_ENDIAN_LITTLE;
  disasm_info.read_memory_func = disasm_read_memory;
  disasm_info.memory_error_func = disasm_perror_memory;

  len = print_insn_avr (pc << 1, &disasm_info);
  len = len / 2;
  for (i = 0; i < len; i++)
    sim_cb_eprintf (callback, " %04x", flash[pc + i].op);
}

static void
do_call (unsigned int npc)
{
  unsigned int sp = read_word (REG_SP);

  /* Big endian!  */
  sram[sp--] = pc;
  sram[sp--] = pc >> 8;
  if (avr_pc22)
    {
      sram[sp--] = pc >> 16;
      cycles++;
    }
  write_word (REG_SP, sp);
  pc = npc & PC_MASK;
  cycles += 3;
}

static int
get_insn_length (unsigned int p)
{
  if (flash[p].code == OP_unknown)
    flash[p].code = decode(p);
  if (flash[p].code >= OP_2words)
    return 2;
  else
    return 1;
}

static unsigned int
get_z (void)
{
  return (sram[RAMPZ] << 16) | (sram[REGZ_HI] << 8) | sram[REGZ_LO];
}

static unsigned char
get_lpm (unsigned int addr)
{
  word w;

  w = flash[(addr >> 1) & PC_MASK].op;
  if (addr & 1)
    w >>= 8;
  return w;
}

static void
gen_mul (unsigned int res)
{
  write_word (0, res);
  sram[SREG] &= ~(SREG_Z | SREG_C);
  if (res == 0)
    sram[SREG] |= SREG_Z;
  if (res & 0x8000)
    sram[SREG] |= SREG_C;
  cycles++;
}

void
sim_resume (SIM_DESC sd, int step, int signal)
{
  unsigned int ipc;

  if (step)
    {
      cpu_exception = sim_stopped;
      cpu_signal = GDB_SIGNAL_TRAP;
    }
  else
    cpu_exception = sim_running;

  do
    {
      int code;
      word op;
      byte res;
      byte r, d, vd;

    again:
      code = flash[pc].code;
      op = flash[pc].op;


      if ((tracing || lock_step) && code != OP_unknown)
	{
	  if (verbose > 0) {
	    int flags;
	    int i;
	    
	    sim_cb_eprintf (callback, "R00-07:");
	    for (i = 0; i < 8; i++)
	      sim_cb_eprintf (callback, " %02x", sram[i]);
	    sim_cb_eprintf (callback, " -");
	    for (i = 8; i < 16; i++)
	      sim_cb_eprintf (callback, " %02x", sram[i]);
	    sim_cb_eprintf (callback, "  SP: %02x %02x",
                            sram[REG_SP + 1], sram[REG_SP]);
	    sim_cb_eprintf (callback, "\n");
	    sim_cb_eprintf (callback, "R16-31:");
	    for (i = 16; i < 24; i++)
	      sim_cb_eprintf (callback, " %02x", sram[i]);
	    sim_cb_eprintf (callback, " -");
	    for (i = 24; i < 32; i++)
	      sim_cb_eprintf (callback, " %02x", sram[i]);
	    sim_cb_eprintf (callback, "  ");
	    flags = sram[SREG];
	    for (i = 0; i < 8; i++)
	      sim_cb_eprintf (callback, "%c",
                              flags & (0x80 >> i) ? "ITHSVNZC"[i] : '-');
	    sim_cb_eprintf (callback, "\n");
	  }

	  if (lock_step && !tracing)
	    sim_cb_eprintf (callback, "%06x: %04x\n", 2 * pc, flash[pc].op);
	  else
	    {
	      sim_cb_eprintf (callback, "pc=0x%06x insn=0x%04x code=%d r=%d\n",
                              2 * pc, flash[pc].op, code, flash[pc].r);
	      disassemble_insn (sd, pc);
	      sim_cb_eprintf (callback, "\n");
	    }
	}

      ipc = pc;
      pc = (pc + 1) & PC_MASK;
      cycles++;

      switch (code)
	{
	case OP_unknown:
          flash[ipc].code = decode(ipc);
	  pc = ipc;
	  cycles--;
	  goto again;
	  break;

	case OP_nop:
          break;

	case OP_jmp:
	  /* 2 words instruction, but we don't care about the pc.  */
	  pc = ((flash[ipc].r << 16) | flash[ipc + 1].op) & PC_MASK;
	  cycles += 2;
	  break;

	case OP_eijmp:
	  pc = ((sram[EIND] << 16) | read_word (REGZ)) & PC_MASK;
	  cycles += 2;
	  break;

	case OP_ijmp:
	  pc = read_word (REGZ) & PC_MASK;
	  cycles += 1;
	  break;

	case OP_call:
	  /* 2 words instruction.  */
	  pc++;
	  do_call ((flash[ipc].r << 16) | flash[ipc + 1].op);
	  break;

	case OP_eicall:
	  do_call ((sram[EIND] << 16) | read_word (REGZ));
	  break;

	case OP_icall:
	  do_call (read_word (REGZ));
	  break;

	case OP_rcall:
	  do_call (pc + sign_ext (op & 0xfff, 12));
	  break;

	case OP_reti:
          sram[SREG] |= SREG_I;
          /* Fall through */
	case OP_ret:
	  {
	    unsigned int sp = read_word (REG_SP);
	    if (avr_pc22)
	      {
		pc = sram[++sp] << 16;
		cycles++;
	      }
	    else
	      pc = 0;
	    pc |= sram[++sp] << 8;
	    pc |= sram[++sp];
	    write_word (REG_SP, sp);
	  }
	  cycles += 3;
	  break;
	  
	case OP_break:
	  /* Stop on this address.  */
	  cpu_exception = sim_stopped;
	  cpu_signal = GDB_SIGNAL_TRAP;
	  pc = ipc;
	  break;

	case OP_bld:
	  d = get_d (op);
	  r = flash[ipc].r;
	  if (sram[SREG] & SREG_T)
	    sram[d] |= r;
	  else
	    sram[d] &= ~r;
	  break;

	case OP_bst:
	  if (sram[get_d (op)] & flash[ipc].r)
	    sram[SREG] |= SREG_T;
	  else
	    sram[SREG] &= ~SREG_T;
	  break;

	case OP_sbrc:
	case OP_sbrs:
	  if (((sram[get_d (op)] & flash[ipc].r) == 0) ^ ((op & 0x0200) != 0))
            {
              int l = get_insn_length(pc);
              pc += l;
              cycles += l;
            }
	  break;

	case OP_push:
	  {
	    unsigned int sp = read_word (REG_SP);
	    sram[sp--] = sram[get_d (op)];
	    write_word (REG_SP, sp);
	  }
	  cycles++;
	  break;

	case OP_pop:
	  {
	    unsigned int sp = read_word (REG_SP);
	    sram[get_d (op)] = sram[++sp];
	    write_word (REG_SP, sp);
	  }
	  cycles++;
	  break;

	case OP_bclr:
	  sram[SREG] &= ~(1 << ((op >> 4) & 0x7));
	  break;

	case OP_bset:
	  sram[SREG] |= 1 << ((op >> 4) & 0x7);
	  break;

	case OP_rjmp:
	  pc = (pc + sign_ext (op & 0xfff, 12)) & PC_MASK;
	  cycles++;
	  break;

	case OP_eor:
	  d = get_d (op);
	  res = sram[d] ^ sram[get_r (op)];
	  sram[d] = res;
	  update_flags_logic (res);
	  break;

	case OP_and:
	  d = get_d (op);
	  res = sram[d] & sram[get_r (op)];
	  sram[d] = res;
	  update_flags_logic (res);
	  break;

	case OP_andi:
	  d = get_d16 (op);
	  res = sram[d] & get_K (op);
	  sram[d] = res;
	  update_flags_logic (res);
	  break;

	case OP_or:
	  d = get_d (op);
	  res = sram[d] | sram[get_r (op)];
	  sram[d] = res;
	  update_flags_logic (res);
	  break;

	case OP_ori:
	  d = get_d16 (op);
	  res = sram[d] | get_K (op);
	  sram[d] = res;
	  update_flags_logic (res);
	  break;

	case OP_com:
	  d = get_d (op);
	  res = ~sram[d];
	  sram[d] = res;
	  update_flags_logic (res);
	  sram[SREG] |= SREG_C;
	  break;

	case OP_swap:
	  d = get_d (op);
	  vd = sram[d];
	  sram[d] = (vd >> 4) | (vd << 4);
	  break;

	case OP_neg:
	  d = get_d (op);
	  vd = sram[d];
	  res = -vd;
	  sram[d] = res;
	  sram[SREG] &= ~(SREG_H | SREG_S | SREG_V | SREG_N | SREG_Z | SREG_C);
	  if (res == 0)
	    sram[SREG] |= SREG_Z;
	  else
	    sram[SREG] |= SREG_C;
	  if (res == 0x80)
	    sram[SREG] |= SREG_V | SREG_N;
	  else if (res & 0x80)
	    sram[SREG] |= SREG_N | SREG_S;
	  if ((res | vd) & 0x08)
	    sram[SREG] |= SREG_H;
	  break;

	case OP_inc:
	  d = get_d (op);
	  res = sram[d] + 1;
	  sram[d] = res;
	  sram[SREG] &= ~(SREG_S | SREG_V | SREG_N | SREG_Z);
	  if (res == 0x80)
	    sram[SREG] |= SREG_V | SREG_N;
	  else if (res & 0x80)
	    sram[SREG] |= SREG_N | SREG_S;
	  else if (res == 0)
	    sram[SREG] |= SREG_Z;
	  break;

	case OP_dec:
	  d = get_d (op);
	  res = sram[d] - 1;
	  sram[d] = res;
	  sram[SREG] &= ~(SREG_S | SREG_V | SREG_N | SREG_Z);
	  if (res == 0x7f)
	    sram[SREG] |= SREG_V | SREG_S;
	  else if (res & 0x80)
	    sram[SREG] |= SREG_N | SREG_S;
	  else if (res == 0)
	    sram[SREG] |= SREG_Z;
	  break;

	case OP_lsr:
	case OP_asr:
	  d = get_d (op);
	  vd = sram[d];
	  res = (vd >> 1) | (vd & flash[ipc].r);
	  sram[d] = res;
	  sram[SREG] &= ~(SREG_S | SREG_V | SREG_N | SREG_Z | SREG_C);
	  if (vd & 1)
	    sram[SREG] |= SREG_C | SREG_S;
	  if (res & 0x80)
	    sram[SREG] |= SREG_N;
          if (!(sram[SREG] & SREG_N) ^ !(sram[SREG] & SREG_C))
            sram[SREG] |= SREG_V;
	  if (res == 0)
	    sram[SREG] |= SREG_Z;
	  break;

	case OP_ror:
	  d = get_d (op);
	  vd = sram[d];
	  res = vd >> 1 | (sram[SREG] << 7);
	  sram[d] = res;
	  sram[SREG] &= ~(SREG_S | SREG_V | SREG_N | SREG_Z | SREG_C);
	  if (vd & 1)
	    sram[SREG] |= SREG_C | SREG_S;
	  if (res & 0x80)
	    sram[SREG] |= SREG_N;
          if (!(sram[SREG] & SREG_N) ^ !(sram[SREG] & SREG_C))
            sram[SREG] |= SREG_V;
	  if (res == 0)
	    sram[SREG] |= SREG_Z;
	  break;

	case OP_mul:
          gen_mul ((word)sram[get_r (op)] * (word)sram[get_d (op)]);
	  break;

	case OP_muls:
	  gen_mul((sword)(sbyte)sram[get_r16 (op)] 
                  * (sword)(sbyte)sram[get_d16 (op)]);
	  break;

	case OP_mulsu:
	  gen_mul ((sword)(word)sram[get_r16_23 (op)] 
                   * (sword)(sbyte)sram[get_d16_23 (op)]);
	  break;

	case OP_fmul:
	  gen_mul(((word)sram[get_r16_23 (op)] 
                   * (word)sram[get_d16_23 (op)]) << 1);
	  break;

	case OP_fmuls:
	  gen_mul(((sword)(sbyte)sram[get_r16_23 (op)] 
                   * (sword)(sbyte)sram[get_d16_23 (op)]) << 1);
	  break;

	case OP_fmulsu:
	  gen_mul(((sword)(word)sram[get_r16_23 (op)] 
                   * (sword)(sbyte)sram[get_d16_23 (op)]) << 1);
	  break;

	case OP_adc:
	case OP_add:
	  r = sram[get_r (op)];
	  d = get_d (op);
	  vd = sram[d];
	  res = r + vd + (sram[SREG] & flash[ipc].r);
	  sram[d] = res;
	  update_flags_add (res, vd, r);
	  break;

	case OP_sub:
	  d = get_d (op);
	  vd = sram[d];
	  r = sram[get_r (op)];
	  res = vd - r;
	  sram[d] = res;
	  update_flags_sub (res, vd, r);
	  if (res == 0)
	    sram[SREG] |= SREG_Z;
	  break;

	case OP_sbc:
	  {
	    byte old = sram[SREG];
	    d = get_d (op);
	    vd = sram[d];
	    r = sram[get_r (op)];
	    res = vd - r - (old & SREG_C);
	    sram[d] = res;
	    update_flags_sub (res, vd, r);
	    if (res == 0 && (old & SREG_Z))
	      sram[SREG] |= SREG_Z;
	  }
	  break;

	case OP_subi:
	  d = get_d16 (op);
	  vd = sram[d];
	  r = get_K (op);
	  res = vd - r;
	  sram[d] = res;
	  update_flags_sub (res, vd, r);
	  if (res == 0)
	    sram[SREG] |= SREG_Z;
	  break;

	case OP_sbci:
	  {
	    byte old = sram[SREG];

	    d = get_d16 (op);
	    vd = sram[d];
	    r = get_K (op);
	    res = vd - r - (old & SREG_C);
	    sram[d] = res;
	    update_flags_sub (res, vd, r);
	    if (res == 0 && (old & SREG_Z))
	      sram[SREG] |= SREG_Z;
	  }
	  break;

	case OP_mov:
	  sram[get_d (op)] = sram[get_r (op)];
	  break;

	case OP_movw:
	  d = (op & 0xf0) >> 3;
	  r = (op & 0x0f) << 1;
	  sram[d] = sram[r];
	  sram[d + 1] = sram[r + 1];
	  break;

	case OP_out:
	  d = get_A (op) + 0x20;
	  res = sram[get_d (op)];
	  sram[d] = res;
	  if (d == STDIO_PORT)
	    putchar (res);
	  else if (d == EXIT_PORT)
	    {
	      cpu_exception = sim_exited;
	      cpu_signal = 0;
	      return;
	    }
	  else if (d == ABORT_PORT)
	    {
	      cpu_exception = sim_exited;
	      cpu_signal = 1;
	      return;
	    }
	  break;

	case OP_in:
	  d = get_A (op) + 0x20;
	  sram[get_d (op)] = sram[d];
	  break;

        case OP_cbi:
	  d = get_biA (op) + 0x20;
          sram[d] &= ~(1 << get_b(op));
          break;

        case OP_sbi:
	  d = get_biA (op) + 0x20;
          sram[d] |= 1 << get_b(op);
          break;

        case OP_sbic:
          if (!(sram[get_biA (op) + 0x20] & 1 << get_b(op)))
            {
              int l = get_insn_length(pc);
              pc += l;
              cycles += l;
            }
          break;

        case OP_sbis:
          if (sram[get_biA (op) + 0x20] & 1 << get_b(op))
            {
              int l = get_insn_length(pc);
              pc += l;
              cycles += l;
            }
          break;

	case OP_ldi:
	  res = get_K (op);
	  d = get_d16 (op);
	  sram[d] = res;
	  break;

	case OP_lds:
	  sram[get_d (op)] = sram[flash[pc].op];
	  pc++;
	  cycles++;
	  break;

	case OP_sts:
	  sram[flash[pc].op] = sram[get_d (op)];
	  pc++;
	  cycles++;
	  break;

	case OP_cpse:
	  if (sram[get_r (op)] == sram[get_d (op)])
            {
              int l = get_insn_length(pc);
              pc += l;
              cycles += l;
            }
	  break;

	case OP_cp:
	  r = sram[get_r (op)];
	  d = sram[get_d (op)];
	  res = d - r;
	  update_flags_sub (res, d, r);
	  if (res == 0)
	    sram[SREG] |= SREG_Z;
	  break;

	case OP_cpi:
	  r = get_K (op);
	  d = sram[get_d16 (op)];
	  res = d - r;
	  update_flags_sub (res, d, r);
	  if (res == 0)
	    sram[SREG] |= SREG_Z;
	  break;

	case OP_cpc:
	  {
	    byte old = sram[SREG];
	    d = sram[get_d (op)];
	    r = sram[get_r (op)];
	    res = d - r - (old & SREG_C);
	    update_flags_sub (res, d, r);
	    if (res == 0 && (old & SREG_Z))
	      sram[SREG] |= SREG_Z;
	  }
	  break;

	case OP_brbc:
	  if (!(sram[SREG] & flash[ipc].r))
	    {
	      pc = (pc + get_k (op)) & PC_MASK;
	      cycles++;
	    }
	  break;

	case OP_brbs:
	  if (sram[SREG] & flash[ipc].r)
	    {
	      pc = (pc + get_k (op)) & PC_MASK;
	      cycles++;
	    }
	  break;

	case OP_lpm:
          sram[0] = get_lpm (read_word (REGZ));
	  cycles += 2;
	  break;

	case OP_lpm_Z:
          sram[get_d (op)] = get_lpm (read_word (REGZ));
	  cycles += 2;
	  break;

	case OP_lpm_inc_Z:
          sram[get_d (op)] = get_lpm (read_word_post_inc (REGZ));
	  cycles += 2;
	  break;

	case OP_elpm:
          sram[0] = get_lpm (get_z ());
	  cycles += 2;
	  break;

	case OP_elpm_Z:
          sram[get_d (op)] = get_lpm (get_z ());
	  cycles += 2;
	  break;

	case OP_elpm_inc_Z:
	  {
	    unsigned int z = get_z ();

	    sram[get_d (op)] = get_lpm (z);
	    z++;
	    sram[REGZ_LO] = z;
	    sram[REGZ_HI] = z >> 8;
	    sram[RAMPZ] = z >> 16;
	  }
	  cycles += 2;
	  break;

	case OP_ld_Z_inc:
	  sram[get_d (op)] = sram[read_word_post_inc (REGZ) & SRAM_MASK];
	  cycles++;
	  break;

	case OP_ld_dec_Z:
	  sram[get_d (op)] = sram[read_word_pre_dec (REGZ) & SRAM_MASK];
	  cycles++;
	  break;

	case OP_ld_X_inc:
	  sram[get_d (op)] = sram[read_word_post_inc (REGX) & SRAM_MASK];
	  cycles++;
	  break;

	case OP_ld_dec_X:
	  sram[get_d (op)] = sram[read_word_pre_dec (REGX) & SRAM_MASK];
	  cycles++;
	  break;

	case OP_ld_Y_inc:
	  sram[get_d (op)] = sram[read_word_post_inc (REGY) & SRAM_MASK];
	  cycles++;
	  break;

	case OP_ld_dec_Y:
	  sram[get_d (op)] = sram[read_word_pre_dec (REGY) & SRAM_MASK];
	  cycles++;
	  break;

	case OP_st_X:
	  sram[read_word (REGX) & SRAM_MASK] = sram[get_d (op)];
	  cycles++;
	  break;

	case OP_st_X_inc:
	  sram[read_word_post_inc (REGX) & SRAM_MASK] = sram[get_d (op)];
	  cycles++;
	  break;

	case OP_st_dec_X:
	  sram[read_word_pre_dec (REGX) & SRAM_MASK] = sram[get_d (op)];
	  cycles++;
	  break;

	case OP_st_Z_inc:
	  sram[read_word_post_inc (REGZ) & SRAM_MASK] = sram[get_d (op)];
	  cycles++;
	  break;

	case OP_st_dec_Z:
	  sram[read_word_pre_dec (REGZ) & SRAM_MASK] = sram[get_d (op)];
	  cycles++;
	  break;

	case OP_st_Y_inc:
	  sram[read_word_post_inc (REGY) & SRAM_MASK] = sram[get_d (op)];
	  cycles++;
	  break;

	case OP_st_dec_Y:
	  sram[read_word_pre_dec (REGY) & SRAM_MASK] = sram[get_d (op)];
	  cycles++;
	  break;

	case OP_std_Y:
	  sram[read_word (REGY) + flash[ipc].r] = sram[get_d (op)];
	  cycles++;
	  break;

	case OP_std_Z:
	  sram[read_word (REGZ) + flash[ipc].r] = sram[get_d (op)];
	  cycles++;
	  break;

	case OP_ldd_Z:
	  sram[get_d (op)] = sram[read_word (REGZ) + flash[ipc].r];
	  cycles++;
	  break;

	case OP_ldd_Y:
	  sram[get_d (op)] = sram[read_word (REGY) + flash[ipc].r];
	  cycles++;
	  break;

	case OP_ld_X:
	  sram[get_d (op)] = sram[read_word (REGX) & SRAM_MASK];
	  cycles++;
	  break;

	case OP_sbiw:
	  {
	    word wk = get_k6 (op);
	    word wres;
	    word wr;
	    
	    d = get_d24 (op);
	    wr = read_word (d);
	    wres = wr - wk;

	    sram[SREG] &= ~(SREG_S | SREG_V | SREG_N | SREG_Z | SREG_C);
	    if (wres == 0)
	      sram[SREG] |= SREG_Z;
	    if (wres & 0x8000)
	      sram[SREG] |= SREG_N;
	    if (wres & ~wr & 0x8000)
	      sram[SREG] |= SREG_C;
	    if (~wres & wr & 0x8000)
	      sram[SREG] |= SREG_V;
	    if (((~wres & wr) ^ wres) & 0x8000)
	      sram[SREG] |= SREG_S;
	    write_word (d, wres);
	  }
	  cycles++;
	  break;

	case OP_adiw:
	  {
	    word wk = get_k6 (op);
	    word wres;
	    word wr;
	    
	    d = get_d24 (op);
	    wr = read_word (d);
	    wres = wr + wk;

	    sram[SREG] &= ~(SREG_S | SREG_V | SREG_N | SREG_Z | SREG_C);
	    if (wres == 0)
	      sram[SREG] |= SREG_Z;
	    if (wres & 0x8000)
	      sram[SREG] |= SREG_N;
	    if (~wres & wr & 0x8000)
	      sram[SREG] |= SREG_C;
	    if (wres & ~wr & 0x8000)
	      sram[SREG] |= SREG_V;
	    if (((wres & ~wr) ^ wres) & 0x8000)
	      sram[SREG] |= SREG_S;
	    write_word (d, wres);
	  }
	  cycles++;
	  break;

	case OP_bad:
	  sim_cb_eprintf (callback, "Bad instruction at pc=0x%x\n", ipc * 2);
	  return;

	default:
	  sim_cb_eprintf (callback,
                          "Unhandled instruction at pc=0x%x, code=%d\n",
                          2 * ipc, code);
	  return;
	}
    }
  while (cpu_exception == sim_running);
}


int
sim_trace (SIM_DESC sd)
{
  tracing = 1;
  
  sim_resume (sd, 0, 0);

  tracing = 0;
  
  return 1;
}

int
sim_write (SIM_DESC sd, SIM_ADDR addr, const unsigned char *buffer, int size)
{
  int osize = size;

  if (addr >= 0 && addr < SRAM_VADDR)
    {
      while (size > 0 && addr < (MAX_AVR_FLASH << 1))
	{
          word val = flash[addr >> 1].op;

          if (addr & 1)
            val = (val & 0xff) | (buffer[0] << 8);
          else
            val = (val & 0xff00) | buffer[0];

	  flash[addr >> 1].op = val;
	  flash[addr >> 1].code = OP_unknown;
	  addr++;
	  buffer++;
	  size--;
	}
      return osize - size;
    }
  else if (addr >= SRAM_VADDR && addr < SRAM_VADDR + MAX_AVR_SRAM)
    {
      addr -= SRAM_VADDR;
      if (addr + size > MAX_AVR_SRAM)
	size = MAX_AVR_SRAM - addr;
      memcpy (sram + addr, buffer, size);
      return size;
    }
  else
    return 0;
}

int
sim_read (SIM_DESC sd, SIM_ADDR addr, unsigned char *buffer, int size)
{
  int osize = size;

  if (addr >= 0 && addr < SRAM_VADDR)
    {
      while (size > 0 && addr < (MAX_AVR_FLASH << 1))
	{
          word val = flash[addr >> 1].op;

          if (addr & 1)
            val >>= 8;

          *buffer++ = val;
	  addr++;
	  size--;
	}
      return osize - size;
    }
  else if (addr >= SRAM_VADDR && addr < SRAM_VADDR + MAX_AVR_SRAM)
    {
      addr -= SRAM_VADDR;
      if (addr + size > MAX_AVR_SRAM)
	size = MAX_AVR_SRAM - addr;
      memcpy (buffer, sram + addr, size);
      return size;
    }
  else
    {
      /* Avoid errors.  */
      memset (buffer, 0, size);
      return size;
    }
}

int
sim_store_register (SIM_DESC sd, int rn, unsigned char *memory, int length)
{
  if (rn < 32 && length == 1)
    {
      sram[rn] = *memory;
      return 1;
    }
  if (rn == AVR_SREG_REGNUM && length == 1)
    {
      sram[SREG] = *memory;
      return 1;
    }
  if (rn == AVR_SP_REGNUM && length == 2)
    {
      sram[REG_SP] = memory[0];
      sram[REG_SP + 1] = memory[1];
      return 2;
    }
  if (rn == AVR_PC_REGNUM && length == 4)
    {
      pc = (memory[0] >> 1) | (memory[1] << 7) 
	| (memory[2] << 15) | (memory[3] << 23);
      pc &= PC_MASK;
      return 4;
    }
  return 0;
}

int
sim_fetch_register (SIM_DESC sd, int rn, unsigned char *memory, int length)
{
  if (rn < 32 && length == 1)
    {
      *memory = sram[rn];
      return 1;
    }
  if (rn == AVR_SREG_REGNUM && length == 1)
    {
      *memory = sram[SREG];
      return 1;
    }
  if (rn == AVR_SP_REGNUM && length == 2)
    {
      memory[0] = sram[REG_SP];
      memory[1] = sram[REG_SP + 1];
      return 2;
    }
  if (rn == AVR_PC_REGNUM && length == 4)
    {
      memory[0] = pc << 1;
      memory[1] = pc >> 7;
      memory[2] = pc >> 15;
      memory[3] = pc >> 23;
      return 4;
    }
  return 0;
}

void
sim_stop_reason (SIM_DESC sd, enum sim_stop * reason,  int *sigrc)
{
  *reason = cpu_exception;
  *sigrc = cpu_signal;
}

int
sim_stop (SIM_DESC sd)
{
  cpu_exception = sim_stopped;
  cpu_signal = GDB_SIGNAL_INT;
  return 1;
}

void
sim_info (SIM_DESC sd, int verbose)
{
  callback->printf_filtered
    (callback, "\n\n# cycles  %10u\n", cycles);
}

SIM_DESC
sim_open (SIM_OPEN_KIND kind, host_callback *cb, struct bfd *abfd, char **argv)
{
  myname = argv[0];
  callback = cb;
  
  cur_bfd = abfd;

  /* Fudge our descriptor for now.  */
  return (SIM_DESC) 1;
}

void
sim_close (SIM_DESC sd, int quitting)
{
  /* nothing to do */
}

SIM_RC
sim_load (SIM_DESC sd, char *prog, bfd *abfd, int from_tty)
{
  bfd *prog_bfd;

  /* Clear all the memory.  */
  memset (sram, 0, sizeof (sram));
  memset (flash, 0, sizeof (flash));

  prog_bfd = sim_load_file (sd, myname, callback, prog, abfd,
                            sim_kind == SIM_OPEN_DEBUG,
                            0, sim_write);
  if (prog_bfd == NULL)
    return SIM_RC_FAIL;

  avr_pc22 = (bfd_get_mach (prog_bfd) >= bfd_mach_avr6);

  if (abfd != NULL)
    cur_bfd = abfd;

  return SIM_RC_OK;
}

SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *prog_bfd, char **argv, char **env)
{
  /* Set the initial register set.  */
  pc = 0;
  
  return SIM_RC_OK;
}

void
sim_kill (SIM_DESC sd)
{
  /* nothing to do */
}

void
sim_do_command (SIM_DESC sd, char *cmd)
{
  /* Nothing there yet; it's all an error.  */
  
  if (cmd == NULL)
    return;

  if (strcmp (cmd, "verbose") == 0)
    verbose = 2;
  else if (strcmp (cmd, "trace") == 0)
    tracing = 1;
  else
    sim_cb_eprintf (callback,
                    "Error: \"%s\" is not a valid avr simulator command.\n",
                    cmd);
}

void
sim_set_callbacks (host_callback *ptr)
{
  callback = ptr; 
}

char **
sim_complete_command (SIM_DESC sd, char *text, char *word)
{
  return NULL;
}
