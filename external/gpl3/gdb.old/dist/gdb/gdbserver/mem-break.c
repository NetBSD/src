/* Memory breakpoint operations for the remote server for GDB.
   Copyright (C) 2002-2014 Free Software Foundation, Inc.

   Contributed by MontaVista Software.

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
#include "regcache.h"
#include "ax.h"
#include <stdint.h>

const unsigned char *breakpoint_data;
int breakpoint_len;

#define MAX_BREAKPOINT_LEN 8

/* GDB will never try to install multiple breakpoints at the same
   address.  But, we need to keep track of internal breakpoints too,
   and so we do need to be able to install multiple breakpoints at the
   same address transparently.  We keep track of two different, and
   closely related structures.  A raw breakpoint, which manages the
   low level, close to the metal aspect of a breakpoint.  It holds the
   breakpoint address, and a buffer holding a copy of the instructions
   that would be in memory had not been a breakpoint there (we call
   that the shadow memory of the breakpoint).  We occasionally need to
   temporarilly uninsert a breakpoint without the client knowing about
   it (e.g., to step over an internal breakpoint), so we keep an
   `inserted' state associated with this low level breakpoint
   structure.  There can only be one such object for a given address.
   Then, we have (a bit higher level) breakpoints.  This structure
   holds a callback to be called whenever a breakpoint is hit, a
   high-level type, and a link to a low level raw breakpoint.  There
   can be many high-level breakpoints at the same address, and all of
   them will point to the same raw breakpoint, which is reference
   counted.  */

/* The low level, physical, raw breakpoint.  */
struct raw_breakpoint
{
  struct raw_breakpoint *next;

  /* A reference count.  Each high level breakpoint referencing this
     raw breakpoint accounts for one reference.  */
  int refcount;

  /* The breakpoint's insertion address.  There can only be one raw
     breakpoint for a given PC.  */
  CORE_ADDR pc;

  /* The breakpoint's shadow memory.  */
  unsigned char old_data[MAX_BREAKPOINT_LEN];

  /* Non-zero if this breakpoint is currently inserted in the
     inferior.  */
  int inserted;

  /* Non-zero if this breakpoint is currently disabled because we no
     longer detect it as inserted.  */
  int shlib_disabled;
};

/* The type of a breakpoint.  */
enum bkpt_type
  {
    /* A GDB breakpoint, requested with a Z0 packet.  */
    gdb_breakpoint,

    /* A basic-software-single-step breakpoint.  */
    reinsert_breakpoint,

    /* Any other breakpoint type that doesn't require specific
       treatment goes here.  E.g., an event breakpoint.  */
    other_breakpoint,
  };

struct point_cond_list
{
  /* Pointer to the agent expression that is the breakpoint's
     conditional.  */
  struct agent_expr *cond;

  /* Pointer to the next condition.  */
  struct point_cond_list *next;
};

struct point_command_list
{
  /* Pointer to the agent expression that is the breakpoint's
     commands.  */
  struct agent_expr *cmd;

  /* Flag that is true if this command should run even while GDB is
     disconnected.  */
  int persistence;

  /* Pointer to the next command.  */
  struct point_command_list *next;
};

/* A high level (in gdbserver's perspective) breakpoint.  */
struct breakpoint
{
  struct breakpoint *next;

  /* The breakpoint's type.  */
  enum bkpt_type type;

  /* Pointer to the condition list that should be evaluated on
     the target or NULL if the breakpoint is unconditional or
     if GDB doesn't want us to evaluate the conditionals on the
     target's side.  */
  struct point_cond_list *cond_list;

  /* Point to the list of commands to run when this is hit.  */
  struct point_command_list *command_list;

  /* Link to this breakpoint's raw breakpoint.  This is always
     non-NULL.  */
  struct raw_breakpoint *raw;

  /* Function to call when we hit this breakpoint.  If it returns 1,
     the breakpoint shall be deleted; 0 or if this callback is NULL,
     it will be left inserted.  */
  int (*handler) (CORE_ADDR);
};

int
any_persistent_commands ()
{
  struct process_info *proc = current_process ();
  struct breakpoint *bp;
  struct point_command_list *cl;

  for (bp = proc->breakpoints; bp != NULL; bp = bp->next)
    {
      for (cl = bp->command_list; cl != NULL; cl = cl->next)
	if (cl->persistence)
	  return 1;
    }

  return 0;
}

static struct raw_breakpoint *
find_raw_breakpoint_at (CORE_ADDR where)
{
  struct process_info *proc = current_process ();
  struct raw_breakpoint *bp;

  for (bp = proc->raw_breakpoints; bp != NULL; bp = bp->next)
    if (bp->pc == where)
      return bp;

  return NULL;
}

