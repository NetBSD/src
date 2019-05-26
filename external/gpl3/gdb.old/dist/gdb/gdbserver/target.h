/* Target operations for the remote server for GDB.
   Copyright (C) 2002-2017 Free Software Foundation, Inc.

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

#ifndef TARGET_H
#define TARGET_H

#include <sys/types.h> /* for mode_t */
#include "target/target.h"
#include "target/resume.h"
#include "target/wait.h"
#include "target/waitstatus.h"
#include "mem-break.h"
#include "btrace-common.h"

struct emit_ops;
struct buffer;
struct process_info;

/* This structure describes how to resume a particular thread (or all
   threads) based on the client's request.  If thread is -1, then this
   entry applies to all threads.  These are passed around as an
   array.  */

struct thread_resume
{
  ptid_t thread;

  /* How to "resume".  */
  enum resume_kind kind;

  /* If non-zero, send this signal when we resume, or to stop the
     thread.  If stopping a thread, and this is 0, the target should
     stop the thread however it best decides to (e.g., SIGSTOP on
     linux; SuspendThread on win32).  This is a host signal value (not
     enum gdb_signal).  */
  int sig;

  /* Range to single step within.  Valid only iff KIND is resume_step.

     Single-step once, and then continuing stepping as long as the
     thread stops in this range.  (If the range is empty
     [STEP_RANGE_START == STEP_RANGE_END], then this is a single-step
     request.)  */
  CORE_ADDR step_range_start;	/* Inclusive */
  CORE_ADDR step_range_end;	/* Exclusive */
};

struct target_ops
{
  /* Start a new process.

     PROGRAM is a path to the program to execute.
     ARGS is a standard NULL-terminated array of arguments,
     to be passed to the inferior as ``argv''.

     Returns the new PID on success, -1 on failure.  Registers the new
     process with the process list.  */

  int (*create_inferior) (char *program, char **args);

  /* Do additional setup after a new process is created, including
     exec-wrapper completion.  */
  void (*post_create_inferior) (void);

  /* Attach to a running process.

     PID is the process ID to attach to, specified by the user
     or a higher layer.

     Returns -1 if attaching is unsupported, 0 on success, and calls
     error() otherwise.  */

  int (*attach) (unsigned long pid);

  /* Kill inferior PID.  Return -1 on failure, and 0 on success.  */

  int (*kill) (int pid);

  /* Detach from inferior PID. Return -1 on failure, and 0 on
     success.  */

  int (*detach) (int pid);

  /* The inferior process has died.  Do what is right.  */

  void (*mourn) (struct process_info *proc);

  /* Wait for inferior PID to exit.  */
  void (*join) (int pid);

  /* Return 1 iff the thread with process ID PID is alive.  */

  int (*thread_alive) (ptid_t pid);

  /* Resume the inferior process.  */

  void (*resume) (struct thread_resume *resume_info, size_t n);

  /* Wait for the inferior process or thread to change state.  Store
     status through argument pointer STATUS.

     PTID = -1 to wait for any pid to do something, PTID(pid,0,0) to
     wait for any thread of process pid to do something.  Return ptid
     of child, or -1 in case of error; store status through argument
     pointer STATUS.  OPTIONS is a bit set of options defined as
     TARGET_W* above.  If options contains TARGET_WNOHANG and there's
     no child stop to report, return is
     null_ptid/TARGET_WAITKIND_IGNORE.  */

  ptid_t (*wait) (ptid_t ptid, struct target_waitstatus *status, int options);

  /* Fetch registers from the inferior process.

     If REGNO is -1, fetch all registers; otherwise, fetch at least REGNO.  */

  void (*fetch_registers) (struct regcache *regcache, int regno);

  /* Store registers to the inferior process.

     If REGNO is -1, store all registers; otherwise, store at least REGNO.  */

  void (*store_registers) (struct regcache *regcache, int regno);

  /* Prepare to read or write memory from the inferior process.
     Targets use this to do what is necessary to get the state of the
     inferior such that it is possible to access memory.

     This should generally only be called from client facing routines,
     such as gdb_read_memory/gdb_write_memory, or the GDB breakpoint
     insertion routine.

     Like `read_memory' and `write_memory' below, returns 0 on success
     and errno on failure.  */

