/* Low level interface to ptrace, for the remote server for GDB.
   Copyright (C) 1995-2013 Free Software Foundation, Inc.

   This file is part of GDB.

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

#include "server.h"
#include "linux-low.h"

#include <sys/ptrace.h>

#include "gdb_proc_service.h"

/* The stack pointer is offset from the stack frame by a BIAS of 2047
   (0x7ff) for 64-bit code.  BIAS is likely to be defined on SPARC
   hosts, so undefine it first.  */
#undef BIAS
#define BIAS 2047

#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif

#define INSN_SIZE 4

#define SPARC_R_REGS_NUM 32
#define SPARC_F_REGS_NUM 48
#define SPARC_CONTROL_REGS_NUM 6

#define sparc_num_regs \
  (SPARC_R_REGS_NUM + SPARC_F_REGS_NUM + SPARC_CONTROL_REGS_NUM)

/* Each offset is multiplied by 8, because of the register size.
   These offsets apply to the buffer sent/filled by ptrace.
   Additionally, the array elements order corresponds to the .dat file, and the
   gdb's registers enumeration order.  */

static int sparc_regmap[] = {
  /* These offsets correspond to GET/SETREGSET.  */
	-1,  0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,	 /* g0 .. g7 */
	7*8,  8*8,  9*8, 10*8, 11*8, 12*8, 13*8, 14*8,	 /* o0 .. o5, sp, o7 */
	-1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,	 /* l0 .. l7 */
	-1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,	 /* i0 .. i5, fp, i7 */

  /* Floating point registers offsets correspond to GET/SETFPREGSET.  */
    0*4,  1*4,  2*4,  3*4,  4*4,  5*4,  6*4,  7*4,	   /*  f0 ..  f7 */
    8*4,  9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4,	   /*  f8 .. f15 */
   16*4, 17*4, 18*4, 19*4, 20*4, 21*4, 22*4, 23*4,	   /* f16 .. f23 */
   24*4, 25*4, 26*4, 27*4, 28*4, 29*4, 30*4, 31*4,	   /* f24 .. f31 */

  /* F32 offset starts next to f31: 31*4+4 = 16 * 8.  */
   16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8,	   /* f32 .. f46 */
   24*8, 25*8, 26*8, 27*8, 28*8, 29*8, 30*8, 31*8,	   /* f48 .. f62 */

   17 *8, /*    pc */
   18 *8, /*   npc */
   16 *8, /* state */
  /* FSR offset also corresponds to GET/SETFPREGSET, ans is placed
     next to f62.  */
   32 *8, /*   fsr */
      -1, /*  fprs */
  /* Y register is 32-bits length, but gdb takes care of that.  */
   19 *8, /*     y */

};


struct regs_range_t
{
  int regno_start;
  int regno_end;
};

static const struct regs_range_t gregs_ranges[] = {
 {  0, 31 }, /*   g0 .. i7  */
 { 80, 82 }, /*   pc .. state */
 { 84, 85 }  /* fprs .. y */
};

#define N_GREGS_RANGES (sizeof (gregs_ranges) / sizeof (struct regs_range_t))

static const struct regs_range_t fpregs_ranges[] = {
 { 32, 79 }, /* f0 .. f62  */
 { 83, 83 }  /* fsr */
};

#define N_FPREGS_RANGES (sizeof (fpregs_ranges) / sizeof (struct regs_range_t))

/* Defined in auto-generated file reg-sparc64.c.  */
void init_registers_sparc64 (void);

static int
sparc_cannot_store_register (int regno)
{
  return (regno >= sparc_num_regs || sparc_regmap[regno] == -1);
}

static int
sparc_cannot_fetch_register (int regno)
{
  return (regno >= sparc_num_regs || sparc_regmap[regno] == -1);
}

static void
sparc_fill_gregset_to_stack (struct regcache *regcache, const void *buf)
{
  int i;
  CORE_ADDR addr = 0;
  unsigned char tmp_reg_buf[8];
  const int l0_regno = find_regno ("l0");
  const int i7_regno = l0_regno + 15;

  /* These registers have to be stored in the stack.  */
  memcpy (&addr,
	  ((char *) buf) + sparc_regmap[find_regno ("sp")],
	  sizeof (addr));

  addr += BIAS;

  for (i = l0_regno; i <= i7_regno; i++)
    {
      collect_register (regcache, i, tmp_reg_buf);
      (*the_target->write_memory) (addr, tmp_reg_buf, sizeof (tmp_reg_buf));
      addr += sizeof (tmp_reg_buf);
    }
}

static void
sparc_fill_gregset (struct regcache *regcache, void *buf)
{
  int i;
  int range;

  for (range = 0; range < N_GREGS_RANGES; range++)
    for (i = gregs_ranges[range].regno_start;
	 i <= gregs_ranges[range].regno_end; i++)
      if (sparc_regmap[i] != -1)
	collect_register (regcache, i, ((char *) buf) + sparc_regmap[i]);

  sparc_fill_gregset_to_stack (regcache, buf);
}

