/* Memory breakpoint operations for the remote server for GDB.
   Copyright (C) 2002, 2003, 2005, 2007, 2008, 2009, 2010, 2011
   Free Software Foundation, Inc.

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

/* A high level (in gdbserver's perspective) breakpoint.  */
struct breakpoint
{
  struct breakpoint *next;

  /* The breakpoint's type.  */
  enum bkpt_type type;

  /* Link to this breakpoint's raw breakpoint.  This is always
     non-NULL.  */
  struct raw_breakpoint *raw;

  /* Function to call when we hit this breakpoint.  If it returns 1,
     the breakpoint shall be deleted; 0 or if this callback is NULL,
     it will be left inserted.  */
  int (*handler) (CORE_ADDR);
};

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
  err = read_inferior_memory (where, bp->old_data, breakpoint_len);
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
	      ret = write_inferior_memory (bp->pc,
					   fast_tracepoint_jump_shadow (bp),
					   bp->length);
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

struct fast_tracepoint_jump *
set_fast_tracepoint_jump (CORE_ADDR where,
			  unsigned char *insn, ULONGEST length)
{
  struct process_info *proc = current_process ();
  struct fast_tracepoint_jump *jp;
  int err;

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

  /* Note that there can be trap breakpoints inserted in the same
     address range.  To access the original memory contents, we use
     `read_inferior_memory', which masks out breakpoints.  */
  err = read_inferior_memory (where,
			      fast_tracepoint_jump_shadow (jp), jp->length);
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
  err = write_inferior_memory (where, fast_tracepoint_jump_shadow (jp),
			       length);
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
      err = write_inferior_memory (jp->pc,
				   fast_tracepoint_jump_shadow (jp),
				   jp->length);
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
  err = write_inferior_memory (where,
			       fast_tracepoint_jump_shadow (jp), jp->length);
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
	      ret = write_inferior_memory (bp->pc, bp->old_data,
					   breakpoint_len);
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

static struct breakpoint *
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

  err = delete_breakpoint (bp);
  if (err)
    return -1;

  return 0;
}

int
gdb_breakpoint_here (CORE_ADDR where)
{
  struct breakpoint *bp = find_gdb_breakpoint_at (where);

  return (bp != NULL);
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

      bp->inserted = 0;
      /* Since there can be fast tracepoint jumps inserted in the same
	 address range, we use `write_inferior_memory', which takes
	 care of layering breakpoints on top of fast tracepoints, and
	 on top of the buffer we pass it.  This works because we've
	 already unlinked the fast tracepoint jump above.  Also note
	 that we need to pass the current shadow contents, because
	 write_inferior_memory updates any shadow memory with what we
	 pass here, and we want that to be a nop.  */
      err = write_inferior_memory (bp->pc, bp->old_data,
				   breakpoint_len);
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
check_mem_write (CORE_ADDR mem_addr, unsigned char *buf, int mem_len)
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
	      buf + buf_offset, copy_len);
      if (jp->inserted)
	memcpy (buf + buf_offset,
		fast_tracepoint_jump_insn (jp) + copy_offset, copy_len);
    }

  for (; bp != NULL; bp = bp->next)
    {
      CORE_ADDR bp_end = bp->pc + breakpoint_len;
      CORE_ADDR start, end;
      int copy_offset, copy_len, buf_offset;

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

      memcpy (bp->old_data + copy_offset, buf + buf_offset, copy_len);
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
