/* GNU/Linux/AArch64 specific low level interface, for the remote server for
   GDB.

   Copyright (C) 2009-2013 Free Software Foundation, Inc.
   Contributed by ARM Ltd.

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
#include "elf/common.h"

#include <signal.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/uio.h>

#include "gdb_proc_service.h"

/* Defined in auto-generated files.  */
void init_registers_aarch64 (void);

/* Defined in auto-generated files.  */
void init_registers_aarch64_without_fpu (void);

#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif

#define AARCH64_X_REGS_NUM 31
#define AARCH64_V_REGS_NUM 32
#define AARCH64_X0_REGNO    0
#define AARCH64_SP_REGNO   31
#define AARCH64_PC_REGNO   32
#define AARCH64_CPSR_REGNO 33
#define AARCH64_V0_REGNO   34

#define AARCH64_NUM_REGS (AARCH64_V0_REGNO + AARCH64_V_REGS_NUM)

static int
aarch64_regmap [] =
{
  /* These offsets correspond to GET/SETREGSET */
  /* x0...  */
   0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8,
   8*8,  9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8,
  16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8,
  24*8, 25*8, 26*8, 27*8, 28*8,
  29*8,
  30*8,				/* x30 lr */
  31*8,				/* x31 sp */
  32*8,				/*     pc */
  33*8,				/*     cpsr    4 bytes!*/

  /* FP register offsets correspond to GET/SETFPREGSET */
   0*16,  1*16,  2*16,  3*16,  4*16,  5*16,  6*16,  7*16,
   8*16,  9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16,
  16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16,
  24*16, 25*16, 26*16, 27*16, 28*16, 29*16, 30*16, 31*16
};

/* Here starts the macro definitions, data structures, and code for
   the hardware breakpoint and hardware watchpoint support.  The
   following is the abbreviations that are used frequently in the code
   and comment:

   hw - hardware
   bp - breakpoint
   wp - watchpoint  */

/* Maximum number of hardware breakpoint and watchpoint registers.
   Neither of these values may exceed the width of dr_changed_t
   measured in bits.  */

#define AARCH64_HBP_MAX_NUM 16
#define AARCH64_HWP_MAX_NUM 16

/* Alignment requirement in bytes of hardware breakpoint and
   watchpoint address.  This is the requirement for the addresses that
   can be written to the hardware breakpoint/watchpoint value
   registers.  The kernel currently does not do any alignment on
   addresses when receiving a writing request (via ptrace call) to
   these debug registers, and it will reject any address that is
   unaligned.
   Some limited support has been provided in this gdbserver port for
   unaligned watchpoints, so that from a gdb user point of view, an
   unaligned watchpoint can still be set.  This is achieved by
   minimally enlarging the watched area to meet the alignment
   requirement, and if necessary, splitting the watchpoint over
   several hardware watchpoint registers.  */

#define AARCH64_HBP_ALIGNMENT 4
#define AARCH64_HWP_ALIGNMENT 8

/* The maximum length of a memory region that can be watched by one
   hardware watchpoint register.  */

#define AARCH64_HWP_MAX_LEN_PER_REG 8

/* Each bit of a variable of this type is used to indicate whether a
   hardware breakpoint or watchpoint setting has been changed since
   the last updating.  Bit N corresponds to the Nth hardware
   breakpoint or watchpoint setting which is managed in
   aarch64_debug_reg_state.  Where N is valid between 0 and the total
   number of the hardware breakpoint or watchpoint debug registers
   minus 1.  When the bit N is 1, it indicates the corresponding
   breakpoint or watchpoint setting is changed, and thus the
   corresponding hardware debug register needs to be updated via the
   ptrace interface.

   In the per-thread arch-specific data area, we define two such
   variables for per-thread hardware breakpoint and watchpoint
   settings respectively.

   This type is part of the mechanism which helps reduce the number of
   ptrace calls to the kernel, i.e. avoid asking the kernel to write
   to the debug registers with unchanged values.  */

typedef unsigned long long dr_changed_t;

/* Set each of the lower M bits of X to 1; assert X is wide enough.  */

#define DR_MARK_ALL_CHANGED(x, m)					\
  do									\
    {									\
      gdb_assert (sizeof ((x)) * 8 >= (m));				\
      (x) = (((dr_changed_t)1 << (m)) - 1);				\
    } while (0)

#define DR_MARK_N_CHANGED(x, n)						\
  do									\
    {									\
      (x) |= ((dr_changed_t)1 << (n));					\
    } while (0)

#define DR_CLEAR_CHANGED(x)						\
  do									\
    {									\
      (x) = 0;								\
    } while (0)

#define DR_HAS_CHANGED(x) ((x) != 0)
#define DR_N_HAS_CHANGED(x, n) ((x) & ((dr_changed_t)1 << (n)))

/* Structure for managing the hardware breakpoint/watchpoint resources.
   DR_ADDR_* stores the address, DR_CTRL_* stores the control register
   content, and DR_REF_COUNT_* counts the numbers of references to the
   corresponding bp/wp, by which way the limited hardware resources
   are not wasted on duplicated bp/wp settings (though so far gdb has
   done a good job by not sending duplicated bp/wp requests).  */

struct aarch64_debug_reg_state
{
  /* hardware breakpoint */
  CORE_ADDR dr_addr_bp[AARCH64_HBP_MAX_NUM];
  unsigned int dr_ctrl_bp[AARCH64_HBP_MAX_NUM];
  unsigned int dr_ref_count_bp[AARCH64_HBP_MAX_NUM];

