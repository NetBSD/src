/* Copyright (C) 2007-2016 Free Software Foundation, Inc.

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
#include "win32-low.h"
#include "x86-low.h"

#ifndef CONTEXT_EXTENDED_REGISTERS
#define CONTEXT_EXTENDED_REGISTERS 0
#endif

#define FCS_REGNUM 27
#define FOP_REGNUM 31

#define FLAG_TRACE_BIT 0x100

#ifdef __x86_64__
/* Defined in auto-generated file reg-amd64.c.  */
void init_registers_amd64 (void);
extern const struct target_desc *tdesc_amd64;
#else
/* Defined in auto-generated file reg-i386.c.  */
void init_registers_i386 (void);
extern const struct target_desc *tdesc_i386;
#endif

static struct x86_debug_reg_state debug_reg_state;

static int
update_debug_registers_callback (struct inferior_list_entry *entry,
				 void *pid_p)
{
  struct thread_info *thr = (struct thread_info *) entry;
  win32_thread_info *th = (win32_thread_info *) inferior_target_data (thr);
  int pid = *(int *) pid_p;

  /* Only update the threads of this process.  */
  if (pid_of (thr) == pid)
    {
      /* The actual update is done later just before resuming the lwp,
	 we just mark that the registers need updating.  */
      th->debug_registers_changed = 1;
    }

  return 0;
}

/* Update the inferior's debug register REGNUM from STATE.  */

static void
x86_dr_low_set_addr (int regnum, CORE_ADDR addr)
{
  /* Only update the threads of this process.  */
  int pid = pid_of (current_thread);

  gdb_assert (DR_FIRSTADDR <= regnum && regnum <= DR_LASTADDR);

  find_inferior (&all_threads, update_debug_registers_callback, &pid);
}

/* Update the inferior's DR7 debug control register from STATE.  */

static void
x86_dr_low_set_control (unsigned long control)
{
  /* Only update the threads of this process.  */
  int pid = pid_of (current_thread);

  find_inferior (&all_threads, update_debug_registers_callback, &pid);
}

/* Return the current value of a DR register of the current thread's
   context.  */

static DWORD64
win32_get_current_dr (int dr)
{
  win32_thread_info *th
    = (win32_thread_info *) inferior_target_data (current_thread);

  win32_require_context (th);

#define RET_DR(DR)				\
  case DR:					\
    return th->context.Dr ## DR

  switch (dr)
    {
      RET_DR (0);
      RET_DR (1);
      RET_DR (2);
      RET_DR (3);
      RET_DR (6);
      RET_DR (7);
    }

#undef RET_DR

  gdb_assert_not_reached ("unhandled dr");
}

static CORE_ADDR
x86_dr_low_get_addr (int regnum)
{
  gdb_assert (DR_FIRSTADDR <= regnum && regnum <= DR_LASTADDR);

  return win32_get_current_dr (regnum - DR_FIRSTADDR);
}

static unsigned long
x86_dr_low_get_control (void)
{
  return win32_get_current_dr (7);
}

/* Get the value of the DR6 debug status register from the inferior
   and record it in STATE.  */

static unsigned long
x86_dr_low_get_status (void)
{
  return win32_get_current_dr (6);
}

/* Low-level function vector.  */
struct x86_dr_low_type x86_dr_low =
  {
    x86_dr_low_set_control,
    x86_dr_low_set_addr,
    x86_dr_low_get_addr,
    x86_dr_low_get_status,
    x86_dr_low_get_control,
    sizeof (void *),
  };

/* Breakpoint/watchpoint support.  */

static int
i386_supports_z_point_type (char z_type)
{
  switch (z_type)
    {
    case Z_PACKET_WRITE_WP:
    case Z_PACKET_ACCESS_WP:
      return 1;
    default:
      return 0;
    }
}

static int
i386_insert_point (enum raw_bkpt_type type, CORE_ADDR addr,
		   int size, struct raw_breakpoint *bp)
{
  switch (type)
    {
    case raw_bkpt_type_write_wp:
    case raw_bkpt_type_access_wp:
      {
	enum target_hw_bp_type hw_type
	  = raw_bkpt_type_to_target_hw_bp_type (type);

	return x86_dr_insert_watchpoint (&debug_reg_state,
					 hw_type, addr, size);
      }
    default:
      /* Unsupported.  */
      return 1;
    }
}

