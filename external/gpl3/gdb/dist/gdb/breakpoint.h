/* Data structures associated with breakpoints in GDB.
   Copyright (C) 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001,
   2002, 2003, 2004, 2007, 2008, 2009, 2010, 2011
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

#if !defined (BREAKPOINT_H)
#define BREAKPOINT_H 1

#include "frame.h"
#include "value.h"
#include "vec.h"

struct value;
struct block;
struct breakpoint_object;
struct get_number_or_range_state;

/* This is the maximum number of bytes a breakpoint instruction can
   take.  Feel free to increase it.  It's just used in a few places to
   size arrays that should be independent of the target
   architecture.  */

#define	BREAKPOINT_MAX	16


/* Type of breakpoint.  */
/* FIXME In the future, we should fold all other breakpoint-like
   things into here.  This includes:

   * single-step (for machines where we have to simulate single
   stepping) (probably, though perhaps it is better for it to look as
   much as possible like a single-step to wait_for_inferior).  */

enum bptype
  {
    bp_none = 0,		/* Eventpoint has been deleted */
    bp_breakpoint,		/* Normal breakpoint */
    bp_hardware_breakpoint,	/* Hardware assisted breakpoint */
    bp_until,			/* used by until command */
    bp_finish,			/* used by finish command */
    bp_watchpoint,		/* Watchpoint */
    bp_hardware_watchpoint,	/* Hardware assisted watchpoint */
    bp_read_watchpoint,		/* read watchpoint, (hardware assisted) */
    bp_access_watchpoint,	/* access watchpoint, (hardware assisted) */
    bp_longjmp,			/* secret breakpoint to find longjmp() */
    bp_longjmp_resume,		/* secret breakpoint to escape longjmp() */

    /* An internal breakpoint that is installed on the unwinder's
       debug hook.  */
    bp_exception,
    /* An internal breakpoint that is set at the point where an
       exception will land.  */
    bp_exception_resume,

    /* Used by wait_for_inferior for stepping over subroutine calls,
       for stepping over signal handlers, and for skipping
       prologues.  */
    bp_step_resume,

    /* Used to detect when a watchpoint expression has gone out of
       scope.  These breakpoints are usually not visible to the user.

       This breakpoint has some interesting properties:

       1) There's always a 1:1 mapping between watchpoints
       on local variables and watchpoint_scope breakpoints.

       2) It automatically deletes itself and the watchpoint it's
       associated with when hit.

       3) It can never be disabled.  */
    bp_watchpoint_scope,

    /* The breakpoint at the end of a call dummy.  */
    /* FIXME: What if the function we are calling longjmp()s out of
       the call, or the user gets out with the "return" command?  We
       currently have no way of cleaning up the breakpoint in these
       (obscure) situations.  (Probably can solve this by noticing
       longjmp, "return", etc., it's similar to noticing when a
       watchpoint on a local variable goes out of scope (with hardware
       support for watchpoints)).  */
    bp_call_dummy,

    /* A breakpoint set on std::terminate, that is used to catch
       otherwise uncaught exceptions thrown during an inferior call.  */
    bp_std_terminate,

    /* Some dynamic linkers (HP, maybe Solaris) can arrange for special
       code in the inferior to run when significant events occur in the
       dynamic linker (for example a library is loaded or unloaded).

       By placing a breakpoint in this magic code GDB will get control
       when these significant events occur.  GDB can then re-examine
       the dynamic linker's data structures to discover any newly loaded
       dynamic libraries.  */
    bp_shlib_event,

    /* Some multi-threaded systems can arrange for a location in the 
       inferior to be executed when certain thread-related events occur
       (such as thread creation or thread death).

       By placing a breakpoint at one of these locations, GDB will get
       control when these events occur.  GDB can then update its thread
       lists etc.  */

    bp_thread_event,

    /* On the same principal, an overlay manager can arrange to call a
       magic location in the inferior whenever there is an interesting
       change in overlay status.  GDB can update its overlay tables
       and fiddle with breakpoints in overlays when this breakpoint 
       is hit.  */

    bp_overlay_event, 

    /* Master copies of longjmp breakpoints.  These are always installed
       as soon as an objfile containing longjmp is loaded, but they are
       always disabled.  While necessary, temporary clones of bp_longjmp
       type will be created and enabled.  */

    bp_longjmp_master,

    /* Master copies of std::terminate breakpoints.  */
    bp_std_terminate_master,

    /* Like bp_longjmp_master, but for exceptions.  */
    bp_exception_master,

    bp_catchpoint,

    bp_tracepoint,
    bp_fast_tracepoint,
    bp_static_tracepoint,

    /* Event for JIT compiled code generation or deletion.  */
    bp_jit_event,

    /* Breakpoint is placed at the STT_GNU_IFUNC resolver.  When hit GDB
       inserts new bp_gnu_ifunc_resolver_return at the caller.
       bp_gnu_ifunc_resolver is still being kept here as a different thread
       may still hit it before bp_gnu_ifunc_resolver_return is hit by the
       original thread.  */
    bp_gnu_ifunc_resolver,

    /* On its hit GDB now know the resolved address of the target
       STT_GNU_IFUNC function.  Associated bp_gnu_ifunc_resolver can be
       deleted now and the breakpoint moved to the target function entry
       point.  */
    bp_gnu_ifunc_resolver_return,
  };

/* States of enablement of breakpoint.  */