  int (*prepare_to_access_memory) (void);

  /* Undo the effects of prepare_to_access_memory.  */

  void (*done_accessing_memory) (void);

  /* Read memory from the inferior process.  This should generally be
     called through read_inferior_memory, which handles breakpoint shadowing.

     Read LEN bytes at MEMADDR into a buffer at MYADDR.
  
     Returns 0 on success and errno on failure.  */

  int (*read_memory) (CORE_ADDR memaddr, unsigned char *myaddr, int len);

  /* Write memory to the inferior process.  This should generally be
     called through write_inferior_memory, which handles breakpoint shadowing.

     Write LEN bytes from the buffer at MYADDR to MEMADDR.

     Returns 0 on success and errno on failure.  */

  int (*write_memory) (CORE_ADDR memaddr, const unsigned char *myaddr,
		       int len);

  /* Query GDB for the values of any symbols we're interested in.
     This function is called whenever we receive a "qSymbols::"
     query, which corresponds to every time more symbols (might)
     become available.  NULL if we aren't interested in any
     symbols.  */

  void (*look_up_symbols) (void);

  /* Send an interrupt request to the inferior process,
     however is appropriate.  */

  void (*request_interrupt) (void);

  /* Read auxiliary vector data from the inferior process.

     Read LEN bytes at OFFSET into a buffer at MYADDR.  */

  int (*read_auxv) (CORE_ADDR offset, unsigned char *myaddr,
		    unsigned int len);

  /* Returns true if GDB Z breakpoint type TYPE is supported, false
     otherwise.  The type is coded as follows:
       '0' - software-breakpoint
       '1' - hardware-breakpoint
       '2' - write watchpoint
       '3' - read watchpoint
       '4' - access watchpoint
  */
  int (*supports_z_point_type) (char z_type);

  /* Insert and remove a break or watchpoint.
     Returns 0 on success, -1 on failure and 1 on unsupported.  */

  int (*insert_point) (enum raw_bkpt_type type, CORE_ADDR addr,
		       int size, struct raw_breakpoint *bp);
  int (*remove_point) (enum raw_bkpt_type type, CORE_ADDR addr,
		       int size, struct raw_breakpoint *bp);

  /* Returns 1 if the target stopped because it executed a software
     breakpoint instruction, 0 otherwise.  */
  int (*stopped_by_sw_breakpoint) (void);

  /* Returns true if the target knows whether a trap was caused by a
     SW breakpoint triggering.  */
  int (*supports_stopped_by_sw_breakpoint) (void);

  /* Returns 1 if the target stopped for a hardware breakpoint.  */
  int (*stopped_by_hw_breakpoint) (void);

  /* Returns true if the target knows whether a trap was caused by a
     HW breakpoint triggering.  */
  int (*supports_stopped_by_hw_breakpoint) (void);

  /* Returns true if the target can do hardware single step.  */
  int (*supports_hardware_single_step) (void);

  /* Returns 1 if target was stopped due to a watchpoint hit, 0 otherwise.  */

  int (*stopped_by_watchpoint) (void);

  /* Returns the address associated with the watchpoint that hit, if any;
     returns 0 otherwise.  */

  CORE_ADDR (*stopped_data_address) (void);

  /* Reports the text, data offsets of the executable.  This is
     needed for uclinux where the executable is relocated during load
     time.  */

  int (*read_offsets) (CORE_ADDR *text, CORE_ADDR *data);

  /* Fetch the address associated with a specific thread local storage
     area, determined by the specified THREAD, OFFSET, and LOAD_MODULE.
     Stores it in *ADDRESS and returns zero on success; otherwise returns
     an error code.  A return value of -1 means this system does not
     support the operation.  */

  int (*get_tls_address) (struct thread_info *thread, CORE_ADDR offset,
			  CORE_ADDR load_module, CORE_ADDR *address);

   /* Read/Write from/to spufs using qXfer packets.  */
  int (*qxfer_spu) (const char *annex, unsigned char *readbuf,
		    unsigned const char *writebuf, CORE_ADDR offset, int len);

