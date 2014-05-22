/* Target operations for the remote server for GDB.
   Copyright (C) 2002-2013 Free Software Foundation, Inc.

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

struct emit_ops;
struct btrace_target_info;
struct buffer;

/* Ways to "resume" a thread.  */

enum resume_kind
{
  /* Thread should continue.  */
  resume_continue,

  /* Thread should single-step.  */
  resume_step,

  /* Thread should be stopped.  */
  resume_stop
};

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
};

/* Generally, what has the program done?  */
enum target_waitkind
  {
    /* The program has exited.  The exit status is in
       value.integer.  */
    TARGET_WAITKIND_EXITED,

    /* The program has stopped with a signal.  Which signal is in
       value.sig.  */
    TARGET_WAITKIND_STOPPED,

    /* The program has terminated with a signal.  Which signal is in
       value.sig.  */
    TARGET_WAITKIND_SIGNALLED,

    /* The program is letting us know that it dynamically loaded
       something.  */
    TARGET_WAITKIND_LOADED,

    /* The program has exec'ed a new executable file.  The new file's
       pathname is pointed to by value.execd_pathname.  */
    TARGET_WAITKIND_EXECD,

    /* Nothing of interest to GDB happened, but we stopped anyway.  */
    TARGET_WAITKIND_SPURIOUS,

    /* An event has occurred, but we should wait again.  In this case,
       we want to go back to the event loop and wait there for another
       event from the inferior.  */
    TARGET_WAITKIND_IGNORE
  };

struct target_waitstatus
  {
    enum target_waitkind kind;

    /* Forked child pid, execd pathname, exit status or signal number.  */
    union
      {
	int integer;
	enum gdb_signal sig;
	ptid_t related_pid;
	char *execd_pathname;
      }
    value;
  };

/* Options that can be passed to target_ops->wait.  */

#define TARGET_WNOHANG 1

struct target_ops
{
  /* Start a new process.

     PROGRAM is a path to the program to execute.
     ARGS is a standard NULL-terminated array of arguments,
     to be passed to the inferior as ``argv''.

     Returns the new PID on success, -1 on failure.  Registers the new
     process with the process list.  */

  int (*create_inferior) (char *program, char **args);

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
     such as gdb_read_memory/gdb_write_memory, or the insert_point
     callbacks.

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

  /* Insert and remove a break or watchpoint.
     Returns 0 on success, -1 on failure and 1 on unsupported.
     The type is coded as follows:
       '0' - software-breakpoint
       '1' - hardware-breakpoint
       '2' - write watchpoint
       '3' - read watchpoint
       '4' - access watchpoint  */

  int (*insert_point) (char type, CORE_ADDR addr, int len);
  int (*remove_point) (char type, CORE_ADDR addr, int len);

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

  /* If not NULL, target-specific routine to process monitor command.
     Returns 1 if handled, or 0 to perform default processing.  */
  int (*handle_monitor_command) (char *);

  /* Returns the core given a thread, or -1 if not known.  */
  int (*core_of_thread) (ptid_t);

  /* Read loadmaps.  Read LEN bytes at OFFSET into a buffer at MYADDR.  */
  int (*read_loadmap) (const char *annex, CORE_ADDR offset,
		       unsigned char *myaddr, unsigned int len);

  /* Target specific qSupported support.  */
  void (*process_qsupported) (const char *);

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

  /* Cancel all pending breakpoints hits in all threads.  */
  void (*cancel_breakpoints) (void);

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
  int (*supports_btrace) (void);

  /* Enable branch tracing for @ptid and allocate a branch trace target
     information struct for reading and for disabling branch trace.  */
  struct btrace_target_info *(*enable_btrace) (ptid_t ptid);

  /* Disable branch tracing.  */
  int (*disable_btrace) (struct btrace_target_info *tinfo);

  /* Read branch trace data into buffer.  We use an int to specify the type
     to break a cyclic dependency.  */
  void (*read_btrace) (struct btrace_target_info *, struct buffer *, int type);

};

extern struct target_ops *the_target;

void set_target_ops (struct target_ops *);

#define create_inferior(program, args) \
  (*the_target->create_inferior) (program, args)

#define myattach(pid) \
  (*the_target->attach) (pid)

int kill_inferior (int);

#define detach_inferior(pid) \
  (*the_target->detach) (pid)

#define mourn_inferior(PROC) \
  (*the_target->mourn) (PROC)

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

#define target_supports_multi_process() \
  (the_target->supports_multi_process ? \
   (*the_target->supports_multi_process) () : 0)

#define target_process_qsupported(query)		\
  do							\
    {							\
      if (the_target->process_qsupported)		\
	the_target->process_qsupported (query);		\
    } while (0)

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

#define cancel_breakpoints()			\
  do						\
    {						\
      if (the_target->cancel_breakpoints)     	\
	(*the_target->cancel_breakpoints) ();  	\
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

#define target_supports_btrace() \
  (the_target->supports_btrace ? (*the_target->supports_btrace) () : 0)

#define target_enable_btrace(ptid) \
  (*the_target->enable_btrace) (ptid)

#define target_disable_btrace(tinfo) \
  (*the_target->disable_btrace) (tinfo)

#define target_read_btrace(tinfo, buffer, type)	\
  (*the_target->read_btrace) (tinfo, buffer, type)

/* Start non-stop mode, returns 0 on success, -1 on failure.   */

int start_non_stop (int nonstop);

ptid_t mywait (ptid_t ptid, struct target_waitstatus *ourstatus, int options,
	       int connected_wait);

#define prepare_to_access_memory()		\
  (the_target->prepare_to_access_memory		\
   ? (*the_target->prepare_to_access_memory) () \
   : 0)

#define done_accessing_memory()				\
  do							\
    {							\
      if (the_target->done_accessing_memory)     	\
	(*the_target->done_accessing_memory) ();  	\
    } while (0)

#define target_core_of_thread(ptid)		\
  (the_target->core_of_thread ? (*the_target->core_of_thread) (ptid) \
   : -1)

int read_inferior_memory (CORE_ADDR memaddr, unsigned char *myaddr, int len);

int write_inferior_memory (CORE_ADDR memaddr, const unsigned char *myaddr,
			   int len);

void set_desired_inferior (int id);

const char *target_pid_to_str (ptid_t);

const char *target_waitstatus_to_string (const struct target_waitstatus *);

#endif /* TARGET_H */