  /* hardware watchpoint */
  CORE_ADDR dr_addr_wp[AARCH64_HWP_MAX_NUM];
  unsigned int dr_ctrl_wp[AARCH64_HWP_MAX_NUM];
  unsigned int dr_ref_count_wp[AARCH64_HWP_MAX_NUM];
};

/* Per-process arch-specific data we want to keep.  */

struct arch_process_info
{
  /* Hardware breakpoint/watchpoint data.
     The reason for them to be per-process rather than per-thread is
     due to the lack of information in the gdbserver environment;
     gdbserver is not told that whether a requested hardware
     breakpoint/watchpoint is thread specific or not, so it has to set
     each hw bp/wp for every thread in the current process.  The
     higher level bp/wp management in gdb will resume a thread if a hw
     bp/wp trap is not expected for it.  Since the hw bp/wp setting is
     same for each thread, it is reasonable for the data to live here.
     */
  struct aarch64_debug_reg_state debug_reg_state;
};

/* Per-thread arch-specific data we want to keep.  */

struct arch_lwp_info
{
  /* When bit N is 1, it indicates the Nth hardware breakpoint or
     watchpoint register pair needs to be updated when the thread is
     resumed; see aarch64_linux_prepare_to_resume.  */
  dr_changed_t dr_changed_bp;
  dr_changed_t dr_changed_wp;
};

/* Number of hardware breakpoints/watchpoints the target supports.
   They are initialized with values obtained via the ptrace calls
   with NT_ARM_HW_BREAK and NT_ARM_HW_WATCH respectively.  */

static int aarch64_num_bp_regs;
static int aarch64_num_wp_regs;

/* Hardware breakpoint/watchpoint types.
   The values map to their encodings in the bit 4 and bit 3 of the
   hardware breakpoint/watchpoint control registers.  */

enum target_point_type
{
  hw_execute = 0,		/* Execute HW breakpoint */
  hw_read = 1,			/* Read    HW watchpoint */
  hw_write = 2,			/* Common  HW watchpoint */
  hw_access = 3,		/* Access  HW watchpoint */
  point_type_unsupported
};

#define Z_PACKET_SW_BP '0'
#define Z_PACKET_HW_BP '1'
#define Z_PACKET_WRITE_WP '2'
#define Z_PACKET_READ_WP '3'
#define Z_PACKET_ACCESS_WP '4'

/* Map the protocol breakpoint/watchpoint type TYPE to
   enum target_point_type.  */

static enum target_point_type
Z_packet_to_point_type (char type)
{
  switch (type)
    {
    case Z_PACKET_SW_BP:
      /* Leave the handling of the sw breakpoint with the gdb client.  */
      return point_type_unsupported;
    case Z_PACKET_HW_BP:
      return hw_execute;
    case Z_PACKET_WRITE_WP:
      return hw_write;
    case Z_PACKET_READ_WP:
      return hw_read;
    case Z_PACKET_ACCESS_WP:
      return hw_access;
    default:
      return point_type_unsupported;
    }
}

static int
aarch64_cannot_store_register (int regno)
{
  return regno >= AARCH64_NUM_REGS;
}

static int
aarch64_cannot_fetch_register (int regno)
{
  return regno >= AARCH64_NUM_REGS;
}

static void
aarch64_fill_gregset (struct regcache *regcache, void *buf)
{
  struct user_pt_regs *regset = buf;
  int i;

  for (i = 0; i < AARCH64_X_REGS_NUM; i++)
    collect_register (regcache, AARCH64_X0_REGNO + i, &regset->regs[i]);
  collect_register (regcache, AARCH64_SP_REGNO, &regset->sp);
  collect_register (regcache, AARCH64_PC_REGNO, &regset->pc);
  collect_register (regcache, AARCH64_CPSR_REGNO, &regset->pstate);
}

static void
aarch64_store_gregset (struct regcache *regcache, const void *buf)
{
  const struct user_pt_regs *regset = buf;
  int i;

  for (i = 0; i < AARCH64_X_REGS_NUM; i++)
    supply_register (regcache, AARCH64_X0_REGNO + i, &regset->regs[i]);
  supply_register (regcache, AARCH64_SP_REGNO, &regset->sp);
  supply_register (regcache, AARCH64_PC_REGNO, &regset->pc);
  supply_register (regcache, AARCH64_CPSR_REGNO, &regset->pstate);
}

static void
aarch64_fill_fpregset (struct regcache *regcache, void *buf)
{
  struct user_fpsimd_state *regset = buf;
  int i;

  for (i = 0; i < AARCH64_V_REGS_NUM; i++)
    collect_register (regcache, AARCH64_V0_REGNO + i, &regset->vregs[i]);
}

static void
aarch64_store_fpregset (struct regcache *regcache, const void *buf)
{
  const struct user_fpsimd_state *regset = buf;
  int i;

  for (i = 0; i < AARCH64_V_REGS_NUM; i++)
    supply_register (regcache, AARCH64_V0_REGNO + i, &regset->vregs[i]);
}

/* Debugging of hardware breakpoint/watchpoint support.  */
extern int debug_hw_points;

/* Enable miscellaneous debugging output.  The name is historical - it
   was originally used to debug LinuxThreads support.  */
extern int debug_threads;

static CORE_ADDR
aarch64_get_pc (struct regcache *regcache)
{
  unsigned long pc;

  collect_register_by_name (regcache, "pc", &pc);
  if (debug_threads)
    fprintf (stderr, "stop pc is %08lx\n", pc);
  return pc;
}

static void
aarch64_set_pc (struct regcache *regcache, CORE_ADDR pc)
{
  unsigned long newpc = pc;
  supply_register_by_name (regcache, "pc", &newpc);
}