enum enable_state
  {
    bp_disabled,	 /* The eventpoint is inactive, and cannot
			    trigger.  */
    bp_enabled,		 /* The eventpoint is active, and can
			    trigger.  */
    bp_call_disabled,	 /* The eventpoint has been disabled while a
			    call into the inferior is "in flight",
			    because some eventpoints interfere with
			    the implementation of a call on some
			    targets.  The eventpoint will be
			    automatically enabled and reset when the
			    call "lands" (either completes, or stops
			    at another eventpoint).  */
    bp_startup_disabled, /* The eventpoint has been disabled during
			    inferior startup.  This is necessary on
			    some targets where the main executable
			    will get relocated during startup, making
			    breakpoint addresses invalid.  The
			    eventpoint will be automatically enabled
			    and reset once inferior startup is
			    complete.  */
    bp_permanent	 /* There is a breakpoint instruction
			    hard-wired into the target's code.  Don't
			    try to write another breakpoint
			    instruction on top of it, or restore its
			    value.  Step over it using the
			    architecture's SKIP_INSN macro.  */
  };


/* Disposition of breakpoint.  Ie: what to do after hitting it.  */

enum bpdisp
  {
    disp_del,			/* Delete it */
    disp_del_at_next_stop,	/* Delete at next stop, 
				   whether hit or not */
    disp_disable,		/* Disable it */
    disp_donttouch		/* Leave it alone */
  };

enum target_hw_bp_type
  {
    hw_write   = 0, 		/* Common  HW watchpoint */
    hw_read    = 1, 		/* Read    HW watchpoint */
    hw_access  = 2, 		/* Access  HW watchpoint */
    hw_execute = 3		/* Execute HW breakpoint */
  };


/* Information used by targets to insert and remove breakpoints.  */

struct bp_target_info
{
  /* Address space at which the breakpoint was placed.  */
  struct address_space *placed_address_space;

  /* Address at which the breakpoint was placed.  This is normally the
     same as ADDRESS from the bp_location, except when adjustment
     happens in gdbarch_breakpoint_from_pc.  The most common form of
     adjustment is stripping an alternate ISA marker from the PC which
     is used to determine the type of breakpoint to insert.  */
  CORE_ADDR placed_address;

  /* If this is a ranged breakpoint, then this field contains the
     length of the range that will be watched for execution.  */
  int length;

  /* If the breakpoint lives in memory and reading that memory would
     give back the breakpoint, instead of the original contents, then
     the original contents are cached here.  Only SHADOW_LEN bytes of
     this buffer are valid, and only when the breakpoint is inserted.  */
  gdb_byte shadow_contents[BREAKPOINT_MAX];

  /* The length of the data cached in SHADOW_CONTENTS.  */
  int shadow_len;

  /* The size of the placed breakpoint, according to
     gdbarch_breakpoint_from_pc, when the breakpoint was inserted.
     This is generally the same as SHADOW_LEN, unless we did not need
     to read from the target to implement the memory breakpoint
     (e.g. if a remote stub handled the details).  We may still need
     the size to remove the breakpoint safely.  */
  int placed_size;
};

/* GDB maintains two types of information about each breakpoint (or
   watchpoint, or other related event).  The first type corresponds
   to struct breakpoint; this is a relatively high-level structure
   which contains the source location(s), stopping conditions, user
   commands to execute when the breakpoint is hit, and so forth.

   The second type of information corresponds to struct bp_location.
   Each breakpoint has one or (eventually) more locations associated
   with it, which represent target-specific and machine-specific
   mechanisms for stopping the program.  For instance, a watchpoint
   expression may require multiple hardware watchpoints in order to
   catch all changes in the value of the expression being watched.  */

enum bp_loc_type
{
  bp_loc_software_breakpoint,
  bp_loc_hardware_breakpoint,
  bp_loc_hardware_watchpoint,
  bp_loc_other			/* Miscellaneous...  */
};

struct bp_location
{
  /* Chain pointer to the next breakpoint location for
     the same parent breakpoint.  */
  struct bp_location *next;

  /* The reference count.  */
  int refc;

  /* Type of this breakpoint location.  */
  enum bp_loc_type loc_type;

  /* Each breakpoint location must belong to exactly one higher-level
     breakpoint.  This pointer is NULL iff this bp_location is no
     longer attached to a breakpoint.  For example, when a breakpoint
     is deleted, its locations may still be found in the
     moribund_locations list, or if we had stopped for it, in
     bpstats.  */
  struct breakpoint *owner;

  /* Conditional.  Break only if this expression's value is nonzero.
     Unlike string form of condition, which is associated with
     breakpoint, this is associated with location, since if breakpoint
     has several locations, the evaluation of expression can be
     different for different locations.  Only valid for real
     breakpoints; a watchpoint's conditional expression is stored in
     the owner breakpoint object.  */
  struct expression *cond;

  /* This location's address is in an unloaded solib, and so this
     location should not be inserted.  It will be automatically
     enabled when that solib is loaded.  */
  char shlib_disabled; 

  /* Is this particular location enabled.  */
  char enabled;
  
  /* Nonzero if this breakpoint is now inserted.  */
  char inserted;

  /* Nonzero if this is not the first breakpoint in the list
     for the given address.  */
  char duplicate;

  /* If we someday support real thread-specific breakpoints, then
     the breakpoint location will need a thread identifier.  */

  /* Data for specific breakpoint types.  These could be a union, but
     simplicity is more important than memory usage for breakpoints.  */