  /* Fill BUF with an hostio error packet representing the last hostio
     error.  */
  void (*hostio_last_error) (char *buf);

  /* Read/Write OS data using qXfer packets.  */
  int (*qxfer_osdata) (const char *annex, unsigned char *readbuf,
		       unsigned const char *writebuf, CORE_ADDR offset,
		       int len);

  /* Read/Write extra signal info.  */
  int (*qxfer_siginfo) (const char *annex, unsigned char *readbuf,
			unsigned const char *writebuf,
			CORE_ADDR offset, int len);

  int (*supports_non_stop) (void);

  /* Enables async target events.  Returns the previous enable
     state.  */
  int (*async) (int enable);

  /* Switch to non-stop (1) or all-stop (0) mode.  Return 0 on
     success, -1 otherwise.  */
  int (*start_non_stop) (int);

  /* Returns true if the target supports multi-process debugging.  */
  int (*supports_multi_process) (void);

  /* Returns true if fork events are supported.  */
  int (*supports_fork_events) (void);

  /* Returns true if vfork events are supported.  */
  int (*supports_vfork_events) (void);

  /* Returns true if exec events are supported.  */
  int (*supports_exec_events) (void);

  /* Allows target to re-initialize connection-specific settings.  */
  void (*handle_new_gdb_connection) (void);

  /* If not NULL, target-specific routine to process monitor command.
     Returns 1 if handled, or 0 to perform default processing.  */
  int (*handle_monitor_command) (char *);

  /* Returns the core given a thread, or -1 if not known.  */
  int (*core_of_thread) (ptid_t);

  /* Read loadmaps.  Read LEN bytes at OFFSET into a buffer at MYADDR.  */
  int (*read_loadmap) (const char *annex, CORE_ADDR offset,
		       unsigned char *myaddr, unsigned int len);

  /* Target specific qSupported support.  FEATURES is an array of
     features with COUNT elements.  */
  void (*process_qsupported) (char **features, int count);

  /* Return 1 if the target supports tracepoints, 0 (or leave the
     callback NULL) otherwise.  */
  int (*supports_tracepoints) (void);

  /* Read PC from REGCACHE.  */
  CORE_ADDR (*read_pc) (struct regcache *regcache);

  /* Write PC to REGCACHE.  */
  void (*write_pc) (struct regcache *regcache, CORE_ADDR pc);

  /* Return true if THREAD is known to be stopped now.  */
  int (*thread_stopped) (struct thread_info *thread);

  /* Read Thread Information Block address.  */
  int (*get_tib_address) (ptid_t ptid, CORE_ADDR *address);

  /* Pause all threads.  If FREEZE, arrange for any resume attempt to
     be ignored until an unpause_all call unfreezes threads again.
     There can be nested calls to pause_all, so a freeze counter
     should be maintained.  */
  void (*pause_all) (int freeze);

  /* Unpause all threads.  Threads that hadn't been resumed by the
     client should be left stopped.  Basically a pause/unpause call
     pair should not end up resuming threads that were stopped before
     the pause call.  */
  void (*unpause_all) (int unfreeze);

  /* Stabilize all threads.  That is, force them out of jump pads.  */
  void (*stabilize_threads) (void);

  /* Install a fast tracepoint jump pad.  TPOINT is the address of the
     tracepoint internal object as used by the IPA agent.  TPADDR is
     the address of tracepoint.  COLLECTOR is address of the function
     the jump pad redirects to.  LOCKADDR is the address of the jump
     pad lock object.  ORIG_SIZE is the size in bytes of the
     instruction at TPADDR.  JUMP_ENTRY points to the address of the
     jump pad entry, and on return holds the address past the end of
     the created jump pad.  If a trampoline is created by the function,
     then TRAMPOLINE and TRAMPOLINE_SIZE return the address and size of
     the trampoline, else they remain unchanged.  JJUMP_PAD_INSN is a
     buffer containing a copy of the instruction at TPADDR.
     ADJUST_INSN_ADDR and ADJUST_INSN_ADDR_END are output parameters that
     return the address range where the instruction at TPADDR was relocated
     to.  If an error occurs, the ERR may be used to pass on an error
     message.  */
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

