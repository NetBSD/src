/* Native-dependent code for the i386.

   Copyright (C) 2001, 2004, 2005, 2007, 2008, 2009, 2010, 2011
   Free Software Foundation, Inc.

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

#include "i386-nat.h"
#include "defs.h"
#include "breakpoint.h"
#include "command.h"
#include "gdbcmd.h"
#include "target.h"
#include "gdb_assert.h"

/* Support for hardware watchpoints and breakpoints using the i386
   debug registers.

   This provides several functions for inserting and removing
   hardware-assisted breakpoints and watchpoints, testing if one or
   more of the watchpoints triggered and at what address, checking
   whether a given region can be watched, etc.

   The functions below implement debug registers sharing by reference
   counts, and allow to watch regions up to 16 bytes long.  */

struct i386_dr_low_type i386_dr_low;


/* Support for 8-byte wide hw watchpoints.  */
#define TARGET_HAS_DR_LEN_8 (i386_dr_low.debug_register_length == 8)

/* Debug registers' indices.  */
#define DR_NADDR	4	/* The number of debug address registers.  */
#define DR_STATUS	6	/* Index of debug status register (DR6).  */
#define DR_CONTROL	7	/* Index of debug control register (DR7).  */

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
#define DR_GLOBAL_SLOWDOWN     	(0x200)

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
#define I386_DR_VACANT(i) \
  ((dr_control_mirror & (3 << (DR_ENABLE_SIZE * (i)))) == 0)

/* Locally enable the break/watchpoint in the I'th debug register.  */
#define I386_DR_LOCAL_ENABLE(i) \
  dr_control_mirror |= (1 << (DR_LOCAL_ENABLE_SHIFT + DR_ENABLE_SIZE * (i)))

/* Globally enable the break/watchpoint in the I'th debug register.  */
#define I386_DR_GLOBAL_ENABLE(i) \
  dr_control_mirror |= (1 << (DR_GLOBAL_ENABLE_SHIFT + DR_ENABLE_SIZE * (i)))

/* Disable the break/watchpoint in the I'th debug register.  */
#define I386_DR_DISABLE(i) \
  dr_control_mirror &= ~(3 << (DR_ENABLE_SIZE * (i)))

/* Set in DR7 the RW and LEN fields for the I'th debug register.  */
#define I386_DR_SET_RW_LEN(i,rwlen) \
  do { \
    dr_control_mirror &= ~(0x0f << (DR_CONTROL_SHIFT+DR_CONTROL_SIZE*(i)));   \
    dr_control_mirror |= ((rwlen) << (DR_CONTROL_SHIFT+DR_CONTROL_SIZE*(i))); \
  } while (0)

/* Get from DR7 the RW and LEN fields for the I'th debug register.  */
#define I386_DR_GET_RW_LEN(i) \
  ((dr_control_mirror >> (DR_CONTROL_SHIFT + DR_CONTROL_SIZE * (i))) & 0x0f)

/* Mask that this I'th watchpoint has triggered.  */
#define I386_DR_WATCH_MASK(i)	(1 << (i))

/* Did the watchpoint whose address is in the I'th register break?  */
#define I386_DR_WATCH_HIT(i)	(dr_status_mirror & I386_DR_WATCH_MASK (i))

/* A macro to loop over all debug registers.  */
#define ALL_DEBUG_REGISTERS(i)	for (i = 0; i < DR_NADDR; i++)

/* Mirror the inferior's DRi registers.  We keep the status and
   control registers separated because they don't hold addresses.  */
static CORE_ADDR dr_mirror[DR_NADDR];
static unsigned long dr_status_mirror, dr_control_mirror;

/* Reference counts for each debug register.  */
static int dr_ref_count[DR_NADDR];

/* Whether or not to print the mirrored debug registers.  */
static int maint_show_dr;

/* Types of operations supported by i386_handle_nonaligned_watchpoint.  */
typedef enum { WP_INSERT, WP_REMOVE, WP_COUNT } i386_wp_op_t;

/* Internal functions.  */

/* Return the value of a 4-bit field for DR7 suitable for watching a
   region of LEN bytes for accesses of type TYPE.  LEN is assumed to
   have the value of 1, 2, or 4.  */