  /* Architecture associated with this location's address.  May be
     different from the breakpoint architecture.  */
  struct gdbarch *gdbarch;

  /* The program space associated with this breakpoint location
     address.  Note that an address space may be represented in more
     than one program space (e.g. each uClinux program will be given
     its own program space, but there will only be one address space
     for all of them), but we must not insert more than one location
     at the same address in the same address space.  */
  struct program_space *pspace;

  /* Note that zero is a perfectly valid code address on some platforms
     (for example, the mn10200 (OBSOLETE) and mn10300 simulators).  NULL
     is not a special value for this field.  Valid for all types except
     bp_loc_other.  */
  CORE_ADDR address;

  /* For hardware watchpoints, the size of the memory region being
     watched.  For hardware ranged breakpoints, the size of the
     breakpoint range.  */
  int length;

  /* Type of hardware watchpoint.  */
  enum target_hw_bp_type watchpoint_type;

  /* For any breakpoint type with an address, this is the section
     associated with the address.  Used primarily for overlay
     debugging.  */
  struct obj_section *section;

  /* Address at which breakpoint was requested, either by the user or
     by GDB for internal breakpoints.  This will usually be the same
     as ``address'' (above) except for cases in which
     ADJUST_BREAKPOINT_ADDRESS has computed a different address at
     which to place the breakpoint in order to comply with a
     processor's architectual constraints.  */
  CORE_ADDR requested_address;

  char *function_name;

  /* Details of the placed breakpoint, when inserted.  */
  struct bp_target_info target_info;

  /* Similarly, for the breakpoint at an overlay's LMA, if necessary.  */
  struct bp_target_info overlay_target_info;

  /* In a non-stop mode, it's possible that we delete a breakpoint,
     but as we do that, some still running thread hits that breakpoint.
     For that reason, we need to keep locations belonging to deleted
     breakpoints for a bit, so that don't report unexpected SIGTRAP.
     We can't keep such locations forever, so we use a heuristic --
     after we process certain number of inferior events since
     breakpoint was deleted, we retire all locations of that breakpoint.
     This variable keeps a number of events still to go, when
     it becomes 0 this location is retired.  */
  int events_till_retirement;
};

/* This structure is a collection of function pointers that, if available,
   will be called instead of the performing the default action for this
   bptype.  */

struct breakpoint_ops
{
  /* Insert the breakpoint or watchpoint or activate the catchpoint.
     Return 0 for success, 1 if the breakpoint, watchpoint or catchpoint
     type is not supported, -1 for failure.  */
  int (*insert_location) (struct bp_location *);

  /* Remove the breakpoint/catchpoint that was previously inserted
     with the "insert" method above.  Return 0 for success, 1 if the
     breakpoint, watchpoint or catchpoint type is not supported,
     -1 for failure.  */
  int (*remove_location) (struct bp_location *);

  /* Return non-zero if the debugger should tell the user that this
     breakpoint was hit.  */
  int (*breakpoint_hit) (const struct bp_location *, struct address_space *,
			 CORE_ADDR);

  /* Tell how many hardware resources (debug registers) are needed
     for this breakpoint.  If this function is not provided, then
     the breakpoint or watchpoint needs one debug register.  */
  int (*resources_needed) (const struct bp_location *);

  /* The normal print routine for this breakpoint, called when we
     hit it.  */
  enum print_stop_action (*print_it) (struct breakpoint *);

  /* Display information about this breakpoint, for "info
     breakpoints".  */
  void (*print_one) (struct breakpoint *, struct bp_location **);

  /* Display extra information about this breakpoint, below the normal
     breakpoint description in "info breakpoints".

     In the example below, the "address range" line was printed
     by print_one_detail_ranged_breakpoint.

     (gdb) info breakpoints
     Num     Type           Disp Enb Address    What
     2       hw breakpoint  keep y              in main at test-watch.c:70
	     address range: [0x10000458, 0x100004c7]

   */
  void (*print_one_detail) (const struct breakpoint *, struct ui_out *);

  /* Display information about this breakpoint after setting it
     (roughly speaking; this is called from "mention").  */
  void (*print_mention) (struct breakpoint *);

  /* Print to FP the CLI command that recreates this breakpoint.  */
  void (*print_recreate) (struct breakpoint *, struct ui_file *fp);
};

enum watchpoint_triggered
{
  /* This watchpoint definitely did not trigger.  */
  watch_triggered_no = 0,

  /* Some hardware watchpoint triggered, and it might have been this
     one, but we do not know which it was.  */
  watch_triggered_unknown,

  /* This hardware watchpoint definitely did trigger.  */
  watch_triggered_yes  
};

/* This is used to declare the VEC syscalls_to_be_caught.  */
DEF_VEC_I(int);

typedef struct bp_location *bp_location_p;
DEF_VEC_P(bp_location_p);

/* A reference-counted struct command_line.  This lets multiple
   breakpoints share a single command list.  This is an implementation
   detail to the breakpoints module.  */
struct counted_command_line;

/* Some targets (e.g., embedded PowerPC) need two debug registers to set
   a watchpoint over a memory region.  If this flag is true, GDB will use
   only one register per watchpoint, thus assuming that all acesses that
   modify a memory location happen at its starting address. */

extern int target_exact_watchpoints;

/* Note that the ->silent field is not currently used by any commands
   (though the code is in there if it was to be, and set_raw_breakpoint
   does set it to 0).  I implemented it because I thought it would be
   useful for a hack I had to put in; I'm going to leave it in because
   I can see how there might be times when it would indeed be useful */

