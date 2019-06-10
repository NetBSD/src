/* Internal interfaces for the GNU/Linux specific target code for gdbserver.
   Copyright (C) 2002-2017 Free Software Foundation, Inc.

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

#include "nat/linux-nat.h"
#include "nat/gdb_thread_db.h"
#include <signal.h>

#include "gdbthread.h"
#include "gdb_proc_service.h"

/* Included for ptrace type definitions.  */
#include "nat/linux-ptrace.h"
#include "target/waitstatus.h" /* For enum target_stop_reason.  */

#define PTRACE_XFER_TYPE long

#ifdef HAVE_LINUX_REGSETS
typedef void (*regset_fill_func) (struct regcache *, void *);
typedef void (*regset_store_func) (struct regcache *, const void *);
enum regset_type {
  GENERAL_REGS,
  FP_REGS,
  EXTENDED_REGS,
};

/* The arch's regsets array initializer must be terminated with a NULL
   regset.  */
#define NULL_REGSET \
  { 0, 0, 0, -1, (enum regset_type) -1, NULL, NULL }

struct regset_info
{
  int get_request, set_request;
  /* If NT_TYPE isn't 0, it will be passed to ptrace as the 3rd
     argument and the 4th argument should be "const struct iovec *".  */
  int nt_type;
  int size;
  enum regset_type type;
  regset_fill_func fill_function;
  regset_store_func store_function;
};

/* Aggregation of all the supported regsets of a given
   architecture/mode.  */

struct regsets_info
{
  /* The regsets array.  */
  struct regset_info *regsets;

  /* The number of regsets in the REGSETS array.  */
  int num_regsets;

  /* If we get EIO on a regset, do not try it again.  Note the set of
     supported regsets may depend on processor mode on biarch
     machines.  This is a (lazily allocated) array holding one boolean
     byte (0/1) per regset, with each element corresponding to the
     regset in the REGSETS array above at the same offset.  */
  char *disabled_regsets;
};

#endif

/* Mapping between the general-purpose registers in `struct user'
   format and GDB's register array layout.  */

struct usrregs_info
{
  /* The number of registers accessible.  */
  int num_regs;

  /* The registers map.  */
  int *regmap;
};

/* All info needed to access an architecture/mode's registers.  */

struct regs_info
{
  /* Regset support bitmap: 1 for registers that are transferred as a part
     of a regset, 0 for ones that need to be handled individually.  This
     can be NULL if all registers are transferred with regsets or regsets
     are not supported.  */
  unsigned char *regset_bitmap;

  /* Info used when accessing registers with PTRACE_PEEKUSER /
     PTRACE_POKEUSER.  This can be NULL if all registers are
     transferred with regsets  .*/
  struct usrregs_info *usrregs;

#ifdef HAVE_LINUX_REGSETS
  /* Info used when accessing registers with regsets.  */
  struct regsets_info *regsets_info;
#endif
};

struct process_info_private
{
  /* Arch-specific additions.  */
  struct arch_process_info *arch_private;

  /* libthread_db-specific additions.  Not NULL if this process has loaded
     thread_db, and it is active.  */
  struct thread_db *thread_db;

  /* &_r_debug.  0 if not yet determined.  -1 if no PT_DYNAMIC in Phdrs.  */
  CORE_ADDR r_debug;
};

struct lwp_info;

struct linux_target_ops
{
  /* Architecture-specific setup.  */
  void (*arch_setup) (void);

  const struct regs_info *(*regs_info) (void);
  int (*cannot_fetch_register) (int);

  /* Returns 0 if we can store the register, 1 if we can not
     store the register, and 2 if failure to store the register
     is acceptable.  */
  int (*cannot_store_register) (int);

  /* Hook to fetch a register in some non-standard way.  Used for
     example by backends that have read-only registers with hardcoded
     values (e.g., IA64's gr0/fr0/fr1).  Returns true if register
     REGNO was supplied, false if not, and we should fallback to the
     standard ptrace methods.  */
  int (*fetch_register) (struct regcache *regcache, int regno);

  CORE_ADDR (*get_pc) (struct regcache *regcache);
  void (*set_pc) (struct regcache *regcache, CORE_ADDR newpc);

  /* See target.h for details.  */
  int (*breakpoint_kind_from_pc) (CORE_ADDR *pcptr);

  /* See target.h for details.  */
  const gdb_byte *(*sw_breakpoint_from_kind) (int kind, int *size);

  /* Find the next possible PCs after the current instruction executes.  */
  VEC (CORE_ADDR) *(*get_next_pcs) (struct regcache *regcache);

  int decr_pc_after_break;
  int (*breakpoint_at) (CORE_ADDR pc);

  /* Breakpoint and watchpoint related functions.  See target.h for
     comments.  */
  int (*supports_z_point_type) (char z_type);
  int (*insert_point) (enum raw_bkpt_type type, CORE_ADDR addr,
		       int size, struct raw_breakpoint *bp);
  int (*remove_point) (enum raw_bkpt_type type, CORE_ADDR addr,
		       int size, struct raw_breakpoint *bp);