/* Correct in either endianness.  */

#define aarch64_breakpoint_len 4

static const unsigned long aarch64_breakpoint = 0x00800011;

static int
aarch64_breakpoint_at (CORE_ADDR where)
{
  unsigned long insn;

  (*the_target->read_memory) (where, (unsigned char *) &insn, 4);
  if (insn == aarch64_breakpoint)
    return 1;

  return 0;
}

/* Print the values of the cached breakpoint/watchpoint registers.
   This is enabled via the "set debug-hw-points" monitor command.  */

static void
aarch64_show_debug_reg_state (struct aarch64_debug_reg_state *state,
			      const char *func, CORE_ADDR addr,
			      int len, enum target_point_type type)
{
  int i;

  fprintf (stderr, "%s", func);
  if (addr || len)
    fprintf (stderr, " (addr=0x%08lx, len=%d, type=%s)",
	     (unsigned long) addr, len,
	     type == hw_write ? "hw-write-watchpoint"
	     : (type == hw_read ? "hw-read-watchpoint"
		: (type == hw_access ? "hw-access-watchpoint"
		   : (type == hw_execute ? "hw-breakpoint"
		      : "??unknown??"))));
  fprintf (stderr, ":\n");

  fprintf (stderr, "\tBREAKPOINTs:\n");
  for (i = 0; i < aarch64_num_bp_regs; i++)
    fprintf (stderr, "\tBP%d: addr=0x%s, ctrl=0x%08x, ref.count=%d\n",
	     i, paddress (state->dr_addr_bp[i]),
	     state->dr_ctrl_bp[i], state->dr_ref_count_bp[i]);

  fprintf (stderr, "\tWATCHPOINTs:\n");
  for (i = 0; i < aarch64_num_wp_regs; i++)
    fprintf (stderr, "\tWP%d: addr=0x%s, ctrl=0x%08x, ref.count=%d\n",
	     i, paddress (state->dr_addr_wp[i]),
	     state->dr_ctrl_wp[i], state->dr_ref_count_wp[i]);
}

static void
aarch64_init_debug_reg_state (struct aarch64_debug_reg_state *state)
{
  int i;

  for (i = 0; i < AARCH64_HBP_MAX_NUM; ++i)
    {
      state->dr_addr_bp[i] = 0;
      state->dr_ctrl_bp[i] = 0;
      state->dr_ref_count_bp[i] = 0;
    }

  for (i = 0; i < AARCH64_HWP_MAX_NUM; ++i)
    {
      state->dr_addr_wp[i] = 0;
      state->dr_ctrl_wp[i] = 0;
      state->dr_ref_count_wp[i] = 0;
    }
}

/* ptrace expects control registers to be formatted as follows:

   31                             13          5      3      1     0
   +--------------------------------+----------+------+------+----+
   |         RESERVED (SBZ)         |  LENGTH  | TYPE | PRIV | EN |
   +--------------------------------+----------+------+------+----+

   The TYPE field is ignored for breakpoints.  */

#define DR_CONTROL_ENABLED(ctrl)	(((ctrl) & 0x1) == 1)
#define DR_CONTROL_LENGTH(ctrl)		(((ctrl) >> 5) & 0xff)

/* Utility function that returns the length in bytes of a watchpoint
   according to the content of a hardware debug control register CTRL.
   Note that the kernel currently only supports the following Byte
   Address Select (BAS) values: 0x1, 0x3, 0xf and 0xff, which means
   that for a hardware watchpoint, its valid length can only be 1
   byte, 2 bytes, 4 bytes or 8 bytes.  */

static inline unsigned int
aarch64_watchpoint_length (unsigned int ctrl)
{
  switch (DR_CONTROL_LENGTH (ctrl))
    {
    case 0x01:
      return 1;
    case 0x03:
      return 2;
    case 0x0f:
      return 4;
    case 0xff:
      return 8;
    default:
      return 0;
    }
}

/* Given the hardware breakpoint or watchpoint type TYPE and its
   length LEN, return the expected encoding for a hardware
   breakpoint/watchpoint control register.  */

static unsigned int
aarch64_point_encode_ctrl_reg (enum target_point_type type, int len)
{
  unsigned int ctrl;

  /* type */
  ctrl = type << 3;
  /* length bitmask */
  ctrl |= ((1 << len) - 1) << 5;
  /* enabled at el0 */
  ctrl |= (2 << 1) | 1;

  return ctrl;
}

/* Addresses to be written to the hardware breakpoint and watchpoint
   value registers need to be aligned; the alignment is 4-byte and
   8-type respectively.  Linux kernel rejects any non-aligned address
   it receives from the related ptrace call.  Furthermore, the kernel
   currently only supports the following Byte Address Select (BAS)
   values: 0x1, 0x3, 0xf and 0xff, which means that for a hardware
   watchpoint to be accepted by the kernel (via ptrace call), its
   valid length can only be 1 byte, 2 bytes, 4 bytes or 8 bytes.
   Despite these limitations, the unaligned watchpoint is supported in
   this gdbserver port.

   Return 0 for any non-compliant ADDR and/or LEN; return 1 otherwise.  */

static int
aarch64_point_is_aligned (int is_watchpoint, CORE_ADDR addr, int len)
{
  unsigned int alignment = is_watchpoint ? AARCH64_HWP_ALIGNMENT
    : AARCH64_HBP_ALIGNMENT;

  if (addr & (alignment - 1))
    return 0;

  if (len != 8 && len != 4 && len != 2 && len != 1)
    return 0;

  return 1;
}