/* This is for a breakpoint or a watchpoint.  */

struct breakpoint
  {
    struct breakpoint *next;
    /* Type of breakpoint.  */
    enum bptype type;
    /* Zero means disabled; remember the info but don't break here.  */
    enum enable_state enable_state;
    /* What to do with this breakpoint after we hit it.  */
    enum bpdisp disposition;
    /* Number assigned to distinguish breakpoints.  */
    int number;

    /* Location(s) associated with this high-level breakpoint.  */
    struct bp_location *loc;

    /* Line number of this address.  */

    int line_number;

    /* Source file name of this address.  */

    char *source_file;

    /* Non-zero means a silent breakpoint (don't print frame info
       if we stop here).  */
    unsigned char silent;
    /* Non-zero means display ADDR_STRING to the user verbatim.  */
    unsigned char display_canonical;
    /* Number of stops at this breakpoint that should
       be continued automatically before really stopping.  */
    int ignore_count;
    /* Chain of command lines to execute when this breakpoint is
       hit.  */
    struct counted_command_line *commands;
    /* Stack depth (address of frame).  If nonzero, break only if fp
       equals this.  */
    struct frame_id frame_id;

    /* The program space used to set the breakpoint.  */
    struct program_space *pspace;

    /* String we used to set the breakpoint (malloc'd).  */
    char *addr_string;

    /* For a ranged breakpoint, the string we used to find
       the end of the range (malloc'd).  */
    char *addr_string_range_end;

    /* Architecture we used to set the breakpoint.  */
    struct gdbarch *gdbarch;
    /* Language we used to set the breakpoint.  */
    enum language language;
    /* Input radix we used to set the breakpoint.  */
    int input_radix;
    /* String form of the breakpoint condition (malloc'd), or NULL if
       there is no condition.  */
    char *cond_string;
    /* String form of exp to use for displaying to the user
       (malloc'd), or NULL if none.  */
    char *exp_string;
    /* String form to use for reparsing of EXP (malloc'd) or NULL.  */
    char *exp_string_reparse;

    /* The expression we are watching, or NULL if not a watchpoint.  */
    struct expression *exp;
    /* The largest block within which it is valid, or NULL if it is
       valid anywhere (e.g. consists just of global symbols).  */
    struct block *exp_valid_block;
    /* The conditional expression if any.  NULL if not a watchpoint.  */
    struct expression *cond_exp;
    /* The largest block within which it is valid, or NULL if it is
       valid anywhere (e.g. consists just of global symbols).  */
    struct block *cond_exp_valid_block;
    /* Value of the watchpoint the last time we checked it, or NULL
       when we do not know the value yet or the value was not
       readable.  VAL is never lazy.  */
    struct value *val;
    /* Nonzero if VAL is valid.  If VAL_VALID is set but VAL is NULL,
       then an error occurred reading the value.  */
    int val_valid;

    /* Holds the address of the related watchpoint_scope breakpoint
       when using watchpoints on local variables (might the concept of
       a related breakpoint be useful elsewhere, if not just call it
       the watchpoint_scope breakpoint or something like that.
       FIXME).  */
    struct breakpoint *related_breakpoint;

    /* Holds the frame address which identifies the frame this
       watchpoint should be evaluated in, or `null' if the watchpoint
       should be evaluated on the outermost frame.  */
    struct frame_id watchpoint_frame;

    /* Holds the thread which identifies the frame this watchpoint
       should be considered in scope for, or `null_ptid' if the
       watchpoint should be evaluated in all threads.  */
    ptid_t watchpoint_thread;

    /* For hardware watchpoints, the triggered status according to the
       hardware.  */
    enum watchpoint_triggered watchpoint_triggered;

    /* Thread number for thread-specific breakpoint, 
       or -1 if don't care.  */
    int thread;

    /* Ada task number for task-specific breakpoint, 
       or 0 if don't care.  */
    int task;

    /* Count of the number of times this breakpoint was taken, dumped
       with the info, but not used for anything else.  Useful for
       seeing how many times you hit a break prior to the program
       aborting, so you can back up to just before the abort.  */
    int hit_count;

    /* Process id of a child process whose forking triggered this
       catchpoint.  This field is only valid immediately after this
       catchpoint has triggered.  */
    ptid_t forked_inferior_pid;

    /* Filename of a program whose exec triggered this catchpoint.
       This field is only valid immediately after this catchpoint has
       triggered.  */
    char *exec_pathname;

    /* Syscall numbers used for the 'catch syscall' feature.  If no
       syscall has been specified for filtering, its value is NULL.
       Otherwise, it holds a list of all syscalls to be caught.  The
       list elements are allocated with xmalloc.  */
    VEC(int) *syscalls_to_be_caught;

    /* Methods associated with this breakpoint.  */
    struct breakpoint_ops *ops;

    /* Is breakpoint's condition not yet parsed because we found
       no location initially so had no context to parse
       the condition in.  */
    int condition_not_parsed;

    /* Number of times this tracepoint should single-step 
       and collect additional data.  */
    long step_count;

    /* Number of times this tracepoint should be hit before 
       disabling/ending.  */
    int pass_count;

    /* The number of the tracepoint on the target.  */
    int number_on_target;

    /* The static tracepoint marker id, if known.  */
    char *static_trace_marker_id;

    /* LTTng/UST allow more than one marker with the same ID string,
       although it unadvised because it confuses tools.  When setting
       static tracepoints by marker ID, this will record the index in
       the array of markers we found for the given marker ID for which
       this static tracepoint corresponds.  When resetting
       breakpoints, we will use this index to try to find the same
       marker again.  */
    int static_trace_marker_id_idx;

    /* With a Python scripting enabled GDB, store a reference to the
       Python object that has been associated with this breakpoint.
       This is always NULL for a GDB that is not script enabled.  It
       can sometimes be NULL for enabled GDBs as not all breakpoint
       types are tracked by the Python scripting API.  */
    struct breakpoint_object *py_bp_object;

    /* Whether this watchpoint is exact (see target_exact_watchpoints).  */
    int exact;
  };

