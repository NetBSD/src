/* GNU/Linux/x86 specific low level interface, for the in-process
   agent library for GDB.

   Copyright (C) 2010, 2011 Free Software Foundation, Inc.

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

/* GDB register numbers.  */

enum i386_gdb_regnum
{
  I386_EAX_REGNUM,		/* %eax */
  I386_ECX_REGNUM,		/* %ecx */
  I386_EDX_REGNUM,		/* %edx */
  I386_EBX_REGNUM,		/* %ebx */
  I386_ESP_REGNUM,		/* %esp */
  I386_EBP_REGNUM,		/* %ebp */
  I386_ESI_REGNUM,		/* %esi */
  I386_EDI_REGNUM,		/* %edi */
  I386_EIP_REGNUM,		/* %eip */
  I386_EFLAGS_REGNUM,		/* %eflags */
  I386_CS_REGNUM,		/* %cs */
  I386_SS_REGNUM,		/* %ss */
  I386_DS_REGNUM,		/* %ds */
  I386_ES_REGNUM,		/* %es */
  I386_FS_REGNUM,		/* %fs */
  I386_GS_REGNUM,		/* %gs */
  I386_ST0_REGNUM		/* %st(0) */
};

#define i386_num_regs 16

/* Defined in auto-generated file i386-linux.c.  */
void init_registers_i386_linux (void);

#define FT_CR_EAX 15
#define FT_CR_ECX 14
#define FT_CR_EDX 13
#define FT_CR_EBX 12
#define FT_CR_UESP 11
#define FT_CR_EBP 10
#define FT_CR_ESI 9
#define FT_CR_EDI 8
#define FT_CR_EIP 7
#define FT_CR_EFL 6
#define FT_CR_DS 5
#define FT_CR_ES 4
#define FT_CR_FS 3
#define FT_CR_GS 2
#define FT_CR_SS 1
#define FT_CR_CS 0

/* Mapping between the general-purpose registers in jump tracepoint
   format and GDB's register array layout.  */

static const int i386_ft_collect_regmap[] =
{
  FT_CR_EAX * 4, FT_CR_ECX * 4, FT_CR_EDX * 4, FT_CR_EBX * 4,
  FT_CR_UESP * 4, FT_CR_EBP * 4, FT_CR_ESI * 4, FT_CR_EDI * 4,
  FT_CR_EIP * 4, FT_CR_EFL * 4, FT_CR_CS * 4, FT_CR_SS * 4,
  FT_CR_DS * 4, FT_CR_ES * 4, FT_CR_FS * 4, FT_CR_GS * 4
};

void
supply_fast_tracepoint_registers (struct regcache *regcache,
				  const unsigned char *buf)
{
  int i;

  for (i = 0; i < i386_num_regs; i++)
    {
      int regval;

      if (i >= I386_CS_REGNUM && i <= I386_GS_REGNUM)
	regval = *(short *) (((char *) buf) + i386_ft_collect_regmap[i]);
      else
	regval = *(int *) (((char *) buf) + i386_ft_collect_regmap[i]);

      supply_register (regcache, i, &regval);
    }
}

ULONGEST __attribute__ ((visibility("default"), used))
gdb_agent_get_raw_reg (unsigned char *raw_regs, int regnum)
{
  /* This should maybe be allowed to return an error code, or perhaps
     better, have the emit_reg detect this, and emit a constant zero,
     or something.  */

  if (regnum > i386_num_regs)
    return 0;
  else if (regnum >= I386_CS_REGNUM && regnum <= I386_GS_REGNUM)
    return *(short *) (raw_regs + i386_ft_collect_regmap[regnum]);
  else
    return *(int *) (raw_regs + i386_ft_collect_regmap[regnum]);
}

#ifdef HAVE_UST

#include <ust/processor.h>

/* "struct registers" is the UST object type holding the registers at
   the time of the static tracepoint marker call.  This doesn't
   contain EIP, but we know what it must have been (the marker
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
} i386_st_collect_regmap[] =
  {
    ST_REGENTRY(eax),
    ST_REGENTRY(ecx),
    ST_REGENTRY(edx),
    ST_REGENTRY(ebx),
    ST_REGENTRY(esp),
    ST_REGENTRY(ebp),
    ST_REGENTRY(esi),
    ST_REGENTRY(edi),
    { -1, 0 }, /* eip */
    ST_REGENTRY(eflags),
    ST_REGENTRY(cs),
    ST_REGENTRY(ss),
  };

#define i386_NUM_ST_COLLECT_GREGS \
  (sizeof (i386_st_collect_regmap) / sizeof (i386_st_collect_regmap[0]))

void
supply_static_tracepoint_registers (struct regcache *regcache,
				    const unsigned char *buf,
				    CORE_ADDR pc)
{
  int i;
  unsigned int newpc = pc;

  supply_register (regcache, I386_EIP_REGNUM, &newpc);

  for (i = 0; i < i386_NUM_ST_COLLECT_GREGS; i++)
    if (i386_st_collect_regmap[i].offset != -1)
      {
	switch (i386_st_collect_regmap[i].size)
	  {
	  case 4:
	    supply_register (regcache, i,
			     ((char *) buf)
			     + i386_st_collect_regmap[i].offset);
	    break;
	  case 2:
	    {
	      unsigned long reg
		= * (short *) (((char *) buf)
			       + i386_st_collect_regmap[i].offset);
	      reg &= 0xffff;
	      supply_register (regcache, i, &reg);
	    }
	    break;
	  default:
	    internal_error (__FILE__, __LINE__, "unhandled register size: %d",
			    i386_st_collect_regmap[i].size);
	  }
      }
}

#endif /* HAVE_UST */


/* This is only needed because reg-i386-linux-lib.o references it.  We
   may use it proper at some point.  */
const char *gdbserver_xmltarget;

void
initialize_low_tracepoint (void)
{
  init_registers_i386_linux ();
}