/* Given the (potentially unaligned) watchpoint address in ADDR and
   length in LEN, return the aligned address and aligned length in
   *ALIGNED_ADDR_P and *ALIGNED_LEN_P, respectively.  The returned
   aligned address and length will be valid to be written to the
   hardware watchpoint value and control registers.  See the comment
   above aarch64_point_is_aligned for the information about the
   alignment requirement.  The given watchpoint may get truncated if
   more than one hardware register is needed to cover the watched
   region.  *NEXT_ADDR_P and *NEXT_LEN_P, if non-NULL, will return the
   address and length of the remaining part of the watchpoint (which
   can be processed by calling this routine again to generate another
   aligned address and length pair.

   Essentially, unaligned watchpoint is achieved by minimally
   enlarging the watched area to meet the alignment requirement, and
   if necessary, splitting the watchpoint over several hardware
   watchpoint registers.  The trade-off is that there will be
   false-positive hits for the read-type or the access-type hardware
   watchpoints; for the write type, which is more commonly used, there
   will be no such issues, as the higher-level breakpoint management
   in gdb always examines the exact watched region for any content
   change, and transparently resumes a thread from a watchpoint trap
   if there is no change to the watched region.

   Another limitation is that because the watched region is enlarged,
   the watchpoint fault address returned by
   aarch64_stopped_data_address may be outside of the original watched
   region, especially when the triggering instruction is accessing a
   larger region.  When the fault address is not within any known
   range, watchpoints_triggered in gdb will get confused, as the
   higher-level watchpoint management is only aware of original
   watched regions, and will think that some unknown watchpoint has
   been triggered.  In such a case, gdb may stop without displaying
   any detailed information.

   Once the kernel provides the full support for Byte Address Select
   (BAS) in the hardware watchpoint control register, these
   limitations can be largely relaxed with some further work.  */

static void
aarch64_align_watchpoint (CORE_ADDR addr, int len, CORE_ADDR *aligned_addr_p,
			  int *aligned_len_p, CORE_ADDR *next_addr_p,
			  int *next_len_p)
{
  int aligned_len;
  unsigned int offset;
  CORE_ADDR aligned_addr;
  const unsigned int alignment = AARCH64_HWP_ALIGNMENT;
  const unsigned int max_wp_len = AARCH64_HWP_MAX_LEN_PER_REG;

  /* As assumed by the algorithm.  */
  gdb_assert (alignment == max_wp_len);

  if (len <= 0)
    return;

  /* Address to be put into the hardware watchpoint value register
     must be aligned.  */
  offset = addr & (alignment - 1);
  aligned_addr = addr - offset;

  gdb_assert (offset >= 0 && offset < alignment);
  gdb_assert (aligned_addr >= 0 && aligned_addr <= addr);
  gdb_assert ((offset + len) > 0);

  if (offset + len >= max_wp_len)
    {
      /* Need more than one watchpoint registers; truncate it at the
	 alignment boundary.  */
      aligned_len = max_wp_len;
      len -= (max_wp_len - offset);
      addr += (max_wp_len - offset);
      gdb_assert ((addr & (alignment - 1)) == 0);
    }
  else
    {
      /* Find the smallest valid length that is large enough to
	 accommodate this watchpoint.  */
      static const unsigned char
	aligned_len_array[AARCH64_HWP_MAX_LEN_PER_REG] =
	{ 1, 2, 4, 4, 8, 8, 8, 8 };

      aligned_len = aligned_len_array[offset + len - 1];
      addr += len;
      len = 0;
    }

  if (aligned_addr_p != NULL)
    *aligned_addr_p = aligned_addr;
  if (aligned_len_p != NULL)
    *aligned_len_p = aligned_len;
  if (next_addr_p != NULL)
    *next_addr_p = addr;
  if (next_len_p != NULL)
    *next_len_p = len;
}

/* Call ptrace to set the thread TID's hardware breakpoint/watchpoint
   registers with data from *STATE.  */

static void
aarch64_linux_set_debug_regs (const struct aarch64_debug_reg_state *state,
			      int tid, int watchpoint)
{
  int i, count;
  struct iovec iov;
  struct user_hwdebug_state regs;
  const CORE_ADDR *addr;
  const unsigned int *ctrl;

  iov.iov_base = &regs;
  iov.iov_len = sizeof (regs);
  count = watchpoint ? aarch64_num_wp_regs : aarch64_num_bp_regs;
  addr = watchpoint ? state->dr_addr_wp : state->dr_addr_bp;
  ctrl = watchpoint ? state->dr_ctrl_wp : state->dr_ctrl_bp;

  for (i = 0; i < count; i++)
    {
      regs.dbg_regs[i].addr = addr[i];
      regs.dbg_regs[i].ctrl = ctrl[i];
    }

  if (ptrace (PTRACE_SETREGSET, tid,
	      watchpoint ? NT_ARM_HW_WATCH : NT_ARM_HW_BREAK,
	      (void *) &iov))
    error (_("Unexpected error setting hardware debug registers"));
}

struct aarch64_dr_update_callback_param
{
  int pid;
  int is_watchpoint;
  unsigned int idx;
};

/* Callback function which records the information about the change of
   one hardware breakpoint/watchpoint setting for the thread ENTRY.
   The information is passed in via PTR.
   N.B.  The actual updating of hardware debug registers is not
   carried out until the moment the thread is resumed.  */