static int
i386_remove_point (enum raw_bkpt_type type, CORE_ADDR addr,
		   int size, struct raw_breakpoint *bp)
{
  switch (type)
    {
    case raw_bkpt_type_write_wp:
    case raw_bkpt_type_access_wp:
      {
	enum target_hw_bp_type hw_type
	  = raw_bkpt_type_to_target_hw_bp_type (type);

	return x86_dr_remove_watchpoint (&debug_reg_state,
					 hw_type, addr, size);
      }
    default:
      /* Unsupported.  */
      return 1;
    }
}

static int
x86_stopped_by_watchpoint (void)
{
  return x86_dr_stopped_by_watchpoint (&debug_reg_state);
}

static CORE_ADDR
x86_stopped_data_address (void)
{
  CORE_ADDR addr;
  if (x86_dr_stopped_data_address (&debug_reg_state, &addr))
    return addr;
  return 0;
}

static void
i386_initial_stuff (void)
{
  x86_low_init_dregs (&debug_reg_state);
}

static void
i386_get_thread_context (win32_thread_info *th)
{
  /* Requesting the CONTEXT_EXTENDED_REGISTERS register set fails if
     the system doesn't support extended registers.  */
  static DWORD extended_registers = CONTEXT_EXTENDED_REGISTERS;

 again:
  th->context.ContextFlags = (CONTEXT_FULL
			      | CONTEXT_FLOATING_POINT
			      | CONTEXT_DEBUG_REGISTERS
			      | extended_registers);

  if (!GetThreadContext (th->h, &th->context))
    {
      DWORD e = GetLastError ();

      if (extended_registers && e == ERROR_INVALID_PARAMETER)
	{
	  extended_registers = 0;
	  goto again;
	}

      error ("GetThreadContext failure %ld\n", (long) e);
    }
}

static void
i386_prepare_to_resume (win32_thread_info *th)
{
  if (th->debug_registers_changed)
    {
      struct x86_debug_reg_state *dr = &debug_reg_state;

      win32_require_context (th);

      th->context.Dr0 = dr->dr_mirror[0];
      th->context.Dr1 = dr->dr_mirror[1];
      th->context.Dr2 = dr->dr_mirror[2];
      th->context.Dr3 = dr->dr_mirror[3];
      /* th->context.Dr6 = dr->dr_status_mirror;
	 FIXME: should we set dr6 also ?? */
      th->context.Dr7 = dr->dr_control_mirror;

      th->debug_registers_changed = 0;
    }
}

static void
i386_thread_added (win32_thread_info *th)
{
  th->debug_registers_changed = 1;
}

static void
i386_single_step (win32_thread_info *th)
{
  th->context.EFlags |= FLAG_TRACE_BIT;
}

#ifndef __x86_64__

/* An array of offset mappings into a Win32 Context structure.
   This is a one-to-one mapping which is indexed by gdb's register
   numbers.  It retrieves an offset into the context structure where
   the 4 byte register is located.
   An offset value of -1 indicates that Win32 does not provide this
   register in it's CONTEXT structure.  In this case regptr will return
   a pointer into a dummy register.  */
#define context_offset(x) ((int)&(((CONTEXT *)NULL)->x))
static const int mappings[] = {
  context_offset (Eax),
  context_offset (Ecx),
  context_offset (Edx),
  context_offset (Ebx),
  context_offset (Esp),
  context_offset (Ebp),
  context_offset (Esi),
  context_offset (Edi),
  context_offset (Eip),
  context_offset (EFlags),
  context_offset (SegCs),
  context_offset (SegSs),
  context_offset (SegDs),
  context_offset (SegEs),
  context_offset (SegFs),
  context_offset (SegGs),
  context_offset (FloatSave.RegisterArea[0 * 10]),
  context_offset (FloatSave.RegisterArea[1 * 10]),
  context_offset (FloatSave.RegisterArea[2 * 10]),
  context_offset (FloatSave.RegisterArea[3 * 10]),
  context_offset (FloatSave.RegisterArea[4 * 10]),
  context_offset (FloatSave.RegisterArea[5 * 10]),
  context_offset (FloatSave.RegisterArea[6 * 10]),
  context_offset (FloatSave.RegisterArea[7 * 10]),
  context_offset (FloatSave.ControlWord),
  context_offset (FloatSave.StatusWord),
  context_offset (FloatSave.TagWord),
  context_offset (FloatSave.ErrorSelector),
  context_offset (FloatSave.ErrorOffset),
  context_offset (FloatSave.DataSelector),
  context_offset (FloatSave.DataOffset),
  context_offset (FloatSave.ErrorSelector),
  /* XMM0-7 */
  context_offset (ExtendedRegisters[10 * 16]),
  context_offset (ExtendedRegisters[11 * 16]),
  context_offset (ExtendedRegisters[12 * 16]),
  context_offset (ExtendedRegisters[13 * 16]),
  context_offset (ExtendedRegisters[14 * 16]),
  context_offset (ExtendedRegisters[15 * 16]),
  context_offset (ExtendedRegisters[16 * 16]),
  context_offset (ExtendedRegisters[17 * 16]),
  /* MXCSR */
  context_offset (ExtendedRegisters[24])
};
#undef context_offset