static struct raw_breakpoint *
set_raw_breakpoint_at (CORE_ADDR where)
{
  struct process_info *proc = current_process ();
  struct raw_breakpoint *bp;
  int err;
  unsigned char buf[MAX_BREAKPOINT_LEN];

  if (breakpoint_data == NULL)
    error ("Target does not support breakpoints.");

  bp = find_raw_breakpoint_at (where);
  if (bp != NULL)
    {
      bp->refcount++;
      return bp;
    }

  bp = xcalloc (1, sizeof (*bp));
  bp->pc = where;
  bp->refcount = 1;

  /* Note that there can be fast tracepoint jumps installed in the
     same memory range, so to get at the original memory, we need to
     use read_inferior_memory, which masks those out.  */
  err = read_inferior_memory (where, buf, breakpoint_len);
  if (err != 0)
    {
      if (debug_threads)
	fprintf (stderr,
		 "Failed to read shadow memory of"
		 " breakpoint at 0x%s (%s).\n",
		 paddress (where), strerror (err));
      free (bp);
      return NULL;
    }
  memcpy (bp->old_data, buf, breakpoint_len);

  err = (*the_target->write_memory) (where, breakpoint_data,
				     breakpoint_len);
  if (err != 0)
    {
      if (debug_threads)
	fprintf (stderr,
		 "Failed to insert breakpoint at 0x%s (%s).\n",
		 paddress (where), strerror (err));
      free (bp);
      return NULL;
    }

  /* Link the breakpoint in.  */
  bp->inserted = 1;
  bp->next = proc->raw_breakpoints;
  proc->raw_breakpoints = bp;
  return bp;
}

/* Notice that breakpoint traps are always installed on top of fast
   tracepoint jumps.  This is even if the fast tracepoint is installed
   at a later time compared to when the breakpoint was installed.
   This means that a stopping breakpoint or tracepoint has higher
   "priority".  In turn, this allows having fast and slow tracepoints
   (and breakpoints) at the same address behave correctly.  */


/* A fast tracepoint jump.  */

struct fast_tracepoint_jump
{
  struct fast_tracepoint_jump *next;

  /* A reference count.  GDB can install more than one fast tracepoint
     at the same address (each with its own action list, for
     example).  */
  int refcount;

  /* The fast tracepoint's insertion address.  There can only be one
     of these for a given PC.  */
  CORE_ADDR pc;

  /* Non-zero if this fast tracepoint jump is currently inserted in
     the inferior.  */
  int inserted;

  /* The length of the jump instruction.  */
  int length;

  /* A poor-man's flexible array member, holding both the jump
     instruction to insert, and a copy of the instruction that would
     be in memory had not been a jump there (the shadow memory of the
     tracepoint jump).  */
  unsigned char insn_and_shadow[0];
};

/* Fast tracepoint FP's jump instruction to insert.  */
#define fast_tracepoint_jump_insn(fp) \
  ((fp)->insn_and_shadow + 0)

/* The shadow memory of fast tracepoint jump FP.  */
#define fast_tracepoint_jump_shadow(fp) \
  ((fp)->insn_and_shadow + (fp)->length)


/* Return the fast tracepoint jump set at WHERE.  */

static struct fast_tracepoint_jump *
find_fast_tracepoint_jump_at (CORE_ADDR where)
{
  struct process_info *proc = current_process ();
  struct fast_tracepoint_jump *jp;

  for (jp = proc->fast_tracepoint_jumps; jp != NULL; jp = jp->next)
    if (jp->pc == where)
      return jp;

  return NULL;
}

int
fast_tracepoint_jump_here (CORE_ADDR where)
{
  struct fast_tracepoint_jump *jp = find_fast_tracepoint_jump_at (where);

  return (jp != NULL);
}

int
delete_fast_tracepoint_jump (struct fast_tracepoint_jump *todel)
{
  struct fast_tracepoint_jump *bp, **bp_link;
  int ret;
  struct process_info *proc = current_process ();

  bp = proc->fast_tracepoint_jumps;
  bp_link = &proc->fast_tracepoint_jumps;

  while (bp)
    {
      if (bp == todel)
	{
	  if (--bp->refcount == 0)
	    {
	      struct fast_tracepoint_jump *prev_bp_link = *bp_link;
	      unsigned char *buf;

	      /* Unlink it.  */
	      *bp_link = bp->next;

	      /* Since there can be breakpoints inserted in the same
		 address range, we use `write_inferior_memory', which
		 takes care of layering breakpoints on top of fast
		 tracepoints, and on top of the buffer we pass it.
		 This works because we've already unlinked the fast
		 tracepoint jump above.  Also note that we need to
		 pass the current shadow contents, because
		 write_inferior_memory updates any shadow memory with
		 what we pass here, and we want that to be a nop.  */
	      buf = alloca (bp->length);
	      memcpy (buf, fast_tracepoint_jump_shadow (bp), bp->length);
	      ret = write_inferior_memory (bp->pc, buf, bp->length);
	      if (ret != 0)
		{
		  /* Something went wrong, relink the jump.  */
		  *bp_link = prev_bp_link;

		  if (debug_threads)
		    fprintf (stderr,
			     "Failed to uninsert fast tracepoint jump "
			     "at 0x%s (%s) while deleting it.\n",
			     paddress (bp->pc), strerror (ret));
		  return ret;
		}

	      free (bp);
	    }

	  return 0;
	}
      else
	{
	  bp_link = &bp->next;
	  bp = *bp_link;
	}
    }

  warning ("Could not find fast tracepoint jump in list.");
  return ENOENT;
}

