/* Simulator breakpoint support.
   Copyright (C) 1997 Free Software Foundation, Inc.
   Contributed by Cygnus Support.

This file is part of GDB, the GNU debugger.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include <stdio.h>

#include "sim-main.h"
#include "sim-assert.h"
#include "sim-break.h"

#ifndef SIM_BREAKPOINT
#define SIM_BREAKPOINT {0x00}
#define SIM_BREAKPOINT_SIZE (1)
#endif

struct
sim_breakpoint
{
  struct sim_breakpoint *next;
  SIM_ADDR addr;		/* Address of this breakpoint */
  int flags;
  unsigned char loc_contents[SIM_BREAKPOINT_SIZE]; /* Contents of addr while
						      BP is enabled */
};

#define SIM_BREAK_INSERTED 0x1	/* Breakpoint has been inserted */
#define SIM_BREAK_DISABLED 0x2	/* Breakpoint is disabled */

static unsigned char sim_breakpoint [] = SIM_BREAKPOINT;

static void insert_breakpoint PARAMS ((SIM_DESC sd,
				       struct sim_breakpoint *bp));
static void remove_breakpoint PARAMS ((SIM_DESC sd,
				       struct sim_breakpoint *bp));
static SIM_RC resume_handler PARAMS ((SIM_DESC sd));
static SIM_RC suspend_handler PARAMS ((SIM_DESC sd));


/* Do the actual work of inserting a breakpoint into the instruction
   stream. */

static void
insert_breakpoint (sd, bp)
     SIM_DESC sd;
     struct sim_breakpoint *bp;
{
  if (bp->flags & (SIM_BREAK_INSERTED | SIM_BREAK_DISABLED))
    return;

  sim_core_read_buffer (sd, NULL, exec_map, bp->loc_contents,
			bp->addr, SIM_BREAKPOINT_SIZE);
  sim_core_write_buffer (sd, NULL, exec_map, sim_breakpoint,
			 bp->addr, SIM_BREAKPOINT_SIZE);
  bp->flags |= SIM_BREAK_INSERTED;
}

/* Do the actual work of removing a breakpoint. */

static void
remove_breakpoint (sd, bp)
     SIM_DESC sd;
     struct sim_breakpoint *bp;
{
  if (!(bp->flags & SIM_BREAK_INSERTED))
    return;

  sim_core_write_buffer (sd, NULL, exec_map, bp->loc_contents,
			 bp->addr, SIM_BREAKPOINT_SIZE);
  bp->flags &= ~SIM_BREAK_INSERTED;
}

/* Come here when a breakpoint insn is hit.  If it's really a breakpoint, we
   halt things, and never return.  If it's a false hit, we return to let the
   caller handle things.  */

void
sim_handle_breakpoint (sd, cpu, cia)
     SIM_DESC sd;
     sim_cpu *cpu;
     sim_cia cia;
{
  struct sim_breakpoint *bp;

  for (bp = STATE_BREAKPOINTS (sd); bp; bp = bp->next)
    if (bp->addr == CIA_ADDR (cia))
      break;

  if (!bp || !(bp->flags & SIM_BREAK_INSERTED))
    return;

  sim_engine_halt (sd, STATE_CPU (sd, 0), NULL, cia, sim_stopped, SIM_SIGTRAP);
}

/* Handler functions for simulator resume and suspend events. */

static SIM_RC
resume_handler (sd)
     SIM_DESC sd;
{
  struct sim_breakpoint *bp;

  for (bp = STATE_BREAKPOINTS (sd); bp; bp = bp->next)
    insert_breakpoint (sd, bp);

  return SIM_RC_OK;
}

static SIM_RC
suspend_handler (sd)
     SIM_DESC sd;
{
  struct sim_breakpoint *bp;

  for (bp = STATE_BREAKPOINTS (sd); bp; bp = bp->next)
    remove_breakpoint (sd, bp);

  return SIM_RC_OK;
}

