/* Data structures associated with tracepoints in GDB.
   Copyright (C) 1997-2016 Free Software Foundation, Inc.

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

#if !defined (TRACEPOINT_H)
#define TRACEPOINT_H 1

#include "breakpoint.h"
#include "target.h"
#include "memrange.h"
#include "gdb_vecs.h"

/* An object describing the contents of a traceframe.  */

struct traceframe_info
{
  /* Collected memory.  */
  VEC(mem_range_s) *memory;

  /* Collected trace state variables.  */
  VEC(int) *tvars;
};

/* A trace state variable is a value managed by a target being
   traced.  A trace state variable (or tsv for short) can be accessed
   and assigned to by tracepoint actions and conditionals, but is not
   part of the program being traced, and it doesn't have to be
   collected.  Effectively the variables are scratch space for
   tracepoints.  */

struct trace_state_variable
  {
    /* The variable's name.  The user has to prefix with a dollar sign,
       but we don't store that internally.  */
    const char *name;

    /* An id number assigned by GDB, and transmitted to targets.  */
    int number;

    /* The initial value of a variable is a 64-bit signed integer.  */
    LONGEST initial_value;

    /* 1 if the value is known, else 0.  The value is known during a
       trace run, or in tfind mode if the variable was collected into
       the current trace frame.  */
    int value_known;

    /* The value of a variable is a 64-bit signed integer.  */
    LONGEST value;

    /* This is true for variables that are predefined and built into
       the target.  */
    int builtin;
   };

/* The trace status encompasses various info about the general state
   of the tracing run.  */

enum trace_stop_reason
  {
    trace_stop_reason_unknown,
    trace_never_run,
    tstop_command,
    trace_buffer_full,
    trace_disconnected,
    tracepoint_passcount,
    tracepoint_error
  };

struct trace_status
{
  /* If the status is coming from a file rather than a live target,
     this points at the file's filename.  Otherwise, this is NULL.  */
  const char *filename;

  /* This is true if the value of the running field is known.  */
  int running_known;

  /* This is true when the trace experiment is actually running.  */
  int running;

  enum trace_stop_reason stop_reason;

  /* If stop_reason is tracepoint_passcount or tracepoint_error, this
     is the (on-target) number of the tracepoint which caused the
     stop.  */
  int stopping_tracepoint;

  /* If stop_reason is tstop_command or tracepoint_error, this is an
     arbitrary string that may describe the reason for the stop in
     more detail.  */

  char *stop_desc;

  /* Number of traceframes currently in the buffer.  */

  int traceframe_count;

  /* Number of traceframes created since start of run.  */

  int traceframes_created;

  /* Total size of the target's trace buffer.  */

  int buffer_size;

  /* Unused bytes left in the target's trace buffer.  */

  int buffer_free;

  /* 1 if the target will continue tracing after disconnection, else
     0.  If the target does not report a value, assume 0.  */

  int disconnected_tracing;

  /* 1 if the target is using a circular trace buffer, else 0.  If the
     target does not report a value, assume 0.  */

  int circular_buffer;

  /* The "name" of the person running the trace.  This is an
     arbitrary string.  */

  char *user_name;

  /* "Notes" about the trace.  This is an arbitrary string not
     interpreted by GDBserver in any special way.  */

  char *notes;

  /* The calendar times at which the trace run started and stopped,
     both expressed in microseconds of Unix time.  */

  LONGEST start_time;
  LONGEST stop_time;
};

struct trace_status *current_trace_status (void);

extern char *default_collect;

extern int trace_regblock_size;

extern const char *stop_reason_names[];

/* Struct to collect random info about tracepoints on the target.  */

struct uploaded_tp
{
  int number;
  enum bptype type;
  ULONGEST addr;
  int enabled;
  int step;
  int pass;
  int orig_size;

  /* String that is the encoded form of the tracepoint's condition.  */
  char *cond;

  /* Vectors of strings that are the encoded forms of a tracepoint's
     actions.  */
  VEC(char_ptr) *actions;
  VEC(char_ptr) *step_actions;

  /* The original string defining the location of the tracepoint.  */
  char *at_string;

  /* The original string defining the tracepoint's condition.  */
  char *cond_string;

  /* List of original strings defining the tracepoint's actions.  */
  VEC(char_ptr) *cmd_strings;

  /* The tracepoint's current hit count.  */
  int hit_count;

  /* The tracepoint's current traceframe usage.  */
  ULONGEST traceframe_usage;

  struct uploaded_tp *next;
};

/* Struct recording info about trace state variables on the target.  */

struct uploaded_tsv
{
  const char *name;
  int number;
  LONGEST initial_value;
  int builtin;
  struct uploaded_tsv *next;
};

/* Struct recording info about a target static tracepoint marker.  */

struct static_tracepoint_marker
{
  struct gdbarch *gdbarch;
  CORE_ADDR address;

