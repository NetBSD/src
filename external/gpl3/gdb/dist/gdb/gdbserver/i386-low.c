/* Debug register code for the i386.

   Copyright (C) 2009-2013 Free Software Foundation, Inc.

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
#include "target.h"
#include "i386-low.h"

/* Support for 8-byte wide hw watchpoints.  */
#ifndef TARGET_HAS_DR_LEN_8
/* NOTE: sizeof (long) == 4 on win64.  */
#define TARGET_HAS_DR_LEN_8 (sizeof (void *) == 8)
#endif

enum target_hw_bp_type
  {
    hw_write   = 0,	/* Common  HW watchpoint */
    hw_read    = 1,	/* Read    HW watchpoint */
    hw_access  = 2,	/* Access  HW watchpoint */
    hw_execute = 3	/* Execute HW breakpoint */
  };

/* DR7 Debug Control register fields.  */

/* How many bits to skip in DR7 to get to R/W and LEN fields.  */
#define DR_CONTROL_SHIFT	16
/* How many bits in DR7 per R/W and LEN field for each watchpoint.  */
#define DR_CONTROL_SIZE		4

/* Watchpoint/breakpoint read/write fields in DR7.  */
#define DR_RW_EXECUTE	(0x0)	/* Break on instruction execution.  */
#define DR_RW_WRITE	(0x1)	/* Break on data writes.  */
#define DR_RW_READ	(0x3)	/* Break on data reads or writes.  */

/* This is here for completeness.  No platform supports this
   functionality yet (as of March 2001).  Note that the DE flag in the
   CR4 register needs to be set to support this.  */
#ifndef DR_RW_IORW
#define DR_RW_IORW	(0x2)	/* Break on I/O reads or writes.  */
#endif

/* Watchpoint/breakpoint length fields in DR7.  The 2-bit left shift
   is so we could OR this with the read/write field defined above.  */
#define DR_LEN_1	(0x0 << 2) /* 1-byte region watch or breakpoint.  */
#define DR_LEN_2	(0x1 << 2) /* 2-byte region watch.  */
#define DR_LEN_4	(0x3 << 2) /* 4-byte region watch.  */
#define DR_LEN_8	(0x2 << 2) /* 8-byte region watch (AMD64).  */

/* Local and Global Enable flags in DR7.

   When the Local Enable flag is set, the breakpoint/watchpoint is
   enabled only for the current task; the processor automatically
   clears this flag on every task switch.  When the Global Enable flag
   is set, the breakpoint/watchpoint is enabled for all tasks; the
   processor never clears this flag.

   Currently, all watchpoint are locally enabled.  If you need to
   enable them globally, read the comment which pertains to this in
   i386_insert_aligned_watchpoint below.  */
#define DR_LOCAL_ENABLE_SHIFT	0 /* Extra shift to the local enable bit.  */
#define DR_GLOBAL_ENABLE_SHIFT	1 /* Extra shift to the global enable bit.  */
#define DR_ENABLE_SIZE		2 /* Two enable bits per debug register.  */

/* Local and global exact breakpoint enable flags (a.k.a. slowdown
   flags).  These are only required on i386, to allow detection of the
   exact instruction which caused a watchpoint to break; i486 and
   later processors do that automatically.  We set these flags for
   backwards compatibility.  */
#define DR_LOCAL_SLOWDOWN	(0x100)
#define DR_GLOBAL_SLOWDOWN	(0x200)

/* Fields reserved by Intel.  This includes the GD (General Detect
   Enable) flag, which causes a debug exception to be generated when a
   MOV instruction accesses one of the debug registers.

   FIXME: My Intel manual says we should use 0xF800, not 0xFC00.  */
#define DR_CONTROL_RESERVED	(0xFC00)

/* Auxiliary helper macros.  */

/* A value that masks all fields in DR7 that are reserved by Intel.  */
#define I386_DR_CONTROL_MASK	(~DR_CONTROL_RESERVED)

