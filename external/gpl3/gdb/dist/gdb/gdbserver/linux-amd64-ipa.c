/* GNU/Linux/x86-64 specific low level interface, for the in-process
   agent library for GDB.

   Copyright (C) 2010-2014 Free Software Foundation, Inc.

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
#include "tracepoint.h"

/* Defined in auto-generated file amd64-linux.c.  */
void init_registers_amd64_linux (void);
extern const struct target_desc *tdesc_amd64_linux;

/* fast tracepoints collect registers.  */

#define FT_CR_RIP 0
#define FT_CR_EFLAGS 1
#define FT_CR_R8 2
#define FT_CR_R9 3
#define FT_CR_R10 4
#define FT_CR_R11 5
#define FT_CR_R12 6
#define FT_CR_R13 7
#define FT_CR_R14 8
#define FT_CR_R15 9
#define FT_CR_RAX 10
#define FT_CR_RBX 11
#define FT_CR_RCX 12
#define FT_CR_RDX 13
#define FT_CR_RSI 14
#define FT_CR_RDI 15
#define FT_CR_RBP 16
#define FT_CR_RSP 17

static const int x86_64_ft_collect_regmap[] = {
  FT_CR_RAX * 8, FT_CR_RBX * 8, FT_CR_RCX * 8, FT_CR_RDX * 8,
  FT_CR_RSI * 8, FT_CR_RDI * 8, FT_CR_RBP * 8, FT_CR_RSP * 8,
  FT_CR_R8 * 8,  FT_CR_R9 * 8,  FT_CR_R10 * 8, FT_CR_R11 * 8,
  FT_CR_R12 * 8, FT_CR_R13 * 8, FT_CR_R14 * 8, FT_CR_R15 * 8,
  FT_CR_RIP * 8, FT_CR_EFLAGS * 8
};

#define X86_64_NUM_FT_COLLECT_GREGS \
  (sizeof (x86_64_ft_collect_regmap) / sizeof(x86_64_ft_collect_regmap[0]))

void
supply_fast_tracepoint_registers (struct regcache *regcache,
				  const unsigned char *buf)
{
  int i;

  for (i = 0; i < X86_64_NUM_FT_COLLECT_GREGS; i++)
    supply_register (regcache, i,
		     ((char *) buf) + x86_64_ft_collect_regmap[i]);
}

ULONGEST __attribute__ ((visibility("default"), used))
gdb_agent_get_raw_reg (const unsigned char *raw_regs, int regnum)
{
  if (regnum >= X86_64_NUM_FT_COLLECT_GREGS)
    return 0;

  return *(ULONGEST *) (raw_regs + x86_64_ft_collect_regmap[regnum]);
}

#ifdef HAVE_UST

#include <ust/processor.h>

/* "struct registers" is the UST object type holding the registers at
   the time of the static tracepoint marker call.  This doesn't
   contain RIP, but we know what it must have been (the marker
   address).  */

#define ST_REGENTRY(REG)			\
  {						\
    offsetof (struct registers, REG),		\
    sizeof (((struct registers *) NULL)->REG)	\
  }

static struct
{
  int offset;
  int size;
} x86_64_st_collect_regmap[] =
  {
    ST_REGENTRY(rax),
    ST_REGENTRY(rbx),
    ST_REGENTRY(rcx),
    ST_REGENTRY(rdx),
    ST_REGENTRY(rsi),
    ST_REGENTRY(rdi),
    ST_REGENTRY(rbp),
    ST_REGENTRY(rsp),
    ST_REGENTRY(r8),
    ST_REGENTRY(r9),
    ST_REGENTRY(r10),
    ST_REGENTRY(r11),
    ST_REGENTRY(r12),
    ST_REGENTRY(r13),
    ST_REGENTRY(r14),
    ST_REGENTRY(r15),
    { -1, 0 },
    ST_REGENTRY(rflags),
    ST_REGENTRY(cs),
    ST_REGENTRY(ss),
  };

#define X86_64_NUM_ST_COLLECT_GREGS \
  (sizeof (x86_64_st_collect_regmap) / sizeof (x86_64_st_collect_regmap[0]))

/* GDB's RIP register number.  */
#define AMD64_RIP_REGNUM 16

void
supply_static_tracepoint_registers (struct regcache *regcache,
				    const unsigned char *buf,
				    CORE_ADDR pc)
{
  int i;
  unsigned long newpc = pc;

  supply_register (regcache, AMD64_RIP_REGNUM, &newpc);

  for (i = 0; i < X86_64_NUM_ST_COLLECT_GREGS; i++)
    if (x86_64_st_collect_regmap[i].offset != -1)
      {
	switch (x86_64_st_collect_regmap[i].size)
	  {
	  case 8:
	    supply_register (regcache, i,
			     ((char *) buf)
			     + x86_64_st_collect_regmap[i].offset);
	    break;
	  case 2:
	    {
	      unsigned long reg
		= * (short *) (((char *) buf)
			       + x86_64_st_collect_regmap[i].offset);
	      reg &= 0xffff;
	      supply_register (regcache, i, &reg);
	    }
	    break;
	  default:
	    internal_error (__FILE__, __LINE__,
			    "unhandled register size: %d",
			    x86_64_st_collect_regmap[i].size);
	    break;
	  }
      }
}

#endif /* HAVE_UST */

void
initialize_low_tracepoint (void)
{
  init_registers_amd64_linux ();
  ipa_tdesc = tdesc_amd64_linux;
}