void
inc_ref_fast_tracepoint_jump (struct fast_tracepoint_jump *jp)
{
  jp->refcount++;
}

struct fast_tracepoint_jump *
set_fast_tracepoint_jump (CORE_ADDR where,
			  unsigned char *insn, ULONGEST length)
{
  struct process_info *proc = current_process ();
  struct fast_tracepoint_jump *jp;
  int err;
  unsigned char *buf;

  /* We refcount fast tracepoint jumps.  Check if we already know
     about a jump at this address.  */
  jp = find_fast_tracepoint_jump_at (where);
  if (jp != NULL)
    {
      jp->refcount++;
      return jp;
    }

  /* We don't, so create a new object.  Double the length, because the
     flexible array member holds both the jump insn, and the
     shadow.  */
  jp = xcalloc (1, sizeof (*jp) + (length * 2));
  jp->pc = where;
  jp->length = length;
  memcpy (fast_tracepoint_jump_insn (jp), insn, length);
  jp->refcount = 1;
  buf = alloca (length);

  /* Note that there can be trap breakpoints inserted in the same
     address range.  To access the original memory contents, we use
     `read_inferior_memory', which masks out breakpoints.  */
  err = read_inferior_memory (where, buf, length);
  if (err != 0)
    {
      if (debug_threads)
	fprintf (stderr,
		 "Failed to read shadow memory of"
		 " fast tracepoint at 0x%s (%s).\n",
		 paddress (where), strerror (err));
      free (jp);
      return NULL;
    }
  memcpy (fast_tracepoint_jump_shadow (jp), buf, length);

  /* Link the jump in.  */
  jp->inserted = 1;
  jp->next = proc->fast_tracepoint_jumps;
  proc->fast_tracepoint_jumps = jp;

  /* Since there can be trap breakpoints inserted in the same address
     range, we use use `write_inferior_memory', which takes care of
     layering breakpoints on top of fast tracepoints, on top of the
     buffer we pass it.  This works because we've already linked in
     the fast tracepoint jump above.  Also note that we need to pass
     the current shadow contents, because write_inferior_memory
     updates any shadow memory with what we pass here, and we want
     that to be a nop.  */
  err = write_inferior_memory (where, buf, length);
  if (err != 0)
    {
      if (debug_threads)
	fprintf (stderr,
		 "Failed to insert fast tracepoint jump at 0x%s (%s).\n",
		 paddress (where), strerror (err));

      /* Unlink it.  */
      proc->fast_tracepoint_jumps = jp->next;
      free (jp);

      return NULL;
    }

  return jp;
}

void
uninsert_fast_tracepoint_jumps_at (CORE_ADDR pc)
{
  struct fast_tracepoint_jump *jp;
  int err;

  jp = find_fast_tracepoint_jump_at (pc);
  if (jp == NULL)
    {
      /* This can happen when we remove all breakpoints while handling
	 a step-over.  */
      if (debug_threads)
	fprintf (stderr,
		 "Could not find fast tracepoint jump at 0x%s "
		 "in list (uninserting).\n",
		 paddress (pc));
      return;
    }

  if (jp->inserted)
    {
      unsigned char *buf;

      jp->inserted = 0;

      /* Since there can be trap breakpoints inserted in the same
	 address range, we use use `write_inferior_memory', which
	 takes care of layering breakpoints on top of fast
	 tracepoints, and on top of the buffer we pass it.  This works
	 because we've already marked the fast tracepoint fast
	 tracepoint jump uninserted above.  Also note that we need to
	 pass the current shadow contents, because
	 write_inferior_memory updates any shadow memory with what we
	 pass here, and we want that to be a nop.  */
      buf = alloca (jp->length);
      memcpy (buf, fast_tracepoint_jump_shadow (jp), jp->length);
      err = write_inferior_memory (jp->pc, buf, jp->length);
      if (err != 0)
	{
	  jp->inserted = 1;

	  if (debug_threads)
	    fprintf (stderr,
		     "Failed to uninsert fast tracepoint jump at 0x%s (%s).\n",
		     paddress (pc), strerror (err));
	}
    }
}