  int (*stopped_by_watchpoint) (void);
  CORE_ADDR (*stopped_data_address) (void);

  /* Hooks to reformat register data for PEEKUSR/POKEUSR (in particular
     for registers smaller than an xfer unit).  */
  void (*collect_ptrace_register) (struct regcache *regcache,
				   int regno, char *buf);
  void (*supply_ptrace_register) (struct regcache *regcache,
				  int regno, const char *buf);

  /* Hook to convert from target format to ptrace format and back.
     Returns true if any conversion was done; false otherwise.
     If DIRECTION is 1, then copy from INF to NATIVE.
     If DIRECTION is 0, copy from NATIVE to INF.  */
  int (*siginfo_fixup) (siginfo_t *native, gdb_byte *inf, int direction);

  /* Hook to call when a new process is created or attached to.
     If extra per-process architecture-specific data is needed,
     allocate it here.  */
  struct arch_process_info * (*new_process) (void);

  /* Hook to call when a new thread is detected.
     If extra per-thread architecture-specific data is needed,
     allocate it here.  */
  void (*new_thread) (struct lwp_info *);

  /* Hook to call, if any, when a new fork is attached.  */
  void (*new_fork) (struct process_info *parent, struct process_info *child);

  /* Hook to call prior to resuming a thread.  */
  void (*prepare_to_resume) (struct lwp_info *);

  /* Hook to support target specific qSupported.  */
  void (*process_qsupported) (char **, int count);

  /* Returns true if the low target supports tracepoints.  */
  int (*supports_tracepoints) (void);

  /* Fill ADDRP with the thread area address of LWPID.  Returns 0 on
     success, -1 on failure.  */
  int (*get_thread_area) (int lwpid, CORE_ADDR *addrp);

  /* Install a fast tracepoint jump pad.  See target.h for
     comments.  */
  int (*install_fast_tracepoint_jump_pad) (CORE_ADDR tpoint, CORE_ADDR tpaddr,
					   CORE_ADDR collector,
					   CORE_ADDR lockaddr,
					   ULONGEST orig_size,
					   CORE_ADDR *jump_entry,
					   CORE_ADDR *trampoline,
					   ULONGEST *trampoline_size,
					   unsigned char *jjump_pad_insn,
					   ULONGEST *jjump_pad_insn_size,
					   CORE_ADDR *adjusted_insn_addr,
					   CORE_ADDR *adjusted_insn_addr_end,
					   char *err);

  /* Return the bytecode operations vector for the current inferior.
     Returns NULL if bytecode compilation is not supported.  */
  struct emit_ops *(*emit_ops) (void);

  /* Return the minimum length of an instruction that can be safely overwritten
     for use as a fast tracepoint.  */
  int (*get_min_fast_tracepoint_insn_len) (void);

  /* Returns true if the low target supports range stepping.  */
  int (*supports_range_stepping) (void);

  /* See target.h.  */
  int (*breakpoint_kind_from_current_state) (CORE_ADDR *pcptr);

  /* See target.h.  */
  int (*supports_hardware_single_step) (void);

  /* Fill *SYSNO with the syscall nr trapped.  Only to be called when
     inferior is stopped due to SYSCALL_SIGTRAP.  */
  void (*get_syscall_trapinfo) (struct regcache *regcache, int *sysno);

  /* See target.h.  */
  int (*get_ipa_tdesc_idx) (void);
};

extern struct linux_target_ops the_low_target;

#define get_thread_lwp(thr) ((struct lwp_info *) (inferior_target_data (thr)))
#define get_lwp_thread(lwp) ((lwp)->thread)

/* This struct is recorded in the target_data field of struct thread_info.

   On linux ``all_threads'' is keyed by the LWP ID, which we use as the
   GDB protocol representation of the thread ID.  Threads also have
   a "process ID" (poorly named) which is (presently) the same as the
   LWP ID.

   There is also ``all_processes'' is keyed by the "overall process ID",
   which GNU/Linux calls tgid, "thread group ID".  */

struct lwp_info
{
  /* Backlink to the parent object.  */
  struct thread_info *thread;

  /* If this flag is set, the next SIGSTOP will be ignored (the
     process will be immediately resumed).  This means that either we
     sent the SIGSTOP to it ourselves and got some other pending event
     (so the SIGSTOP is still pending), or that we stopped the
     inferior implicitly via PTRACE_ATTACH and have not waited for it
     yet.  */
  int stop_expected;

  /* When this is true, we shall not try to resume this thread, even
     if last_resume_kind isn't resume_stop.  */
  int suspended;

  /* If this flag is set, the lwp is known to be stopped right now (stop
     event already received in a wait()).  */
  int stopped;

