/* Variables that describe the inferior process running under GDB:
   Where it is, why it stopped, and how to step it.
   Copyright 1986, 1989, 1992, 1996, 1998 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#if !defined (INFERIOR_H)
#define INFERIOR_H 1

/* For bpstat.  */
#include "breakpoint.h"

/* For enum target_signal.  */
#include "target.h"

/* Structure in which to save the status of the inferior.  Create/Save
   through "save_inferior_status", restore through
   "restore_inferior_status".

   This pair of routines should be called around any transfer of
   control to the inferior which you don't want showing up in your
   control variables.  */

struct inferior_status;

extern struct inferior_status *save_inferior_status PARAMS ((int));

extern void restore_inferior_status PARAMS ((struct inferior_status *));

extern void discard_inferior_status PARAMS ((struct inferior_status *));

extern void write_inferior_status_register PARAMS ((struct inferior_status * inf_status, int regno, LONGEST val));

/* This macro gives the number of registers actually in use by the
   inferior.  This may be less than the total number of registers,
   perhaps depending on the actual CPU in use or program being run.  */

#ifndef ARCH_NUM_REGS
#define ARCH_NUM_REGS NUM_REGS
#endif

extern void set_sigint_trap PARAMS ((void));

extern void clear_sigint_trap PARAMS ((void));

extern void set_sigio_trap PARAMS ((void));

extern void clear_sigio_trap PARAMS ((void));

/* File name for default use for standard in/out in the inferior.  */

extern char *inferior_io_terminal;

/* Pid of our debugged inferior, or 0 if no inferior now.  */

extern int inferior_pid;

/* Is the inferior running right now, as a result of a 'run&',
   'continue&' etc command? This is used in asycn gdb to determine
   whether a command that the user enters while the target is running
   is allowed or not. */
extern int target_executing;

/* Are we simulating synchronous execution? This is used in async gdb
   to implement the 'run', 'continue' etc commands, which will not
   redisplay the prompt until the execution is actually over. */
extern int sync_execution;

/* This is only valid when inferior_pid is non-zero.

   If this is 0, then exec events should be noticed and responded to
   by the debugger (i.e., be reported to the user).

   If this is > 0, then that many subsequent exec events should be
   ignored (i.e., not be reported to the user).
 */
extern int inferior_ignoring_startup_exec_events;

/* This is only valid when inferior_ignoring_startup_exec_events is
   zero.

   Some targets (stupidly) report more than one exec event per actual
   call to an event() system call.  If only the last such exec event
   need actually be noticed and responded to by the debugger (i.e.,
   be reported to the user), then this is the number of "leading"
   exec events which should be ignored.
 */
extern int inferior_ignoring_leading_exec_events;

/* Inferior environment. */

extern struct environ *inferior_environ;

/* Character array containing an image of the inferior programs'
   registers. */

extern char *registers;

/* Character array containing the current state of each register
   (unavailable<0, valid=0, invalid>0). */

extern signed char *register_valid;

extern void clear_proceed_status PARAMS ((void));

extern void proceed PARAMS ((CORE_ADDR, enum target_signal, int));

extern void kill_inferior PARAMS ((void));

extern void generic_mourn_inferior PARAMS ((void));

extern void terminal_ours PARAMS ((void));

extern int run_stack_dummy PARAMS ((CORE_ADDR, char *));

extern CORE_ADDR read_pc PARAMS ((void));

extern CORE_ADDR read_pc_pid PARAMS ((int));

extern CORE_ADDR generic_target_read_pc PARAMS ((int));

extern void write_pc PARAMS ((CORE_ADDR));

extern void write_pc_pid PARAMS ((CORE_ADDR, int));

extern void generic_target_write_pc PARAMS ((CORE_ADDR, int));

extern CORE_ADDR read_sp PARAMS ((void));

extern CORE_ADDR generic_target_read_sp PARAMS ((void));

extern void write_sp PARAMS ((CORE_ADDR));

extern void generic_target_write_sp PARAMS ((CORE_ADDR));

extern CORE_ADDR read_fp PARAMS ((void));

extern CORE_ADDR generic_target_read_fp PARAMS ((void));

extern void write_fp PARAMS ((CORE_ADDR));

extern void generic_target_write_fp PARAMS ((CORE_ADDR));

extern void wait_for_inferior PARAMS ((void));

extern void fetch_inferior_event PARAMS ((void *));

extern void init_wait_for_inferior PARAMS ((void));

extern void close_exec_file PARAMS ((void));

extern void reopen_exec_file PARAMS ((void));

/* The `resume' routine should only be called in special circumstances.
   Normally, use `proceed', which handles a lot of bookkeeping.  */