/* The I'th debug register is vacant if its Local and Global Enable
   bits are reset in the Debug Control register.  */
#define I386_DR_VACANT(state, i) \
  (((state)->dr_control_mirror & (3 << (DR_ENABLE_SIZE * (i)))) == 0)

/* Locally enable the break/watchpoint in the I'th debug register.  */
#define I386_DR_LOCAL_ENABLE(state, i) \
  do { \
    (state)->dr_control_mirror |= \
      (1 << (DR_LOCAL_ENABLE_SHIFT + DR_ENABLE_SIZE * (i))); \
  } while (0)

/* Globally enable the break/watchpoint in the I'th debug register.  */
#define I386_DR_GLOBAL_ENABLE(state, i) \
  do { \
    (state)->dr_control_mirror |= \
      (1 << (DR_GLOBAL_ENABLE_SHIFT + DR_ENABLE_SIZE * (i))); \
  } while (0)

/* Disable the break/watchpoint in the I'th debug register.  */
#define I386_DR_DISABLE(state, i) \
  do { \
    (state)->dr_control_mirror &= \
      ~(3 << (DR_ENABLE_SIZE * (i))); \
  } while (0)

/* Set in DR7 the RW and LEN fields for the I'th debug register.  */
#define I386_DR_SET_RW_LEN(state, i,rwlen) \
  do { \
    (state)->dr_control_mirror &= \
      ~(0x0f << (DR_CONTROL_SHIFT + DR_CONTROL_SIZE * (i))); \
    (state)->dr_control_mirror |= \
      ((rwlen) << (DR_CONTROL_SHIFT + DR_CONTROL_SIZE * (i))); \
  } while (0)

/* Get from DR7 the RW and LEN fields for the I'th debug register.  */
#define I386_DR_GET_RW_LEN(dr7, i) \
  (((dr7) \
    >> (DR_CONTROL_SHIFT + DR_CONTROL_SIZE * (i))) & 0x0f)

/* Did the watchpoint whose address is in the I'th register break?  */
#define I386_DR_WATCH_HIT(dr6, i) ((dr6) & (1 << (i)))

/* A macro to loop over all debug registers.  */
#define ALL_DEBUG_REGISTERS(i)	for (i = 0; i < DR_NADDR; i++)

/* Types of operations supported by i386_handle_nonaligned_watchpoint.  */
typedef enum { WP_INSERT, WP_REMOVE, WP_COUNT } i386_wp_op_t;

/* Implementation.  */

/* Clear the reference counts and forget everything we knew about the
   debug registers.  */

void
i386_low_init_dregs (struct i386_debug_reg_state *state)
{
  int i;

  ALL_DEBUG_REGISTERS (i)
    {
      state->dr_mirror[i] = 0;
      state->dr_ref_count[i] = 0;
    }
  state->dr_control_mirror = 0;
  state->dr_status_mirror  = 0;
}

/* Print the values of the mirrored debug registers.  This is enabled via
   the "set debug-hw-points 1" monitor command.  */

static void
i386_show_dr (struct i386_debug_reg_state *state,
	      const char *func, CORE_ADDR addr,
	      int len, enum target_hw_bp_type type)
{
  int i;

  fprintf (stderr, "%s", func);
  if (addr || len)
    fprintf (stderr, " (addr=%lx, len=%d, type=%s)",
	     (unsigned long) addr, len,
	     type == hw_write ? "data-write"
	     : (type == hw_read ? "data-read"
		: (type == hw_access ? "data-read/write"
		   : (type == hw_execute ? "instruction-execute"
		      /* FIXME: if/when I/O read/write
			 watchpoints are supported, add them
			 here.  */
		      : "??unknown??"))));
  fprintf (stderr, ":\n");
  fprintf (stderr, "\tCONTROL (DR7): %08x          STATUS (DR6): %08x\n",
	   state->dr_control_mirror, state->dr_status_mirror);
  ALL_DEBUG_REGISTERS (i)
    {
      fprintf (stderr, "\
\tDR%d: addr=0x%s, ref.count=%d  DR%d: addr=0x%s, ref.count=%d\n",
	      i, paddress (state->dr_mirror[i]),
	      state->dr_ref_count[i],
	      i + 1, paddress (state->dr_mirror[i + 1]),
	      state->dr_ref_count[i + 1]);
      i++;
    }
}

