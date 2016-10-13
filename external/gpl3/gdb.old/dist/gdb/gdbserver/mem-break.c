/* Memory breakpoint operations for the remote server for GDB.
   Copyright (C) 2002-2015 Free Software Foundation, Inc.

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
const unsigned char *breakpoint_data;
int breakpoint_len;

#define MAX_BREAKPOINT_LEN 8

/* Helper macro used in loops that append multiple items to a singly-linked
   list instead of inserting items at the head of the list, as, say, in the
   breakpoint lists.  LISTPP is a pointer to the pointer that is the head of
   the new list.  ITEMP is a pointer to the item to be added to the list.
   TAILP must be defined to be the same type as ITEMP, and initialized to
   NULL.  */

#define APPEND_TO_LIST(listpp, itemp, tailp) \
	  do \
	    { \
	      if ((tailp) == NULL) \
		*(listpp) = (itemp); \
	      else \
		(tailp)->next = (itemp); \
	      (tailp) = (itemp); \
	    } \
	  while (0)

/* GDB will never try to install multiple breakpoints at the same
   address.  However, we can see GDB requesting to insert a breakpoint
   at an address is had already inserted one previously in a few
   situations.

   - The RSP documentation on Z packets says that to avoid potential
   problems with duplicate packets, the operations should be
   implemented in an idempotent way.

   - A breakpoint is set at ADDR, an address in a shared library.
   Then the shared library is unloaded.  And then another, unrelated,
   breakpoint at ADDR is set.  There is not breakpoint removal request
   between the first and the second breakpoint.

   - When GDB wants to update the target-side breakpoint conditions or
   commands, it re-inserts the breakpoint, with updated
   conditions/commands associated.

   Also, we need to keep track of internal breakpoints too, so we do
   need to be able to install multiple breakpoints at the same address
   transparently.

   We keep track of two different, and closely related structures.  A
   raw breakpoint, which manages the low level, close to the metal
   aspect of a breakpoint.  It holds the breakpoint address, and for
   software breakpoints, a buffer holding a copy of the instructions
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

  /* The low level type of the breakpoint (software breakpoint,
     watchpoint, etc.)  */
  enum raw_bkpt_type raw_type;

  /* A reference count.  Each high level breakpoint referencing this
     raw breakpoint accounts for one reference.  */
  int refcount;

  /* The breakpoint's insertion address.  There can only be one raw
     breakpoint for a given PC.  */
  CORE_ADDR pc;

  /* The breakpoint's size.  */
  int size;

  /* The breakpoint's shadow memory.  */
  unsigned char old_data[MAX_BREAKPOINT_LEN];

  /* Positive if this breakpoint is currently inserted in the
     inferior.  Negative if it was, but we've detected that it's now
     gone.  Zero if not inserted.  */
  int inserted;
};