static unsigned i386_length_and_rw_bits (int len, enum target_hw_bp_type type);

/* Insert a watchpoint at address ADDR, which is assumed to be aligned
   according to the length of the region to watch.  LEN_RW_BITS is the
   value of the bit-field from DR7 which describes the length and
   access type of the region to be watched by this watchpoint.  Return
   0 on success, -1 on failure.  */
static int i386_insert_aligned_watchpoint (CORE_ADDR addr,
					   unsigned len_rw_bits);

/* Remove a watchpoint at address ADDR, which is assumed to be aligned
   according to the length of the region to watch.  LEN_RW_BITS is the
   value of the bits from DR7 which describes the length and access
   type of the region watched by this watchpoint.  Return 0 on
   success, -1 on failure.  */
static int i386_remove_aligned_watchpoint (CORE_ADDR addr,
					   unsigned len_rw_bits);

/* Insert or remove a (possibly non-aligned) watchpoint, or count the
   number of debug registers required to watch a region at address
   ADDR whose length is LEN for accesses of type TYPE.  Return 0 on
   successful insertion or removal, a positive number when queried
   about the number of registers, or -1 on failure.  If WHAT is not a
   valid value, bombs through internal_error.  */
static int i386_handle_nonaligned_watchpoint (i386_wp_op_t what,
					      CORE_ADDR addr, int len,
					      enum target_hw_bp_type type);

/* Implementation.  */

/* Clear the reference counts and forget everything we knew about the
   debug registers.  */

void
i386_cleanup_dregs (void)
{
  int i;

  ALL_DEBUG_REGISTERS(i)
    {
      dr_mirror[i] = 0;
      dr_ref_count[i] = 0;
    }
  dr_control_mirror = 0;
  dr_status_mirror  = 0;
}

/* Print the values of the mirrored debug registers.  This is called
   when maint_show_dr is non-zero.  To set that up, type "maint
   show-debug-regs" at GDB's prompt.  */

