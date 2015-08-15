/* Simulator for TI MSP430 and MSP430X

   Copyright (C) 2013-2014 Free Software Foundation, Inc.
   Contributed by Red Hat.
   Based on sim/bfin/bfin-sim.c which was contributed by Analog Devices, Inc.

   This file is part of simulators.

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>
#include "bfd.h"
#include "opcode/msp430-decode.h"
#include "sim-main.h"
#include "dis-asm.h"
#include "targ-vals.h"

static int
loader_write_mem (SIM_DESC sd,
		  SIM_ADDR taddr,
		  const unsigned char *buf,
		  int bytes)
{
  SIM_CPU *cpu = MSP430_CPU (sd);
  return sim_core_write_buffer (sd, cpu, write_map, buf, taddr, bytes);
}

static sim_cia
msp430_pc_fetch (SIM_CPU *cpu)
{
  return cpu->state.regs[0];
}

static void
msp430_pc_store (SIM_CPU *cpu, sim_cia newpc)
{
  cpu->state.regs[0] = newpc;
}

static long
lookup_symbol (SIM_DESC sd, const char *name)
{
  struct bfd *abfd = STATE_PROG_BFD (sd);
  asymbol **symbol_table = STATE_SYMBOL_TABLE (sd);
  long number_of_symbols = STATE_NUM_SYMBOLS (sd);
  long i;

  if (symbol_table == NULL)
    {
      long storage_needed;

      storage_needed = bfd_get_symtab_upper_bound (abfd);
      if (storage_needed <= 0)
	return -1;

      STATE_SYMBOL_TABLE (sd) = symbol_table = xmalloc (storage_needed);
      STATE_NUM_SYMBOLS (sd) = number_of_symbols =
	bfd_canonicalize_symtab (abfd, symbol_table);
    }

  for (i = 0; i < number_of_symbols; i++)
    if (strcmp (symbol_table[i]->name, name) == 0)
      {
	long val = symbol_table[i]->section->vma + symbol_table[i]->value;
	return val;
      }
  return -1;
}

static int
msp430_reg_fetch (SIM_CPU *cpu, int regno, unsigned char *buf, int len)
{
  if (0 <= regno && regno < 16)
    {
      if (len == 2)
	{
	  int val = cpu->state.regs[regno];
	  buf[0] = val & 0xff;
	  buf[1] = (val >> 8) & 0xff;
	  return 0;
	}
      else if (len == 4)
	{
	  int val = cpu->state.regs[regno];
	  buf[0] = val & 0xff;
	  buf[1] = (val >> 8) & 0xff;
	  buf[2] = (val >> 16) & 0x0f; /* Registers are only 20 bits wide.  */
	  buf[3] = 0;
	  return 0;
	}
      else
	return -1;
    }
  else
    return -1;
}

static int
msp430_reg_store (SIM_CPU *cpu, int regno, unsigned char *buf, int len)
{
  if (0 <= regno && regno < 16)
    {
      if (len == 2)
	{
	  cpu->state.regs[regno] = (buf[1] << 8) | buf[0];
	  return len;
	}

      if (len == 4)
	{
	  cpu->state.regs[regno] = ((buf[2] << 16) & 0xf0000)
				   | (buf[1] << 8) | buf[0];
	  return len;
	}
    }

  return -1;
}

static inline void
msp430_initialize_cpu (SIM_DESC sd, SIM_CPU *cpu)
{
  memset (&cpu->state, 0, sizeof (cpu->state));
}

SIM_DESC
sim_open (SIM_OPEN_KIND kind,
	  struct host_callback_struct *callback,
	  struct bfd *abfd,
	  char **argv)
{
  SIM_DESC sd = sim_state_alloc (kind, callback);
  char c;
  struct bfd *prog_bfd;

  /* Initialise the simulator.  */

  if (sim_cpu_alloc_all (sd, 1, /*cgen_cpu_max_extra_bytes ()*/0) != SIM_RC_OK)
    {
      sim_state_free (sd);
      return 0;
    }

  if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
    {
      sim_state_free (sd);
      return 0;
    }

  if (sim_parse_args (sd, argv) != SIM_RC_OK)
    {
      sim_state_free (sd);
      return 0;
    }

  CPU_PC_FETCH (MSP430_CPU (sd)) = msp430_pc_fetch;
  CPU_PC_STORE (MSP430_CPU (sd)) = msp430_pc_store;
  CPU_REG_FETCH (MSP430_CPU (sd)) = msp430_reg_fetch;
  CPU_REG_STORE (MSP430_CPU (sd)) = msp430_reg_store;

  /* Allocate memory if none specified by user.  */
  if (sim_core_read_buffer (sd, MSP430_CPU (sd), read_map, &c, 0x200, 1) == 0)
    sim_do_commandf (sd, "memory-region 0,0x10000");
  if (sim_core_read_buffer (sd, MSP430_CPU (sd), read_map, &c, 0xfffe, 1) == 0)
    sim_do_commandf (sd, "memory-region 0xfffe,2");
  if (sim_core_read_buffer (sd, MSP430_CPU (sd), read_map, &c, 0x10000, 1) == 0)
    sim_do_commandf (sd, "memory-region 0x10000,0x100000");

  /* Check for/establish the a reference program image.  */
  if (sim_analyze_program (sd,
			   (STATE_PROG_ARGV (sd) != NULL
			    ? *STATE_PROG_ARGV (sd)
			    : NULL), abfd) != SIM_RC_OK)
    {
      sim_state_free (sd);
      return 0;
    }

  prog_bfd = sim_load_file (sd, argv[0], callback,
			    "the program",
			    STATE_PROG_BFD (sd),
			    0 /* verbose */,
			    1 /* use LMA instead of VMA */,
			    loader_write_mem);
  if (prog_bfd == NULL)
    {
      sim_state_free (sd);
      return 0;
    }

  /* Establish any remaining configuration options.  */
  if (sim_config (sd) != SIM_RC_OK)
    {
      sim_state_free (sd);
      return 0;
    }

  if (sim_post_argv_init (sd) != SIM_RC_OK)
    {
      sim_state_free (sd);
      return 0;
    }

  /* CPU specific initialization.  */
  assert (MAX_NR_PROCESSORS == 1);
  msp430_initialize_cpu (sd, MSP430_CPU (sd));

  msp430_trace_init (STATE_PROG_BFD (sd));

  MSP430_CPU (sd)->state.cio_breakpoint = lookup_symbol (sd, "C$$IO$$");
  MSP430_CPU (sd)->state.cio_buffer = lookup_symbol (sd, "__CIOBUF__");
  if (MSP430_CPU (sd)->state.cio_buffer == -1)
    MSP430_CPU (sd)->state.cio_buffer = lookup_symbol (sd, "_CIOBUF_");

  return sd;
}