  /* Returns true if the target supports disabling randomization.  */
  int (*supports_disable_randomization) (void);

  /* Return the minimum length of an instruction that can be safely overwritten
     for use as a fast tracepoint.  */
  int (*get_min_fast_tracepoint_insn_len) (void);

  /* Read solib info on SVR4 platforms.  */
  int (*qxfer_libraries_svr4) (const char *annex, unsigned char *readbuf,
			       unsigned const char *writebuf,
			       CORE_ADDR offset, int len);

  /* Return true if target supports debugging agent.  */
  int (*supports_agent) (void);

  /* Check whether the target supports branch tracing.  */
  int (*supports_btrace) (struct target_ops *, enum btrace_format);

  /* Enable branch tracing for PTID based on CONF and allocate a branch trace
     target information struct for reading and for disabling branch trace.  */
  struct btrace_target_info *(*enable_btrace)
    (ptid_t ptid, const struct btrace_config *conf);

  /* Disable branch tracing.
     Returns zero on success, non-zero otherwise.  */
  int (*disable_btrace) (struct btrace_target_info *tinfo);

  /* Read branch trace data into buffer.
     Return 0 on success; print an error message into BUFFER and return -1,
     otherwise.  */
  int (*read_btrace) (struct btrace_target_info *, struct buffer *,
		      enum btrace_read_type type);

  /* Read the branch trace configuration into BUFFER.
     Return 0 on success; print an error message into BUFFER and return -1
     otherwise.  */
  int (*read_btrace_conf) (const struct btrace_target_info *, struct buffer *);

  /* Return true if target supports range stepping.  */
  int (*supports_range_stepping) (void);

  /* Return the full absolute name of the executable file that was
     run to create the process PID.  If the executable file cannot
     be determined, NULL is returned.  Otherwise, a pointer to a
     character string containing the pathname is returned.  This
     string should be copied into a buffer by the client if the string
     will not be immediately used, or if it must persist.  */
  char *(*pid_to_exec_file) (int pid);

  /* Multiple-filesystem-aware open.  Like open(2), but operating in
     the filesystem as it appears to process PID.  Systems where all
     processes share a common filesystem should set this to NULL.
     If NULL, the caller should fall back to open(2).  */
  int (*multifs_open) (int pid, const char *filename,
		       int flags, mode_t mode);

  /* Multiple-filesystem-aware unlink.  Like unlink(2), but operates
     in the filesystem as it appears to process PID.  Systems where
     all processes share a common filesystem should set this to NULL.
     If NULL, the caller should fall back to unlink(2).  */
  int (*multifs_unlink) (int pid, const char *filename);

  /* Multiple-filesystem-aware readlink.  Like readlink(2), but
     operating in the filesystem as it appears to process PID.
     Systems where all processes share a common filesystem should
     set this to NULL.  If NULL, the caller should fall back to
     readlink(2).  */
  ssize_t (*multifs_readlink) (int pid, const char *filename,
			       char *buf, size_t bufsiz);

  /* Return the breakpoint kind for this target based on PC.  The PCPTR is
     adjusted to the real memory location in case a flag (e.g., the Thumb bit on
     ARM) was present in the PC.  */
  int (*breakpoint_kind_from_pc) (CORE_ADDR *pcptr);

  /* Return the software breakpoint from KIND.  KIND can have target
     specific meaning like the Z0 kind parameter.
     SIZE is set to the software breakpoint's length in memory.  */
  const gdb_byte *(*sw_breakpoint_from_kind) (int kind, int *size);

  /* Return the thread's name, or NULL if the target is unable to determine it.
     The returned value must not be freed by the caller.  */
  const char *(*thread_name) (ptid_t thread);

  /* Return the breakpoint kind for this target based on the current
     processor state (e.g. the current instruction mode on ARM) and the
     PC.  The PCPTR is adjusted to the real memory location in case a flag
     (e.g., the Thumb bit on ARM) is present in the PC.  */
  int (*breakpoint_kind_from_current_state) (CORE_ADDR *pcptr);

  /* Returns true if the target can software single step.  */
  int (*supports_software_single_step) (void);