void
reinsert_fast_tracepoint_jumps_at (CORE_ADDR where)
{
  struct fast_tracepoint_jump *jp;
  int err;
  unsigned char *buf;

  jp = find_fast_tracepoint_jump_at (where);
  if (jp == NULL)
    {
      /* This can happen when we remove breakpoints when a tracepoint
	 hit causes a tracing stop, while handling a step-over.  */
      if (debug_threads)
	fprintf (stderr,
		 "Could not find fast tracepoint jump at 0x%s "
		 "in list (reinserting).\n",
		 paddress (where));
      return;
    }

  if (jp->inserted)
    error ("Jump already inserted at reinsert time.");

  jp->inserted = 1;

  /* Since there can be trap breakpoints inserted in the same address
     range, we use `write_inferior_memory', which takes care of
     layering breakpoints on top of fast tracepoints, and on top of
     the buffer we pass it.  This works because we've already marked
     the fast tracepoint jump inserted above.  Also note that we need
     to pass the current shadow contents, because
     write_inferior_memory updates any shadow memory with what we pass
     here, and we want that to be a nop.  */
  buf = alloca (jp->length);
  memcpy (buf, fast_tracepoint_jump_shadow (jp), jp->length);
  err = write_inferior_memory (where, buf, jp->length);
  if (err != 0)
    {
      jp->inserted = 0;

      if (debug_threads)
	fprintf (stderr,
		 "Failed to reinsert fast tracepoint jump at 0x%s (%s).\n",
		 paddress (where), strerror (err));
    }
}

struct breakpoint *
set_breakpoint_at (CORE_ADDR where, int (*handler) (CORE_ADDR))
{
  struct process_info *proc = current_process ();
  struct breakpoint *bp;
  struct raw_breakpoint *raw;

  raw = set_raw_breakpoint_at (where);

  if (raw == NULL)
    {
      /* warn? */
      return NULL;
    }

  bp = xcalloc (1, sizeof (struct breakpoint));
  bp->type = other_breakpoint;

  bp->raw = raw;
  bp->handler = handler;

  bp->next = proc->breakpoints;
  proc->breakpoints = bp;

  return bp;
}

static int
delete_raw_breakpoint (struct process_info *proc, struct raw_breakpoint *todel)
{
  struct raw_breakpoint *bp, **bp_link;
  int ret;

  bp = proc->raw_breakpoints;
  bp_link = &proc->raw_breakpoints;

  while (bp)
    {
      if (bp == todel)
	{
	  if (bp->inserted)
	    {
	      struct raw_breakpoint *prev_bp_link = *bp_link;
	      unsigned char buf[MAX_BREAKPOINT_LEN];

	      *bp_link = bp->next;

	      /* Since there can be trap breakpoints inserted in the
		 same address range, we use `write_inferior_memory',
		 which takes care of layering breakpoints on top of
		 fast tracepoints, and on top of the buffer we pass
		 it.  This works because we've already unlinked the
		 fast tracepoint jump above.  Also note that we need
		 to pass the current shadow contents, because
		 write_inferior_memory updates any shadow memory with
		 what we pass here, and we want that to be a nop.  */
	      memcpy (buf, bp->old_data, breakpoint_len);
	      ret = write_inferior_memory (bp->pc, buf, breakpoint_len);
	      if (ret != 0)
		{
		  /* Something went wrong, relink the breakpoint.  */
		  *bp_link = prev_bp_link;

		  if (debug_threads)
		    fprintf (stderr,
			     "Failed to uninsert raw breakpoint "
			     "at 0x%s (%s) while deleting it.\n",
			     paddress (bp->pc), strerror (ret));
		  return ret;
		}

	    }
	  else
	    *bp_link = bp->next;

	  free (bp);
	  return 0;
	}
      else
	{
	  bp_link = &bp->next;
	  bp = *bp_link;
	}
    }

  warning ("Could not find raw breakpoint in list.");
  return ENOENT;
}

static int
release_breakpoint (struct process_info *proc, struct breakpoint *bp)
{
  int newrefcount;
  int ret;

  newrefcount = bp->raw->refcount - 1;
  if (newrefcount == 0)
    {
      ret = delete_raw_breakpoint (proc, bp->raw);
      if (ret != 0)
	return ret;
    }
  else
    bp->raw->refcount = newrefcount;

  free (bp);

  return 0;
}

static int
delete_breakpoint_1 (struct process_info *proc, struct breakpoint *todel)
{
  struct breakpoint *bp, **bp_link;
  int err;

  bp = proc->breakpoints;
  bp_link = &proc->breakpoints;

  while (bp)
    {
      if (bp == todel)
	{
	  *bp_link = bp->next;

	  err = release_breakpoint (proc, bp);
	  if (err != 0)
	    return err;

	  bp = *bp_link;
	  return 0;
	}
      else
	{
	  bp_link = &bp->next;
	  bp = *bp_link;
	}
    }

  warning ("Could not find breakpoint in list.");
  return ENOENT;
}

int
delete_breakpoint (struct breakpoint *todel)
{
  struct process_info *proc = current_process ();
  return delete_breakpoint_1 (proc, todel);
}

struct breakpoint *
find_gdb_breakpoint_at (CORE_ADDR where)
{
  struct process_info *proc = current_process ();
  struct breakpoint *bp;

  for (bp = proc->breakpoints; bp != NULL; bp = bp->next)
    if (bp->type == gdb_breakpoint && bp->raw->pc == where)
      return bp;

  return NULL;
}