  /* The string ID of the marker.  */
  char *str_id;

  /* Extra target reported info associated with the marker.  */
  char *extra;
};

struct memrange
{
  /* memrange_absolute for absolute memory range, else basereg
     number.  */
  int type;
  bfd_signed_vma start;
  bfd_signed_vma end;
};

struct collection_list
{
  /* room for up to 256 regs */
  unsigned char regs_mask[32];
  long listsize;
  long next_memrange;
  struct memrange *list;

  /* size of array pointed to by expr_list elt.  */
  long aexpr_listsize;
  long next_aexpr_elt;
  struct agent_expr **aexpr_list;

  /* True is the user requested a collection of "$_sdata", "static
     tracepoint data".  */
  int strace_data;

  /* A set of names of wholly collected objects.  */
  VEC(char_ptr) *wholly_collected;
  /* A set of computed expressions.  */
  VEC(char_ptr) *computed;
};

extern void parse_static_tracepoint_marker_definition
  (char *line, char **pp,
   struct static_tracepoint_marker *marker);
extern void release_static_tracepoint_marker (struct static_tracepoint_marker *);
extern void free_current_marker (void *arg);

/* A hook used to notify the UI of tracepoint operations.  */

extern void (*deprecated_trace_find_hook) (char *arg, int from_tty);
extern void (*deprecated_trace_start_stop_hook) (int start, int from_tty);

/* Returns the current traceframe number.  */
extern int get_traceframe_number (void);

/* Returns the tracepoint number for current traceframe.  */
extern int get_tracepoint_number (void);

/* Make the traceframe NUM be the current trace frame, all the way to
   the target, and flushes all global state (register/frame caches,
   etc.).  */
extern void set_current_traceframe (int num);

struct cleanup *make_cleanup_restore_current_traceframe (void);

void free_actions (struct breakpoint *);

extern const char *decode_agent_options (const char *exp, int *trace_string);

extern struct cleanup *
  encode_actions_and_make_cleanup (struct bp_location *tloc,
				   struct collection_list *tracepoint_list,
				   struct collection_list *stepping_list);

extern void encode_actions_rsp (struct bp_location *tloc,
				char ***tdp_actions, char ***stepping_actions);

extern void validate_actionline (const char *, struct breakpoint *);
extern void validate_trace_state_variable_name (const char *name);

extern struct trace_state_variable *find_trace_state_variable (const char *name);
extern struct trace_state_variable *
  find_trace_state_variable_by_number (int number);

extern struct trace_state_variable *create_trace_state_variable (const char *name);

extern int encode_source_string (int num, ULONGEST addr,
				 char *srctype, const char *src,
				 char *buf, int buf_size);

extern void parse_trace_status (char *line, struct trace_status *ts);

extern void parse_tracepoint_status (char *p, struct breakpoint *tp,
				     struct uploaded_tp *utp);

extern void parse_tracepoint_definition (char *line,
					 struct uploaded_tp **utpp);
extern void parse_tsv_definition (char *line, struct uploaded_tsv **utsvp);

extern struct uploaded_tp *get_uploaded_tp (int num, ULONGEST addr,
					    struct uploaded_tp **utpp);
extern void free_uploaded_tps (struct uploaded_tp **utpp);

extern struct uploaded_tsv *get_uploaded_tsv (int num,
					      struct uploaded_tsv **utsvp);
extern void free_uploaded_tsvs (struct uploaded_tsv **utsvp);
extern struct tracepoint *create_tracepoint_from_upload (struct uploaded_tp *utp);
extern void merge_uploaded_tracepoints (struct uploaded_tp **utpp);
extern void merge_uploaded_trace_state_variables (struct uploaded_tsv **utsvp);

extern void query_if_trace_running (int from_tty);
extern void disconnect_tracing (void);
extern void trace_reset_local_state (void);

extern void check_trace_running (struct trace_status *);

extern void start_tracing (char *notes);
extern void stop_tracing (char *notes);

extern void trace_status_mi (int on_stop);

extern void tvariables_info_1 (void);
extern void save_trace_state_variables (struct ui_file *fp);

extern void tfind_1 (enum trace_find_type type, int num,
		     CORE_ADDR addr1, CORE_ADDR addr2,
		     int from_tty);

extern void trace_save_tfile (const char *filename,
			      int target_does_save);
extern void trace_save_ctf (const char *dirname,
			    int target_does_save);

extern struct traceframe_info *parse_traceframe_info (const char *tframe_info);

extern int traceframe_available_memory (VEC(mem_range_s) **result,
					CORE_ADDR memaddr, ULONGEST len);

extern struct traceframe_info *get_traceframe_info (void);

extern struct bp_location *get_traceframe_location (int *stepping_frame_p);

#endif	/* TRACEPOINT_H */