extern void resume PARAMS ((int, enum target_signal));

/* From misc files */

extern void store_inferior_registers PARAMS ((int));

extern void fetch_inferior_registers PARAMS ((int));

extern void solib_create_inferior_hook PARAMS ((void));

extern void child_terminal_info PARAMS ((char *, int));

extern void term_info PARAMS ((char *, int));

extern void terminal_ours_for_output PARAMS ((void));

extern void terminal_inferior PARAMS ((void));

extern void terminal_init_inferior PARAMS ((void));

extern void terminal_init_inferior_with_pgrp PARAMS ((int pgrp));

/* From infptrace.c or infttrace.c */

extern int attach PARAMS ((int));

#if !defined(REQUIRE_ATTACH)
#define REQUIRE_ATTACH attach
#endif

#if !defined(REQUIRE_DETACH)
#define REQUIRE_DETACH(pid,siggnal) detach (siggnal)
#endif

extern void detach PARAMS ((int));

/* PTRACE method of waiting for inferior process.  */
int ptrace_wait PARAMS ((int, int *));

extern void child_resume PARAMS ((int, int, enum target_signal));

#ifndef PTRACE_ARG3_TYPE
#define PTRACE_ARG3_TYPE int	/* Correct definition for most systems. */
#endif

extern int call_ptrace PARAMS ((int, int, PTRACE_ARG3_TYPE, int));

extern void pre_fork_inferior PARAMS ((void));

/* From procfs.c */

extern int proc_iterate_over_mappings PARAMS ((int (*)(int, CORE_ADDR)));

extern int procfs_first_available PARAMS ((void));

/* From fork-child.c */

extern void fork_inferior PARAMS ((char *, char *, char **,
				   void (*)(void),
				   void (*)(int),
				   void (*)(void),
				   char *));


extern void
clone_and_follow_inferior PARAMS ((int, int *));

extern void startup_inferior PARAMS ((int));

/* From inflow.c */

extern void new_tty_prefork PARAMS ((char *));

extern int gdb_has_a_terminal PARAMS ((void));

/* From infrun.c */

extern void start_remote PARAMS ((void));

extern void normal_stop PARAMS ((void));

extern int signal_stop_state PARAMS ((int));

extern int signal_print_state PARAMS ((int));

extern int signal_pass_state PARAMS ((int));

extern int signal_stop_update PARAMS ((int, int));

extern int signal_print_update PARAMS ((int, int));

extern int signal_pass_update PARAMS ((int, int));

/* From infcmd.c */

extern void tty_command PARAMS ((char *, int));

extern void attach_command PARAMS ((char *, int));

/* Last signal that the inferior received (why it stopped).  */

extern enum target_signal stop_signal;

/* Address at which inferior stopped.  */

extern CORE_ADDR stop_pc;

/* Chain containing status of breakpoint(s) that we have stopped at.  */

extern bpstat stop_bpstat;

/* Flag indicating that a command has proceeded the inferior past the
   current breakpoint.  */

extern int breakpoint_proceeded;

/* Nonzero if stopped due to a step command.  */

extern int stop_step;

/* Nonzero if stopped due to completion of a stack dummy routine.  */

extern int stop_stack_dummy;

/* Nonzero if program stopped due to a random (unexpected) signal in
   inferior process.  */

extern int stopped_by_random_signal;

/* Range to single step within.
   If this is nonzero, respond to a single-step signal
   by continuing to step if the pc is in this range.

   If step_range_start and step_range_end are both 1, it means to step for
   a single instruction (FIXME: it might clean up wait_for_inferior in a
   minor way if this were changed to the address of the instruction and
   that address plus one.  But maybe not.).  */

extern CORE_ADDR step_range_start;	/* Inclusive */
extern CORE_ADDR step_range_end;	/* Exclusive */

/* Stack frame address as of when stepping command was issued.
   This is how we know when we step into a subroutine call,
   and how to set the frame for the breakpoint used to step out.  */

extern CORE_ADDR step_frame_address;

/* Our notion of the current stack pointer.  */

extern CORE_ADDR step_sp;

/* 1 means step over all subroutine calls.
   -1 means step over calls to undebuggable functions.  */

extern int step_over_calls;

/* If stepping, nonzero means step count is > 1
   so don't print frame next time inferior stops
   if it stops due to stepping.  */

extern int step_multi;

/* Nonzero means expecting a trap and caller will handle it themselves.
   It is used after attach, due to attaching to a process;
   when running in the shell before the child program has been exec'd;
   and when running some kinds of remote stuff (FIXME?).  */

extern int stop_soon_quietly;

