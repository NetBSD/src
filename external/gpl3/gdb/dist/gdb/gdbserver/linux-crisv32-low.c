/* GNU/Linux/CRIS specific low level interface, for the remote server for GDB.
   Copyright (C) 1995-2014 Free Software Foundation, Inc.

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

/* Defined in auto-generated file reg-crisv32.c.  */
void init_registers_crisv32 (void);
extern const struct target_desc *tdesc_crisv32;

/* CRISv32 */
#define cris_num_regs 49

#ifndef PTRACE_GET_THREAD_AREA
#define PTRACE_GET_THREAD_AREA 25
#endif

/* Note: Ignoring USP (having the stack pointer in two locations causes trouble
   without any significant gain).  */

/* Locations need to match <include/asm/arch/ptrace.h>.  */
static int cris_regmap[] = {
  1*4, 2*4, 3*4, 4*4,
  5*4, 6*4, 7*4, 8*4,
  9*4, 10*4, 11*4, 12*4,
  13*4, 14*4, 24*4, 15*4,

  -1, -1, -1, 16*4,
  -1, 22*4, 23*4, 17*4,
  -1, -1, 21*4, 20*4,
  -1, 19*4, -1, 18*4,

  25*4,

  26*4, -1,   -1,   29*4,
  30*4, 31*4, 32*4, 33*4,
  34*4, 35*4, 36*4, 37*4,
  38*4, 39*4, 40*4, -1

};

extern int debug_threads;

static CORE_ADDR
cris_get_pc (struct regcache *regcache)
{
  unsigned long pc;
  collect_register_by_name (regcache, "pc", &pc);
  if (debug_threads)
    fprintf (stderr, "stop pc is %08lx\n", pc);
  return pc;
}

static void
cris_set_pc (struct regcache *regcache, CORE_ADDR pc)
{
  unsigned long newpc = pc;
  supply_register_by_name (regcache, "pc", &newpc);
}

static const unsigned short cris_breakpoint = 0xe938;
#define cris_breakpoint_len 2

static int
cris_breakpoint_at (CORE_ADDR where)
{
  unsigned short insn;

  (*the_target->read_memory) (where, (unsigned char *) &insn,
			      cris_breakpoint_len);
  if (insn == cris_breakpoint)
    return 1;

  /* If necessary, recognize more trap instructions here.  GDB only uses the
     one.  */
  return 0;
}

/* We only place breakpoints in empty marker functions, and thread locking
   is outside of the function.  So rather than importing software single-step,
   we can just run until exit.  */

/* FIXME: This function should not be needed, since we have PTRACE_SINGLESTEP
   for CRISv32.  Without it, td_ta_event_getmsg in thread_db_create_event
   will fail when debugging multi-threaded applications.  */

static CORE_ADDR
cris_reinsert_addr (void)
{
  struct regcache *regcache = get_thread_regcache (current_inferior, 1);
  unsigned long pc;
  collect_register_by_name (regcache, "srp", &pc);
  return pc;
}

static void
cris_write_data_breakpoint (struct regcache *regcache,
			    int bp, unsigned long start, unsigned long end)
{
  switch (bp)
    {
    case 0:
      supply_register_by_name (regcache, "s3", &start);
      supply_register_by_name (regcache, "s4", &end);
      break;
    case 1:
      supply_register_by_name (regcache, "s5", &start);
      supply_register_by_name (regcache, "s6", &end);
      break;
    case 2:
      supply_register_by_name (regcache, "s7", &start);
      supply_register_by_name (regcache, "s8", &end);
      break;
    case 3:
      supply_register_by_name (regcache, "s9", &start);
      supply_register_by_name (regcache, "s10", &end);
      break;
    case 4:
      supply_register_by_name (regcache, "s11", &start);
      supply_register_by_name (regcache, "s12", &end);
      break;
    case 5:
      supply_register_by_name (regcache, "s13", &start);
      supply_register_by_name (regcache, "s14", &end);
      break;
    }
}