/* Return the value of a 4-bit field for DR7 suitable for watching a
   region of LEN bytes for accesses of type TYPE.  LEN is assumed to
   have the value of 1, 2, or 4.  */

static unsigned
i386_length_and_rw_bits (int len, enum target_hw_bp_type type)
{
  unsigned rw;

  switch (type)
    {
      case hw_execute:
	rw = DR_RW_EXECUTE;
	break;
      case hw_write:
	rw = DR_RW_WRITE;
	break;
      case hw_read:
	fatal ("The i386 doesn't support data-read watchpoints.\n");
      case hw_access:
	rw = DR_RW_READ;
	break;
#if 0
	/* Not yet supported.  */
      case hw_io_access:
	rw = DR_RW_IORW;
	break;
#endif
      default:
	error ("\
Invalid hardware breakpoint type %d in i386_length_and_rw_bits.\n",
			(int) type);
    }

  switch (len)
    {
      case 1:
	return (DR_LEN_1 | rw);
      case 2:
	return (DR_LEN_2 | rw);
      case 4:
	return (DR_LEN_4 | rw);
	/* ELSE FALL THROUGH */
      case 8:
        if (TARGET_HAS_DR_LEN_8)
 	  return (DR_LEN_8 | rw);
      default:
	error ("\
Invalid hardware breakpoint length %d in i386_length_and_rw_bits.\n", len);
    }
}

/* Insert a watchpoint at address ADDR, which is assumed to be aligned
   according to the length of the region to watch.  LEN_RW_BITS is the
   value of the bits from DR7 which describes the length and access
   type of the region to be watched by this watchpoint.  Return 0 on
   success, -1 on failure.  */

static int
i386_insert_aligned_watchpoint (struct i386_debug_reg_state *state,
				CORE_ADDR addr, unsigned len_rw_bits)
{
  int i;

  /* First, look for an occupied debug register with the same address
     and the same RW and LEN definitions.  If we find one, we can
     reuse it for this watchpoint as well (and save a register).  */
  ALL_DEBUG_REGISTERS (i)
    {
      if (!I386_DR_VACANT (state, i)
	  && state->dr_mirror[i] == addr
	  && I386_DR_GET_RW_LEN (state->dr_control_mirror, i) == len_rw_bits)
	{
	  state->dr_ref_count[i]++;
	  return 0;
	}
    }

  /* Next, look for a vacant debug register.  */
  ALL_DEBUG_REGISTERS (i)
    {
      if (I386_DR_VACANT (state, i))
	break;
    }

  /* No more debug registers!  */
  if (i >= DR_NADDR)
    return -1;

  /* Now set up the register I to watch our region.  */

  /* Record the info in our local mirrored array.  */
  state->dr_mirror[i] = addr;
  state->dr_ref_count[i] = 1;
  I386_DR_SET_RW_LEN (state, i, len_rw_bits);
  /* Note: we only enable the watchpoint locally, i.e. in the current
     task.  Currently, no i386 target allows or supports global
     watchpoints; however, if any target would want that in the
     future, GDB should probably provide a command to control whether
     to enable watchpoints globally or locally, and the code below
     should use global or local enable and slow-down flags as
     appropriate.  */
  I386_DR_LOCAL_ENABLE (state, i);
  state->dr_control_mirror |= DR_LOCAL_SLOWDOWN;
  state->dr_control_mirror &= I386_DR_CONTROL_MASK;

  return 0;
}