static void
i386_show_dr (const char *func, CORE_ADDR addr,
	      int len, enum target_hw_bp_type type)
{
  int addr_size = gdbarch_addr_bit (target_gdbarch) / 8;
  int i;

  puts_unfiltered (func);
  if (addr || len)
    printf_unfiltered (" (addr=%lx, len=%d, type=%s)",
		       /* This code is for ia32, so casting CORE_ADDR
			  to unsigned long should be okay.  */
		       (unsigned long)addr, len,
		       type == hw_write ? "data-write"
		       : (type == hw_read ? "data-read"
			  : (type == hw_access ? "data-read/write"
			     : (type == hw_execute ? "instruction-execute"
				/* FIXME: if/when I/O read/write
				   watchpoints are supported, add them
				   here.  */
				: "??unknown??"))));
  puts_unfiltered (":\n");
  printf_unfiltered ("\tCONTROL (DR7): %s          STATUS (DR6): %s\n",
		     phex (dr_control_mirror, 8), phex (dr_status_mirror, 8));
  ALL_DEBUG_REGISTERS(i)
    {
      printf_unfiltered ("\
\tDR%d: addr=0x%s, ref.count=%d  DR%d: addr=0x%s, ref.count=%d\n",
		 i, phex (dr_mirror[i], addr_size), dr_ref_count[i],
		 i+1, phex (dr_mirror[i+1], addr_size), dr_ref_count[i+1]);
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
	internal_error (__FILE__, __LINE__,
			_("The i386 doesn't support "
			  "data-read watchpoints.\n"));
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
	internal_error (__FILE__, __LINE__, _("\
Invalid hardware breakpoint type %d in i386_length_and_rw_bits.\n"),
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
      case 8:
        if (TARGET_HAS_DR_LEN_8)
 	  return (DR_LEN_8 | rw);
	/* ELSE FALL THROUGH */
      default:
	internal_error (__FILE__, __LINE__, _("\
Invalid hardware breakpoint length %d in i386_length_and_rw_bits.\n"), len);
    }
}

/* Insert a watchpoint at address ADDR, which is assumed to be aligned
   according to the length of the region to watch.  LEN_RW_BITS is the
   value of the bits from DR7 which describes the length and access
   type of the region to be watched by this watchpoint.  Return 0 on
   success, -1 on failure.  */

static int
i386_insert_aligned_watchpoint (CORE_ADDR addr, unsigned len_rw_bits)
{
  int i;

  if (!i386_dr_low.set_addr || !i386_dr_low.set_control)
    return -1;

  /* First, look for an occupied debug register with the same address
     and the same RW and LEN definitions.  If we find one, we can
     reuse it for this watchpoint as well (and save a register).  */
  ALL_DEBUG_REGISTERS(i)
    {
      if (!I386_DR_VACANT (i)
	  && dr_mirror[i] == addr
	  && I386_DR_GET_RW_LEN (i) == len_rw_bits)
	{
	  dr_ref_count[i]++;
	  return 0;
	}
    }

  /* Next, look for a vacant debug register.  */
  ALL_DEBUG_REGISTERS(i)
    {
      if (I386_DR_VACANT (i))
	break;
    }

  /* No more debug registers!  */
  if (i >= DR_NADDR)
    return -1;

  /* Now set up the register I to watch our region.  */

  /* Record the info in our local mirrored array.  */
  dr_mirror[i] = addr;
  dr_ref_count[i] = 1;
  I386_DR_SET_RW_LEN (i, len_rw_bits);
  /* Note: we only enable the watchpoint locally, i.e. in the current
     task.  Currently, no i386 target allows or supports global
     watchpoints; however, if any target would want that in the
     future, GDB should probably provide a command to control whether
     to enable watchpoints globally or locally, and the code below
     should use global or local enable and slow-down flags as
     appropriate.  */
  I386_DR_LOCAL_ENABLE (i);
  dr_control_mirror |= DR_LOCAL_SLOWDOWN;
  dr_control_mirror &= I386_DR_CONTROL_MASK;

  /* Finally, actually pass the info to the inferior.  */
  i386_dr_low.set_addr (i, addr);
  i386_dr_low.set_control (dr_control_mirror);

  /* Only a sanity check for leftover bits (set possibly only by inferior).  */
  if (i386_dr_low.unset_status)
    i386_dr_low.unset_status (I386_DR_WATCH_MASK (i));

  return 0;
}

/* Remove a watchpoint at address ADDR, which is assumed to be aligned
   according to the length of the region to watch.  LEN_RW_BITS is the
   value of the bits from DR7 which describes the length and access
   type of the region watched by this watchpoint.  Return 0 on
   success, -1 on failure.  */

static int
i386_remove_aligned_watchpoint (CORE_ADDR addr, unsigned len_rw_bits)
{
  int i, retval = -1;

  ALL_DEBUG_REGISTERS(i)
    {
      if (!I386_DR_VACANT (i)
	  && dr_mirror[i] == addr
	  && I386_DR_GET_RW_LEN (i) == len_rw_bits)
	{
	  if (--dr_ref_count[i] == 0) /* no longer in use?  */
	    {
	      /* Reset our mirror.  */
	      dr_mirror[i] = 0;
	      I386_DR_DISABLE (i);
	      /* Reset it in the inferior.  */
	      i386_dr_low.set_control (dr_control_mirror);
	      if (i386_dr_low.reset_addr)
		i386_dr_low.reset_addr (i);
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
i386_handle_nonaligned_watchpoint (i386_wp_op_t what, CORE_ADDR addr, int len,
				   enum target_hw_bp_type type)
{
  int retval = 0, status = 0;
  int max_wp_len = TARGET_HAS_DR_LEN_8 ? 8 : 4;

  static int size_try_array[8][8] =
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
	    status = i386_insert_aligned_watchpoint (addr, len_rw);
	  else if (what == WP_REMOVE)
	    status = i386_remove_aligned_watchpoint (addr, len_rw);
	  else
	    internal_error (__FILE__, __LINE__, _("\
Invalid value %d of operation in i386_handle_nonaligned_watchpoint.\n"),
			    (int)what);
	  /* We keep the loop going even after a failure, because some
	     of the other aligned watchpoints might still succeed
	     (e.g. if they watch addresses that are already watched,
	     in which case we just increment the reference counts of
	     occupied debug registers).  If we break out of the loop
	     too early, we could cause those addresses watched by
	     other watchpoints to be disabled when breakpoint.c reacts
	     to our failure to insert this watchpoint and tries to
	     remove it.  */
	  if (status)
	    retval = status;
	}

      addr += size;
      len -= size;
    }

  return retval;
}

/* Insert a watchpoint to watch a memory region which starts at
   address ADDR and whose length is LEN bytes.  Watch memory accesses
   of the type TYPE.  Return 0 on success, -1 on failure.  */

static int
i386_insert_watchpoint (CORE_ADDR addr, int len, int type,
			struct expression *cond)
{
  int retval;

  if (type == hw_read)
    return 1; /* unsupported */

  if (((len != 1 && len !=2 && len !=4) && !(TARGET_HAS_DR_LEN_8 && len == 8))
      || addr % len != 0)
    retval = i386_handle_nonaligned_watchpoint (WP_INSERT, addr, len, type);
  else
    {
      unsigned len_rw = i386_length_and_rw_bits (len, type);

      retval = i386_insert_aligned_watchpoint (addr, len_rw);
    }

  if (maint_show_dr)
    i386_show_dr ("insert_watchpoint", addr, len, type);

  return retval;
}

/* Remove a watchpoint that watched the memory region which starts at
   address ADDR, whose length is LEN bytes, and for accesses of the
   type TYPE.  Return 0 on success, -1 on failure.  */
static int
i386_remove_watchpoint (CORE_ADDR addr, int len, int type,
			struct expression *cond)
{
  int retval;

  if (((len != 1 && len !=2 && len !=4) && !(TARGET_HAS_DR_LEN_8 && len == 8))
      || addr % len != 0)
    retval = i386_handle_nonaligned_watchpoint (WP_REMOVE, addr, len, type);
  else
    {
      unsigned len_rw = i386_length_and_rw_bits (len, type);

      retval = i386_remove_aligned_watchpoint (addr, len_rw);
    }

  if (maint_show_dr)
    i386_show_dr ("remove_watchpoint", addr, len, type);

  return retval;
}

/* Return non-zero if we can watch a memory region that starts at
   address ADDR and whose length is LEN bytes.  */

static int
i386_region_ok_for_watchpoint (CORE_ADDR addr, int len)
{
  int nregs;

  /* Compute how many aligned watchpoints we would need to cover this
     region.  */
  nregs = i386_handle_nonaligned_watchpoint (WP_COUNT, addr, len, hw_write);
  return nregs <= DR_NADDR ? 1 : 0;
}

/* If the inferior has some watchpoint that triggered, set the
   address associated with that watchpoint and return non-zero.
   Otherwise, return zero.  */

static int
i386_stopped_data_address (struct target_ops *ops, CORE_ADDR *addr_p)
{
  CORE_ADDR addr = 0;
  int i;
  int rc = 0;

  dr_status_mirror = i386_dr_low.get_status ();

  ALL_DEBUG_REGISTERS(i)
    {
      if (I386_DR_WATCH_HIT (i)
	  /* This second condition makes sure DRi is set up for a data
	     watchpoint, not a hardware breakpoint.  The reason is
	     that GDB doesn't call the target_stopped_data_address
	     method except for data watchpoints.  In other words, I'm
	     being paranoiac.  */
	  && I386_DR_GET_RW_LEN (i) != 0
	  /* This third condition makes sure DRi is not vacant, this
	     avoids false positives in windows-nat.c.  */
	  && !I386_DR_VACANT (i))
	{
	  addr = dr_mirror[i];
	  rc = 1;
	  if (maint_show_dr)
	    i386_show_dr ("watchpoint_hit", addr, -1, hw_write);
	}
    }
  if (maint_show_dr && addr == 0)
    i386_show_dr ("stopped_data_addr", 0, 0, hw_write);

  if (rc)
    *addr_p = addr;
  return rc;
}

static int
i386_stopped_by_watchpoint (void)
{
  CORE_ADDR addr = 0;
  return i386_stopped_data_address (&current_target, &addr);
}

/* Insert a hardware-assisted breakpoint at BP_TGT->placed_address.
   Return 0 on success, EBUSY on failure.  */
static int
i386_insert_hw_breakpoint (struct gdbarch *gdbarch,
			   struct bp_target_info *bp_tgt)
{
  unsigned len_rw = i386_length_and_rw_bits (1, hw_execute);
  CORE_ADDR addr = bp_tgt->placed_address;
  int retval = i386_insert_aligned_watchpoint (addr, len_rw) ? EBUSY : 0;

  if (maint_show_dr)
    i386_show_dr ("insert_hwbp", addr, 1, hw_execute);

  return retval;
}

/* Remove a hardware-assisted breakpoint at BP_TGT->placed_address.
   Return 0 on success, -1 on failure.  */

static int
i386_remove_hw_breakpoint (struct gdbarch *gdbarch,
			   struct bp_target_info *bp_tgt)
{
  unsigned len_rw = i386_length_and_rw_bits (1, hw_execute);
  CORE_ADDR addr = bp_tgt->placed_address;
  int retval = i386_remove_aligned_watchpoint (addr, len_rw);

  if (maint_show_dr)
    i386_show_dr ("remove_hwbp", addr, 1, hw_execute);

  return retval;
}

/* Returns the number of hardware watchpoints of type TYPE that we can
   set.  Value is positive if we can set CNT watchpoints, zero if
   setting watchpoints of type TYPE is not supported, and negative if
   CNT is more than the maximum number of watchpoints of type TYPE
   that we can support.  TYPE is one of bp_hardware_watchpoint,
   bp_read_watchpoint, bp_write_watchpoint, or bp_hardware_breakpoint.
   CNT is the number of such watchpoints used so far (including this
   one).  OTHERTYPE is non-zero if other types of watchpoints are
   currently enabled.

   We always return 1 here because we don't have enough information
   about possible overlap of addresses that they want to watch.  As an
   extreme example, consider the case where all the watchpoints watch
   the same address and the same region length: then we can handle a
   virtually unlimited number of watchpoints, due to debug register
   sharing implemented via reference counts in i386-nat.c.  */

static int
i386_can_use_hw_breakpoint (int type, int cnt, int othertype)
{
  return 1;
}

static void
add_show_debug_regs_command (void)
{
  /* A maintenance command to enable printing the internal DRi mirror
     variables.  */
  add_setshow_boolean_cmd ("show-debug-regs", class_maintenance,
			   &maint_show_dr, _("\
Set whether to show variables that mirror the x86 debug registers."), _("\
Show whether to show variables that mirror the x86 debug registers."), _("\
Use \"on\" to enable, \"off\" to disable.\n\
If enabled, the debug registers values are shown when GDB inserts\n\
or removes a hardware breakpoint or watchpoint, and when the inferior\n\
triggers a breakpoint or watchpoint."),
			   NULL,
			   NULL,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);
}

/* There are only two global functions left.  */

void
i386_use_watchpoints (struct target_ops *t)
{
  /* After a watchpoint trap, the PC points to the instruction after the
     one that caused the trap.  Therefore we don't need to step over it.
     But we do need to reset the status register to avoid another trap.  */
  t->to_have_continuable_watchpoint = 1;

  t->to_can_use_hw_breakpoint = i386_can_use_hw_breakpoint;
  t->to_region_ok_for_hw_watchpoint = i386_region_ok_for_watchpoint;
  t->to_stopped_by_watchpoint = i386_stopped_by_watchpoint;
  t->to_stopped_data_address = i386_stopped_data_address;
  t->to_insert_watchpoint = i386_insert_watchpoint;
  t->to_remove_watchpoint = i386_remove_watchpoint;
  t->to_insert_hw_breakpoint = i386_insert_hw_breakpoint;
  t->to_remove_hw_breakpoint = i386_remove_hw_breakpoint;
}

void
i386_set_debug_register_length (int len)
{
  /* This function should be called only once for each native target.  */
  gdb_assert (i386_dr_low.debug_register_length == 0);
  gdb_assert (len == 4 || len == 8);
  i386_dr_low.debug_register_length = len;
  add_show_debug_regs_command ();
}