int
set_gdb_breakpoint_at (CORE_ADDR where)
{
  struct breakpoint *bp;

  if (breakpoint_data == NULL)
    return 1;

  /* If we see GDB inserting a second breakpoint at the same address,
     then the first breakpoint must have disappeared due to a shared
     library unload.  On targets where the shared libraries are
     handled by userspace, like SVR4, for example, GDBserver can't
     tell if a library was loaded or unloaded.  Since we refcount
     breakpoints, if we didn't do this, we'd just increase the
     refcount of the previous breakpoint at this address, but the trap
     was not planted in the inferior anymore, thus the breakpoint
     would never be hit.  */
  bp = find_gdb_breakpoint_at (where);
  if (bp != NULL)
    {
      delete_gdb_breakpoint_at (where);

      /* Might as well validate all other breakpoints.  */
      validate_breakpoints ();
    }

  bp = set_breakpoint_at (where, NULL);
  if (bp == NULL)
    return -1;

  bp->type = gdb_breakpoint;
  return 0;
}

int
delete_gdb_breakpoint_at (CORE_ADDR addr)
{
  struct breakpoint *bp;
  int err;

  if (breakpoint_data == NULL)
    return 1;

  bp = find_gdb_breakpoint_at (addr);
  if (bp == NULL)
    return -1;

  /* Before deleting the breakpoint, make sure to free
     its condition list.  */
  clear_gdb_breakpoint_conditions (addr);
  err = delete_breakpoint (bp);
  if (err)
    return -1;

  return 0;
}

/* Clear all conditions associated with this breakpoint address.  */

void
clear_gdb_breakpoint_conditions (CORE_ADDR addr)
{
  struct breakpoint *bp = find_gdb_breakpoint_at (addr);
  struct point_cond_list *cond;

  if (bp == NULL || bp->cond_list == NULL)
    return;

  cond = bp->cond_list;

  while (cond != NULL)
    {
      struct point_cond_list *cond_next;

      cond_next = cond->next;
      free (cond->cond->bytes);
      free (cond->cond);
      free (cond);
      cond = cond_next;
    }

  bp->cond_list = NULL;
}

/* Add condition CONDITION to GDBserver's breakpoint BP.  */

void
add_condition_to_breakpoint (struct breakpoint *bp,
			     struct agent_expr *condition)
{
  struct point_cond_list *new_cond;

  /* Create new condition.  */
  new_cond = xcalloc (1, sizeof (*new_cond));
  new_cond->cond = condition;

  /* Add condition to the list.  */
  new_cond->next = bp->cond_list;
  bp->cond_list = new_cond;
}

/* Add a target-side condition CONDITION to the breakpoint at ADDR.  */

int
add_breakpoint_condition (CORE_ADDR addr, char **condition)
{
  struct breakpoint *bp = find_gdb_breakpoint_at (addr);
  char *actparm = *condition;
  struct agent_expr *cond;

  if (bp == NULL)
    return 1;

  if (condition == NULL)
    return 1;

  cond = gdb_parse_agent_expr (&actparm);

  if (cond == NULL)
    {
      fprintf (stderr, "Condition evaluation failed. "
	       "Assuming unconditional.\n");
      return 0;
    }

  add_condition_to_breakpoint (bp, cond);

  *condition = actparm;

  return 0;
}

/* Evaluate condition (if any) at breakpoint BP.  Return 1 if
   true and 0 otherwise.  */

int
gdb_condition_true_at_breakpoint (CORE_ADDR where)
{
  /* Fetch registers for the current inferior.  */
  struct breakpoint *bp = find_gdb_breakpoint_at (where);
  ULONGEST value = 0;
  struct point_cond_list *cl;
  int err = 0;
  struct eval_agent_expr_context ctx;

  if (bp == NULL)
    return 0;

  /* Check if the breakpoint is unconditional.  If it is,
     the condition always evaluates to TRUE.  */
  if (bp->cond_list == NULL)
    return 1;

  ctx.regcache = get_thread_regcache (current_inferior, 1);
  ctx.tframe = NULL;
  ctx.tpoint = NULL;

  /* Evaluate each condition in the breakpoint's list of conditions.
     Return true if any of the conditions evaluates to TRUE.

     If we failed to evaluate the expression, TRUE is returned.  This
     forces GDB to reevaluate the conditions.  */
  for (cl = bp->cond_list;
       cl && !value && !err; cl = cl->next)
    {
      /* Evaluate the condition.  */
      err = gdb_eval_agent_expr (&ctx, cl->cond, &value);
    }

  if (err)
    return 1;

  return (value != 0);
}

/* Add commands COMMANDS to GDBserver's breakpoint BP.  */

void
add_commands_to_breakpoint (struct breakpoint *bp,
			    struct agent_expr *commands, int persist)
{
  struct point_command_list *new_cmd;

  /* Create new command.  */
  new_cmd = xcalloc (1, sizeof (*new_cmd));
  new_cmd->cmd = commands;
  new_cmd->persistence = persist;

  /* Add commands to the list.  */
  new_cmd->next = bp->command_list;
  bp->command_list = new_cmd;
}

/* Add a target-side command COMMAND to the breakpoint at ADDR.  */