static int
debug_reg_change_callback (struct inferior_list_entry *entry, void *ptr)
{
  struct lwp_info *lwp = (struct lwp_info *) entry;
  struct aarch64_dr_update_callback_param *param_p
    = (struct aarch64_dr_update_callback_param *) ptr;
  int pid = param_p->pid;
  int idx = param_p->idx;
  int is_watchpoint = param_p->is_watchpoint;
  struct arch_lwp_info *info = lwp->arch_private;
  dr_changed_t *dr_changed_ptr;
  dr_changed_t dr_changed;

  if (debug_hw_points)
    {
      fprintf (stderr, "debug_reg_change_callback: \n\tOn entry:\n");
      fprintf (stderr, "\tpid%d, tid: %ld, dr_changed_bp=0x%llx, "
	       "dr_changed_wp=0x%llx\n",
	       pid, lwpid_of (lwp), info->dr_changed_bp,
	       info->dr_changed_wp);
    }

  dr_changed_ptr = is_watchpoint ? &info->dr_changed_wp
    : &info->dr_changed_bp;
  dr_changed = *dr_changed_ptr;

  /* Only update the threads of this process.  */
  if (pid_of (lwp) == pid)
    {
      gdb_assert (idx >= 0
		  && (idx <= (is_watchpoint ? aarch64_num_wp_regs
			      : aarch64_num_bp_regs)));

      /* The following assertion is not right, as there can be changes
	 that have not been made to the hardware debug registers
	 before new changes overwrite the old ones.  This can happen,
	 for instance, when the breakpoint/watchpoint hit one of the
	 threads and the user enters continue; then what happens is:
	 1) all breakpoints/watchpoints are removed for all threads;
	 2) a single step is carried out for the thread that was hit;
	 3) all of the points are inserted again for all threads;
	 4) all threads are resumed.
	 The 2nd step will only affect the one thread in which the
	 bp/wp was hit, which means only that one thread is resumed;
	 remember that the actual updating only happen in
	 aarch64_linux_prepare_to_resume, so other threads remain
	 stopped during the removal and insertion of bp/wp.  Therefore
	 for those threads, the change of insertion of the bp/wp
	 overwrites that of the earlier removals.  (The situation may
	 be different when bp/wp is steppable, or in the non-stop
	 mode.)  */
      /* gdb_assert (DR_N_HAS_CHANGED (dr_changed, idx) == 0);  */

      /* The actual update is done later just before resuming the lwp,
         we just mark that one register pair needs updating.  */
      DR_MARK_N_CHANGED (dr_changed, idx);
      *dr_changed_ptr = dr_changed;

      /* If the lwp isn't stopped, force it to momentarily pause, so
         we can update its debug registers.  */
      if (!lwp->stopped)
	linux_stop_lwp (lwp);
    }

  if (debug_hw_points)
    {
      fprintf (stderr, "\tOn exit:\n\tpid%d, tid: %ld, dr_changed_bp=0x%llx, "
	       "dr_changed_wp=0x%llx\n",
	       pid, lwpid_of (lwp), info->dr_changed_bp, info->dr_changed_wp);
    }

  return 0;
}

/* Notify each thread that their IDXth breakpoint/watchpoint register
   pair needs to be updated.  The message will be recorded in each
   thread's arch-specific data area, the actual updating will be done
   when the thread is resumed.  */

void
aarch64_notify_debug_reg_change (const struct aarch64_debug_reg_state *state,
				 int is_watchpoint, unsigned int idx)
{
  struct aarch64_dr_update_callback_param param;

  /* Only update the threads of this process.  */
  param.pid = pid_of (get_thread_lwp (current_inferior));

  param.is_watchpoint = is_watchpoint;
  param.idx = idx;

  find_inferior (&all_lwps, debug_reg_change_callback, (void *) &param);
}


/* Return the pointer to the debug register state structure in the
   current process' arch-specific data area.  */

static struct aarch64_debug_reg_state *
aarch64_get_debug_reg_state ()
{
  struct process_info *proc;

  proc = current_process ();
  return &proc->private->arch_private->debug_reg_state;
}

/* Record the insertion of one breakpoint/watchpoint, as represented
   by ADDR and CTRL, in the process' arch-specific data area *STATE.  */

static int
aarch64_dr_state_insert_one_point (struct aarch64_debug_reg_state *state,
				   enum target_point_type type,
				   CORE_ADDR addr, int len)
{
  int i, idx, num_regs, is_watchpoint;
  unsigned int ctrl, *dr_ctrl_p, *dr_ref_count;
  CORE_ADDR *dr_addr_p;

  /* Set up state pointers.  */
  is_watchpoint = (type != hw_execute);
  gdb_assert (aarch64_point_is_aligned (is_watchpoint, addr, len));
  if (is_watchpoint)
    {
      num_regs = aarch64_num_wp_regs;
      dr_addr_p = state->dr_addr_wp;
      dr_ctrl_p = state->dr_ctrl_wp;
      dr_ref_count = state->dr_ref_count_wp;
    }
  else
    {
      num_regs = aarch64_num_bp_regs;
      dr_addr_p = state->dr_addr_bp;
      dr_ctrl_p = state->dr_ctrl_bp;
      dr_ref_count = state->dr_ref_count_bp;
    }

  ctrl = aarch64_point_encode_ctrl_reg (type, len);

  /* Find an existing or free register in our cache.  */
  idx = -1;
  for (i = 0; i < num_regs; ++i)
    {
      if ((dr_ctrl_p[i] & 1) == 0)
	{
	  gdb_assert (dr_ref_count[i] == 0);
	  idx = i;
	  /* no break; continue hunting for an exising one.  */
	}
      else if (dr_addr_p[i] == addr && dr_ctrl_p[i] == ctrl)
	{
	  gdb_assert (dr_ref_count[i] != 0);
	  idx = i;
	  break;
	}
    }

  /* No space.  */
  if (idx == -1)
    return -1;

  /* Update our cache.  */
  if ((dr_ctrl_p[idx] & 1) == 0)
    {
      /* new entry */
      dr_addr_p[idx] = addr;
      dr_ctrl_p[idx] = ctrl;
      dr_ref_count[idx] = 1;
      /* Notify the change.  */
      aarch64_notify_debug_reg_change (state, is_watchpoint, idx);
    }
  else
    {
      /* existing entry */
      dr_ref_count[idx]++;
    }

  return 0;
}