/* Remove a watchpoint at address ADDR, which is assumed to be aligned
   according to the length of the region to watch.  LEN_RW_BITS is the
   value of the bits from DR7 which describes the length and access
   type of the region watched by this watchpoint.  Return 0 on
   success, -1 on failure.  */

static int
i386_remove_aligned_watchpoint (struct i386_debug_reg_state *state,
				CORE_ADDR addr, unsigned len_rw_bits)
{
  int i, retval = -1;

  ALL_DEBUG_REGISTERS (i)
    {
      if (!I386_DR_VACANT (state, i)
	  && state->dr_mirror[i] == addr
	  && I386_DR_GET_RW_LEN (state->dr_control_mirror, i) == len_rw_bits)
	{
	  if (--state->dr_ref_count[i] == 0) /* No longer in use?  */
	    {
	      /* Reset our mirror.  */
	      state->dr_mirror[i] = 0;
	      I386_DR_DISABLE (state, i);
	    }
	  retval = 0;
	}
    }

  return retval;
}

/* Insert or remove a (possibly non-aligned) watchpoint, or count the
   number of debug registers required to watch a region at address
   ADDR whose length is LEN for accesses of type TYPE.  Return 0 on
   successful insertion or removal, a positive number when queried
   about the number of registers, or -1 on failure.  If WHAT is not a
   valid value, bombs through internal_error.  */

static int
i386_handle_nonaligned_watchpoint (struct i386_debug_reg_state *state,
				   i386_wp_op_t what, CORE_ADDR addr, int len,
				   enum target_hw_bp_type type)
{
  int retval = 0;
  int max_wp_len = TARGET_HAS_DR_LEN_8 ? 8 : 4;

  static const int size_try_array[8][8] =
  {
    {1, 1, 1, 1, 1, 1, 1, 1},	/* Trying size one.  */
    {2, 1, 2, 1, 2, 1, 2, 1},	/* Trying size two.  */
    {2, 1, 2, 1, 2, 1, 2, 1},	/* Trying size three.  */
    {4, 1, 2, 1, 4, 1, 2, 1},	/* Trying size four.  */
    {4, 1, 2, 1, 4, 1, 2, 1},	/* Trying size five.  */
    {4, 1, 2, 1, 4, 1, 2, 1},	/* Trying size six.  */
    {4, 1, 2, 1, 4, 1, 2, 1},	/* Trying size seven.  */
    {8, 1, 2, 1, 4, 1, 2, 1},	/* Trying size eight.  */
  };

  while (len > 0)
    {
      int align = addr % max_wp_len;
      /* Four (eight on AMD64) is the maximum length a debug register
	 can watch.  */
      int try = (len > max_wp_len ? (max_wp_len - 1) : len - 1);
      int size = size_try_array[try][align];

      if (what == WP_COUNT)
	{
	  /* size_try_array[] is defined such that each iteration
	     through the loop is guaranteed to produce an address and a
	     size that can be watched with a single debug register.
	     Thus, for counting the registers required to watch a
	     region, we simply need to increment the count on each
	     iteration.  */
	  retval++;
	}
      else
	{
	  unsigned len_rw = i386_length_and_rw_bits (size, type);

	  if (what == WP_INSERT)
	    retval = i386_insert_aligned_watchpoint (state, addr, len_rw);
	  else if (what == WP_REMOVE)
	    retval = i386_remove_aligned_watchpoint (state, addr, len_rw);
	  else
	    fatal ("\
Invalid value %d of operation in i386_handle_nonaligned_watchpoint.\n",
		   (int) what);

	  if (retval)
	    break;
	}

      addr += size;
      len -= size;
    }

  return retval;
}

#define Z_PACKET_HW_BP '1'
#define Z_PACKET_WRITE_WP '2'
#define Z_PACKET_READ_WP '3'
#define Z_PACKET_ACCESS_WP '4'