/* Called from simulator module initialization.  */

SIM_RC
sim_break_install (sd)
     SIM_DESC sd;
{
  sim_module_add_resume_fn (sd, resume_handler);
  sim_module_add_suspend_fn (sd, suspend_handler);

  return SIM_RC_OK;
}

/* Install a breakpoint.  This is a user-function.  The breakpoint isn't
   actually installed here.  We just record it.  Resume_handler does the
   actual work.
*/

SIM_RC
sim_set_breakpoint (sd, addr)
     SIM_DESC sd;
     SIM_ADDR addr;
{
  struct sim_breakpoint *bp;

  for (bp = STATE_BREAKPOINTS (sd); bp; bp = bp->next)
    if (bp->addr == addr)
      return SIM_RC_DUPLICATE_BREAKPOINT; /* Already there */
    else
      break; /* FIXME: why not scan all bp's? */

  bp = ZALLOC (struct sim_breakpoint);

  bp->addr = addr;
  bp->next = STATE_BREAKPOINTS (sd);
  bp->flags = 0;
  STATE_BREAKPOINTS (sd) = bp;

  return SIM_RC_OK;
}

/* Delete a breakpoint.  All knowlege of the breakpoint is removed from the
   simulator.
*/

SIM_RC
sim_clear_breakpoint (sd, addr)
     SIM_DESC sd;
     SIM_ADDR addr;
{
  struct sim_breakpoint *bp, *bpprev;

  for (bp = STATE_BREAKPOINTS (sd), bpprev = NULL;
       bp;
       bpprev = bp, bp = bp->next)
    if (bp->addr == addr)
      break;

  if (!bp)
    return SIM_RC_UNKNOWN_BREAKPOINT;

  remove_breakpoint (sd, bp);

  if (bpprev)
    bpprev->next = bp->next;
  else
    STATE_BREAKPOINTS (sd) = NULL;

  zfree (bp);

  return SIM_RC_OK;
}

SIM_RC
sim_clear_all_breakpoints (sd)
     SIM_DESC sd;
{
  while (STATE_BREAKPOINTS (sd))
    sim_clear_breakpoint (sd, STATE_BREAKPOINTS (sd)->addr);

  return SIM_RC_OK;
}

SIM_RC
sim_enable_breakpoint (sd, addr)
     SIM_DESC sd;
     SIM_ADDR addr;
{
  struct sim_breakpoint *bp;

  for (bp = STATE_BREAKPOINTS (sd); bp; bp = bp->next)
    if (bp->addr == addr)
      break;

  if (!bp)
    return SIM_RC_UNKNOWN_BREAKPOINT;

  bp->flags &= ~SIM_BREAK_DISABLED;

  return SIM_RC_OK;
}

SIM_RC
sim_disable_breakpoint (sd, addr)
     SIM_DESC sd;
     SIM_ADDR addr;
{
  struct sim_breakpoint *bp;

  for (bp = STATE_BREAKPOINTS (sd); bp; bp = bp->next)
    if (bp->addr == addr)
      break;

  if (!bp)
    return SIM_RC_UNKNOWN_BREAKPOINT;

  bp->flags |= SIM_BREAK_DISABLED;

  return SIM_RC_OK;
}

SIM_RC
sim_enable_all_breakpoints (sd)
     SIM_DESC sd;
{
  struct sim_breakpoint *bp;

  for (bp = STATE_BREAKPOINTS (sd); bp; bp = bp->next)
    bp->flags &= ~SIM_BREAK_DISABLED;

  return SIM_RC_OK;
}

SIM_RC
sim_disable_all_breakpoints (sd)
     SIM_DESC sd;
{
  struct sim_breakpoint *bp;

  for (bp = STATE_BREAKPOINTS (sd); bp; bp = bp->next)
    bp->flags |= SIM_BREAK_DISABLED;

  return SIM_RC_OK;
}