  /* Return 1 if the target supports catch syscall, 0 (or leave the
     callback NULL) otherwise.  */
  int (*supports_catch_syscall) (void);

  /* Return tdesc index for IPA.  */
  int (*get_ipa_tdesc_idx) (void);
};

extern struct target_ops *the_target;

void set_target_ops (struct target_ops *);

#define create_inferior(program, args) \
  (*the_target->create_inferior) (program, args)

#define target_post_create_inferior()			 \
  do							 \
    {							 \
      if (the_target->post_create_inferior != NULL)	 \
	(*the_target->post_create_inferior) ();		 \
    } while (0)

#define myattach(pid) \
  (*the_target->attach) (pid)

int kill_inferior (int);

#define target_supports_fork_events() \
  (the_target->supports_fork_events ? \
   (*the_target->supports_fork_events) () : 0)

#define target_supports_vfork_events() \
  (the_target->supports_vfork_events ? \
   (*the_target->supports_vfork_events) () : 0)

#define target_supports_exec_events() \
  (the_target->supports_exec_events ? \
   (*the_target->supports_exec_events) () : 0)

#define target_handle_new_gdb_connection()		 \
  do							 \
    {							 \
      if (the_target->handle_new_gdb_connection != NULL) \
	(*the_target->handle_new_gdb_connection) ();	 \
    } while (0)

#define detach_inferior(pid) \
  (*the_target->detach) (pid)

#define mythread_alive(pid) \
  (*the_target->thread_alive) (pid)

#define fetch_inferior_registers(regcache, regno)	\
  (*the_target->fetch_registers) (regcache, regno)

#define store_inferior_registers(regcache, regno) \
  (*the_target->store_registers) (regcache, regno)

#define join_inferior(pid) \
  (*the_target->join) (pid)

#define target_supports_non_stop() \
  (the_target->supports_non_stop ? (*the_target->supports_non_stop ) () : 0)

#define target_async(enable) \
  (the_target->async ? (*the_target->async) (enable) : 0)

#define target_process_qsupported(features, count)	\
  do							\
    {							\
      if (the_target->process_qsupported)		\
	the_target->process_qsupported (features, count); \
    } while (0)

#define target_supports_catch_syscall()              	\
  (the_target->supports_catch_syscall ?			\
   (*the_target->supports_catch_syscall) () : 0)

#define target_get_ipa_tdesc_idx()			\
  (the_target->get_ipa_tdesc_idx			\
   ? (*the_target->get_ipa_tdesc_idx) () : 0)

#define target_supports_tracepoints()			\
  (the_target->supports_tracepoints			\
   ? (*the_target->supports_tracepoints) () : 0)

#define target_supports_fast_tracepoints()		\
  (the_target->install_fast_tracepoint_jump_pad != NULL)

#define target_get_min_fast_tracepoint_insn_len()	\
  (the_target->get_min_fast_tracepoint_insn_len		\
   ? (*the_target->get_min_fast_tracepoint_insn_len) () : 0)

#define thread_stopped(thread) \
  (*the_target->thread_stopped) (thread)

#define pause_all(freeze)			\
  do						\
    {						\
      if (the_target->pause_all)		\
	(*the_target->pause_all) (freeze);	\
    } while (0)

#define unpause_all(unfreeze)			\
  do						\
    {						\
      if (the_target->unpause_all)		\
	(*the_target->unpause_all) (unfreeze);	\
    } while (0)

#define stabilize_threads()			\
  do						\
    {						\
      if (the_target->stabilize_threads)     	\
	(*the_target->stabilize_threads) ();  	\
    } while (0)

#define install_fast_tracepoint_jump_pad(tpoint, tpaddr,		\
					 collector, lockaddr,		\
					 orig_size,			\
					 jump_entry,			\
					 trampoline, trampoline_size,	\
					 jjump_pad_insn,		\
					 jjump_pad_insn_size,		\
					 adjusted_insn_addr,		\
					 adjusted_insn_addr_end,	\
					 err)				\
  (*the_target->install_fast_tracepoint_jump_pad) (tpoint, tpaddr,	\
						   collector,lockaddr,	\
						   orig_size, jump_entry, \
						   trampoline,		\
						   trampoline_size,	\
						   jjump_pad_insn,	\
						   jjump_pad_insn_size, \
						   adjusted_insn_addr,	\
						   adjusted_insn_addr_end, \
						   err)