/* Nonzero if proceed is being used for a "finish" command or a similar
   situation when stop_registers should be saved.  */

extern int proceed_to_finish;

/* Save register contents here when about to pop a stack dummy frame,
   if-and-only-if proceed_to_finish is set.
   Thus this contains the return value from the called function (assuming
   values are returned in a register).  */

extern char *stop_registers;

/* Nonzero if the child process in inferior_pid was attached rather
   than forked.  */

extern int attach_flag;

/* Sigtramp is a routine that the kernel calls (which then calls the
   signal handler).  On most machines it is a library routine that
   is linked into the executable.

   This macro, given a program counter value and the name of the
   function in which that PC resides (which can be null if the
   name is not known), returns nonzero if the PC and name show
   that we are in sigtramp.

   On most machines just see if the name is sigtramp (and if we have
   no name, assume we are not in sigtramp).  */
#if !defined (IN_SIGTRAMP)
#if defined (SIGTRAMP_START)
#define IN_SIGTRAMP(pc, name) \
       ((pc) >= SIGTRAMP_START(pc)   \
        && (pc) < SIGTRAMP_END(pc) \
        )
#else
#define IN_SIGTRAMP(pc, name) \
       (name && STREQ ("_sigtramp", name))
#endif
#endif

/* Possible values for CALL_DUMMY_LOCATION.  */
#define ON_STACK 1
#define BEFORE_TEXT_END 2
#define AFTER_TEXT_END 3
#define AT_ENTRY_POINT 4

#if !defined (USE_GENERIC_DUMMY_FRAMES)
#define USE_GENERIC_DUMMY_FRAMES 0
#endif

#if !defined (CALL_DUMMY_LOCATION)
#define CALL_DUMMY_LOCATION ON_STACK
#endif /* No CALL_DUMMY_LOCATION.  */

#if !defined (CALL_DUMMY_ADDRESS)
#define CALL_DUMMY_ADDRESS() (internal_error ("CALL_DUMMY_ADDRESS"), 0)
#endif
#if !defined (CALL_DUMMY_START_OFFSET)
#define CALL_DUMMY_START_OFFSET (internal_error ("CALL_DUMMY_START_OFFSET"), 0)
#endif
#if !defined (CALL_DUMMY_BREAKPOINT_OFFSET)
#define CALL_DUMMY_BREAKPOINT_OFFSET_P (0)
#define CALL_DUMMY_BREAKPOINT_OFFSET (internal_error ("CALL_DUMMY_BREAKPOINT_OFFSET"), 0)
#endif
#if !defined CALL_DUMMY_BREAKPOINT_OFFSET_P
#define CALL_DUMMY_BREAKPOINT_OFFSET_P (1)
#endif
#if !defined (CALL_DUMMY_LENGTH)
#define CALL_DUMMY_LENGTH (internal_error ("CALL_DUMMY_LENGTH"), 0)
#endif

#if defined (CALL_DUMMY_STACK_ADJUST)
#if !defined (CALL_DUMMY_STACK_ADJUST_P)
#define CALL_DUMMY_STACK_ADJUST_P (1)
#endif
#endif
#if !defined (CALL_DUMMY_STACK_ADJUST)
#define CALL_DUMMY_STACK_ADJUST (internal_error ("CALL_DUMMY_STACK_ADJUST"), 0)
#endif
#if !defined (CALL_DUMMY_STACK_ADJUST_P)
#define CALL_DUMMY_STACK_ADJUST_P (0)
#endif

#if !defined (CALL_DUMMY_P)
#if defined (CALL_DUMMY)
#define CALL_DUMMY_P 1
#else
#define CALL_DUMMY_P 0
#endif
#endif

#if !defined (CALL_DUMMY_WORDS)
#if defined (CALL_DUMMY)
extern LONGEST call_dummy_words[];
#define CALL_DUMMY_WORDS (call_dummy_words)
#else
#define CALL_DUMMY_WORDS (internal_error ("CALL_DUMMY_WORDS"), (void*) 0)
#endif
#endif

#if !defined (SIZEOF_CALL_DUMMY_WORDS)
#if defined (CALL_DUMMY)
extern int sizeof_call_dummy_words;
#define SIZEOF_CALL_DUMMY_WORDS (sizeof_call_dummy_words)
#else
#define SIZEOF_CALL_DUMMY_WORDS (internal_error ("SIZEOF_CALL_DUMMY_WORDS"), 0)
#endif
#endif

#if !defined PUSH_DUMMY_FRAME
#define PUSH_DUMMY_FRAME (internal_error ("PUSH_DUMMY_FRAME"), 0)
#endif