int
add_breakpoint_commands (CORE_ADDR addr, char **command, int persist)
{
  struct breakpoint *bp = find_gdb_breakpoint_at (addr);
  char *actparm = *command;
  struct agent_expr *cmd;

  if (bp == NULL)
    return 1;

  if (command == NULL)
    return 1;

  cmd = gdb_parse_agent_expr (&actparm);

  if (cmd == NULL)
    {
      fprintf (stderr, "Command evaluation failed. "
	       "Disabling.\n");
      return 0;
    }

  add_commands_to_breakpoint (bp, cmd, persist);

  *command = actparm;

  return 0;
}

/* Return true if there are no commands to run at this location,
   which likely means we want to report back to GDB.  */
int
gdb_no_commands_at_breakpoint (CORE_ADDR where)
{
  struct breakpoint *bp = find_gdb_breakpoint_at (where);

  if (bp == NULL)
    return 0;

  if (debug_threads)
    fprintf (stderr, "at 0x%s, bp command_list is 0x%s\n",
	     paddress (where),
	     phex_nz ((uintptr_t) bp->command_list, 0));
  return (bp->command_list == NULL);
}

void
run_breakpoint_commands (CORE_ADDR where)
{
  /* Fetch registers for the current inferior.  */
  struct breakpoint *bp = find_gdb_breakpoint_at (where);
  ULONGEST value = 0;
  struct point_command_list *cl;
  int err = 0;
  struct eval_agent_expr_context ctx;

  if (bp == NULL)
    return;

  ctx.regcache = get_thread_regcache (current_inferior, 1);
  ctx.tframe = NULL;
  ctx.tpoint = NULL;

  for (cl = bp->command_list;
       cl && !value && !err; cl = cl->next)
    {
      /* Run the command.  */
      err = gdb_eval_agent_expr (&ctx, cl->cmd, &value);

      /* If one command has a problem, stop digging the hole deeper.  */
      if (err)
	break;
    }
}

/* Return 1 if there is a breakpoint inserted in address WHERE
   and if its condition, if it exists, is true.  */

int
gdb_breakpoint_here (CORE_ADDR where)
{
  return (find_gdb_breakpoint_at (where) != NULL);
}

void
set_reinsert_breakpoint (CORE_ADDR stop_at)
{
  struct breakpoint *bp;

  bp = set_breakpoint_at (stop_at, NULL);
  bp->type = reinsert_breakpoint;
}

void
delete_reinsert_breakpoints (void)
{
  struct process_info *proc = current_process ();
  struct breakpoint *bp, **bp_link;

  bp = proc->breakpoints;
  bp_link = &proc->breakpoints;

  while (bp)
    {
      if (bp->type == reinsert_breakpoint)
	{
	  *bp_link = bp->next;
	  release_breakpoint (proc, bp);
	  bp = *bp_link;
	}
      else
	{
	  bp_link = &bp->next;
	  bp = *bp_link;
	}
    }
}

static void
uninsert_raw_breakpoint (struct raw_breakpoint *bp)
{
  if (bp->inserted)
    {
      int err;
      unsigned char buf[MAX_BREAKPOINT_LEN];

      bp->inserted = 0;
      /* Since there can be fast tracepoint jumps inserted in the same
	 address range, we use `write_inferior_memory', which takes
	 care of layering breakpoints on top of fast tracepoints, and
	 on top of the buffer we pass it.  This works because we've
	 already unlinked the fast tracepoint jump above.  Also note
	 that we need to pass the current shadow contents, because
	 write_inferior_memory updates any shadow memory with what we
	 pass here, and we want that to be a nop.  */
      memcpy (buf, bp->old_data, breakpoint_len);
      err = write_inferior_memory (bp->pc, buf, breakpoint_len);
      if (err != 0)
	{
	  bp->inserted = 1;

	  if (debug_threads)
	    fprintf (stderr,
		     "Failed to uninsert raw breakpoint at 0x%s (%s).\n",
		     paddress (bp->pc), strerror (err));
	}
    }
}

void
uninsert_breakpoints_at (CORE_ADDR pc)
{
  struct raw_breakpoint *bp;

  bp = find_raw_breakpoint_at (pc);
  if (bp == NULL)
    {
      /* This can happen when we remove all breakpoints while handling
	 a step-over.  */
      if (debug_threads)
	fprintf (stderr,
		 "Could not find breakpoint at 0x%s "
		 "in list (uninserting).\n",
		 paddress (pc));
      return;
    }

  if (bp->inserted)
    uninsert_raw_breakpoint (bp);
}

void
uninsert_all_breakpoints (void)
{
  struct process_info *proc = current_process ();
  struct raw_breakpoint *bp;

  for (bp = proc->raw_breakpoints; bp != NULL; bp = bp->next)
    if (bp->inserted)
      uninsert_raw_breakpoint (bp);
}

static void
reinsert_raw_breakpoint (struct raw_breakpoint *bp)
{
  int err;

  if (bp->inserted)
    error ("Breakpoint already inserted at reinsert time.");

  err = (*the_target->write_memory) (bp->pc, breakpoint_data,
				     breakpoint_len);
  if (err == 0)
    bp->inserted = 1;
  else if (debug_threads)
    fprintf (stderr,
	     "Failed to reinsert breakpoint at 0x%s (%s).\n",
	     paddress (bp->pc), strerror (err));
}

