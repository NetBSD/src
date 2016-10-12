/* Copyright (C) 2009-2016 Free Software Foundation, Inc.
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

#ifndef AARCH64_LINUX_HW_POINT_H
#define AARCH64_LINUX_HW_POINT_H 1

/* Macro definitions, data structures, and code for the hardware
   breakpoint and hardware watchpoint support follow.  We use the
   following abbreviations throughout the code:

   hw - hardware
   bp - breakpoint
   wp - watchpoint  */

/* Maximum number of hardware breakpoint and watchpoint registers.
   Neither of these values may exceed the width of dr_changed_t
   measured in bits.  */

#define AARCH64_HBP_MAX_NUM 16
#define AARCH64_HWP_MAX_NUM 16

/* Alignment requirement in bytes for addresses written to
   hardware breakpoint and watchpoint value registers.

   A ptrace call attempting to set an address that does not meet the
   alignment criteria will fail.  Limited support has been provided in
   this port for unaligned watchpoints, such that from a GDB user
   perspective, an unaligned watchpoint may be requested.

   This is achieved by minimally enlarging the watched area to meet the
   alignment requirement, and if necessary, splitting the watchpoint
   over several hardware watchpoint registers.  */

#define AARCH64_HBP_ALIGNMENT 4
#define AARCH64_HWP_ALIGNMENT 8

/* The maximum length of a memory region that can be watched by one
   hardware watchpoint register.  */

#define AARCH64_HWP_MAX_LEN_PER_REG 8

/* ptrace hardware breakpoint resource info is formatted as follows:

   31             24             16               8              0
   +---------------+--------------+---------------+---------------+
   |   RESERVED    |   RESERVED   |   DEBUG_ARCH  |  NUM_SLOTS    |
   +---------------+--------------+---------------+---------------+  */


/* Macros to extract fields from the hardware debug information word.  */
#define AARCH64_DEBUG_NUM_SLOTS(x) ((x) & 0xff)
#define AARCH64_DEBUG_ARCH(x) (((x) >> 8) & 0xff)

/* Macro for the expected version of the ARMv8-A debug architecture.  */
#define AARCH64_DEBUG_ARCH_V8 0x6
#define AARCH64_DEBUG_ARCH_V8_1 0x7
#define AARCH64_DEBUG_ARCH_V8_2 0x8

/* ptrace expects control registers to be formatted as follows:

   31                             13          5      3      1     0
   +--------------------------------+----------+------+------+----+
   |         RESERVED (SBZ)         |  LENGTH  | TYPE | PRIV | EN |
   +--------------------------------+----------+------+------+----+

   The TYPE field is ignored for breakpoints.  */

#define DR_CONTROL_ENABLED(ctrl)	(((ctrl) & 0x1) == 1)
#define DR_CONTROL_LENGTH(ctrl)		(((ctrl) >> 5) & 0xff)

/* Each bit of a variable of this type is used to indicate whether a
   hardware breakpoint or watchpoint setting has been changed since
   the last update.

   Bit N corresponds to the Nth hardware breakpoint or watchpoint
   setting which is managed in aarch64_debug_reg_state, where N is
   valid between 0 and the total number of the hardware breakpoint or
   watchpoint debug registers minus 1.

   When bit N is 1, the corresponding breakpoint or watchpoint setting
   has changed, and therefore the corresponding hardware debug
   register needs to be updated via the ptrace interface.

   In the per-thread arch-specific data area, we define two such
   variables for per-thread hardware breakpoint and watchpoint
   settings respectively.

   This type is part of the mechanism which helps reduce the number of
   ptrace calls to the kernel, i.e. avoid asking the kernel to write
   to the debug registers with unchanged values.  */

typedef ULONGEST dr_changed_t;

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

/* Per-thread arch-specific data we want to keep.  */

struct arch_lwp_info
{
  /* When bit N is 1, it indicates the Nth hardware breakpoint or
     watchpoint register pair needs to be updated when the thread is
     resumed; see aarch64_linux_prepare_to_resume.  */
  dr_changed_t dr_changed_bp;
  dr_changed_t dr_changed_wp;
};

extern int aarch64_num_bp_regs;
extern int aarch64_num_wp_regs;

unsigned int aarch64_watchpoint_length (unsigned int ctrl);

int aarch64_handle_breakpoint (enum target_hw_bp_type type, CORE_ADDR addr,
			       int len, int is_insert,
			       struct aarch64_debug_reg_state *state);
int aarch64_handle_watchpoint (enum target_hw_bp_type type, CORE_ADDR addr,
			       int len, int is_insert,
			       struct aarch64_debug_reg_state *state);

void aarch64_linux_set_debug_regs (const struct aarch64_debug_reg_state *state,
				   int tid, int watchpoint);

void aarch64_show_debug_reg_state (struct aarch64_debug_reg_state *state,
				   const char *func, CORE_ADDR addr,
				   int len, enum target_hw_bp_type type);

void aarch64_linux_get_debug_reg_capacity (int tid);

struct aarch64_debug_reg_state *aarch64_get_debug_reg_state (pid_t pid);

int aarch64_linux_region_ok_for_watchpoint (CORE_ADDR addr, int len);

#endif /* AARCH64_LINUX_HW_POINT_H */