#if !defined FIX_CALL_DUMMY
#define FIX_CALL_DUMMY(a1,a2,a3,a4,a5,a6,a7) (internal_error ("FIX_CALL_DUMMY"), 0)
#endif

#if !defined STORE_STRUCT_RETURN
#define STORE_STRUCT_RETURN(a1,a2) (internal_error ("STORE_STRUCT_RETURN"), 0)
#endif


/* Are we in a call dummy? */

extern int pc_in_call_dummy_before_text_end PARAMS ((CORE_ADDR pc, CORE_ADDR sp, CORE_ADDR frame_address));
#if !GDB_MULTI_ARCH
#if !defined (PC_IN_CALL_DUMMY) && CALL_DUMMY_LOCATION == BEFORE_TEXT_END
#define PC_IN_CALL_DUMMY(pc, sp, frame_address) pc_in_call_dummy_before_text_end (pc, sp, frame_address)
#endif /* Before text_end.  */
#endif

extern int pc_in_call_dummy_after_text_end PARAMS ((CORE_ADDR pc, CORE_ADDR sp, CORE_ADDR frame_address));
#if !GDB_MULTI_ARCH
#if !defined (PC_IN_CALL_DUMMY) && CALL_DUMMY_LOCATION == AFTER_TEXT_END
#define PC_IN_CALL_DUMMY(pc, sp, frame_address) pc_in_call_dummy_after_text_end (pc, sp, frame_address)
#endif
#endif

extern int pc_in_call_dummy_on_stack PARAMS ((CORE_ADDR pc, CORE_ADDR sp, CORE_ADDR frame_address));
#if !GDB_MULTI_ARCH
#if !defined (PC_IN_CALL_DUMMY) && CALL_DUMMY_LOCATION == ON_STACK
#define PC_IN_CALL_DUMMY(pc, sp, frame_address) pc_in_call_dummy_on_stack (pc, sp, frame_address)
#endif
#endif

extern int pc_in_call_dummy_at_entry_point PARAMS ((CORE_ADDR pc, CORE_ADDR sp, CORE_ADDR frame_address));
#if !GDB_MULTI_ARCH
#if !defined (PC_IN_CALL_DUMMY) && CALL_DUMMY_LOCATION == AT_ENTRY_POINT
#define PC_IN_CALL_DUMMY(pc, sp, frame_address) pc_in_call_dummy_at_entry_point (pc, sp, frame_address)
#endif
#endif

/* It's often not enough for our clients to know whether the PC is merely
   somewhere within the call dummy.  They may need to know whether the
   call dummy has actually completed.  (For example, wait_for_inferior
   wants to know when it should truly stop because the call dummy has
   completed.  If we're single-stepping because of slow watchpoints,
   then we may find ourselves stopped at the entry of the call dummy,
   and want to continue stepping until we reach the end.)

   Note that this macro is intended for targets (like HP-UX) which
   require more than a single breakpoint in their call dummies, and
   therefore cannot use the CALL_DUMMY_BREAKPOINT_OFFSET mechanism.

   If a target does define CALL_DUMMY_BREAKPOINT_OFFSET, then this
   default implementation of CALL_DUMMY_HAS_COMPLETED is sufficient.
   Else, a target may wish to supply an implementation that works in
   the presense of multiple breakpoints in its call dummy.
 */
#if !defined(CALL_DUMMY_HAS_COMPLETED)
#define CALL_DUMMY_HAS_COMPLETED(pc, sp, frame_address) \
  PC_IN_CALL_DUMMY((pc), (sp), (frame_address))
#endif

/* If STARTUP_WITH_SHELL is set, GDB's "run"
   will attempts to start up the debugee under a shell.
   This is in order for argument-expansion to occur. E.g.,
   (gdb) run *
   The "*" gets expanded by the shell into a list of files.
   While this is a nice feature, it turns out to interact badly
   with some of the catch-fork/catch-exec features we have added.
   In particular, if the shell does any fork/exec's before
   the exec of the target program, that can confuse GDB.
   To disable this feature, set STARTUP_WITH_SHELL to 0.
   To enable this feature, set STARTUP_WITH_SHELL to 1.
   The catch-exec traps expected during start-up will
   be 1 if target is not started up with a shell, 2 if it is.
   - RT
   If you disable this, you need to decrement
   START_INFERIOR_TRAPS_EXPECTED in tm.h. */
#define STARTUP_WITH_SHELL 1
#if !defined(START_INFERIOR_TRAPS_EXPECTED)
#define START_INFERIOR_TRAPS_EXPECTED	2
#endif
#endif /* !defined (INFERIOR_H) */