  /* Signal whether we are in a SYSCALL_ENTRY or
     in a SYSCALL_RETURN event.
     Values:
     - TARGET_WAITKIND_SYSCALL_ENTRY
     - TARGET_WAITKIND_SYSCALL_RETURN */
  enum target_waitkind syscall_state;

  /* When stopped is set, the last wait status recorded for this lwp.  */
  int last_status;

  /* If WAITSTATUS->KIND != TARGET_WAITKIND_IGNORE, the waitstatus for
     this LWP's last event, to pass to GDB without any further
     processing.  This is used to store extended ptrace event
     information or exit status until it can be reported to GDB.  */
  struct target_waitstatus waitstatus;

  /* A pointer to the fork child/parent relative.  Valid only while
     the parent fork event is not reported to higher layers.  Used to
     avoid wildcard vCont actions resuming a fork child before GDB is
     notified about the parent's fork event.  */
  struct lwp_info *fork_relative;

  /* When stopped is set, this is where the lwp last stopped, with
     decr_pc_after_break already accounted for.  If the LWP is
     running, this is the address at which the lwp was resumed.  */
  CORE_ADDR stop_pc;

  /* If this flag is set, STATUS_PENDING is a waitstatus that has not yet
     been reported.  */
  int status_pending_p;
  int status_pending;

  /* The reason the LWP last stopped, if we need to track it
     (breakpoint, watchpoint, etc.)  */
  enum target_stop_reason stop_reason;

  /* On architectures where it is possible to know the data address of
     a triggered watchpoint, STOPPED_DATA_ADDRESS is non-zero, and
     contains such data address.  Only valid if STOPPED_BY_WATCHPOINT
     is true.  */
  CORE_ADDR stopped_data_address;

  /* If this is non-zero, it is a breakpoint to be reinserted at our next
     stop (SIGTRAP stops only).  */
  CORE_ADDR bp_reinsert;

  /* If this flag is set, the last continue operation at the ptrace
     level on this process was a single-step.  */
  int stepping;

  /* Range to single step within.  This is a copy of the step range
     passed along the last resume request.  See 'struct
     thread_resume'.  */
  CORE_ADDR step_range_start;	/* Inclusive */
  CORE_ADDR step_range_end;	/* Exclusive */

  /* If this flag is set, we need to set the event request flags the
     next time we see this LWP stop.  */
  int must_set_ptrace_flags;

  /* If this is non-zero, it points to a chain of signals which need to
     be delivered to this process.  */
  struct pending_signals *pending_signals;

  /* A link used when resuming.  It is initialized from the resume request,
     and then processed and cleared in linux_resume_one_lwp.  */
  struct thread_resume *resume;

  /* True if it is known that this lwp is presently collecting a fast
     tracepoint (it is in the jump pad or in some code that will
     return to the jump pad.  Normally, we won't care about this, but
     we will if a signal arrives to this lwp while it is
     collecting.  */
  int collecting_fast_tracepoint;

  /* If this is non-zero, it points to a chain of signals which need
     to be reported to GDB.  These were deferred because the thread
     was doing a fast tracepoint collect when they arrived.  */
  struct pending_signals *pending_signals_to_report;

  /* When collecting_fast_tracepoint is first found to be 1, we insert
     a exit-jump-pad-quickly breakpoint.  This is it.  */
  struct breakpoint *exit_jump_pad_bkpt;

#ifdef USE_THREAD_DB
  int thread_known;
  /* The thread handle, used for e.g. TLS access.  Only valid if
     THREAD_KNOWN is set.  */
  td_thrhandle_t th;
#endif

  /* Arch-specific additions.  */
  struct arch_lwp_info *arch_private;
};

int linux_pid_exe_is_elf_64_file (int pid, unsigned int *machine);

/* Attach to PTID.  Returns 0 on success, non-zero otherwise (an
   errno).  */
int linux_attach_lwp (ptid_t ptid);

struct lwp_info *find_lwp_pid (ptid_t ptid);
/* For linux_stop_lwp see nat/linux-nat.h.  */

#ifdef HAVE_LINUX_REGSETS
void initialize_regsets_info (struct regsets_info *regsets_info);
#endif

void initialize_low_arch (void);

void linux_set_pc_32bit (struct regcache *regcache, CORE_ADDR pc);
CORE_ADDR linux_get_pc_32bit (struct regcache *regcache);

void linux_set_pc_64bit (struct regcache *regcache, CORE_ADDR pc);
CORE_ADDR linux_get_pc_64bit (struct regcache *regcache);

/* From thread-db.c  */
int thread_db_init (void);
void thread_db_detach (struct process_info *);
void thread_db_mourn (struct process_info *);
int thread_db_handle_monitor_command (char *);
int thread_db_get_tls_address (struct thread_info *thread, CORE_ADDR offset,
			       CORE_ADDR load_module, CORE_ADDR *address);
int thread_db_look_up_one_symbol (const char *name, CORE_ADDR *addrp);

extern int have_ptrace_getregset;