/* Record the removal of one breakpoint/watchpoint, as represented by
   ADDR and CTRL, in the process' arch-specific data area *STATE.  */

static int
aarch64_dr_state_remove_one_point (struct aarch64_debug_reg_state *state,
				   enum target_point_type type,
				   CORE_ADDR addr, int len)
{
  int i, num_regs, is_watchpoint;
  unsigned int ctrl, *dr_ctrl_p, *dr_ref_count;
  CORE_ADDR *dr_addr_p;

  /* Set up state pointers.  */
  is_watchpoint = (type != hw_execute);
  gdb_assert (aarch64_point_is_aligned (is_watchpoint, addr, len));
  if (is_watchpoint)
    {
      num_regs = aarch64_num_wp_regs;
      dr_addr_p = state->dr_addr_wp;
      dr_ctrl_p = state->dr_ctrl_wp;
      dr_ref_count = state->dr_ref_count_wp;
    }
  else
    {
      num_regs = aarch64_num_bp_regs;
      dr_addr_p = state->dr_addr_bp;
      dr_ctrl_p = state->dr_ctrl_bp;
      dr_ref_count = state->dr_ref_count_bp;
    }

  ctrl = aarch64_point_encode_ctrl_reg (type, len);

  /* Find the entry that matches the ADDR and CTRL.  */
  for (i = 0; i < num_regs; ++i)
    if (dr_addr_p[i] == addr && dr_ctrl_p[i] == ctrl)
      {
	gdb_assert (dr_ref_count[i] != 0);
	break;
      }

  /* Not found.  */
  if (i == num_regs)
    return -1;

  /* Clear our cache.  */
  if (--dr_ref_count[i] == 0)
    {
      /* Clear the enable bit.  */
      ctrl &= ~1;
      dr_addr_p[i] = 0;
      dr_ctrl_p[i] = ctrl;
      /* Notify the change.  */
      aarch64_notify_debug_reg_change (state, is_watchpoint, i);
    }

  return 0;
}

static int
aarch64_handle_breakpoint (enum target_point_type type, CORE_ADDR addr,
			   int len, int is_insert)
{
  struct aarch64_debug_reg_state *state;

  /* The hardware breakpoint on AArch64 should always be 4-byte
     aligned.  */
  if (!aarch64_point_is_aligned (0 /* is_watchpoint */ , addr, len))
    return -1;

  state = aarch64_get_debug_reg_state ();

  if (is_insert)
    return aarch64_dr_state_insert_one_point (state, type, addr, len);
  else
    return aarch64_dr_state_remove_one_point (state, type, addr, len);
}

/* This is essentially the same as aarch64_handle_breakpoint, apart
   from that it is an aligned watchpoint to be handled.  */

static int
aarch64_handle_aligned_watchpoint (enum target_point_type type,
				   CORE_ADDR addr, int len, int is_insert)
{
  struct aarch64_debug_reg_state *state;

  state = aarch64_get_debug_reg_state ();

  if (is_insert)
    return aarch64_dr_state_insert_one_point (state, type, addr, len);
  else
    return aarch64_dr_state_remove_one_point (state, type, addr, len);
}

/* Insert/remove unaligned watchpoint by calling
   aarch64_align_watchpoint repeatedly until the whole watched region,
   as represented by ADDR and LEN, has been properly aligned and ready
   to be written to one or more hardware watchpoint registers.
   IS_INSERT indicates whether this is an insertion or a deletion.
   Return 0 if succeed.  */

static int
aarch64_handle_unaligned_watchpoint (enum target_point_type type,
				     CORE_ADDR addr, int len, int is_insert)
{
  struct aarch64_debug_reg_state *state
    = aarch64_get_debug_reg_state ();

  while (len > 0)
    {
      CORE_ADDR aligned_addr;
      int aligned_len, ret;

      aarch64_align_watchpoint (addr, len, &aligned_addr, &aligned_len,
				&addr, &len);

      if (is_insert)
	ret = aarch64_dr_state_insert_one_point (state, type, aligned_addr,
						 aligned_len);
      else
	ret = aarch64_dr_state_remove_one_point (state, type, aligned_addr,
						 aligned_len);

      if (debug_hw_points)
	fprintf (stderr,
 "handle_unaligned_watchpoint: is_insert: %d\n"
 "                             aligned_addr: 0x%s, aligned_len: %d\n"
 "                                next_addr: 0x%s,    next_len: %d\n",
		 is_insert, paddress (aligned_addr), aligned_len,
		 paddress (addr), len);

      if (ret != 0)
	return ret;
    }

  return 0;
}

static int
aarch64_handle_watchpoint (enum target_point_type type, CORE_ADDR addr,
			   int len, int is_insert)
{
  if (aarch64_point_is_aligned (1 /* is_watchpoint */ , addr, len))
    return aarch64_handle_aligned_watchpoint (type, addr, len, is_insert);
  else
    return aarch64_handle_unaligned_watchpoint (type, addr, len, is_insert);
}