void
sim_close (SIM_DESC sd,
	   int quitting)
{
  free (STATE_SYMBOL_TABLE (sd));
  sim_state_free (sd);
}

SIM_RC
sim_create_inferior (SIM_DESC sd,
		     struct bfd *abfd,
		     char **argv,
		     char **env)
{
  unsigned char resetv[2];
  int c;
  int new_pc;

  c = sim_core_read_buffer (sd, MSP430_CPU (sd), read_map, resetv, 0xfffe, 2);

  new_pc = resetv[0] + 256 * resetv[1];
  sim_pc_set (MSP430_CPU (sd), new_pc);
  msp430_pc_store (MSP430_CPU (sd), new_pc);

  return SIM_RC_OK;
}

typedef struct
{
  SIM_DESC sd;
  int gb_addr;
} Get_Byte_Local_Data;

static int
msp430_getbyte (void *vld)
{
  Get_Byte_Local_Data *ld = (Get_Byte_Local_Data *)vld;
  char buf[1];
  SIM_DESC sd = ld->sd;

  sim_core_read_buffer (sd, MSP430_CPU (sd), read_map, buf, ld->gb_addr, 1);
  ld->gb_addr ++;
  return buf[0];
}

#define REG(N) MSP430_CPU (sd)->state.regs[(N)]
#define PC REG(MSR_PC)
#define SP REG(MSR_SP)
#define SR REG(MSR_SR)

static const char *
register_names[] =
{
  "PC", "SP", "SR", "CG", "R4", "R5", "R6", "R7", "R8",
  "R9", "R10", "R11", "R12", "R13", "R14", "R15"
};

static void
trace_reg_put (SIM_DESC sd, int n, unsigned int v)
{
  if (TRACE_VPU_P (MSP430_CPU (sd)))
    trace_generic (sd, MSP430_CPU (sd), TRACE_VPU_IDX,
		   "PUT: %#x -> %s", v, register_names [n]);
  REG (n) = v;
}

static unsigned int
trace_reg_get (SIM_DESC sd, int n)
{
  if (TRACE_VPU_P (MSP430_CPU (sd)))
    trace_generic (sd, MSP430_CPU (sd), TRACE_VPU_IDX,
		   "GET: %s -> %#x", register_names [n], REG (n));
  return REG (n);
}

#define REG_PUT(N,V) trace_reg_put (sd, N, V)
#define REG_GET(N)   trace_reg_get (sd, N)

static int
get_op (SIM_DESC sd, MSP430_Opcode_Decoded *opc, int n)
{
  MSP430_Opcode_Operand *op = opc->op + n;
  int rv;
  int addr;
  unsigned char buf[4];
  int incval = 0;

  switch (op->type)
    {
    case MSP430_Operand_Immediate:
      rv =  op->addend;
      break;
    case MSP430_Operand_Register:
      rv = REG_GET (op->reg);
      break;
    case MSP430_Operand_Indirect:
    case MSP430_Operand_Indirect_Postinc:
      addr = op->addend;
      if (op->reg != MSR_None)
	{
	  int reg;
	  /* Index values are signed, but the sum is limited to 16
	     bits if the register < 64k, for MSP430 compatibility in
	     MSP430X chips.  */
	  if (addr & 0x8000)
	    addr |= -1 << 16;
	  reg = REG_GET (op->reg);
	  addr += reg;
	  if (reg < 0x10000 && ! opc->ofs_430x)
	    addr &= 0xffff;
	}
      addr &= 0xfffff;
      switch (opc->size)
	{
	case 8:
	  sim_core_read_buffer (sd, MSP430_CPU (sd), read_map, buf, addr, 1);
	  rv = buf[0];
	  break;
	case 16:
	  sim_core_read_buffer (sd, MSP430_CPU (sd), read_map, buf, addr, 2);
	  rv = buf[0] | (buf[1] << 8);
	  break;
	case 20:
	case 32:
	  sim_core_read_buffer (sd, MSP430_CPU (sd), read_map, buf, addr, 4);
	  rv = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
	  break;
	default:
	  assert (! opc->size);
	  break;
	}
#if 0
      /* Hack - MSP430X5438 serial port status register.  */
      if (addr == 0x5dd)
	rv = 2;
#endif
      if (TRACE_MEMORY_P (MSP430_CPU (sd)))
	trace_generic (sd, MSP430_CPU (sd), TRACE_MEMORY_IDX,
		       "GET: [%#x].%d -> %#x", addr, opc->size, rv);
      break;
    default:
      fprintf (stderr, "invalid operand %d type %d\n", n, op->type);
      abort ();
    }

  switch (opc->size)
    {
    case 8:
      rv &= 0xff;
      incval = 1;
      break;
    case 16:
      rv &= 0xffff;
      incval = 2;
      break;
    case 20:
      rv &= 0xfffff;
      incval = 4;
      break;
    case 32:
      rv &= 0xffffffff;
      incval = 4;
      break;
    }

  if (op->type == MSP430_Operand_Indirect_Postinc)
    REG_PUT (op->reg, REG_GET (op->reg) + incval);

  return rv;
}