static int
cris_insert_point (char type, CORE_ADDR addr, int len)
{
  int bp;
  unsigned long bp_ctrl;
  unsigned long start, end;
  unsigned long ccs;
  struct regcache *regcache;

  /* Breakpoint/watchpoint types (GDB terminology):
     0 = memory breakpoint for instructions
     (not supported; done via memory write instead)
     1 = hardware breakpoint for instructions (not supported)
     2 = write watchpoint (supported)
     3 = read watchpoint (supported)
     4 = access watchpoint (supported).  */

  if (type < '2' || type > '4')
    {
      /* Unsupported.  */
      return 1;
    }

  regcache = get_thread_regcache (current_inferior, 1);

  /* Read watchpoints are set as access watchpoints, because of GDB's
     inability to deal with pure read watchpoints.  */
  if (type == '3')
    type = '4';

  /* Get the configuration register.  */
  collect_register_by_name (regcache, "s0", &bp_ctrl);

  /* The watchpoint allocation scheme is the simplest possible.
     For example, if a region is watched for read and
     a write watch is requested, a new watchpoint will
     be used.  Also, if a watch for a region that is already
     covered by one or more existing watchpoints, a new
     watchpoint will be used.  */

  /* First, find a free data watchpoint.  */
  for (bp = 0; bp < 6; bp++)
    {
      /* Each data watchpoint's control registers occupy 2 bits
	 (hence the 3), starting at bit 2 for D0 (hence the 2)
	 with 4 bits between for each watchpoint (yes, the 4).  */
      if (!(bp_ctrl & (0x3 << (2 + (bp * 4)))))
	break;
    }

  if (bp > 5)
    {
      /* We're out of watchpoints.  */
      return -1;
    }

  /* Configure the control register first.  */
  if (type == '3' || type == '4')
    {
      /* Trigger on read.  */
      bp_ctrl |= (1 << (2 + bp * 4));
    }
  if (type == '2' || type == '4')
    {
      /* Trigger on write.  */
      bp_ctrl |= (2 << (2 + bp * 4));
    }

  /* Setup the configuration register.  */
  supply_register_by_name (regcache, "s0", &bp_ctrl);

  /* Setup the range.  */
  start = addr;
  end = addr + len - 1;

  /* Configure the watchpoint register.  */
  cris_write_data_breakpoint (regcache, bp, start, end);

  collect_register_by_name (regcache, "ccs", &ccs);
  /* Set the S1 flag to enable watchpoints.  */
  ccs |= (1 << 19);
  supply_register_by_name (regcache, "ccs", &ccs);

  return 0;
}

static int
cris_remove_point (char type, CORE_ADDR addr, int len)
{
  int bp;
  unsigned long bp_ctrl;
  unsigned long start, end;
  struct regcache *regcache;
  unsigned long bp_d_regs[12];

  /* Breakpoint/watchpoint types:
     0 = memory breakpoint for instructions
     (not supported; done via memory write instead)
     1 = hardware breakpoint for instructions (not supported)
     2 = write watchpoint (supported)
     3 = read watchpoint (supported)
     4 = access watchpoint (supported).  */
  if (type < '2' || type > '4')
    return -1;

  regcache = get_thread_regcache (current_inferior, 1);

  /* Read watchpoints are set as access watchpoints, because of GDB's
     inability to deal with pure read watchpoints.  */
  if (type == '3')
    type = '4';

  /* Get the configuration register.  */
  collect_register_by_name (regcache, "s0", &bp_ctrl);

  /* Try to find a watchpoint that is configured for the
     specified range, then check that read/write also matches.  */

  /* Ugly pointer arithmetic, since I cannot rely on a
     single switch (addr) as there may be several watchpoints with
     the same start address for example.  */

  /* Get all range registers to simplify search.  */
  collect_register_by_name (regcache, "s3", &bp_d_regs[0]);
  collect_register_by_name (regcache, "s4", &bp_d_regs[1]);
  collect_register_by_name (regcache, "s5", &bp_d_regs[2]);
  collect_register_by_name (regcache, "s6", &bp_d_regs[3]);
  collect_register_by_name (regcache, "s7", &bp_d_regs[4]);
  collect_register_by_name (regcache, "s8", &bp_d_regs[5]);
  collect_register_by_name (regcache, "s9", &bp_d_regs[6]);
  collect_register_by_name (regcache, "s10", &bp_d_regs[7]);
  collect_register_by_name (regcache, "s11", &bp_d_regs[8]);
  collect_register_by_name (regcache, "s12", &bp_d_regs[9]);
  collect_register_by_name (regcache, "s13", &bp_d_regs[10]);
  collect_register_by_name (regcache, "s14", &bp_d_regs[11]);

  for (bp = 0; bp < 6; bp++)
    {
      if (bp_d_regs[bp * 2] == addr
	  && bp_d_regs[bp * 2 + 1] == (addr + len - 1)) {
	/* Matching range.  */
	int bitpos = 2 + bp * 4;
	int rw_bits;

	/* Read/write bits for this BP.  */
	rw_bits = (bp_ctrl & (0x3 << bitpos)) >> bitpos;

	if ((type == '3' && rw_bits == 0x1)
	    || (type == '2' && rw_bits == 0x2)
	    || (type == '4' && rw_bits == 0x3))
	  {
	    /* Read/write matched.  */
	    break;
	  }
      }
    }

  if (bp > 5)
    {
      /* No watchpoint matched.  */
      return -1;
    }

  /* Found a matching watchpoint.  Now, deconfigure it by
     both disabling read/write in bp_ctrl and zeroing its
     start/end addresses.  */
  bp_ctrl &= ~(3 << (2 + (bp * 4)));
  /* Setup the configuration register.  */
  supply_register_by_name (regcache, "s0", &bp_ctrl);

  start = end = 0;
  /* Configure the watchpoint register.  */
  cris_write_data_breakpoint (regcache, bp, start, end);

  /* Note that we don't clear the S1 flag here.  It's done when continuing.  */
  return 0;
}