#else /* __x86_64__ */

#define context_offset(x) (offsetof (CONTEXT, x))
static const int mappings[] =
{
  context_offset (Rax),
  context_offset (Rbx),
  context_offset (Rcx),
  context_offset (Rdx),
  context_offset (Rsi),
  context_offset (Rdi),
  context_offset (Rbp),
  context_offset (Rsp),
  context_offset (R8),
  context_offset (R9),
  context_offset (R10),
  context_offset (R11),
  context_offset (R12),
  context_offset (R13),
  context_offset (R14),
  context_offset (R15),
  context_offset (Rip),
  context_offset (EFlags),
  context_offset (SegCs),
  context_offset (SegSs),
  context_offset (SegDs),
  context_offset (SegEs),
  context_offset (SegFs),
  context_offset (SegGs),
  context_offset (FloatSave.FloatRegisters[0]),
  context_offset (FloatSave.FloatRegisters[1]),
  context_offset (FloatSave.FloatRegisters[2]),
  context_offset (FloatSave.FloatRegisters[3]),
  context_offset (FloatSave.FloatRegisters[4]),
  context_offset (FloatSave.FloatRegisters[5]),
  context_offset (FloatSave.FloatRegisters[6]),
  context_offset (FloatSave.FloatRegisters[7]),
  context_offset (FloatSave.ControlWord),
  context_offset (FloatSave.StatusWord),
  context_offset (FloatSave.TagWord),
  context_offset (FloatSave.ErrorSelector),
  context_offset (FloatSave.ErrorOffset),
  context_offset (FloatSave.DataSelector),
  context_offset (FloatSave.DataOffset),
  context_offset (FloatSave.ErrorSelector)
  /* XMM0-7 */ ,
  context_offset (Xmm0),
  context_offset (Xmm1),
  context_offset (Xmm2),
  context_offset (Xmm3),
  context_offset (Xmm4),
  context_offset (Xmm5),
  context_offset (Xmm6),
  context_offset (Xmm7),
  context_offset (Xmm8),
  context_offset (Xmm9),
  context_offset (Xmm10),
  context_offset (Xmm11),
  context_offset (Xmm12),
  context_offset (Xmm13),
  context_offset (Xmm14),
  context_offset (Xmm15),
  /* MXCSR */
  context_offset (FloatSave.MxCsr)
};
#undef context_offset

#endif /* __x86_64__ */

/* Fetch register from gdbserver regcache data.  */
static void
i386_fetch_inferior_register (struct regcache *regcache,
			      win32_thread_info *th, int r)
{
  char *context_offset = (char *) &th->context + mappings[r];

  long l;
  if (r == FCS_REGNUM)
    {
      l = *((long *) context_offset) & 0xffff;
      supply_register (regcache, r, (char *) &l);
    }
  else if (r == FOP_REGNUM)
    {
      l = (*((long *) context_offset) >> 16) & ((1 << 11) - 1);
      supply_register (regcache, r, (char *) &l);
    }
  else
    supply_register (regcache, r, context_offset);
}

/* Store a new register value into the thread context of TH.  */
static void
i386_store_inferior_register (struct regcache *regcache,
			      win32_thread_info *th, int r)
{
  char *context_offset = (char *) &th->context + mappings[r];
  collect_register (regcache, r, context_offset);
}

static const unsigned char i386_win32_breakpoint = 0xcc;
#define i386_win32_breakpoint_len 1

static void
i386_arch_setup (void)
{
#ifdef __x86_64__
  init_registers_amd64 ();
  win32_tdesc = tdesc_amd64;
#else
  init_registers_i386 ();
  win32_tdesc = tdesc_i386;
#endif
}

struct win32_target_ops the_low_target = {
  i386_arch_setup,
  sizeof (mappings) / sizeof (mappings[0]),
  i386_initial_stuff,
  i386_get_thread_context,
  i386_prepare_to_resume,
  i386_thread_added,
  i386_fetch_inferior_register,
  i386_store_inferior_register,
  i386_single_step,
  &i386_win32_breakpoint,
  i386_win32_breakpoint_len,
  i386_supports_z_point_type,
  i386_insert_point,
  i386_remove_point,
  x86_stopped_by_watchpoint,
  x86_stopped_data_address
};