static int
put_op (SIM_DESC sd, MSP430_Opcode_Decoded *opc, int n, int val)
{
  MSP430_Opcode_Operand *op = opc->op + n;
  int rv;
  int addr;
  unsigned char buf[4];
  int incval = 0;

  switch (opc->size)
    {
    case 8:
      val &= 0xff;
      break;
    case 16:
      val &= 0xffff;
      break;
    case 20:
      val &= 0xfffff;
      break;
    case 32:
      val &= 0xffffffff;
      break;
    }

  switch (op->type)
    {
    case MSP430_Operand_Register:
      REG (op->reg) = val;
      REG_PUT (op->reg, val);
      break;
    case MSP430_Operand_Indirect:
    case MSP430_Operand_Indirect_Postinc:
      addr = op->addend;
      if (op->reg != MSR_None)
	{
	  int reg;
	  /* Index values are signed, but the sum is limited to 16
	     bits if the register < 64k, for MSP430 compatibility in
	     MSP430X chips.  */
	  if (addr & 0x8000)
	    addr |= -1 << 16;
	  reg = REG_GET (op->reg);
	  addr += reg;
	  if (reg < 0x10000)
	    addr &= 0xffff;
	}
      addr &= 0xfffff;

      if (TRACE_MEMORY_P (MSP430_CPU (sd)))
	trace_generic (sd, MSP430_CPU (sd), TRACE_MEMORY_IDX,
		       "PUT: [%#x].%d <- %#x", addr, opc->size, val);
#if 0
      /* Hack - MSP430X5438 serial port transmit register.  */
      if (addr == 0x5ce)
	putchar (val);
#endif
      switch (opc->size)
	{
	case 8:
	  buf[0] = val;
	  sim_core_write_buffer (sd, MSP430_CPU (sd), write_map, buf, addr, 1);
	  break;
	case 16:
	  buf[0] = val;
	  buf[1] = val >> 8;
	  sim_core_write_buffer (sd, MSP430_CPU (sd), write_map, buf, addr, 2);
	  break;
	case 20:
	case 32:
	  buf[0] = val;
	  buf[1] = val >> 8;
	  buf[2] = val >> 16;
	  buf[3] = val >> 24;
	  sim_core_write_buffer (sd, MSP430_CPU (sd), write_map, buf, addr, 4);
	  break;
	default:
	  assert (! opc->size);
	  break;
	}
      break;
    default:
      fprintf (stderr, "invalid operand %d type %d\n", n, op->type);
      abort ();
    }

  switch (opc->size)
    {
    case 8:
      rv &= 0xff;
      incval = 1;
      break;
    case 16:
      rv &= 0xffff;
      incval = 2;
      break;
    case 20:
      rv &= 0xfffff;
      incval = 4;
      break;
    case 32:
      rv &= 0xffffffff;
      incval = 4;
      break;
    }

  if (op->type == MSP430_Operand_Indirect_Postinc)
    {
      int new_val = REG_GET (op->reg) + incval;
      /* SP is always word-aligned.  */
      if (op->reg == MSR_SP && (new_val & 1))
	new_val ++;
      REG_PUT (op->reg, new_val);
    }

  return rv;
}

static void
mem_put_val (SIM_DESC sd, int addr, int val, int bits)
{
  MSP430_Opcode_Decoded opc;

  opc.size = bits;
  opc.op[0].type = MSP430_Operand_Indirect;
  opc.op[0].addend = addr;
  opc.op[0].reg = MSR_None;
  put_op (sd, &opc, 0, val);
}

static int
mem_get_val (SIM_DESC sd, int addr, int bits)
{
  MSP430_Opcode_Decoded opc;

  opc.size = bits;
  opc.op[0].type = MSP430_Operand_Indirect;
  opc.op[0].addend = addr;
  opc.op[0].reg = MSR_None;
  return get_op (sd, &opc, 0);
}

#define CIO_OPEN    (0xF0)
#define CIO_CLOSE   (0xF1)
#define CIO_READ    (0xF2)
#define CIO_WRITE   (0xF3)
#define CIO_LSEEK   (0xF4)
#define CIO_UNLINK  (0xF5)
#define CIO_GETENV  (0xF6)
#define CIO_RENAME  (0xF7)
#define CIO_GETTIME (0xF8)
#define CIO_GETCLK  (0xF9)
#define CIO_SYNC    (0xFF)

#define CIO_I(n) (parms[(n)] + parms[(n)+1] * 256)
#define CIO_L(n) (parms[(n)] + parms[(n)+1] * 256 \
		  + parms[(n)+2] * 65536 + parms[(n)+3] * 16777216)