static void
sparc_fill_fpregset (struct regcache *regcache, void *buf)
{
  int i;
  int range;

  for (range = 0; range < N_FPREGS_RANGES; range++)
    for (i = fpregs_ranges[range].regno_start;
	 i <= fpregs_ranges[range].regno_end; i++)
      collect_register (regcache, i, ((char *) buf) + sparc_regmap[i]);

}

static void
sparc_store_gregset_from_stack (struct regcache *regcache, const void *buf)
{
  int i;
  CORE_ADDR addr = 0;
  unsigned char tmp_reg_buf[8];
  const int l0_regno = find_regno ("l0");
  const int i7_regno = l0_regno + 15;

  /* These registers have to be obtained from the stack.  */
  memcpy (&addr,
	  ((char *) buf) + sparc_regmap[find_regno ("sp")],
	  sizeof (addr));

  addr += BIAS;

  for (i = l0_regno; i <= i7_regno; i++)
    {
      (*the_target->read_memory) (addr, tmp_reg_buf, sizeof (tmp_reg_buf));
      supply_register (regcache, i, tmp_reg_buf);
      addr += sizeof (tmp_reg_buf);
    }
}

static void
sparc_store_gregset (struct regcache *regcache, const void *buf)
{
  int i;
  char zerobuf[8];
  int range;

  memset (zerobuf, 0, sizeof (zerobuf));

  for (range = 0; range < N_GREGS_RANGES; range++)
    for (i = gregs_ranges[range].regno_start;
	 i <= gregs_ranges[range].regno_end; i++)
      if (sparc_regmap[i] != -1)
	supply_register (regcache, i, ((char *) buf) + sparc_regmap[i]);
      else
	supply_register (regcache, i, zerobuf);

  sparc_store_gregset_from_stack (regcache, buf);
}

static void
sparc_store_fpregset (struct regcache *regcache, const void *buf)
{
  int i;
  int range;

  for (range = 0; range < N_FPREGS_RANGES; range++)
    for (i = fpregs_ranges[range].regno_start;
	 i <= fpregs_ranges[range].regno_end;
	 i++)
      supply_register (regcache, i, ((char *) buf) + sparc_regmap[i]);
}

extern int debug_threads;

static CORE_ADDR
sparc_get_pc (struct regcache *regcache)
{
  unsigned long pc;
  collect_register_by_name (regcache, "pc", &pc);
  if (debug_threads)
    fprintf (stderr, "stop pc is %08lx\n", pc);
  return pc;
}

static const unsigned char sparc_breakpoint[INSN_SIZE] = {
  0x91, 0xd0, 0x20, 0x01
};
#define sparc_breakpoint_len INSN_SIZE


static int
sparc_breakpoint_at (CORE_ADDR where)
{
  unsigned char insn[INSN_SIZE];

  (*the_target->read_memory) (where, (unsigned char *) insn, sizeof (insn));

  if (memcmp (sparc_breakpoint, insn, sizeof (insn)) == 0)
    return 1;

  /* If necessary, recognize more trap instructions here.  GDB only
     uses TRAP Always.  */

  return 0;
}

/* We only place breakpoints in empty marker functions, and thread locking
   is outside of the function.  So rather than importing software single-step,
   we can just run until exit.  */
static CORE_ADDR
sparc_reinsert_addr (void)
{
  struct regcache *regcache = get_thread_regcache (current_inferior, 1);
  CORE_ADDR lr;
  /* O7 is the equivalent to the 'lr' of other archs.  */
  collect_register_by_name (regcache, "o7", &lr);
  return lr;
}


struct regset_info target_regsets[] = {
  { PTRACE_GETREGS, PTRACE_SETREGS, 0, sizeof (elf_gregset_t),
    GENERAL_REGS,
    sparc_fill_gregset, sparc_store_gregset },
  { PTRACE_GETFPREGS, PTRACE_SETFPREGS, 0, sizeof (fpregset_t),
    FP_REGS,
    sparc_fill_fpregset, sparc_store_fpregset },
  { 0, 0, 0, -1, -1, NULL, NULL }
};

struct linux_target_ops the_low_target = {
  init_registers_sparc64,
  sparc_num_regs,
  /* No regmap needs to be provided since this impl. doesn't use USRREGS.  */
  NULL,
  NULL,
  sparc_cannot_fetch_register,
  sparc_cannot_store_register,
  NULL, /* fetch_register */
  sparc_get_pc,
  /* No sparc_set_pc is needed.  */
  NULL,
  (const unsigned char *) sparc_breakpoint,
  sparc_breakpoint_len,
  sparc_reinsert_addr,
  0,
  sparc_breakpoint_at,
  NULL, NULL, NULL, NULL,
  NULL, NULL
};