typedef struct breakpoint *breakpoint_p;
DEF_VEC_P(breakpoint_p);

/* The following stuff is an abstract data type "bpstat" ("breakpoint
   status").  This provides the ability to determine whether we have
   stopped at a breakpoint, and what we should do about it.  */

typedef struct bpstats *bpstat;

/* Clears a chain of bpstat, freeing storage
   of each.  */
extern void bpstat_clear (bpstat *);

/* Return a copy of a bpstat.  Like "bs1 = bs2" but all storage that
   is part of the bpstat is copied as well.  */
extern bpstat bpstat_copy (bpstat);

extern bpstat bpstat_stop_status (struct address_space *aspace,
				  CORE_ADDR pc, ptid_t ptid);

/* This bpstat_what stuff tells wait_for_inferior what to do with a
   breakpoint (a challenging task).

   The enum values order defines priority-like order of the actions.
   Once you've decided that some action is appropriate, you'll never
   go back and decide something of a lower priority is better.  Each
   of these actions is mutually exclusive with the others.  That
   means, that if you find yourself adding a new action class here and
   wanting to tell GDB that you have two simultaneous actions to
   handle, something is wrong, and you probably don't actually need a
   new action type.

   Note that a step resume breakpoint overrides another breakpoint of
   signal handling (see comment in wait_for_inferior at where we set
   the step_resume breakpoint).  */

enum bpstat_what_main_action
  {
    /* Perform various other tests; that is, this bpstat does not
       say to perform any action (e.g. failed watchpoint and nothing
       else).  */
    BPSTAT_WHAT_KEEP_CHECKING,

    /* Remove breakpoints, single step once, then put them back in and
       go back to what we were doing.  It's possible that this should
       be removed from the main_action and put into a separate field,
       to more cleanly handle
       BPSTAT_WHAT_CLEAR_LONGJMP_RESUME_SINGLE.  */
    BPSTAT_WHAT_SINGLE,

    /* Set longjmp_resume breakpoint, remove all other breakpoints,
       and continue.  The "remove all other breakpoints" part is
       required if we are also stepping over another breakpoint as
       well as doing the longjmp handling.  */
    BPSTAT_WHAT_SET_LONGJMP_RESUME,

    /* Clear longjmp_resume breakpoint, then handle as
       BPSTAT_WHAT_KEEP_CHECKING.  */
    BPSTAT_WHAT_CLEAR_LONGJMP_RESUME,

    /* Rather than distinguish between noisy and silent stops here, it
       might be cleaner to have bpstat_print make that decision (also
       taking into account stop_print_frame and source_only).  But the
       implications are a bit scary (interaction with auto-displays,
       etc.), so I won't try it.  */

    /* Stop silently.  */
    BPSTAT_WHAT_STOP_SILENT,

    /* Stop and print.  */
    BPSTAT_WHAT_STOP_NOISY,

    /* Clear step resume breakpoint, and keep checking.  */
    BPSTAT_WHAT_STEP_RESUME,
  };

/* An enum indicating the kind of "stack dummy" stop.  This is a bit
   of a misnomer because only one kind of truly a stack dummy.  */
enum stop_stack_kind
  {
    /* We didn't stop at a stack dummy breakpoint.  */
    STOP_NONE = 0,

    /* Stopped at a stack dummy.  */
    STOP_STACK_DUMMY,

    /* Stopped at std::terminate.  */
    STOP_STD_TERMINATE
  };

struct bpstat_what
  {
    enum bpstat_what_main_action main_action;

    /* Did we hit a call dummy breakpoint?  This only goes with a
       main_action of BPSTAT_WHAT_STOP_SILENT or
       BPSTAT_WHAT_STOP_NOISY (the concept of continuing from a call
       dummy without popping the frame is not a useful one).  */
    enum stop_stack_kind call_dummy;

    /* Used for BPSTAT_WHAT_SET_LONGJMP_RESUME and
       BPSTAT_WHAT_CLEAR_LONGJMP_RESUME.  True if we are handling a
       longjmp, false if we are handling an exception.  */
    int is_longjmp;
  };

/* The possible return values for print_bpstat, print_it_normal,
   print_it_done, print_it_noop.  */
enum print_stop_action
  {
    PRINT_UNKNOWN = -1,
    PRINT_SRC_AND_LOC,
    PRINT_SRC_ONLY,
    PRINT_NOTHING
  };

/* Tell what to do about this bpstat.  */
struct bpstat_what bpstat_what (bpstat);

/* Find the bpstat associated with a breakpoint.  NULL otherwise.  */
bpstat bpstat_find_breakpoint (bpstat, struct breakpoint *);

/* Nonzero if a signal that we got in wait() was due to circumstances
   explained by the BS.  */