#define target_emit_ops() \
  (the_target->emit_ops ? (*the_target->emit_ops) () : NULL)

#define target_supports_disable_randomization() \
  (the_target->supports_disable_randomization ? \
   (*the_target->supports_disable_randomization) () : 0)

#define target_supports_agent() \
  (the_target->supports_agent ? \
   (*the_target->supports_agent) () : 0)

#define target_supports_btrace(format)			\
  (the_target->supports_btrace				\
   ? (*the_target->supports_btrace) (the_target, format) : 0)

#define target_enable_btrace(ptid, conf) \
  (*the_target->enable_btrace) (ptid, conf)

#define target_disable_btrace(tinfo) \
  (*the_target->disable_btrace) (tinfo)

#define target_read_btrace(tinfo, buffer, type)	\
  (*the_target->read_btrace) (tinfo, buffer, type)

#define target_read_btrace_conf(tinfo, buffer)	\
  (*the_target->read_btrace_conf) (tinfo, buffer)

#define target_supports_range_stepping() \
  (the_target->supports_range_stepping ? \
   (*the_target->supports_range_stepping) () : 0)

#define target_supports_stopped_by_sw_breakpoint() \
  (the_target->supports_stopped_by_sw_breakpoint ? \
   (*the_target->supports_stopped_by_sw_breakpoint) () : 0)

#define target_stopped_by_sw_breakpoint() \
  (the_target->stopped_by_sw_breakpoint ? \
   (*the_target->stopped_by_sw_breakpoint) () : 0)

#define target_supports_stopped_by_hw_breakpoint() \
  (the_target->supports_stopped_by_hw_breakpoint ? \
   (*the_target->supports_stopped_by_hw_breakpoint) () : 0)

#define target_supports_hardware_single_step() \
  (the_target->supports_hardware_single_step ? \
   (*the_target->supports_hardware_single_step) () : 0)

#define target_stopped_by_hw_breakpoint() \
  (the_target->stopped_by_hw_breakpoint ? \
   (*the_target->stopped_by_hw_breakpoint) () : 0)

#define target_breakpoint_kind_from_pc(pcptr) \
  (the_target->breakpoint_kind_from_pc \
   ? (*the_target->breakpoint_kind_from_pc) (pcptr) \
   : default_breakpoint_kind_from_pc (pcptr))

#define target_breakpoint_kind_from_current_state(pcptr) \
  (the_target->breakpoint_kind_from_current_state \
   ? (*the_target->breakpoint_kind_from_current_state) (pcptr) \
   : target_breakpoint_kind_from_pc (pcptr))

#define target_supports_software_single_step() \
  (the_target->supports_software_single_step ? \
   (*the_target->supports_software_single_step) () : 0)

/* Start non-stop mode, returns 0 on success, -1 on failure.   */

int start_non_stop (int nonstop);

ptid_t mywait (ptid_t ptid, struct target_waitstatus *ourstatus, int options,
	       int connected_wait);

/* Prepare to read or write memory from the inferior process.  See the
   corresponding target_ops methods for more details.  */

int prepare_to_access_memory (void);
void done_accessing_memory (void);

#define target_core_of_thread(ptid)		\
  (the_target->core_of_thread ? (*the_target->core_of_thread) (ptid) \
   : -1)

#define target_thread_name(ptid)                                \
  (the_target->thread_name ? (*the_target->thread_name) (ptid)  \
   : NULL)

int read_inferior_memory (CORE_ADDR memaddr, unsigned char *myaddr, int len);

int write_inferior_memory (CORE_ADDR memaddr, const unsigned char *myaddr,
			   int len);

int set_desired_thread (int id);

const char *target_pid_to_str (ptid_t);

int target_can_do_hardware_single_step (void);

int default_breakpoint_kind_from_pc (CORE_ADDR *pcptr);

#endif /* TARGET_H */