void
reinsert_breakpoints_at (CORE_ADDR pc)
{
  struct raw_breakpoint *bp;

  bp = find_raw_breakpoint_at (pc);
  if (bp == NULL)
    {
      /* This can happen when we remove all breakpoints while handling
	 a step-over.  */
      if (debug_threads)
	fprintf (stderr,
		 "Could not find raw breakpoint at 0x%s "
		 "in list (reinserting).\n",
		 paddress (pc));
      return;
    }

  reinsert_raw_breakpoint (bp);
}

void
reinsert_all_breakpoints (void)
{
  struct process_info *proc = current_process ();
  struct raw_breakpoint *bp;

  for (bp = proc->raw_breakpoints; bp != NULL; bp = bp->next)
    if (!bp->inserted)
      reinsert_raw_breakpoint (bp);
}

void
check_breakpoints (CORE_ADDR stop_pc)
{
  struct process_info *proc = current_process ();
  struct breakpoint *bp, **bp_link;

  bp = proc->breakpoints;
  bp_link = &proc->breakpoints;

  while (bp)
    {
      if (bp->raw->pc == stop_pc)
	{
	  if (!bp->raw->inserted)
	    {
	      warning ("Hit a removed breakpoint?");
	      return;
	    }

	  if (bp->handler != NULL && (*bp->handler) (stop_pc))
	    {
	      *bp_link = bp->next;

	      release_breakpoint (proc, bp);

	      bp = *bp_link;
	      continue;
	    }
	}

      bp_link = &bp->next;
      bp = *bp_link;
    }
}

void
set_breakpoint_data (const unsigned char *bp_data, int bp_len)
{
  breakpoint_data = bp_data;
  breakpoint_len = bp_len;
}

int
breakpoint_here (CORE_ADDR addr)
{
  return (find_raw_breakpoint_at (addr) != NULL);
}

int
breakpoint_inserted_here (CORE_ADDR addr)
{
  struct raw_breakpoint *bp;

  bp = find_raw_breakpoint_at (addr);

  return (bp != NULL && bp->inserted);
}

static int
validate_inserted_breakpoint (struct raw_breakpoint *bp)
{
  unsigned char *buf;
  int err;

  gdb_assert (bp->inserted);

  buf = alloca (breakpoint_len);
  err = (*the_target->read_memory) (bp->pc, buf, breakpoint_len);
  if (err || memcmp (buf, breakpoint_data, breakpoint_len) != 0)
    {
      /* Tag it as gone.  */
      bp->inserted = 0;
      bp->shlib_disabled = 1;
      return 0;
    }

  return 1;
}

static void
delete_disabled_breakpoints (void)
{
  struct process_info *proc = current_process ();
  struct breakpoint *bp, *next;

  for (bp = proc->breakpoints; bp != NULL; bp = next)
    {
      next = bp->next;
      if (bp->raw->shlib_disabled)
	delete_breakpoint_1 (proc, bp);
    }
}

/* Check if breakpoints we inserted still appear to be inserted.  They
   may disappear due to a shared library unload, and worse, a new
   shared library may be reloaded at the same address as the
   previously unloaded one.  If that happens, we should make sure that
   the shadow memory of the old breakpoints isn't used when reading or
   writing memory.  */

void
validate_breakpoints (void)
{
  struct process_info *proc = current_process ();
  struct breakpoint *bp;

  for (bp = proc->breakpoints; bp != NULL; bp = bp->next)
    {
      if (bp->raw->inserted)
	validate_inserted_breakpoint (bp->raw);
    }

  delete_disabled_breakpoints ();
}

void
check_mem_read (CORE_ADDR mem_addr, unsigned char *buf, int mem_len)
{
  struct process_info *proc = current_process ();
  struct raw_breakpoint *bp = proc->raw_breakpoints;
  struct fast_tracepoint_jump *jp = proc->fast_tracepoint_jumps;
  CORE_ADDR mem_end = mem_addr + mem_len;
  int disabled_one = 0;

  for (; jp != NULL; jp = jp->next)
    {
      CORE_ADDR bp_end = jp->pc + jp->length;
      CORE_ADDR start, end;
      int copy_offset, copy_len, buf_offset;

      gdb_assert (fast_tracepoint_jump_shadow (jp) >= buf + mem_len
		  || buf >= fast_tracepoint_jump_shadow (jp) + (jp)->length);

      if (mem_addr >= bp_end)
	continue;
      if (jp->pc >= mem_end)
	continue;

      start = jp->pc;
      if (mem_addr > start)
	start = mem_addr;

      end = bp_end;
      if (end > mem_end)
	end = mem_end;

      copy_len = end - start;
      copy_offset = start - jp->pc;
      buf_offset = start - mem_addr;

      if (jp->inserted)
	memcpy (buf + buf_offset,
		fast_tracepoint_jump_shadow (jp) + copy_offset,
		copy_len);
    }

  for (; bp != NULL; bp = bp->next)
    {
      CORE_ADDR bp_end = bp->pc + breakpoint_len;
      CORE_ADDR start, end;
      int copy_offset, copy_len, buf_offset;

      gdb_assert (bp->old_data >= buf + mem_len
		  || buf >= &bp->old_data[sizeof (bp->old_data)]);

      if (mem_addr >= bp_end)
	continue;
      if (bp->pc >= mem_end)
	continue;

      start = bp->pc;
      if (mem_addr > start)
	start = mem_addr;

      end = bp_end;
      if (end > mem_end)
	end = mem_end;

      copy_len = end - start;
      copy_offset = start - bp->pc;
      buf_offset = start - mem_addr;

      if (bp->inserted)
	{
	  if (validate_inserted_breakpoint (bp))
	    memcpy (buf + buf_offset, bp->old_data + copy_offset, copy_len);
	  else
	    disabled_one = 1;
	}
    }

  if (disabled_one)
    delete_disabled_breakpoints ();
}