/* Insert a hardware breakpoint/watchpoint.
   It actually only records the info of the to-be-inserted bp/wp;
   the actual insertion will happen when threads are resumed.

   Return 0 if succeed;
   Return 1 if TYPE is unsupported type;
   Return -1 if an error occurs.  */

static int
aarch64_insert_point (char type, CORE_ADDR addr, int len)
{
  int ret;
  enum target_point_type targ_type;

  if (debug_hw_points)
    fprintf (stderr, "insert_point on entry (addr=0x%08lx, len=%d)\n",
	     (unsigned long) addr, len);

  /* Determine the type from the packet.  */
  targ_type = Z_packet_to_point_type (type);
  if (targ_type == point_type_unsupported)
    return 1;

  if (targ_type != hw_execute)
    ret =
      aarch64_handle_watchpoint (targ_type, addr, len, 1 /* is_insert */);
  else
    ret =
      aarch64_handle_breakpoint (targ_type, addr, len, 1 /* is_insert */);

  if (debug_hw_points > 1)
    aarch64_show_debug_reg_state (aarch64_get_debug_reg_state (),
				  "insert_point", addr, len, targ_type);

  return ret;
}

/* Remove a hardware breakpoint/watchpoint.
   It actually only records the info of the to-be-removed bp/wp,
   the actual removal will be done when threads are resumed.

   Return 0 if succeed;
   Return 1 if TYPE is an unsupported type;
   Return -1 if an error occurs.  */

static int
aarch64_remove_point (char type, CORE_ADDR addr, int len)
{
  int ret;
  enum target_point_type targ_type;

  if (debug_hw_points)
    fprintf (stderr, "remove_point on entry (addr=0x%08lx, len=%d)\n",
	     (unsigned long) addr, len);

  /* Determine the type from the packet.  */
  targ_type = Z_packet_to_point_type (type);
  if (targ_type == point_type_unsupported)
    return 1;

  /* Set up state pointers.  */
  if (targ_type != hw_execute)
    ret =
      aarch64_handle_watchpoint (targ_type, addr, len, 0 /* is_insert */);
  else
    ret =
      aarch64_handle_breakpoint (targ_type, addr, len, 0 /* is_insert */);

  if (debug_hw_points > 1)
    aarch64_show_debug_reg_state (aarch64_get_debug_reg_state (),
				  "remove_point", addr, len, targ_type);

  return ret;
}

/* Returns the address associated with the watchpoint that hit, if
   any; returns 0 otherwise.  */

static CORE_ADDR
aarch64_stopped_data_address (void)
{
  siginfo_t siginfo;
  int pid, i;
  struct aarch64_debug_reg_state *state;

  pid = lwpid_of (get_thread_lwp (current_inferior));

  /* Get the siginfo.  */
  if (ptrace (PTRACE_GETSIGINFO, pid, NULL, &siginfo) != 0)
    return (CORE_ADDR) 0;

  /* Need to be a hardware breakpoint/watchpoint trap.  */
  if (siginfo.si_signo != SIGTRAP
      || (siginfo.si_code & 0xffff) != 0x0004 /* TRAP_HWBKPT */)
    return (CORE_ADDR) 0;

  /* Check if the address matches any watched address.  */
  state = aarch64_get_debug_reg_state ();
  for (i = aarch64_num_wp_regs - 1; i >= 0; --i)
    {
      const unsigned int len = aarch64_watchpoint_length (state->dr_ctrl_wp[i]);
      const CORE_ADDR addr_trap = (CORE_ADDR) siginfo.si_addr;
      const CORE_ADDR addr_watch = state->dr_addr_wp[i];
      if (state->dr_ref_count_wp[i]
	  && DR_CONTROL_ENABLED (state->dr_ctrl_wp[i])
	  && addr_trap >= addr_watch
	  && addr_trap < addr_watch + len)
	return addr_trap;
    }

  return (CORE_ADDR) 0;
}

/* Returns 1 if target was stopped due to a watchpoint hit, 0
   otherwise.  */

static int
aarch64_stopped_by_watchpoint (void)
{
  if (aarch64_stopped_data_address () != 0)
    return 1;
  else
    return 0;
}

/* Fetch the thread-local storage pointer for libthread_db.  */

ps_err_e
ps_get_thread_area (const struct ps_prochandle *ph,
		    lwpid_t lwpid, int idx, void **base)
{
  struct iovec iovec;
  uint64_t reg;

  iovec.iov_base = &reg;
  iovec.iov_len = sizeof (reg);

  if (ptrace (PTRACE_GETREGSET, lwpid, NT_ARM_TLS, &iovec) != 0)
    return PS_ERR;

  /* IDX is the bias from the thread pointer to the beginning of the
     thread descriptor.  It has to be subtracted due to implementation
     quirks in libthread_db.  */
  *base = (void *) (reg - idx);

  return PS_OK;
}

/* Called when a new process is created.  */

static struct arch_process_info *
aarch64_linux_new_process (void)
{
  struct arch_process_info *info = xcalloc (1, sizeof (*info));

  aarch64_init_debug_reg_state (&info->debug_reg_state);

  return info;
}

/* Called when a new thread is detected.  */

static struct arch_lwp_info *
aarch64_linux_new_thread (void)
{
  struct arch_lwp_info *info = xcalloc (1, sizeof (*info));

  /* Mark that all the hardware breakpoint/watchpoint register pairs
     for this thread need to be initialized (with data from
     aarch_process_info.debug_reg_state).  */
  DR_MARK_ALL_CHANGED (info->dr_changed_bp, aarch64_num_bp_regs);
  DR_MARK_ALL_CHANGED (info->dr_changed_wp, aarch64_num_wp_regs);

  return info;
}