static void
msp430_cio (SIM_DESC sd)
{
  /* A block of data at __CIOBUF__ describes the I/O operation to
     perform.  */

  unsigned char raw_parms[13];
  unsigned char parms[8];
  long length;
  int command;
  unsigned char buffer[512];
  long ret_buflen = 0;
  long fd, addr, len, rv;

  sim_core_read_buffer (sd, MSP430_CPU (sd), 0, parms,
			MSP430_CPU (sd)->state.cio_buffer, 5);
  length = CIO_I (0);
  command = parms[2];

  sim_core_read_buffer (sd, MSP430_CPU (sd), 0, parms,
			MSP430_CPU (sd)->state.cio_buffer + 3, 8);

  sim_core_read_buffer (sd, MSP430_CPU (sd), 0, buffer,
			MSP430_CPU (sd)->state.cio_buffer + 11, length);

  switch (command)
    {
    case CIO_WRITE:
      fd = CIO_I (0);
      len = CIO_I (2);

      rv = write (fd, buffer, len);
      parms[0] = rv & 0xff;
      parms[1] = rv >> 8;

      break;
    }

  sim_core_write_buffer (sd, MSP430_CPU (sd), 0, parms,
			 MSP430_CPU (sd)->state.cio_buffer + 4, 8);
  if (ret_buflen)
    sim_core_write_buffer (sd, MSP430_CPU (sd), 0, buffer,
			   MSP430_CPU (sd)->state.cio_buffer + 12, ret_buflen);
}

#define SRC     get_op (sd, opcode, 1)
#define DSRC    get_op (sd, opcode, 0)
#define DEST(V) put_op (sd, opcode, 0, (V))

static int
msp430_dis_read (bfd_vma memaddr,
		 bfd_byte *myaddr,
		 unsigned int length,
		 struct disassemble_info *dinfo)
{
  SIM_DESC sd = dinfo->private_data;
  sim_core_read_buffer (sd, MSP430_CPU (sd), 0, myaddr, memaddr, length);
  return 0;
}