void
check_mem_write (CORE_ADDR mem_addr, unsigned char *buf,
		 const unsigned char *myaddr, int mem_len)
{
  struct process_info *proc = current_process ();
  struct raw_breakpoint *bp = proc->raw_breakpoints;
  struct fast_tracepoint_jump *jp = proc->fast_tracepoint_jumps;
  CORE_ADDR mem_end = mem_addr + mem_len;
  int disabled_one = 0;

  /* First fast tracepoint jumps, then breakpoint traps on top.  */

  for (; jp != NULL; jp = jp->next)
    {
      CORE_ADDR jp_end = jp->pc + jp->length;
      CORE_ADDR start, end;
      int copy_offset, copy_len, buf_offset;

      gdb_assert (fast_tracepoint_jump_shadow (jp) >= myaddr + mem_len
		  || myaddr >= fast_tracepoint_jump_shadow (jp) + (jp)->length);
      gdb_assert (fast_tracepoint_jump_insn (jp) >= buf + mem_len
		  || buf >= fast_tracepoint_jump_insn (jp) + (jp)->length);

      if (mem_addr >= jp_end)
	continue;
      if (jp->pc >= mem_end)
	continue;

      start = jp->pc;
      if (mem_addr > start)
	start = mem_addr;

      end = jp_end;
      if (end > mem_end)
	end = mem_end;

      copy_len = end - start;
      copy_offset = start - jp->pc;
      buf_offset = start - mem_addr;

      memcpy (fast_tracepoint_jump_shadow (jp) + copy_offset,
	      myaddr + buf_offset, copy_len);
      if (jp->inserted)
	memcpy (buf + buf_offset,
		fast_tracepoint_jump_insn (jp) + copy_offset, copy_len);
    }

  for (; bp != NULL; bp = bp->next)
    {
      CORE_ADDR bp_end = bp->pc + breakpoint_len;
      CORE_ADDR start, end;
      int copy_offset, copy_len, buf_offset;

      gdb_assert (bp->old_data >= myaddr + mem_len
		  || myaddr >= &bp->old_data[sizeof (bp->old_data)]);

      if (mem_addr >= bp_end)
	continue;
      if (bp->pc >= mem_end)
	continue;

      start = bp->pc;
      if (mem_addr > start)
	start = mem_addr;

      end = bp_end;
      if (end > mem_end)
	end = mem_end;

      copy_len = end - start;
      copy_offset = start - bp->pc;
      buf_offset = start - mem_addr;

      memcpy (bp->old_data + copy_offset, myaddr + buf_offset, copy_len);
      if (bp->inserted)
	{
	  if (validate_inserted_breakpoint (bp))
	    memcpy (buf + buf_offset, breakpoint_data + copy_offset, copy_len);
	  else
	    disabled_one = 1;
	}
    }

  if (disabled_one)
    delete_disabled_breakpoints ();
}

/* Delete all breakpoints, and un-insert them from the inferior.  */

void
delete_all_breakpoints (void)
{
  struct process_info *proc = current_process ();

  while (proc->breakpoints)
    delete_breakpoint_1 (proc, proc->breakpoints);
}

/* Clear the "inserted" flag in all breakpoints.  */

void
mark_breakpoints_out (struct process_info *proc)
{
  struct raw_breakpoint *raw_bp;

  for (raw_bp = proc->raw_breakpoints; raw_bp != NULL; raw_bp = raw_bp->next)
    raw_bp->inserted = 0;
}

/* Release all breakpoints, but do not try to un-insert them from the
   inferior.  */

void
free_all_breakpoints (struct process_info *proc)
{
  mark_breakpoints_out (proc);

  /* Note: use PROC explicitly instead of deferring to
     delete_all_breakpoints --- CURRENT_INFERIOR may already have been
     released when we get here.  There should be no call to
     current_process from here on.  */
  while (proc->breakpoints)
    delete_breakpoint_1 (proc, proc->breakpoints);
}
