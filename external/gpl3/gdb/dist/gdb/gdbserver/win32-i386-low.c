/* Copyright (C) 2007-2013 Free Software Foundation, Inc.

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
#include "i386-low.h"

#ifndef CONTEXT_EXTENDED_REGISTERS
#define CONTEXT_EXTENDED_REGISTERS 0
#endif

#define FCS_REGNUM 27
#define FOP_REGNUM 31

#define FLAG_TRACE_BIT 0x100

#ifdef __x86_64__
/* Defined in auto-generated file reg-amd64.c.  */
void init_registers_amd64 (void);
#else
/* Defined in auto-generated file reg-i386.c.  */
void init_registers_i386 (void);
#endif

static struct i386_debug_reg_state debug_reg_state;

static int debug_registers_changed = 0;
static int debug_registers_used = 0;

/* Update the inferior's debug register REGNUM from STATE.  */

void
i386_dr_low_set_addr (const struct i386_debug_reg_state *state, int regnum)
{
  if (! (regnum >= 0 && regnum <= DR_LASTADDR - DR_FIRSTADDR))
    fatal ("Invalid debug register %d", regnum);

  /* debug_reg_state.dr_mirror is already set.
     Just notify i386_set_thread_context, i386_thread_added
     that the registers need to be updated.  */
  debug_registers_changed = 1;
  debug_registers_used = 1;
}

CORE_ADDR
i386_dr_low_get_addr (int regnum)
{
  gdb_assert (DR_FIRSTADDR <= regnum && regnum <= DR_LASTADDR);

  return debug_reg_state.dr_mirror[regnum];
}

/* Update the inferior's DR7 debug control register from STATE.  */

void
i386_dr_low_set_control (const struct i386_debug_reg_state *state)
{
  /* debug_reg_state.dr_control_mirror is already set.
     Just notify i386_set_thread_context, i386_thread_added
     that the registers need to be updated.  */
  debug_registers_changed = 1;
  debug_registers_used = 1;
}

unsigned
i386_dr_low_get_control (void)
{
  return debug_reg_state.dr_control_mirror;
}

/* Get the value of the DR6 debug status register from the inferior
   and record it in STATE.  */

unsigned
i386_dr_low_get_status (void)
{
  /* We don't need to do anything here, the last call to thread_rec for
     current_event.dwThreadId id has already set it.  */
  return debug_reg_state.dr_status_mirror;
}

/* Watchpoint support.  */

static int
i386_insert_point (char type, CORE_ADDR addr, int len)
{
  switch (type)
    {
    case '2':
    case '3':
    case '4':
      return i386_low_insert_watchpoint (&debug_reg_state,
					 type, addr, len);
    default:
      /* Unsupported.  */
      return 1;
    }
}

static int
i386_remove_point (char type, CORE_ADDR addr, int len)
{
  switch (type)
    {
    case '2':
    case '3':
    case '4':
      return i386_low_remove_watchpoint (&debug_reg_state,
					 type, addr, len);
    default:
      /* Unsupported.  */
      return 1;
    }
}

static int
i386_stopped_by_watchpoint (void)
{
  return i386_low_stopped_by_watchpoint (&debug_reg_state);
}

static CORE_ADDR
i386_stopped_data_address (void)
{
  CORE_ADDR addr;
  if (i386_low_stopped_data_address (&debug_reg_state, &addr))
    return addr;
  return 0;
}

static void
i386_initial_stuff (void)
{
  i386_low_init_dregs (&debug_reg_state);
  debug_registers_changed = 0;
  debug_registers_used = 0;
}

static void
i386_get_thread_context (win32_thread_info *th, DEBUG_EVENT* current_event)
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

  debug_registers_changed = 0;

  if (th->tid == current_event->dwThreadId)
    {
      /* Copy dr values from the current thread.  */
      struct i386_debug_reg_state *dr = &debug_reg_state;
      dr->dr_mirror[0] = th->context.Dr0;
      dr->dr_mirror[1] = th->context.Dr1;
      dr->dr_mirror[2] = th->context.Dr2;
      dr->dr_mirror[3] = th->context.Dr3;
      dr->dr_status_mirror = th->context.Dr6;
      dr->dr_control_mirror = th->context.Dr7;
    }
}

static void
i386_set_thread_context (win32_thread_info *th, DEBUG_EVENT* current_event)
{
  if (debug_registers_changed)
    {
      struct i386_debug_reg_state *dr = &debug_reg_state;
      th->context.Dr0 = dr->dr_mirror[0];
      th->context.Dr1 = dr->dr_mirror[1];
      th->context.Dr2 = dr->dr_mirror[2];
      th->context.Dr3 = dr->dr_mirror[3];
      /* th->context.Dr6 = dr->dr_status_mirror;
	 FIXME: should we set dr6 also ?? */
      th->context.Dr7 = dr->dr_control_mirror;
    }

  SetThreadContext (th->h, &th->context);
}

static void
i386_thread_added (win32_thread_info *th)
{
  /* Set the debug registers for the new thread if they are used.  */
  if (debug_registers_used)
    {
      struct i386_debug_reg_state *dr = &debug_reg_state;
      th->context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
      GetThreadContext (th->h, &th->context);

      th->context.Dr0 = dr->dr_mirror[0];
      th->context.Dr1 = dr->dr_mirror[1];
      th->context.Dr2 = dr->dr_mirror[2];
      th->context.Dr3 = dr->dr_mirror[3];
      /* th->context.Dr6 = dr->dr_status_mirror;
	 FIXME: should we set dr6 also ?? */
      th->context.Dr7 = dr->dr_control_mirror;

      SetThreadContext (th->h, &th->context);
      th->context.ContextFlags = 0;
    }
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
init_windows_x86 (void)
{
#ifdef __x86_64__
  init_registers_amd64 ();
#else
  init_registers_i386 ();
#endif
}

struct win32_target_ops the_low_target = {
  init_windows_x86,
  sizeof (mappings) / sizeof (mappings[0]),
  i386_initial_stuff,
  i386_get_thread_context,
  i386_set_thread_context,
  i386_thread_added,
  i386_fetch_inferior_register,
  i386_store_inferior_register,
  i386_single_step,
  &i386_win32_breakpoint,
  i386_win32_breakpoint_len,
  i386_insert_point,
  i386_remove_point,
  i386_stopped_by_watchpoint,
  i386_stopped_data_address
};