/* Called when resuming a thread.
   If the debug regs have changed, update the thread's copies.  */

static void
aarch64_linux_prepare_to_resume (struct lwp_info *lwp)
{
  ptid_t ptid = ptid_of (lwp);
  struct arch_lwp_info *info = lwp->arch_private;

  if (DR_HAS_CHANGED (info->dr_changed_bp)
      || DR_HAS_CHANGED (info->dr_changed_wp))
    {
      int tid = ptid_get_lwp (ptid);
      struct process_info *proc = find_process_pid (ptid_get_pid (ptid));
      struct aarch64_debug_reg_state *state
	= &proc->private->arch_private->debug_reg_state;

      if (debug_hw_points)
	fprintf (stderr, "prepare_to_resume thread %ld\n", lwpid_of (lwp));

      /* Watchpoints.  */
      if (DR_HAS_CHANGED (info->dr_changed_wp))
	{
	  aarch64_linux_set_debug_regs (state, tid, 1);
	  DR_CLEAR_CHANGED (info->dr_changed_wp);
	}

      /* Breakpoints.  */
      if (DR_HAS_CHANGED (info->dr_changed_bp))
	{
	  aarch64_linux_set_debug_regs (state, tid, 0);
	  DR_CLEAR_CHANGED (info->dr_changed_bp);
	}
    }
}

/* ptrace hardware breakpoint resource info is formatted as follows:

   31             24             16               8              0
   +---------------+--------------+---------------+---------------+
   |   RESERVED    |   RESERVED   |   DEBUG_ARCH  |  NUM_SLOTS    |
   +---------------+--------------+---------------+---------------+  */

#define AARCH64_DEBUG_NUM_SLOTS(x) ((x) & 0xff)
#define AARCH64_DEBUG_ARCH(x) (((x) >> 8) & 0xff)
#define AARCH64_DEBUG_ARCH_V8 0x6

static void
aarch64_arch_setup (void)
{
  int pid;
  struct iovec iov;
  struct user_hwdebug_state dreg_state;

  init_registers_aarch64 ();

  pid = lwpid_of (get_thread_lwp (current_inferior));
  iov.iov_base = &dreg_state;
  iov.iov_len = sizeof (dreg_state);

  /* Get hardware watchpoint register info.  */
  if (ptrace (PTRACE_GETREGSET, pid, NT_ARM_HW_WATCH, &iov) == 0
      && AARCH64_DEBUG_ARCH (dreg_state.dbg_info) == AARCH64_DEBUG_ARCH_V8)
    {
      aarch64_num_wp_regs = AARCH64_DEBUG_NUM_SLOTS (dreg_state.dbg_info);
      if (aarch64_num_wp_regs > AARCH64_HWP_MAX_NUM)
	{
	  warning ("Unexpected number of hardware watchpoint registers reported"
		   " by ptrace, got %d, expected %d.",
		   aarch64_num_wp_regs, AARCH64_HWP_MAX_NUM);
	  aarch64_num_wp_regs = AARCH64_HWP_MAX_NUM;
	}
    }
  else
    {
      warning ("Unable to determine the number of hardware watchpoints"
	       " available.");
      aarch64_num_wp_regs = 0;
    }

  /* Get hardware breakpoint register info.  */
  if (ptrace (PTRACE_GETREGSET, pid, NT_ARM_HW_BREAK, &iov) == 0
      && AARCH64_DEBUG_ARCH (dreg_state.dbg_info) == AARCH64_DEBUG_ARCH_V8)
    {
      aarch64_num_bp_regs = AARCH64_DEBUG_NUM_SLOTS (dreg_state.dbg_info);
      if (aarch64_num_bp_regs > AARCH64_HBP_MAX_NUM)
	{
	  warning ("Unexpected number of hardware breakpoint registers reported"
		   " by ptrace, got %d, expected %d.",
		   aarch64_num_bp_regs, AARCH64_HBP_MAX_NUM);
	  aarch64_num_bp_regs = AARCH64_HBP_MAX_NUM;
	}
    }
  else
    {
      warning ("Unable to determine the number of hardware breakpoints"
	       " available.");
      aarch64_num_bp_regs = 0;
    }
}

struct regset_info target_regsets[] =
{
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_PRSTATUS,
    sizeof (struct user_pt_regs), GENERAL_REGS,
    aarch64_fill_gregset, aarch64_store_gregset },
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_FPREGSET,
    sizeof (struct user_fpsimd_state), FP_REGS,
    aarch64_fill_fpregset, aarch64_store_fpregset
  },
  { 0, 0, 0, -1, -1, NULL, NULL }
};

struct linux_target_ops the_low_target =
{
  aarch64_arch_setup,
  AARCH64_NUM_REGS,
  aarch64_regmap,
  NULL,
  aarch64_cannot_fetch_register,
  aarch64_cannot_store_register,
  NULL,
  aarch64_get_pc,
  aarch64_set_pc,
  (const unsigned char *) &aarch64_breakpoint,
  aarch64_breakpoint_len,
  NULL,
  0,
  aarch64_breakpoint_at,
  aarch64_insert_point,
  aarch64_remove_point,
  aarch64_stopped_by_watchpoint,
  aarch64_stopped_data_address,
  NULL,
  NULL,
  NULL,
  aarch64_linux_new_process,
  aarch64_linux_new_thread,
  aarch64_linux_prepare_to_resume,
};