/* The type of a breakpoint.  */
enum bkpt_type
  {
    /* A GDB breakpoint, requested with a Z0 packet.  */
    gdb_breakpoint_Z0,

    /* A GDB hardware breakpoint, requested with a Z1 packet.  */
    gdb_breakpoint_Z1,

    /* A GDB write watchpoint, requested with a Z2 packet.  */
    gdb_breakpoint_Z2,

    /* A GDB read watchpoint, requested with a Z3 packet.  */
    gdb_breakpoint_Z3,

    /* A GDB access watchpoint, requested with a Z4 packet.  */
    gdb_breakpoint_Z4,

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

/* See mem-break.h.  */

enum target_hw_bp_type
raw_bkpt_type_to_target_hw_bp_type (enum raw_bkpt_type raw_type)
{
  switch (raw_type)
    {
    case raw_bkpt_type_hw:
      return hw_execute;
    case raw_bkpt_type_write_wp:
      return hw_write;
    case raw_bkpt_type_read_wp:
      return hw_read;
    case raw_bkpt_type_access_wp:
      return hw_access;
    default:
      internal_error (__FILE__, __LINE__,
		      "bad raw breakpoint type %d", (int) raw_type);
    }
}

/* See mem-break.h.  */

static enum bkpt_type
Z_packet_to_bkpt_type (char z_type)
{
  gdb_assert ('0' <= z_type && z_type <= '4');

  return gdb_breakpoint_Z0 + (z_type - '0');
}

/* See mem-break.h.  */

enum raw_bkpt_type
Z_packet_to_raw_bkpt_type (char z_type)
{
  switch (z_type)
    {
    case Z_PACKET_SW_BP:
      return raw_bkpt_type_sw;
    case Z_PACKET_HW_BP:
      return raw_bkpt_type_hw;
    case Z_PACKET_WRITE_WP:
      return raw_bkpt_type_write_wp;
    case Z_PACKET_READ_WP:
      return raw_bkpt_type_read_wp;
    case Z_PACKET_ACCESS_WP:
      return raw_bkpt_type_access_wp;
    default:
      gdb_assert_not_reached ("unhandled Z packet type.");
    }
}

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

/* Find low-level breakpoint of type TYPE at address ADDR that is not
   insert-disabled.  Returns NULL if not found.  */

static struct raw_breakpoint *
find_enabled_raw_code_breakpoint_at (CORE_ADDR addr, enum raw_bkpt_type type)
{
  struct process_info *proc = current_process ();
  struct raw_breakpoint *bp;

  for (bp = proc->raw_breakpoints; bp != NULL; bp = bp->next)
    if (bp->pc == addr
	&& bp->raw_type == type
	&& bp->inserted >= 0)
      return bp;

  return NULL;
}

/* Find low-level breakpoint of type TYPE at address ADDR.  Returns
   NULL if not found.  */

static struct raw_breakpoint *
find_raw_breakpoint_at (CORE_ADDR addr, enum raw_bkpt_type type, int size)
{
  struct process_info *proc = current_process ();
  struct raw_breakpoint *bp;

  for (bp = proc->raw_breakpoints; bp != NULL; bp = bp->next)
    if (bp->pc == addr && bp->raw_type == type && bp->size == size)
      return bp;

  return NULL;
}

/* See mem-break.h.  */

int
insert_memory_breakpoint (struct raw_breakpoint *bp)
{
  unsigned char buf[MAX_BREAKPOINT_LEN];
  int err;

  if (breakpoint_data == NULL)
    return 1;

  /* If the architecture treats the size field of Z packets as a
     'kind' field, then we'll need to be able to know which is the
     breakpoint instruction too.  */
  if (bp->size != breakpoint_len)
    {
      if (debug_threads)
	debug_printf ("Don't know how to insert breakpoints of size %d.\n",
		      bp->size);
      return -1;
    }

  /* Note that there can be fast tracepoint jumps installed in the
     same memory range, so to get at the original memory, we need to
     use read_inferior_memory, which masks those out.  */
  err = read_inferior_memory (bp->pc, buf, breakpoint_len);
  if (err != 0)
    {
      if (debug_threads)
	debug_printf ("Failed to read shadow memory of"
		      " breakpoint at 0x%s (%s).\n",
		      paddress (bp->pc), strerror (err));
    }
  else
    {
      memcpy (bp->old_data, buf, breakpoint_len);

      err = (*the_target->write_memory) (bp->pc, breakpoint_data,
					 breakpoint_len);
      if (err != 0)
	{
	  if (debug_threads)
	    debug_printf ("Failed to insert breakpoint at 0x%s (%s).\n",
			  paddress (bp->pc), strerror (err));
	}
    }
  return err != 0 ? -1 : 0;
}

/* See mem-break.h  */

int
remove_memory_breakpoint (struct raw_breakpoint *bp)
{
  unsigned char buf[MAX_BREAKPOINT_LEN];
  int err;

  /* Since there can be trap breakpoints inserted in the same address
     range, we use `write_inferior_memory', which takes care of
     layering breakpoints on top of fast tracepoints, and on top of
     the buffer we pass it.  This works because the caller has already
     either unlinked the breakpoint or marked it uninserted.  Also
     note that we need to pass the current shadow contents, because
     write_inferior_memory updates any shadow memory with what we pass
     here, and we want that to be a nop.  */
  memcpy (buf, bp->old_data, breakpoint_len);
  err = write_inferior_memory (bp->pc, buf, breakpoint_len);
  if (err != 0)
    {
      if (debug_threads)
	debug_printf ("Failed to uninsert raw breakpoint "
		      "at 0x%s (%s) while deleting it.\n",
		      paddress (bp->pc), strerror (err));
    }
  return err != 0 ? -1 : 0;
}

/* Set a RAW breakpoint of type TYPE and size SIZE at WHERE.  On
   success, a pointer to the new breakpoint is returned.  On failure,
   returns NULL and writes the error code to *ERR.  */

static struct raw_breakpoint *
set_raw_breakpoint_at (enum raw_bkpt_type type, CORE_ADDR where, int size,
		       int *err)
{
  struct process_info *proc = current_process ();
  struct raw_breakpoint *bp;

  if (type == raw_bkpt_type_sw || type == raw_bkpt_type_hw)
    {
      bp = find_enabled_raw_code_breakpoint_at (where, type);
      if (bp != NULL && bp->size != size)
	{
	  /* A different size than previously seen.  The previous
	     breakpoint must be gone then.  */
	  if (debug_threads)
	    debug_printf ("Inconsistent breakpoint size?  Was %d, now %d.\n",
			  bp->size, size);
	  bp->inserted = -1;
	  bp = NULL;
	}
    }
  else
    bp = find_raw_breakpoint_at (where, type, size);

  if (bp != NULL)
    {
      bp->refcount++;
      return bp;
    }

  bp = xcalloc (1, sizeof (*bp));
  bp->pc = where;
  bp->size = size;
  bp->refcount = 1;
  bp->raw_type = type;

  *err = the_target->insert_point (bp->raw_type, bp->pc, bp->size, bp);
  if (*err != 0)
    {
      if (debug_threads)
	debug_printf ("Failed to insert breakpoint at 0x%s (%d).\n",
		      paddress (where), *err);
      free (bp);
      return NULL;
    }

  bp->inserted = 1;
  /* Link the breakpoint in.  */
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
		    debug_printf ("Failed to uninsert fast tracepoint jump "
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
	debug_printf ("Failed to read shadow memory of"
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
	debug_printf ("Failed to insert fast tracepoint jump at 0x%s (%s).\n",
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
	debug_printf ("Could not find fast tracepoint jump at 0x%s "
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
	    debug_printf ("Failed to uninsert fast tracepoint jump at"
			  " 0x%s (%s).\n",
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
	debug_printf ("Could not find fast tracepoint jump at 0x%s "
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
	debug_printf ("Failed to reinsert fast tracepoint jump at"
		      " 0x%s (%s).\n",
		      paddress (where), strerror (err));
    }
}

/* Set a high-level breakpoint of type TYPE, with low level type
   RAW_TYPE and size SIZE, at WHERE.  On success, a pointer to the new
   breakpoint is returned.  On failure, returns NULL and writes the
   error code to *ERR.  HANDLER is called when the breakpoint is hit.
   HANDLER should return 1 if the breakpoint should be deleted, 0
   otherwise.  */

static struct breakpoint *
set_breakpoint (enum bkpt_type type, enum raw_bkpt_type raw_type,
		CORE_ADDR where, int size,
		int (*handler) (CORE_ADDR), int *err)
{
  struct process_info *proc = current_process ();
  struct breakpoint *bp;
  struct raw_breakpoint *raw;

  raw = set_raw_breakpoint_at (raw_type, where, size, err);

  if (raw == NULL)
    {
      /* warn? */
      return NULL;
    }

  bp = xcalloc (1, sizeof (struct breakpoint));
  bp->type = type;

  bp->raw = raw;
  bp->handler = handler;

  bp->next = proc->breakpoints;
  proc->breakpoints = bp;

  return bp;
}

/* See mem-break.h  */

struct breakpoint *
set_breakpoint_at (CORE_ADDR where, int (*handler) (CORE_ADDR))
{
  int err_ignored;

  return set_breakpoint (other_breakpoint, raw_bkpt_type_sw,
			 where, breakpoint_len, handler,
			 &err_ignored);
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
	  if (bp->inserted > 0)
	    {
	      struct raw_breakpoint *prev_bp_link = *bp_link;

	      *bp_link = bp->next;

	      ret = the_target->remove_point (bp->raw_type, bp->pc, bp->size,
					      bp);
	      if (ret != 0)
		{
		  /* Something went wrong, relink the breakpoint.  */
		  *bp_link = prev_bp_link;

		  if (debug_threads)
		    debug_printf ("Failed to uninsert raw breakpoint "
				  "at 0x%s while deleting it.\n",
				  paddress (bp->pc));
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

/* Locate a GDB breakpoint of type Z_TYPE and size SIZE placed at
   address ADDR and return a pointer to its structure.  If SIZE is -1,
   the breakpoints' sizes are ignored.  */

static struct breakpoint *
find_gdb_breakpoint (char z_type, CORE_ADDR addr, int size)
{
  struct process_info *proc = current_process ();
  struct breakpoint *bp;
  enum bkpt_type type = Z_packet_to_bkpt_type (z_type);

  for (bp = proc->breakpoints; bp != NULL; bp = bp->next)
    if (bp->type == type && bp->raw->pc == addr
	&& (size == -1 || bp->raw->size == size))
      return bp;

  return NULL;
}

static int
z_type_supported (char z_type)
{
  return (z_type >= '0' && z_type <= '4'
	  && the_target->supports_z_point_type != NULL
	  && the_target->supports_z_point_type (z_type));
}

/* Create a new GDB breakpoint of type Z_TYPE at ADDR with size SIZE.
   Returns a pointer to the newly created breakpoint on success.  On
   failure returns NULL and sets *ERR to either -1 for error, or 1 if
   Z_TYPE breakpoints are not supported on this target.  */

static struct breakpoint *
set_gdb_breakpoint_1 (char z_type, CORE_ADDR addr, int size, int *err)
{
  struct breakpoint *bp;
  enum bkpt_type type;
  enum raw_bkpt_type raw_type;

  /* If we see GDB inserting a second code breakpoint at the same
     address, then either: GDB is updating the breakpoint's conditions
     or commands; or, the first breakpoint must have disappeared due
     to a shared library unload.  On targets where the shared
     libraries are handled by userspace, like SVR4, for example,
     GDBserver can't tell if a library was loaded or unloaded.  Since
     we refcount raw breakpoints, we must be careful to make sure GDB
     breakpoints never contribute more than one reference.  if we
     didn't do this, in case the previous breakpoint is gone due to a
     shared library unload, we'd just increase the refcount of the
     previous breakpoint at this address, but the trap was not planted
     in the inferior anymore, thus the breakpoint would never be hit.
     Note this must be careful to not create a window where
     breakpoints are removed from the target, for non-stop, in case
     the target can poke at memory while the program is running.  */
  if (z_type == Z_PACKET_SW_BP
      || z_type == Z_PACKET_HW_BP)
    {
      bp = find_gdb_breakpoint (z_type, addr, -1);

      if (bp != NULL)
	{
	  if (bp->raw->size != size)
	    {
	      /* A different size than previously seen.  The previous
		 breakpoint must be gone then.  */
	      bp->raw->inserted = -1;
	      delete_breakpoint (bp);
	      bp = NULL;
	    }
	  else if (z_type == Z_PACKET_SW_BP)
	    {
	      /* Check if the breakpoint is actually gone from the
		 target, due to an solib unload, for example.  Might
		 as well validate _all_ breakpoints.  */
	      validate_breakpoints ();

	      /* Breakpoints that don't pass validation are
		 deleted.  */
	      bp = find_gdb_breakpoint (z_type, addr, -1);
	    }
	}
    }
  else
    {
      /* Data breakpoints for the same address but different size are
	 expected.  GDB doesn't merge these.  The backend gets to do
	 that if it wants/can.  */
      bp = find_gdb_breakpoint (z_type, addr, size);
    }

  if (bp != NULL)
    {
      /* We already know about this breakpoint, there's nothing else
	 to do - GDB's reference is already accounted for.  Note that
	 whether the breakpoint inserted is left as is - we may be
	 stepping over it, for example, in which case we don't want to
	 force-reinsert it.  */
      return bp;
    }

  raw_type = Z_packet_to_raw_bkpt_type (z_type);
  type = Z_packet_to_bkpt_type (z_type);
  return set_breakpoint (type, raw_type, addr, size, NULL, err);
}

static int
check_gdb_bp_preconditions (char z_type, int *err)
{
  /* As software/memory breakpoints work by poking at memory, we need
     to prepare to access memory.  If that operation fails, we need to
     return error.  Seeing an error, if this is the first breakpoint
     of that type that GDB tries to insert, GDB would then assume the
     breakpoint type is supported, but it may actually not be.  So we
     need to check whether the type is supported at all before
     preparing to access memory.  */
  if (!z_type_supported (z_type))
    {
      *err = 1;
      return 0;
    }
  else if (current_thread == NULL)
    {
      *err = -1;
      return 0;
    }
  else
    return 1;
}

/* See mem-break.h.  This is a wrapper for set_gdb_breakpoint_1 that
   knows to prepare to access memory for Z0 breakpoints.  */

struct breakpoint *
set_gdb_breakpoint (char z_type, CORE_ADDR addr, int size, int *err)
{
  struct breakpoint *bp;

  if (!check_gdb_bp_preconditions (z_type, err))
    return NULL;

  /* If inserting a software/memory breakpoint, need to prepare to
     access memory.  */
  if (z_type == Z_PACKET_SW_BP)
    {
      *err = prepare_to_access_memory ();
      if (*err != 0)
	return NULL;
    }

  bp = set_gdb_breakpoint_1 (z_type, addr, size, err);

  if (z_type == Z_PACKET_SW_BP)
    done_accessing_memory ();

  return bp;
}

/* Delete a GDB breakpoint of type Z_TYPE and size SIZE previously
   inserted at ADDR with set_gdb_breakpoint_at.  Returns 0 on success,
   -1 on error, and 1 if Z_TYPE breakpoints are not supported on this
   target.  */

static int
delete_gdb_breakpoint_1 (char z_type, CORE_ADDR addr, int size)
{
  struct breakpoint *bp;
  int err;

  bp = find_gdb_breakpoint (z_type, addr, size);
  if (bp == NULL)
    return -1;

  /* Before deleting the breakpoint, make sure to free its condition
     and command lists.  */
  clear_breakpoint_conditions_and_commands (bp);
  err = delete_breakpoint (bp);
  if (err != 0)
    return -1;

  return 0;
}

/* See mem-break.h.  This is a wrapper for delete_gdb_breakpoint that
   knows to prepare to access memory for Z0 breakpoints.  */

int
delete_gdb_breakpoint (char z_type, CORE_ADDR addr, int size)
{
  int ret;

  if (!check_gdb_bp_preconditions (z_type, &ret))
    return ret;

  /* If inserting a software/memory breakpoint, need to prepare to
     access memory.  */
  if (z_type == Z_PACKET_SW_BP)
    {
      int err;

      err = prepare_to_access_memory ();
      if (err != 0)
	return -1;
    }

  ret = delete_gdb_breakpoint_1 (z_type, addr, size);

  if (z_type == Z_PACKET_SW_BP)
    done_accessing_memory ();

  return ret;
}

/* Clear all conditions associated with a breakpoint.  */

static void
clear_breakpoint_conditions (struct breakpoint *bp)
{
  struct point_cond_list *cond;

  if (bp->cond_list == NULL)
    return;

  cond = bp->cond_list;

  while (cond != NULL)
    {
      struct point_cond_list *cond_next;

      cond_next = cond->next;
      gdb_free_agent_expr (cond->cond);
      free (cond);
      cond = cond_next;
    }

  bp->cond_list = NULL;
}

/* Clear all commands associated with a breakpoint.  */

static void
clear_breakpoint_commands (struct breakpoint *bp)
{
  struct point_command_list *cmd;

  if (bp->command_list == NULL)
    return;

  cmd = bp->command_list;

  while (cmd != NULL)
    {
      struct point_command_list *cmd_next;

      cmd_next = cmd->next;
      gdb_free_agent_expr (cmd->cmd);
      free (cmd);
      cmd = cmd_next;
    }

  bp->command_list = NULL;
}

void
clear_breakpoint_conditions_and_commands (struct breakpoint *bp)
{
  clear_breakpoint_conditions (bp);
  clear_breakpoint_commands (bp);
}

/* Add condition CONDITION to GDBserver's breakpoint BP.  */

static void
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

/* Add a target-side condition CONDITION to a breakpoint.  */

int
add_breakpoint_condition (struct breakpoint *bp, char **condition)
{
  char *actparm = *condition;
  struct agent_expr *cond;

  if (condition == NULL)
    return 1;

  if (bp == NULL)
    return 0;

  cond = gdb_parse_agent_expr (&actparm);

  if (cond == NULL)
    {
      fprintf (stderr, "Condition evaluation failed. "
	       "Assuming unconditional.\n");
      return 0;
    }

  add_condition_to_breakpoint (bp, cond);

  *condition = actparm;

  return 1;
}

/* Evaluate condition (if any) at breakpoint BP.  Return 1 if
   true and 0 otherwise.  */

static int
gdb_condition_true_at_breakpoint_z_type (char z_type, CORE_ADDR addr)
{
  /* Fetch registers for the current inferior.  */
  struct breakpoint *bp = find_gdb_breakpoint (z_type, addr, -1);
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

  ctx.regcache = get_thread_regcache (current_thread, 1);
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

int
gdb_condition_true_at_breakpoint (CORE_ADDR where)
{
  /* Only check code (software or hardware) breakpoints.  */
  return (gdb_condition_true_at_breakpoint_z_type (Z_PACKET_SW_BP, where)
	  || gdb_condition_true_at_breakpoint_z_type (Z_PACKET_HW_BP, where));
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
add_breakpoint_commands (struct breakpoint *bp, char **command,
			 int persist)
{
  char *actparm = *command;
  struct agent_expr *cmd;

  if (command == NULL)
    return 1;

  if (bp == NULL)
    return 0;

  cmd = gdb_parse_agent_expr (&actparm);

  if (cmd == NULL)
    {
      fprintf (stderr, "Command evaluation failed. "
	       "Disabling.\n");
      return 0;
    }

  add_commands_to_breakpoint (bp, cmd, persist);

  *command = actparm;

  return 1;
}

/* Return true if there are no commands to run at this location,
   which likely means we want to report back to GDB.  */

static int
gdb_no_commands_at_breakpoint_z_type (char z_type, CORE_ADDR addr)
{
  struct breakpoint *bp = find_gdb_breakpoint (z_type, addr, -1);

  if (bp == NULL)
    return 1;

  if (debug_threads)
    debug_printf ("at 0x%s, type Z%c, bp command_list is 0x%s\n",
		  paddress (addr), z_type,
		  phex_nz ((uintptr_t) bp->command_list, 0));
  return (bp->command_list == NULL);
}

/* Return true if there are no commands to run at this location,
   which likely means we want to report back to GDB.  */

int
gdb_no_commands_at_breakpoint (CORE_ADDR where)
{
  /* Only check code (software or hardware) breakpoints.  */
  return (gdb_no_commands_at_breakpoint_z_type (Z_PACKET_SW_BP, where)
	  && gdb_no_commands_at_breakpoint_z_type (Z_PACKET_HW_BP, where));
}

/* Run a breakpoint's commands.  Returns 0 if there was a problem
   running any command, 1 otherwise.  */

static int
run_breakpoint_commands_z_type (char z_type, CORE_ADDR addr)
{
  /* Fetch registers for the current inferior.  */
  struct breakpoint *bp = find_gdb_breakpoint (z_type, addr, -1);
  ULONGEST value = 0;
  struct point_command_list *cl;
  int err = 0;
  struct eval_agent_expr_context ctx;

  if (bp == NULL)
    return 1;

  ctx.regcache = get_thread_regcache (current_thread, 1);
  ctx.tframe = NULL;
  ctx.tpoint = NULL;

  for (cl = bp->command_list;
       cl && !value && !err; cl = cl->next)
    {
      /* Run the command.  */
      err = gdb_eval_agent_expr (&ctx, cl->cmd, &value);

      /* If one command has a problem, stop digging the hole deeper.  */
      if (err)
	return 0;
    }

  return 1;
}

void
run_breakpoint_commands (CORE_ADDR where)
{
  /* Only check code (software or hardware) breakpoints.  If one
     command has a problem, stop digging the hole deeper.  */
  if (run_breakpoint_commands_z_type (Z_PACKET_SW_BP, where))
    run_breakpoint_commands_z_type (Z_PACKET_HW_BP, where);
}

/* See mem-break.h.  */

int
gdb_breakpoint_here (CORE_ADDR where)
{
  /* Only check code (software or hardware) breakpoints.  */
  return (find_gdb_breakpoint (Z_PACKET_SW_BP, where, -1) != NULL
	  || find_gdb_breakpoint (Z_PACKET_HW_BP, where, -1) != NULL);
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
  if (bp->inserted < 0)
    {
      if (debug_threads)
	debug_printf ("Breakpoint at %s is marked insert-disabled.\n",
		      paddress (bp->pc));
    }
  else if (bp->inserted > 0)
    {
      int err;

      bp->inserted = 0;

      err = the_target->remove_point (bp->raw_type, bp->pc, bp->size, bp);
      if (err != 0)
	{
	  bp->inserted = 1;

	  if (debug_threads)
	    debug_printf ("Failed to uninsert raw breakpoint at 0x%s.\n",
			  paddress (bp->pc));
	}
    }
}

void
uninsert_breakpoints_at (CORE_ADDR pc)
{
  struct process_info *proc = current_process ();
  struct raw_breakpoint *bp;
  int found = 0;

  for (bp = proc->raw_breakpoints; bp != NULL; bp = bp->next)
    if ((bp->raw_type == raw_bkpt_type_sw
	 || bp->raw_type == raw_bkpt_type_hw)
	&& bp->pc == pc)
      {
	found = 1;

	if (bp->inserted)
	  uninsert_raw_breakpoint (bp);
      }

  if (!found)
    {
      /* This can happen when we remove all breakpoints while handling
	 a step-over.  */
      if (debug_threads)
	debug_printf ("Could not find breakpoint at 0x%s "
		      "in list (uninserting).\n",
		      paddress (pc));
    }
}

void
uninsert_all_breakpoints (void)
{
  struct process_info *proc = current_process ();
  struct raw_breakpoint *bp;

  for (bp = proc->raw_breakpoints; bp != NULL; bp = bp->next)
    if ((bp->raw_type == raw_bkpt_type_sw
	 || bp->raw_type == raw_bkpt_type_hw)
	&& bp->inserted)
      uninsert_raw_breakpoint (bp);
}

static void
reinsert_raw_breakpoint (struct raw_breakpoint *bp)
{
  int err;

  if (bp->inserted)
    error ("Breakpoint already inserted at reinsert time.");

  err = the_target->insert_point (bp->raw_type, bp->pc, bp->size, bp);
  if (err == 0)
    bp->inserted = 1;
  else if (debug_threads)
    debug_printf ("Failed to reinsert breakpoint at 0x%s (%d).\n",
		  paddress (bp->pc), err);
}

void
reinsert_breakpoints_at (CORE_ADDR pc)
{
  struct process_info *proc = current_process ();
  struct raw_breakpoint *bp;
  int found = 0;

  for (bp = proc->raw_breakpoints; bp != NULL; bp = bp->next)
    if ((bp->raw_type == raw_bkpt_type_sw
	 || bp->raw_type == raw_bkpt_type_hw)
	&& bp->pc == pc)
      {
	found = 1;

	reinsert_raw_breakpoint (bp);
      }

  if (!found)
    {
      /* This can happen when we remove all breakpoints while handling
	 a step-over.  */
      if (debug_threads)
	debug_printf ("Could not find raw breakpoint at 0x%s "
		      "in list (reinserting).\n",
		      paddress (pc));
    }
}

void
reinsert_all_breakpoints (void)
{
  struct process_info *proc = current_process ();
  struct raw_breakpoint *bp;

  for (bp = proc->raw_breakpoints; bp != NULL; bp = bp->next)
    if ((bp->raw_type == raw_bkpt_type_sw
	 || bp->raw_type == raw_bkpt_type_hw)
	&& !bp->inserted)
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
      struct raw_breakpoint *raw = bp->raw;

      if ((raw->raw_type == raw_bkpt_type_sw
	   || raw->raw_type == raw_bkpt_type_hw)
	  && raw->pc == stop_pc)
	{
	  if (!raw->inserted)
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
  struct process_info *proc = current_process ();
  struct raw_breakpoint *bp;

  for (bp = proc->raw_breakpoints; bp != NULL; bp = bp->next)
    if ((bp->raw_type == raw_bkpt_type_sw
	 || bp->raw_type == raw_bkpt_type_hw)
	&& bp->pc == addr)
      return 1;

  return 0;
}

int
breakpoint_inserted_here (CORE_ADDR addr)
{
  struct process_info *proc = current_process ();
  struct raw_breakpoint *bp;

  for (bp = proc->raw_breakpoints; bp != NULL; bp = bp->next)
    if ((bp->raw_type == raw_bkpt_type_sw
	 || bp->raw_type == raw_bkpt_type_hw)
	&& bp->pc == addr
	&& bp->inserted)
      return 1;

  return 0;
}

/* See mem-break.h.  */

int
software_breakpoint_inserted_here (CORE_ADDR addr)
{
  struct process_info *proc = current_process ();
  struct raw_breakpoint *bp;

  for (bp = proc->raw_breakpoints; bp != NULL; bp = bp->next)
    if (bp->raw_type == raw_bkpt_type_sw
	&& bp->pc == addr
	&& bp->inserted)
      return 1;

  return 0;
}

/* See mem-break.h.  */

int
hardware_breakpoint_inserted_here (CORE_ADDR addr)
{
  struct process_info *proc = current_process ();
  struct raw_breakpoint *bp;

  for (bp = proc->raw_breakpoints; bp != NULL; bp = bp->next)
    if (bp->raw_type == raw_bkpt_type_hw
	&& bp->pc == addr
	&& bp->inserted)
      return 1;

  return 0;
}

static int
validate_inserted_breakpoint (struct raw_breakpoint *bp)
{
  unsigned char *buf;
  int err;

  gdb_assert (bp->inserted);
  gdb_assert (bp->raw_type == raw_bkpt_type_sw);

  buf = alloca (breakpoint_len);
  err = (*the_target->read_memory) (bp->pc, buf, breakpoint_len);
  if (err || memcmp (buf, breakpoint_data, breakpoint_len) != 0)
    {
      /* Tag it as gone.  */
      bp->inserted = -1;
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
      if (bp->raw->inserted < 0)
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
      struct raw_breakpoint *raw = bp->raw;

      if (raw->raw_type == raw_bkpt_type_sw && raw->inserted > 0)
	validate_inserted_breakpoint (raw);
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

      if (bp->raw_type != raw_bkpt_type_sw)
	continue;

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

      if (bp->inserted > 0)
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

      if (bp->raw_type != raw_bkpt_type_sw)
	continue;

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
      if (bp->inserted > 0)
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

/* Clone an agent expression.  */

static struct agent_expr *
clone_agent_expr (const struct agent_expr *src_ax)
{
  struct agent_expr *ax;

  ax = xcalloc (1, sizeof (*ax));
  ax->length = src_ax->length;
  ax->bytes = xcalloc (ax->length, 1);
  memcpy (ax->bytes, src_ax->bytes, ax->length);
  return ax;
}

/* Deep-copy the contents of one breakpoint to another.  */

static struct breakpoint *
clone_one_breakpoint (const struct breakpoint *src)
{
  struct breakpoint *dest;
  struct raw_breakpoint *dest_raw;
  struct point_cond_list *current_cond;
  struct point_cond_list *new_cond;
  struct point_cond_list *cond_tail = NULL;
  struct point_command_list *current_cmd;
  struct point_command_list *new_cmd;
  struct point_command_list *cmd_tail = NULL;

  /* Clone the raw breakpoint.  */
  dest_raw = xcalloc (1, sizeof (*dest_raw));
  dest_raw->raw_type = src->raw->raw_type;
  dest_raw->refcount = src->raw->refcount;
  dest_raw->pc = src->raw->pc;
  dest_raw->size = src->raw->size;
  memcpy (dest_raw->old_data, src->raw->old_data, MAX_BREAKPOINT_LEN);
  dest_raw->inserted = src->raw->inserted;

  /* Clone the high-level breakpoint.  */
  dest = xcalloc (1, sizeof (*dest));
  dest->type = src->type;
  dest->raw = dest_raw;
  dest->handler = src->handler;

  /* Clone the condition list.  */
  for (current_cond = src->cond_list; current_cond != NULL;
       current_cond = current_cond->next)
    {
      new_cond = xcalloc (1, sizeof (*new_cond));
      new_cond->cond = clone_agent_expr (current_cond->cond);
      APPEND_TO_LIST (&dest->cond_list, new_cond, cond_tail);
    }

  /* Clone the command list.  */
  for (current_cmd = src->command_list; current_cmd != NULL;
       current_cmd = current_cmd->next)
    {
      new_cmd = xcalloc (1, sizeof (*new_cmd));
      new_cmd->cmd = clone_agent_expr (current_cmd->cmd);
      new_cmd->persistence = current_cmd->persistence;
      APPEND_TO_LIST (&dest->command_list, new_cmd, cmd_tail);
    }

  return dest;
}

/* Create a new breakpoint list NEW_LIST that is a copy of the
   list starting at SRC_LIST.  Create the corresponding new
   raw_breakpoint list NEW_RAW_LIST as well.  */

void
clone_all_breakpoints (struct breakpoint **new_list,
		       struct raw_breakpoint **new_raw_list,
		       const struct breakpoint *src_list)
{
  const struct breakpoint *bp;
  struct breakpoint *new_bkpt;
  struct breakpoint *bkpt_tail = NULL;
  struct raw_breakpoint *raw_bkpt_tail = NULL;

  for (bp = src_list; bp != NULL; bp = bp->next)
    {
      new_bkpt = clone_one_breakpoint (bp);
      APPEND_TO_LIST (new_list, new_bkpt, bkpt_tail);
      APPEND_TO_LIST (new_raw_list, new_bkpt->raw, raw_bkpt_tail);
    }
}