/* Map the protocol watchpoint type TYPE to enum target_hw_bp_type.  */

static enum target_hw_bp_type
Z_packet_to_hw_type (char type)
{
  switch (type)
    {
    case Z_PACKET_HW_BP:
      return hw_execute;
    case Z_PACKET_WRITE_WP:
      return hw_write;
    case Z_PACKET_READ_WP:
      return hw_read;
    case Z_PACKET_ACCESS_WP:
      return hw_access;
    default:
      fatal ("Z_packet_to_hw_type: bad watchpoint type %c", type);
    }
}

/* Update the inferior debug registers state, in INF_STATE, with the
   new debug registers state, in NEW_STATE.  */

static void
i386_update_inferior_debug_regs (struct i386_debug_reg_state *inf_state,
				 struct i386_debug_reg_state *new_state)
{
  int i;

  ALL_DEBUG_REGISTERS (i)
    {
      if (I386_DR_VACANT (new_state, i) != I386_DR_VACANT (inf_state, i))
	i386_dr_low_set_addr (new_state, i);
      else
	gdb_assert (new_state->dr_mirror[i] == inf_state->dr_mirror[i]);
    }

  if (new_state->dr_control_mirror != inf_state->dr_control_mirror)
    i386_dr_low_set_control (new_state);

  *inf_state = *new_state;
}

/* Insert a watchpoint to watch a memory region which starts at
   address ADDR and whose length is LEN bytes.  Watch memory accesses
   of the type TYPE_FROM_PACKET.  Return 0 on success, -1 on failure.  */

int
i386_low_insert_watchpoint (struct i386_debug_reg_state *state,
			    char type_from_packet, CORE_ADDR addr, int len)
{
  int retval;
  enum target_hw_bp_type type = Z_packet_to_hw_type (type_from_packet);
  /* Work on a local copy of the debug registers, and on success,
     commit the change back to the inferior.  */
  struct i386_debug_reg_state local_state = *state;

  if (type == hw_read)
    return 1; /* unsupported */

  if (((len != 1 && len != 2 && len != 4)
       && !(TARGET_HAS_DR_LEN_8 && len == 8))
      || addr % len != 0)
    {
      retval = i386_handle_nonaligned_watchpoint (&local_state, WP_INSERT,
						  addr, len, type);
    }
  else
    {
      unsigned len_rw = i386_length_and_rw_bits (len, type);

      retval = i386_insert_aligned_watchpoint (&local_state, addr, len_rw);
    }

  if (retval == 0)
    i386_update_inferior_debug_regs (state, &local_state);

  if (debug_hw_points)
    i386_show_dr (state, "insert_watchpoint", addr, len, type);

  return retval;
}

/* Remove a watchpoint that watched the memory region which starts at
   address ADDR, whose length is LEN bytes, and for accesses of the
   type TYPE_FROM_PACKET.  Return 0 on success, -1 on failure.  */

int
i386_low_remove_watchpoint (struct i386_debug_reg_state *state,
			    char type_from_packet, CORE_ADDR addr, int len)
{
  int retval;
  enum target_hw_bp_type type = Z_packet_to_hw_type (type_from_packet);
  /* Work on a local copy of the debug registers, and on success,
     commit the change back to the inferior.  */
  struct i386_debug_reg_state local_state = *state;

  if (((len != 1 && len != 2 && len != 4)
       && !(TARGET_HAS_DR_LEN_8 && len == 8))
      || addr % len != 0)
    {
      retval = i386_handle_nonaligned_watchpoint (&local_state, WP_REMOVE,
						  addr, len, type);
    }
  else
    {
      unsigned len_rw = i386_length_and_rw_bits (len, type);

      retval = i386_remove_aligned_watchpoint (&local_state, addr, len_rw);
    }

  if (retval == 0)
    i386_update_inferior_debug_regs (state, &local_state);

  if (debug_hw_points)
    i386_show_dr (state, "remove_watchpoint", addr, len, type);

  return retval;
}