/* Currently that is true if we have hit a breakpoint, or if there is
   a watchpoint enabled.  */
#define bpstat_explains_signal(bs) ((bs) != NULL)

/* Nonzero is this bpstat causes a stop.  */
extern int bpstat_causes_stop (bpstat);

/* Nonzero if we should step constantly (e.g. watchpoints on machines
   without hardware support).  This isn't related to a specific bpstat,
   just to things like whether watchpoints are set.  */
extern int bpstat_should_step (void);

/* Print a message indicating what happened.  Returns nonzero to
   say that only the source line should be printed after this (zero
   return means print the frame as well as the source line).  */
extern enum print_stop_action bpstat_print (bpstat);

/* Put in *NUM the breakpoint number of the first breakpoint we are
   stopped at.  *BSP upon return is a bpstat which points to the
   remaining breakpoints stopped at (but which is not guaranteed to be
   good for anything but further calls to bpstat_num).

   Return 0 if passed a bpstat which does not indicate any breakpoints.
   Return -1 if stopped at a breakpoint that has been deleted since
   we set it.
   Return 1 otherwise.  */
extern int bpstat_num (bpstat *, int *);

/* Perform actions associated with the stopped inferior.  Actually, we
   just use this for breakpoint commands.  Perhaps other actions will
   go here later, but this is executed at a late time (from the
   command loop).  */
extern void bpstat_do_actions (void);

/* Modify BS so that the actions will not be performed.  */
extern void bpstat_clear_actions (bpstat);

/* Implementation:  */

/* Values used to tell the printing routine how to behave for this
   bpstat.  */
enum bp_print_how
  {
    /* This is used when we want to do a normal printing of the reason
       for stopping.  The output will depend on the type of eventpoint
       we are dealing with.  This is the default value, most commonly
       used.  */
    print_it_normal,
    /* This is used when nothing should be printed for this bpstat
       entry.  */
    print_it_noop,
    /* This is used when everything which needs to be printed has
       already been printed.  But we still want to print the frame.  */
    print_it_done
  };

struct bpstats
  {
    /* Linked list because there can be more than one breakpoint at
       the same place, and a bpstat reflects the fact that all have
       been hit.  */
    bpstat next;

    /* Location that caused the stop.  Locations are refcounted, so
       this will never be NULL.  Note that this location may end up
       detached from a breakpoint, but that does not necessary mean
       that the struct breakpoint is gone.  E.g., consider a
       watchpoint with a condition that involves an inferior function
       call.  Watchpoint locations are recreated often (on resumes,
       hence on infcalls too).  Between creating the bpstat and after
       evaluating the watchpoint condition, this location may hence
       end up detached from its original owner watchpoint, even though
       the watchpoint is still listed.  If it's condition evaluates as
       true, we still want this location to cause a stop, and we will
       still need to know which watchpoint it was originally attached.
       What this means is that we should not (in most cases) follow
       the `bpstat->bp_location->owner' link, but instead use the
       `breakpoint_at' field below.  */
    struct bp_location *bp_location_at;

    /* Breakpoint that caused the stop.  This is nullified if the
       breakpoint ends up being deleted.  See comments on
       `bp_location_at' above for why do we need this field instead of
       following the location's owner.  */
    struct breakpoint *breakpoint_at;

    /* The associated command list.  */
    struct counted_command_line *commands;

    /* Commands left to be done.  This points somewhere in
       base_command.  */
    struct command_line *commands_left;

    /* Old value associated with a watchpoint.  */
    struct value *old_val;

    /* Nonzero if this breakpoint tells us to print the frame.  */
    char print;

    /* Nonzero if this breakpoint tells us to stop.  */
    char stop;

    /* Tell bpstat_print and print_bp_stop_message how to print stuff
       associated with this element of the bpstat chain.  */
    enum bp_print_how print_it;
  };

enum inf_context
  {
    inf_starting,
    inf_running,
    inf_exited,
    inf_execd
  };

/* The possible return values for breakpoint_here_p.
   We guarantee that zero always means "no breakpoint here".  */
enum breakpoint_here
  {
    no_breakpoint_here = 0,
    ordinary_breakpoint_here,
    permanent_breakpoint_here
  };


/* Prototypes for breakpoint-related functions.  */

extern enum breakpoint_here breakpoint_here_p (struct address_space *, 
					       CORE_ADDR);

extern int moribund_breakpoint_here_p (struct address_space *, CORE_ADDR);

extern int breakpoint_inserted_here_p (struct address_space *, CORE_ADDR);

extern int regular_breakpoint_inserted_here_p (struct address_space *, 
					       CORE_ADDR);

extern int software_breakpoint_inserted_here_p (struct address_space *, 
						CORE_ADDR);

/* Returns true if there's a hardware watchpoint or access watchpoint
   inserted in the range defined by ADDR and LEN.  */
extern int hardware_watchpoint_inserted_in_range (struct address_space *,
						  CORE_ADDR addr,
						  ULONGEST len);

extern int breakpoint_thread_match (struct address_space *, 
				    CORE_ADDR, ptid_t);

extern void until_break_command (char *, int, int);

extern void update_breakpoint_locations (struct breakpoint *b,
					 struct symtabs_and_lines sals,
					 struct symtabs_and_lines sals_end);

extern void breakpoint_re_set (void);

extern void breakpoint_re_set_thread (struct breakpoint *);

extern struct breakpoint *set_momentary_breakpoint
  (struct gdbarch *, struct symtab_and_line, struct frame_id, enum bptype);