#define DO_ALU(OP,SOP,MORE)						\
  {									\
    int s1 = DSRC;							\
    int s2 = SRC;							\
    int result = s1 OP s2 MORE;						\
    if (TRACE_ALU_P (MSP430_CPU (sd)))					\
      trace_generic (sd, MSP430_CPU (sd), TRACE_ALU_IDX,		\
		     "ALU: %#x %s %#x %s = %#x", s1, SOP, s2, #MORE, result); \
    DEST (result);							\
  }

#define SIGN   (1 << (opcode->size - 1))
#define POS(x) (((x) & SIGN) ? 0 : 1)
#define NEG(x) (((x) & SIGN) ? 1 : 0)

static int
zero_ext (int v, int bits)
{
  v &= ((1 << bits) - 1);
  return v;
}

static int
sign_ext (int v, int bits)
{
  int sb = 1 << (bits-1);	/* Sign bit.  */
  int mb = (1 << (bits-1)) - 1; /* Mantissa bits.  */

  if (v & sb)
    v = v | ~mb;
  else
    v = v & mb;
  return v;
}

#define SX(v) sign_ext (v, opcode->size)
#define ZX(v) zero_ext (v, opcode->size)

static char *
flags2string (int f)
{
  static char buf[2][6];
  static int bi = 0;
  char *bp = buf[bi];

  bi = (bi + 1) % 2;

  bp[0] = f & MSP430_FLAG_V ? 'V' : '-';
  bp[1] = f & MSP430_FLAG_N ? 'N' : '-';
  bp[2] = f & MSP430_FLAG_Z ? 'Z' : '-';
  bp[3] = f & MSP430_FLAG_C ? 'C' : '-';
  bp[4] = 0;
  return bp;
}

/* Random number that won't show up in our usual logic.  */
#define MAGIC_OVERFLOW 0x55000F

static void
do_flags (SIM_DESC sd,
	  MSP430_Opcode_Decoded *opcode,
	  int vnz_val, /* Signed result.  */
	  int carry,
	  int overflow)
{
  int f = SR;
  int new_f = 0;
  int signbit = 1 << (opcode->size - 1);

  f &= ~opcode->flags_0;
  f &= ~opcode->flags_set;
  f |= opcode->flags_1;

  if (vnz_val & signbit)
    new_f |= MSP430_FLAG_N;
  if (! (vnz_val & ((signbit << 1) - 1)))
    new_f |= MSP430_FLAG_Z;
  if (overflow == MAGIC_OVERFLOW)
    {
      if (vnz_val != SX (vnz_val))
	new_f |= MSP430_FLAG_V;
    }
  else
    if (overflow)
      new_f |= MSP430_FLAG_V;
  if (carry)
    new_f |= MSP430_FLAG_C;

  new_f = f | (new_f & opcode->flags_set);
  if (TRACE_ALU_P (MSP430_CPU (sd)))
    {
      if (SR != new_f)
	trace_generic (sd, MSP430_CPU (sd), TRACE_ALU_IDX,
		       "FLAGS: %s -> %s", flags2string (SR),
		       flags2string (new_f));
      else
	trace_generic (sd, MSP430_CPU (sd), TRACE_ALU_IDX,
		       "FLAGS: %s", flags2string (new_f));
    }
  SR = new_f;
}

#define FLAGS(vnz,c)    do_flags (sd, opcode, vnz, c, MAGIC_OVERFLOW)
#define FLAGSV(vnz,c,v) do_flags (sd, opcode, vnz, c, v)

/* These two assume unsigned 16-bit (four digit) words.
   Mask off unwanted bits for byte operations.  */

static int
bcd_to_binary (int v)
{
  int r = (  ((v >>  0) & 0xf) * 1
	   + ((v >>  4) & 0xf) * 10
	   + ((v >>  8) & 0xf) * 100
	   + ((v >> 12) & 0xf) * 1000);
  return r;
}

static int
binary_to_bcd (int v)
{
  int r = ( ((v /    1) % 10) <<  0
	  | ((v /   10) % 10) <<  4
	  | ((v /  100) % 10) <<  8
	  | ((v / 1000) % 10) << 12);
  return r;
}

static int
syscall_read_mem (host_callback *cb, struct cb_syscall *sc,
		  unsigned long taddr, char *buf, int bytes)
{
  SIM_DESC sd = (SIM_DESC) sc->p1;
  SIM_CPU *cpu = (SIM_CPU *) sc->p2;

  return sim_core_read_buffer (sd, cpu, read_map, buf, taddr, bytes);
}

static int
syscall_write_mem (host_callback *cb, struct cb_syscall *sc,
		  unsigned long taddr, const char *buf, int bytes)
{
  SIM_DESC sd = (SIM_DESC) sc->p1;
  SIM_CPU *cpu = (SIM_CPU *) sc->p2;

  return sim_core_write_buffer (sd, cpu, write_map, buf, taddr, bytes);
}

static const char *
cond_string (int cond)
{
  switch (cond)
    {
    case MSC_nz:
      return "NZ";
    case MSC_z:
      return "Z";
    case MSC_nc:
      return "NC";
    case MSC_c:
      return "C";
    case MSC_n:
      return "N";
    case MSC_ge:
      return "GE";
    case MSC_l:
      return "L";
    case MSC_true:
      return "MP";
    default:
      return "??";
    }
}

/* Checks a CALL to address CALL_ADDR.  If this is a special
   syscall address then the call is simulated and non-zero is
   returned.  Otherwise 0 is returned.  */

static int
maybe_perform_syscall (SIM_DESC sd, int call_addr)
{
  if (call_addr == 0x00160)
    {
      int i;

      for (i = 0; i < 16; i++)
	{
	  if (i % 4 == 0)
	    fprintf (stderr, "\t");
	  fprintf (stderr, "R%-2d %05x   ", i, MSP430_CPU (sd)->state.regs[i]);
	  if (i % 4 == 3)
	    {
	      int sp = SP + (3 - (i / 4)) * 2;
	      unsigned char buf[2];

	      sim_core_read_buffer (sd, MSP430_CPU (sd), read_map, buf, sp, 2);

	      fprintf (stderr, "\tSP%+d: %04x", sp - SP,
		       buf[0] + buf[1] * 256);

	      if (i / 4 == 0)
		{
		  int flags = SR;

		  fprintf (stderr, flags & 0x100 ? "   V" : "   -");
		  fprintf (stderr, flags & 0x004 ? "N" : "-");
		  fprintf (stderr, flags & 0x002 ? "Z" : "-");
		  fprintf (stderr, flags & 0x001 ? "C" : "-");
		}

	      fprintf (stderr, "\n");
	    }
	}
      return 1;
    }

  if ((call_addr & ~0x3f) == 0x00180)
    {
      /* Syscall!  */
      int syscall_num = call_addr & 0x3f;
      host_callback *cb = STATE_CALLBACK (sd);
      CB_SYSCALL sc;

      CB_SYSCALL_INIT (&sc);

      sc.func = syscall_num;
      sc.arg1 = MSP430_CPU (sd)->state.regs[12];
      sc.arg2 = MSP430_CPU (sd)->state.regs[13];
      sc.arg3 = MSP430_CPU (sd)->state.regs[14];
      sc.arg4 = MSP430_CPU (sd)->state.regs[15];

      if (TRACE_SYSCALL_P (MSP430_CPU (sd)))
	{
	  const char *syscall_name = "*unknown*";

	  switch (syscall_num)
	    {
	    case TARGET_SYS_exit:
	      syscall_name = "exit(%d)";
	      break;
	    case TARGET_SYS_open:
	      syscall_name = "open(%#x,%#x)";
	      break;
	    case TARGET_SYS_close:
	      syscall_name = "close(%d)";
	      break;
	    case TARGET_SYS_read:
	      syscall_name = "read(%d,%#x,%d)";
	      break;
	    case TARGET_SYS_write:
	      syscall_name = "write(%d,%#x,%d)";
	      break;
	    }
	  trace_generic (sd, MSP430_CPU (sd), TRACE_SYSCALL_IDX,
			 syscall_name, sc.arg1, sc.arg2, sc.arg3, sc.arg4);
	}

      /* Handle SYS_exit here.  */
      if (syscall_num == 1)
	{
	  sim_engine_halt (sd, MSP430_CPU (sd), NULL,
			   MSP430_CPU (sd)->state.regs[0],
			   sim_exited, sc.arg1);
	  return 1;
	}

      sc.p1 = sd;
      sc.p2 = MSP430_CPU (sd);
      sc.read_mem = syscall_read_mem;
      sc.write_mem = syscall_write_mem;

      cb_syscall (cb, &sc);

      if (TRACE_SYSCALL_P (MSP430_CPU (sd)))
	trace_generic (sd, MSP430_CPU (sd), TRACE_SYSCALL_IDX,
		       "returns %d", sc.result);

      MSP430_CPU (sd)->state.regs[12] = sc.result;
      return 1;
    }

  return 0;
}

static void
msp430_step_once (SIM_DESC sd)
{
  Get_Byte_Local_Data ld;
  unsigned char buf[100];
  int i;
  int opsize;
  unsigned int opcode_pc;
  MSP430_Opcode_Decoded opcode_buf;
  MSP430_Opcode_Decoded *opcode = &opcode_buf;
  int s1, s2, result;
  int u1, u2, uresult;
  int c, reg;
  int sp;
  int carry_to_use;
  int n_repeats;
  int rept;
  int op_bytes, op_bits;

  PC &= 0xfffff;
  opcode_pc = PC;

  if (opcode_pc < 0x10)
    {
      fprintf (stderr, "Fault: PC(%#x) is less than 0x10\n", opcode_pc);
      sim_engine_halt (sd, MSP430_CPU (sd), NULL,
		       MSP430_CPU (sd)->state.regs[0],
		       sim_exited, -1);
      return;
    }

  if (PC == MSP430_CPU (sd)->state.cio_breakpoint
      && STATE_OPEN_KIND (sd) != SIM_OPEN_DEBUG)
    msp430_cio (sd);

  ld.sd = sd;
  ld.gb_addr = PC;
  opsize = msp430_decode_opcode (MSP430_CPU (sd)->state.regs[0],
				 opcode, msp430_getbyte, &ld);
  PC += opsize;
  if (opsize <= 0)
    {
      fprintf (stderr, "Fault: undecodable opcode at %#x\n", opcode_pc);
      sim_engine_halt (sd, MSP430_CPU (sd), NULL,
		       MSP430_CPU (sd)->state.regs[0],
		       sim_exited, -1);
      return;
    }

  if (opcode->repeat_reg)
    n_repeats = (MSP430_CPU (sd)->state.regs[opcode->repeats] & 0x000f) + 1;
  else
    n_repeats = opcode->repeats + 1;

  op_bits = opcode->size;
  switch (op_bits)
    {
    case 8:
      op_bytes = 1;
      break;
    case 16:
      op_bytes = 2;
      break;
    case 20:
    case 32:
      op_bytes = 4;
      break;
    }

  if (TRACE_INSN_P (MSP430_CPU (sd)))
    {
      disassemble_info info;
      unsigned char b[10];

      msp430_trace_one (opcode_pc);

      sim_core_read_buffer (sd, MSP430_CPU (sd), 0, b, opcode_pc, opsize);

      init_disassemble_info (&info, stderr, fprintf);
      info.private_data = sd;
      info.read_memory_func = msp430_dis_read;
      fprintf (stderr, "%#8x  ", opcode_pc);
      for (i = 0; i < opsize; i += 2)
	fprintf (stderr, " %02x%02x", b[i+1], b[i]);
      for (; i < 6; i += 2)
	fprintf (stderr, "     ");
      fprintf (stderr, "  ");
      print_insn_msp430 (opcode_pc, &info);
      fprintf (stderr, "\n");
      fflush (stdout);
    }

  if (TRACE_ANY_P (MSP430_CPU (sd)))
    trace_prefix (sd, MSP430_CPU (sd), NULL_CIA, opcode_pc,
    TRACE_LINENUM_P (MSP430_CPU (sd)), NULL, 0, "");

  carry_to_use = 0;
  switch (opcode->id)
    {
    case MSO_unknown:
      break;

      /* Double-operand instructions.  */
    case MSO_mov:
      if (opcode->n_bytes == 2
	  && opcode->op[0].type == MSP430_Operand_Register
	  && opcode->op[0].reg == MSR_CG
	  && opcode->op[1].type == MSP430_Operand_Immediate
	  && opcode->op[1].addend == 0
	  /* A 16-bit write of #0 is a NOP; an 8-bit write is a BRK.  */
	  && opcode->size == 8)
	{
	  /* This is the designated software breakpoint instruction.  */
	  PC -= opsize;
	  sim_engine_halt (sd, MSP430_CPU (sd), NULL,
			   MSP430_CPU (sd)->state.regs[0],
			   sim_stopped, SIM_SIGTRAP);

	}
      else
	{
	  /* Otherwise, do the move.  */
	  for (rept = 0; rept < n_repeats; rept ++)
	    {
	      DEST (SRC);
	    }
	}
      break;

    case MSO_addc:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  carry_to_use = (SR & MSP430_FLAG_C) ? 1 : 0;
	  u1 = DSRC;
	  u2 = SRC;
	  s1 = SX (u1);
	  s2 = SX (u2);
	  uresult = u1 + u2 + carry_to_use;
	  result = s1 + s2 + carry_to_use;
	  if (TRACE_ALU_P (MSP430_CPU (sd)))
	    trace_generic (sd, MSP430_CPU (sd), TRACE_ALU_IDX,
			   "ADDC: %#x + %#x + %d = %#x",
			   u1, u2, carry_to_use, uresult);
	  DEST (result);
	  FLAGS (result, uresult != ZX (uresult));
	}
      break;

    case MSO_add:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = DSRC;
	  u2 = SRC;
	  s1 = SX (u1);
	  s2 = SX (u2);
	  uresult = u1 + u2;
	  result = s1 + s2;
	  if (TRACE_ALU_P (MSP430_CPU (sd)))
	    trace_generic (sd, MSP430_CPU (sd), TRACE_ALU_IDX,
			   "ADD: %#x + %#x = %#x",
			   u1, u2, uresult);
	  DEST (result);
	  FLAGS (result, uresult != ZX (uresult));
	}
      break;

    case MSO_subc:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  carry_to_use = (SR & MSP430_FLAG_C) ? 1 : 0;
	  u1 = DSRC;
	  u2 = SRC;
	  s1 = SX (u1);
	  s2 = SX (u2);
	  uresult = ZX (~u2) + u1 + carry_to_use;
	  result = s1 - s2 + (carry_to_use - 1);
	  if (TRACE_ALU_P (MSP430_CPU (sd)))
	    trace_generic (sd, MSP430_CPU (sd), TRACE_ALU_IDX,
			   "SUBC: %#x - %#x + %d = %#x",
			   u1, u2, carry_to_use, uresult);
	  DEST (result);
	  FLAGS (result, uresult != ZX (uresult));
	}
      break;

    case MSO_sub:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = DSRC;
	  u2 = SRC;
	  s1 = SX (u1);
	  s2 = SX (u2);
	  uresult = ZX (~u2) + u1 + 1;
	  result = SX (uresult);
	  if (TRACE_ALU_P (MSP430_CPU (sd)))
	    trace_generic (sd, MSP430_CPU (sd), TRACE_ALU_IDX,
			   "SUB: %#x - %#x = %#x",
			   u1, u2, uresult);
	  DEST (result);
	  FLAGS (result, uresult != ZX (uresult));
	}
      break;

    case MSO_cmp:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = DSRC;
	  u2 = SRC;
	  s1 = SX (u1);
	  s2 = SX (u2);
	  uresult = ZX (~u2) + u1 + 1;
	  result = s1 - s2;
	  if (TRACE_ALU_P (MSP430_CPU (sd)))
	    trace_generic (sd, MSP430_CPU (sd), TRACE_ALU_IDX,
			   "CMP: %#x - %#x = %x",
			   u1, u2, uresult);
	  FLAGS (result, uresult != ZX (uresult));
	}
      break;

    case MSO_dadd:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  carry_to_use = (SR & MSP430_FLAG_C) ? 1 : 0;
	  u1 = DSRC;
	  u2 = SRC;
	  uresult = bcd_to_binary (u1) + bcd_to_binary (u2) + carry_to_use;
	  result = binary_to_bcd (uresult);
	  if (TRACE_ALU_P (MSP430_CPU (sd)))
	    trace_generic (sd, MSP430_CPU (sd), TRACE_ALU_IDX,
			   "DADD: %#x + %#x + %d = %#x",
			   u1, u2, carry_to_use, result);
	  DEST (result);
	  FLAGS (result, uresult > ((opcode->size == 8) ? 99 : 9999));
	}
      break;

    case MSO_and:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = DSRC;
	  u2 = SRC;
	  uresult = u1 & u2;
	  if (TRACE_ALU_P (MSP430_CPU (sd)))
	    trace_generic (sd, MSP430_CPU (sd), TRACE_ALU_IDX,
			   "AND: %#x & %#x = %#x",
			   u1, u2, uresult);
	  DEST (uresult);
	  FLAGS (uresult, uresult != 0);
	}
      break;

    case MSO_bit:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = DSRC;
	  u2 = SRC;
	  uresult = u1 & u2;
	  if (TRACE_ALU_P (MSP430_CPU (sd)))
	    trace_generic (sd, MSP430_CPU (sd), TRACE_ALU_IDX,
			   "BIT: %#x & %#x -> %#x",
			   u1, u2, uresult);
	  FLAGS (uresult, uresult != 0);
	}
      break;

    case MSO_bic:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = DSRC;
	  u2 = SRC;
	  uresult = u1 & ~ u2;
	  if (TRACE_ALU_P (MSP430_CPU (sd)))
	    trace_generic (sd, MSP430_CPU (sd), TRACE_ALU_IDX,
			   "BIC: %#x & ~ %#x = %#x",
			   u1, u2, uresult);
	  DEST (uresult);
	}
      break;

    case MSO_bis:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = DSRC;
	  u2 = SRC;
	  uresult = u1 | u2;
	  if (TRACE_ALU_P (MSP430_CPU (sd)))
	    trace_generic (sd, MSP430_CPU (sd), TRACE_ALU_IDX,
			   "BIS: %#x | %#x = %#x",
			   u1, u2, uresult);
	  DEST (uresult);
	}
      break;

    case MSO_xor:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  s1 = 1 << (opcode->size - 1);
	  u1 = DSRC;
	  u2 = SRC;
	  uresult = u1 ^ u2;
	  if (TRACE_ALU_P (MSP430_CPU (sd)))
	    trace_generic (sd, MSP430_CPU (sd), TRACE_ALU_IDX,
			   "XOR: %#x & %#x = %#x",
			   u1, u2, uresult);
	  DEST (uresult);
	  FLAGSV (uresult, uresult != 0, (u1 & s1) && (u2 & s1));
	}
      break;

    /* Single-operand instructions.  Note: the decoder puts the same
       operand in SRC as in DEST, for our convenience.  */

    case MSO_rrc:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = SRC;
	  carry_to_use = u1 & 1;
	  uresult = u1 >> 1;
	  if (SR & MSP430_FLAG_C)
	  uresult |= (1 << (opcode->size - 1));
	  if (TRACE_ALU_P (MSP430_CPU (sd)))
	    trace_generic (sd, MSP430_CPU (sd), TRACE_ALU_IDX,
			   "RRC: %#x >>= %#x",
			   u1, uresult);
	  DEST (uresult);
	  FLAGS (uresult, carry_to_use);
	}
      break;

    case MSO_swpb:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = SRC;
	  uresult = ((u1 >> 8) & 0x00ff) | ((u1 << 8) & 0xff00);
	  if (TRACE_ALU_P (MSP430_CPU (sd)))
	    trace_generic (sd, MSP430_CPU (sd), TRACE_ALU_IDX,
			   "SWPB: %#x -> %#x",
			   u1, uresult);
	  DEST (uresult);
	}
      break;

    case MSO_rra:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = SRC;
	  c = u1 & 1;
	  s1 = 1 << (opcode->size - 1);
	  uresult = (u1 >> 1) | (u1 & s1);
	  if (TRACE_ALU_P (MSP430_CPU (sd)))
	    trace_generic (sd, MSP430_CPU (sd), TRACE_ALU_IDX,
			   "RRA: %#x >>= %#x",
			   u1, uresult);
	  DEST (uresult);
	  FLAGS (uresult, c);
	}
      break;

    case MSO_rru:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = SRC;
	  c = u1 & 1;
	  uresult = (u1 >> 1);
	  if (TRACE_ALU_P (MSP430_CPU (sd)))
	    trace_generic (sd, MSP430_CPU (sd), TRACE_ALU_IDX,
			   "RRU: %#x >>= %#x",
			   u1, uresult);
	  DEST (uresult);
	  FLAGS (uresult, c);
	}
      break;

    case MSO_sxt:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  u1 = SRC;
	  if (u1 & 0x80)
	    uresult = u1 | 0xfff00;
	  else
	    uresult = u1 & 0x000ff;
	  if (TRACE_ALU_P (MSP430_CPU (sd)))
	    trace_generic (sd, MSP430_CPU (sd), TRACE_ALU_IDX,
			   "SXT: %#x -> %#x",
			   u1, uresult);
	  DEST (uresult);
	  FLAGS (uresult, c);
	}
      break;

    case MSO_push:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  int new_sp;

	  new_sp = REG_GET (MSR_SP) - op_bytes;
	  /* SP is always word-aligned.  */
	  if (new_sp & 1)
	    new_sp --;
	  REG_PUT (MSR_SP, new_sp);
	  u1 = SRC;
	  mem_put_val (sd, SP, u1, op_bits);
	  if (opcode->op[1].type == MSP430_Operand_Register)
	    opcode->op[1].reg --;
	}
      break;

    case MSO_pop:
      for (rept = 0; rept < n_repeats; rept ++)
	{
	  int new_sp;

	  u1 = mem_get_val (sd, SP, op_bits);
	  DEST (u1);
	  if (opcode->op[0].type == MSP430_Operand_Register)
	    opcode->op[0].reg ++;
	  new_sp = REG_GET (MSR_SP) + op_bytes;
	  /* SP is always word-aligned.  */
	  if (new_sp & 1)
	    new_sp ++;
	  REG_PUT (MSR_SP, new_sp);
	}
      break;

    case MSO_call:
      u1 = SRC;

      if (maybe_perform_syscall (sd, u1))
	break;

      REG_PUT (MSR_SP, REG_GET (MSR_SP) - op_bytes);
      mem_put_val (sd, SP, PC, op_bits);
      if (TRACE_ALU_P (MSP430_CPU (sd)))
	trace_generic (sd, MSP430_CPU (sd), TRACE_ALU_IDX,
		       "CALL: func %#x ret %#x, sp %#x",
		       u1, PC, SP);
      REG_PUT (MSR_PC, u1);
      break;

    case MSO_reti:
      SR = mem_get_val (sd, SP, op_bits);
      SP += 2;
      PC = mem_get_val (sd, SP, op_bits);
      SP += 2;
      if (TRACE_ALU_P (MSP430_CPU (sd)))
	trace_generic (sd, MSP430_CPU (sd), TRACE_ALU_IDX,
		       "RETI: pc %#x sr %#x",
		       PC, SR);
      break;

      /* Jumps.  */

    case MSO_jmp:
      i = SRC;
      switch (opcode->cond)
	{
	case MSC_nz:
	  u1 = (SR & MSP430_FLAG_Z) ? 0 : 1;
	  break;
	case MSC_z:
	  u1 = (SR & MSP430_FLAG_Z) ? 1 : 0;
	  break;
	case MSC_nc:
	  u1 = (SR & MSP430_FLAG_C) ? 0 : 1;
	  break;
	case MSC_c:
	  u1 = (SR & MSP430_FLAG_C) ? 1 : 0;
	  break;
	case MSC_n:
	  u1 = (SR & MSP430_FLAG_N) ? 1 : 0;
	  break;
	case MSC_ge:
	  u1 = (!!(SR & MSP430_FLAG_N) == !!(SR & MSP430_FLAG_V)) ? 1 : 0;
	  break;
	case MSC_l:
	  u1 = (!!(SR & MSP430_FLAG_N) == !!(SR & MSP430_FLAG_V)) ? 0 : 1;
	  break;
	case MSC_true:
	  u1 = 1;
	  break;
	}

      if (u1)
	{
	  if (TRACE_BRANCH_P (MSP430_CPU (sd)))
	    trace_generic (sd, MSP430_CPU (sd), TRACE_BRANCH_IDX,
			   "J%s: pc %#x -> %#x sr %#x, taken",
			   cond_string (opcode->cond), PC, i, SR);
	  PC = i;
	  if (PC == opcode_pc)
	    exit (0);
	}
      else
	if (TRACE_BRANCH_P (MSP430_CPU (sd)))
	  trace_generic (sd, MSP430_CPU (sd), TRACE_BRANCH_IDX,
			 "J%s: pc %#x to %#x sr %#x, not taken",
			 cond_string (opcode->cond), PC, i, SR);
      break;

    default:
      fprintf (stderr, "error: unexpected opcode id %d\n", opcode->id);
      exit (1);
    }
}

void
sim_engine_run (SIM_DESC sd,
		int next_cpu_nr,
		int nr_cpus,
		int siggnal)
{
  while (1)
    {
      msp430_step_once (sd);
      if (sim_events_tick (sd))
	sim_events_process (sd);
    }
}