static int
cris_stopped_by_watchpoint (void)
{
  unsigned long exs;
  struct regcache *regcache = get_thread_regcache (current_inferior, 1);

  collect_register_by_name (regcache, "exs", &exs);

  return (((exs & 0xff00) >> 8) == 0xc);
}

static CORE_ADDR
cris_stopped_data_address (void)
{
  unsigned long eda;
  struct regcache *regcache = get_thread_regcache (current_inferior, 1);

  collect_register_by_name (regcache, "eda", &eda);

  /* FIXME: Possibly adjust to match watched range.  */
  return eda;
}

ps_err_e
ps_get_thread_area (const struct ps_prochandle *ph,
                    lwpid_t lwpid, int idx, void **base)
{
  if (ptrace (PTRACE_GET_THREAD_AREA, lwpid, NULL, base) != 0)
    return PS_ERR;

  /* IDX is the bias from the thread pointer to the beginning of the
     thread descriptor.  It has to be subtracted due to implementation
     quirks in libthread_db.  */
  *base = (void *) ((char *) *base - idx);
  return PS_OK;
}

static void
cris_fill_gregset (struct regcache *regcache, void *buf)
{
  int i;

  for (i = 0; i < cris_num_regs; i++)
    {
      if (cris_regmap[i] != -1)
	collect_register (regcache, i, ((char *) buf) + cris_regmap[i]);
    }
}

static void
cris_store_gregset (struct regcache *regcache, const void *buf)
{
  int i;

  for (i = 0; i < cris_num_regs; i++)
    {
      if (cris_regmap[i] != -1)
	supply_register (regcache, i, ((char *) buf) + cris_regmap[i]);
    }
}

static void
cris_arch_setup (void)
{
  current_process ()->tdesc = tdesc_crisv32;
}

static struct regset_info cris_regsets[] = {
  { PTRACE_GETREGS, PTRACE_SETREGS, 0, cris_num_regs * 4,
    GENERAL_REGS, cris_fill_gregset, cris_store_gregset },
  { 0, 0, 0, -1, -1, NULL, NULL }
};


static struct regsets_info cris_regsets_info =
  {
    cris_regsets, /* regsets */
    0, /* num_regsets */
    NULL, /* disabled_regsets */
  };

static struct usrregs_info cris_usrregs_info =
  {
    cris_num_regs,
    cris_regmap,
  };

static struct regs_info regs_info =
  {
    NULL, /* regset_bitmap */
    &cris_usrregs_info,
    &cris_regsets_info
  };

static const struct regs_info *
cris_regs_info (void)
{
  return &regs_info;
}

struct linux_target_ops the_low_target = {
  cris_arch_setup,
  cris_regs_info,
  NULL,
  NULL,
  NULL, /* fetch_register */
  cris_get_pc,
  cris_set_pc,
  (const unsigned char *) &cris_breakpoint,
  cris_breakpoint_len,
  cris_reinsert_addr,
  0,
  cris_breakpoint_at,
  cris_insert_point,
  cris_remove_point,
  cris_stopped_by_watchpoint,
  cris_stopped_data_address,
};

void
initialize_low_arch (void)
{
  init_registers_crisv32 ();

  initialize_regsets_info (&cris_regsets_info);
}