extern struct breakpoint *set_momentary_breakpoint_at_pc
  (struct gdbarch *, CORE_ADDR pc, enum bptype type);

extern struct breakpoint *clone_momentary_breakpoint (struct breakpoint *bpkt);

extern void set_ignore_count (int, int, int);

extern void set_default_breakpoint (int, struct program_space *,
				    CORE_ADDR, struct symtab *, int);

extern void breakpoint_init_inferior (enum inf_context);

extern struct cleanup *make_cleanup_delete_breakpoint (struct breakpoint *);

extern void delete_breakpoint (struct breakpoint *);

extern void breakpoint_auto_delete (bpstat);

/* Return the chain of command lines to execute when this breakpoint
   is hit.  */
extern struct command_line *breakpoint_commands (struct breakpoint *b);

/* Return a string image of DISP.  The string is static, and thus should
   NOT be deallocated after use.  */
const char *bpdisp_text (enum bpdisp disp);

extern void break_command (char *, int);

extern void hbreak_command_wrapper (char *, int);
extern void thbreak_command_wrapper (char *, int);
extern void rbreak_command_wrapper (char *, int);
extern void watch_command_wrapper (char *, int, int);
extern void awatch_command_wrapper (char *, int, int);
extern void rwatch_command_wrapper (char *, int, int);
extern void tbreak_command (char *, int);

extern int create_breakpoint (struct gdbarch *gdbarch, char *arg,
			      char *cond_string, int thread,
			      int parse_condition_and_thread,
			      int tempflag, enum bptype wanted_type,
			      int ignore_count,
			      enum auto_boolean pending_break_support,
			      struct breakpoint_ops *ops,
			      int from_tty,
			      int enabled,
			      int internal);

extern void insert_breakpoints (void);

extern int remove_breakpoints (void);

extern int remove_breakpoints_pid (int pid);

/* This function can be used to physically insert eventpoints from the
   specified traced inferior process, without modifying the breakpoint
   package's state.  This can be useful for those targets which
   support following the processes of a fork() or vfork() system call,
   when both of the resulting two processes are to be followed.  */
extern int reattach_breakpoints (int);

/* This function can be used to update the breakpoint package's state
   after an exec() system call has been executed.

   This function causes the following:

   - All eventpoints are marked "not inserted".
   - All eventpoints with a symbolic address are reset such that
   the symbolic address must be reevaluated before the eventpoints
   can be reinserted.
   - The solib breakpoints are explicitly removed from the breakpoint
   list.
   - A step-resume breakpoint, if any, is explicitly removed from the
   breakpoint list.
   - All eventpoints without a symbolic address are removed from the
   breakpoint list.  */
extern void update_breakpoints_after_exec (void);

/* This function can be used to physically remove hardware breakpoints
   and watchpoints from the specified traced inferior process, without
   modifying the breakpoint package's state.  This can be useful for
   those targets which support following the processes of a fork() or
   vfork() system call, when one of the resulting two processes is to
   be detached and allowed to run free.

   It is an error to use this function on the process whose id is
   inferior_ptid.  */
extern int detach_breakpoints (int);

/* This function is called when program space PSPACE is about to be
   deleted.  It takes care of updating breakpoints to not reference
   this PSPACE anymore.  */
extern void breakpoint_program_space_exit (struct program_space *pspace);

extern void set_longjmp_breakpoint (struct thread_info *tp,
				    struct frame_id frame);
extern void delete_longjmp_breakpoint (int thread);

extern void enable_overlay_breakpoints (void);
extern void disable_overlay_breakpoints (void);

extern void set_std_terminate_breakpoint (void);
extern void delete_std_terminate_breakpoint (void);

/* These functions respectively disable or reenable all currently
   enabled watchpoints.  When disabled, the watchpoints are marked
   call_disabled.  When reenabled, they are marked enabled.

   The intended client of these functions is call_function_by_hand.

   The inferior must be stopped, and all breakpoints removed, when
   these functions are used.

   The need for these functions is that on some targets (e.g., HP-UX),
   gdb is unable to unwind through the dummy frame that is pushed as
   part of the implementation of a call command.  Watchpoints can
   cause the inferior to stop in places where this frame is visible,
   and that can cause execution control to become very confused.

   Note that if a user sets breakpoints in an interactively called
   function, the call_disabled watchpoints will have been reenabled
   when the first such breakpoint is reached.  However, on targets
   that are unable to unwind through the call dummy frame, watches
   of stack-based storage may then be deleted, because gdb will
   believe that their watched storage is out of scope.  (Sigh.) */
extern void disable_watchpoints_before_interactive_call_start (void);

extern void enable_watchpoints_after_interactive_call_stop (void);

/* These functions disable and re-enable all breakpoints during
   inferior startup.  They are intended to be called from solib
   code where necessary.  This is needed on platforms where the
   main executable is relocated at some point during startup
   processing, making breakpoint addresses invalid.

   If additional breakpoints are created after the routine
   disable_breakpoints_before_startup but before the routine
   enable_breakpoints_after_startup was called, they will also
   be marked as disabled.  */
extern void disable_breakpoints_before_startup (void);
extern void enable_breakpoints_after_startup (void);

/* For script interpreters that need to define breakpoint commands
   after they've already read the commands into a struct
   command_line.  */
extern enum command_control_type commands_from_control_command
  (char *arg, struct command_line *cmd);

extern void clear_breakpoint_hit_counts (void);