/* Return non-zero if we can watch a memory region that starts at
   address ADDR and whose length is LEN bytes.  */

int
i386_low_region_ok_for_watchpoint (struct i386_debug_reg_state *state,
				   CORE_ADDR addr, int len)
{
  int nregs;

  /* Compute how many aligned watchpoints we would need to cover this
     region.  */
  nregs = i386_handle_nonaligned_watchpoint (state, WP_COUNT,
					     addr, len, hw_write);
  return nregs <= DR_NADDR ? 1 : 0;
}

/* If the inferior has some break/watchpoint that triggered, set the
   address associated with that break/watchpoint and return true.
   Otherwise, return false.  */

int
i386_low_stopped_data_address (struct i386_debug_reg_state *state,
			       CORE_ADDR *addr_p)
{
  CORE_ADDR addr = 0;
  int i;
  int rc = 0;
  /* The current thread's DR_STATUS.  We always need to read this to
     check whether some watchpoint caused the trap.  */
  unsigned status;
  /* We need DR_CONTROL as well, but only iff DR_STATUS indicates a
     data breakpoint trap.  Only fetch it when necessary, to avoid an
     unnecessary extra syscall when no watchpoint triggered.  */
  int control_p = 0;
  unsigned control = 0;

  /* In non-stop/async, threads can be running while we change the
     global dr_mirror (and friends).  Say, we set a watchpoint, and
     let threads resume.  Now, say you delete the watchpoint, or
     add/remove watchpoints such that dr_mirror changes while threads
     are running.  On targets that support non-stop,
     inserting/deleting watchpoints updates the global dr_mirror only.
     It does not update the real thread's debug registers; that's only
     done prior to resume.  Instead, if threads are running when the
     mirror changes, a temporary and transparent stop on all threads
     is forced so they can get their copy of the debug registers
     updated on re-resume.  Now, say, a thread hit a watchpoint before
     having been updated with the new dr_mirror contents, and we
     haven't yet handled the corresponding SIGTRAP.  If we trusted
     dr_mirror below, we'd mistake the real trapped address (from the
     last time we had updated debug registers in the thread) with
     whatever was currently in dr_mirror.  So to fix this, dr_mirror
     always represents intention, what we _want_ threads to have in
     debug registers.  To get at the address and cause of the trap, we
     need to read the state the thread still has in its debug
     registers.

     In sum, always get the current debug register values the current
     thread has, instead of trusting the global mirror.  If the thread
     was running when we last changed watchpoints, the mirror no
     longer represents what was set in this thread's debug
     registers.  */
  status = i386_dr_low_get_status ();

  ALL_DEBUG_REGISTERS (i)
    {
      if (!I386_DR_WATCH_HIT (status, i))
	continue;

      if (!control_p)
	{
	  control = i386_dr_low_get_control ();
	  control_p = 1;
	}

      /* This second condition makes sure DRi is set up for a data
	 watchpoint, not a hardware breakpoint.  The reason is that
	 GDB doesn't call the target_stopped_data_address method
	 except for data watchpoints.  In other words, I'm being
	 paranoiac.  */
      if (I386_DR_GET_RW_LEN (control, i) != 0)
	{
	  addr = i386_dr_low_get_addr (i);
	  rc = 1;
	  if (debug_hw_points)
	    i386_show_dr (state, "watchpoint_hit", addr, -1, hw_write);
	}
    }

  if (debug_hw_points && addr == 0)
    i386_show_dr (state, "stopped_data_addr", 0, 0, hw_write);

  if (rc)
    *addr_p = addr;
  return rc;
}

/* Return true if the inferior has some watchpoint that triggered.
   Otherwise return false.  */

int
i386_low_stopped_by_watchpoint (struct i386_debug_reg_state *state)
{
  CORE_ADDR addr = 0;
  return i386_low_stopped_data_address (state, &addr);
}