extern struct breakpoint *get_breakpoint (int num);

/* The following are for displays, which aren't really breakpoints,
   but here is as good a place as any for them.  */

extern void disable_current_display (void);

extern void do_displays (void);

extern void disable_display (int);

extern void clear_displays (void);

extern void disable_breakpoint (struct breakpoint *);

extern void enable_breakpoint (struct breakpoint *);

extern void breakpoint_set_commands (struct breakpoint *b, 
				     struct command_line *commands);

extern void breakpoint_set_silent (struct breakpoint *b, int silent);

extern void breakpoint_set_thread (struct breakpoint *b, int thread);

extern void breakpoint_set_task (struct breakpoint *b, int task);

/* Clear the "inserted" flag in all breakpoints.  */
extern void mark_breakpoints_out (void);

extern void make_breakpoint_permanent (struct breakpoint *);

extern struct breakpoint *create_jit_event_breakpoint (struct gdbarch *,
                                                       CORE_ADDR);

extern struct breakpoint *create_solib_event_breakpoint (struct gdbarch *,
							 CORE_ADDR);

extern struct breakpoint *create_thread_event_breakpoint (struct gdbarch *,
							  CORE_ADDR);

extern void remove_jit_event_breakpoints (void);

extern void remove_solib_event_breakpoints (void);

extern void remove_thread_event_breakpoints (void);

extern void disable_breakpoints_in_shlibs (void);

/* This function returns TRUE if ep is a catchpoint.  */
extern int ep_is_catchpoint (struct breakpoint *);

/* Enable breakpoints and delete when hit.  Called with ARG == NULL
   deletes all breakpoints.  */
extern void delete_command (char *arg, int from_tty);

/* Pull all H/W watchpoints from the target.  Return non-zero if the
   remove fails.  */
extern int remove_hw_watchpoints (void);

/* Manage a software single step breakpoint (or two).  Insert may be
   called twice before remove is called.  */
extern void insert_single_step_breakpoint (struct gdbarch *,
					   struct address_space *, 
					   CORE_ADDR);
extern int single_step_breakpoints_inserted (void);
extern void remove_single_step_breakpoints (void);
extern void cancel_single_step_breakpoints (void);

/* Manage manual breakpoints, separate from the normal chain of
   breakpoints.  These functions are used in murky target-specific
   ways.  Please do not add more uses!  */
extern void *deprecated_insert_raw_breakpoint (struct gdbarch *,
					       struct address_space *, 
					       CORE_ADDR);
extern int deprecated_remove_raw_breakpoint (struct gdbarch *, void *);

/* Check if any hardware watchpoints have triggered, according to the
   target.  */
int watchpoints_triggered (struct target_waitstatus *);

/* Update BUF, which is LEN bytes read from the target address MEMADDR,
   by replacing any memory breakpoints with their shadowed contents.  */
void breakpoint_restore_shadows (gdb_byte *buf, ULONGEST memaddr, 
				 LONGEST len);

extern int breakpoints_always_inserted_mode (void);

/* Called each time new event from target is processed.
   Retires previously deleted breakpoint locations that
   in our opinion won't ever trigger.  */
extern void breakpoint_retire_moribund (void);

/* Set break condition of breakpoint B to EXP.  */
extern void set_breakpoint_condition (struct breakpoint *b, char *exp,
				      int from_tty);

/* Checks if we are catching syscalls or not.
   Returns 0 if not, greater than 0 if we are.  */
extern int catch_syscall_enabled (void);

/* Checks if we are catching syscalls with the specific
   syscall_number.  Used for "filtering" the catchpoints.
   Returns 0 if not, greater than 0 if we are.  */
extern int catching_syscall_number (int syscall_number);

/* Return a tracepoint with the given number if found.  */
extern struct breakpoint *get_tracepoint (int num);

extern struct breakpoint *get_tracepoint_by_number_on_target (int num);

/* Find a tracepoint by parsing a number in the supplied string.  */
extern struct breakpoint *
     get_tracepoint_by_number (char **arg, 
			       struct get_number_or_range_state *state,
			       int optional_p);

/* Return a vector of all tracepoints currently defined.  The vector
   is newly allocated; the caller should free when done with it.  */
extern VEC(breakpoint_p) *all_tracepoints (void);

extern int is_tracepoint (const struct breakpoint *b);

/* Return a vector of all static tracepoints defined at ADDR.  The
   vector is newly allocated; the caller should free when done with
   it.  */
extern VEC(breakpoint_p) *static_tracepoints_here (CORE_ADDR addr);

/* Function that can be passed to read_command_line to validate
   that each command is suitable for tracepoint command list.  */
extern void check_tracepoint_command (char *line, void *closure);

/* Call at the start and end of an "rbreak" command to register
   breakpoint numbers for a later "commands" command.  */
extern void start_rbreak_breakpoints (void);
extern void end_rbreak_breakpoints (void);

/* Breakpoint iterator function.

   Calls a callback function once for each breakpoint, so long as the
   callback function returns false.  If the callback function returns
   true, the iteration will end and the current breakpoint will be
   returned.  This can be useful for implementing a search for a
   breakpoint with arbitrary attributes, or for applying an operation
   to every breakpoint.  */
extern struct breakpoint *iterate_over_breakpoints (int (*) (struct breakpoint *,
							     void *), void *);

extern int user_breakpoint_p (struct breakpoint *);

#endif /* !defined (BREAKPOINT_H) */
