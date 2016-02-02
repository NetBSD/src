/* Branch trace support for GDB, the GNU debugger.

   Copyright (C) 2013-2015 Free Software Foundation, Inc.

   Contributed by Intel Corp. <markus.t.metzger@intel.com>.

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

#ifndef BTRACE_H
#define BTRACE_H

/* Branch tracing (btrace) is a per-thread control-flow execution trace of the
   inferior.  For presentation purposes, the branch trace is represented as a
   list of sequential control-flow blocks, one such list per thread.  */

#include "btrace-common.h"

struct thread_info;
struct btrace_function;

/* A branch trace instruction.

   This represents a single instruction in a branch trace.  */
struct btrace_insn
{
  /* The address of this instruction.  */
  CORE_ADDR pc;
};

/* A vector of branch trace instructions.  */
typedef struct btrace_insn btrace_insn_s;
DEF_VEC_O (btrace_insn_s);

/* A doubly-linked list of branch trace function segments.  */
struct btrace_func_link
{
  struct btrace_function *prev;
  struct btrace_function *next;
};

/* Flags for btrace function segments.  */
enum btrace_function_flag
{
  /* The 'up' link interpretation.
     If set, it points to the function segment we returned to.
     If clear, it points to the function segment we called from.  */
  BFUN_UP_LINKS_TO_RET = (1 << 0),

  /* The 'up' link points to a tail call.  This obviously only makes sense
     if bfun_up_links_to_ret is clear.  */
  BFUN_UP_LINKS_TO_TAILCALL = (1 << 1)
};

/* A branch trace function segment.

   This represents a function segment in a branch trace, i.e. a consecutive
   number of instructions belonging to the same function.

   We do not allow function segments without any instructions.  */
struct btrace_function
{
  /* The full and minimal symbol for the function.  Both may be NULL.  */
  struct minimal_symbol *msym;
  struct symbol *sym;

  /* The previous and next segment belonging to the same function.
     If a function calls another function, the former will have at least
     two segments: one before the call and another after the return.  */
  struct btrace_func_link segment;

  /* The previous and next function in control flow order.  */
  struct btrace_func_link flow;

  /* The directly preceding function segment in a (fake) call stack.  */
  struct btrace_function *up;

  /* The instructions in this function segment.
     The instruction vector will never be empty.  */
  VEC (btrace_insn_s) *insn;

  /* The instruction number offset for the first instruction in this
     function segment.  */
  unsigned int insn_offset;

  /* The function number in control-flow order.  */
  unsigned int number;

  /* The function level in a back trace across the entire branch trace.
     A caller's level is one lower than the level of its callee.

     Levels can be negative if we see returns for which we have not seen
     the corresponding calls.  The branch trace thread information provides
     a fixup to normalize function levels so the smallest level is zero.  */
  int level;

  /* The source line range of this function segment (both inclusive).  */
  int lbegin, lend;

  /* A bit-vector of btrace_function_flag.  */
  enum btrace_function_flag flags;
};

/* A branch trace instruction iterator.  */
struct btrace_insn_iterator
{
  /* The branch trace function segment containing the instruction.
     Will never be NULL.  */
  const struct btrace_function *function;

  /* The index into the function segment's instruction vector.  */
  unsigned int index;
};

/* A branch trace function call iterator.  */
struct btrace_call_iterator
{
  /* The branch trace information for this thread.  Will never be NULL.  */
  const struct btrace_thread_info *btinfo;

  /* The branch trace function segment.
     This will be NULL for the iterator pointing to the end of the trace.  */
  const struct btrace_function *function;
};

/* Branch trace iteration state for "record instruction-history".  */
struct btrace_insn_history
{
  /* The branch trace instruction range from BEGIN (inclusive) to
     END (exclusive) that has been covered last time.  */
  struct btrace_insn_iterator begin;
  struct btrace_insn_iterator end;
};

/* Branch trace iteration state for "record function-call-history".  */
struct btrace_call_history
{
  /* The branch trace function range from BEGIN (inclusive) to END (exclusive)
     that has been covered last time.  */
  struct btrace_call_iterator begin;
  struct btrace_call_iterator end;
};

/* Branch trace thread flags.  */
enum btrace_thread_flag
{
  /* The thread is to be stepped forwards.  */
  BTHR_STEP = (1 << 0),

  /* The thread is to be stepped backwards.  */
  BTHR_RSTEP = (1 << 1),

  /* The thread is to be continued forwards.  */
  BTHR_CONT = (1 << 2),

  /* The thread is to be continued backwards.  */
  BTHR_RCONT = (1 << 3),

  /* The thread is to be moved.  */
  BTHR_MOVE = (BTHR_STEP | BTHR_RSTEP | BTHR_CONT | BTHR_RCONT)
};

/* Branch trace information per thread.

   This represents the branch trace configuration as well as the entry point
   into the branch trace data.  For the latter, it also contains the index into
   an array of branch trace blocks used for iterating though the branch trace
   blocks of a thread.  */
struct btrace_thread_info
{
  /* The target branch trace information for this thread.

     This contains the branch trace configuration as well as any
     target-specific information necessary for implementing branch tracing on
     the underlying architecture.  */
  struct btrace_target_info *target;

  /* The current branch trace for this thread (both inclusive).

     The last instruction of END is the current instruction, which is not
     part of the execution history.
     Both will be NULL if there is no branch trace available.  If there is
     branch trace available, both will be non-NULL.  */
  struct btrace_function *begin;
  struct btrace_function *end;

  /* The function level offset.  When added to each function's LEVEL,
     this normalizes the function levels such that the smallest level
     becomes zero.  */
  int level;

  /* A bit-vector of btrace_thread_flag.  */
  enum btrace_thread_flag flags;

  /* The instruction history iterator.  */
  struct btrace_insn_history *insn_history;

  /* The function call history iterator.  */
  struct btrace_call_history *call_history;

  /* The current replay position.  NULL if not replaying.  */
  struct btrace_insn_iterator *replay;
};

/* Enable branch tracing for a thread.  */
extern void btrace_enable (struct thread_info *tp);

/* Disable branch tracing for a thread.
   This will also delete the current branch trace data.  */
extern void btrace_disable (struct thread_info *);

/* Disable branch tracing for a thread during teardown.
   This is similar to btrace_disable, except that it will use
   target_teardown_btrace instead of target_disable_btrace.  */
extern void btrace_teardown (struct thread_info *);

/* Fetch the branch trace for a single thread.  */
extern void btrace_fetch (struct thread_info *);

/* Clear the branch trace for a single thread.  */
extern void btrace_clear (struct thread_info *);

/* Clear the branch trace for all threads when an object file goes away.  */
extern void btrace_free_objfile (struct objfile *);

/* Parse a branch trace xml document into a block vector.  */
extern VEC (btrace_block_s) *parse_xml_btrace (const char*);

/* Dereference a branch trace instruction iterator.  Return a pointer to the
   instruction the iterator points to.  */
extern const struct btrace_insn *
  btrace_insn_get (const struct btrace_insn_iterator *);

/* Return the instruction number for a branch trace iterator.
   Returns one past the maximum instruction number for the end iterator.
   Returns zero if the iterator does not point to a valid instruction.  */
extern unsigned int btrace_insn_number (const struct btrace_insn_iterator *);

/* Initialize a branch trace instruction iterator to point to the begin/end of
   the branch trace.  Throws an error if there is no branch trace.  */
extern void btrace_insn_begin (struct btrace_insn_iterator *,
			       const struct btrace_thread_info *);
extern void btrace_insn_end (struct btrace_insn_iterator *,
			     const struct btrace_thread_info *);

/* Increment/decrement a branch trace instruction iterator by at most STRIDE
   instructions.  Return the number of instructions by which the instruction
   iterator has been advanced.
   Returns zero, if the operation failed or STRIDE had been zero.  */
extern unsigned int btrace_insn_next (struct btrace_insn_iterator *,
				      unsigned int stride);
extern unsigned int btrace_insn_prev (struct btrace_insn_iterator *,
				      unsigned int stride);

/* Compare two branch trace instruction iterators.
   Return a negative number if LHS < RHS.
   Return zero if LHS == RHS.
   Return a positive number if LHS > RHS.  */
extern int btrace_insn_cmp (const struct btrace_insn_iterator *lhs,
			    const struct btrace_insn_iterator *rhs);

/* Find an instruction in the function branch trace by its number.
   If the instruction is found, initialize the branch trace instruction
   iterator to point to this instruction and return non-zero.
   Return zero otherwise.  */
extern int btrace_find_insn_by_number (struct btrace_insn_iterator *,
				       const struct btrace_thread_info *,
				       unsigned int number);

/* Dereference a branch trace call iterator.  Return a pointer to the
   function the iterator points to or NULL if the interator points past
   the end of the branch trace.  */
extern const struct btrace_function *
  btrace_call_get (const struct btrace_call_iterator *);

/* Return the function number for a branch trace call iterator.
   Returns one past the maximum function number for the end iterator.
   Returns zero if the iterator does not point to a valid function.  */
extern unsigned int btrace_call_number (const struct btrace_call_iterator *);

/* Initialize a branch trace call iterator to point to the begin/end of
   the branch trace.  Throws an error if there is no branch trace.  */
extern void btrace_call_begin (struct btrace_call_iterator *,
			       const struct btrace_thread_info *);
extern void btrace_call_end (struct btrace_call_iterator *,
			     const struct btrace_thread_info *);

/* Increment/decrement a branch trace call iterator by at most STRIDE function
   segments.  Return the number of function segments by which the call
   iterator has been advanced.
   Returns zero, if the operation failed or STRIDE had been zero.  */
extern unsigned int btrace_call_next (struct btrace_call_iterator *,
				      unsigned int stride);
extern unsigned int btrace_call_prev (struct btrace_call_iterator *,
				      unsigned int stride);

/* Compare two branch trace call iterators.
   Return a negative number if LHS < RHS.
   Return zero if LHS == RHS.
   Return a positive number if LHS > RHS.  */
extern int btrace_call_cmp (const struct btrace_call_iterator *lhs,
			    const struct btrace_call_iterator *rhs);

/* Find a function in the function branch trace by its NUMBER.
   If the function is found, initialize the branch trace call
   iterator to point to this function and return non-zero.
   Return zero otherwise.  */
extern int btrace_find_call_by_number (struct btrace_call_iterator *,
				       const struct btrace_thread_info *,
				       unsigned int number);

/* Set the branch trace instruction history from BEGIN (inclusive) to
   END (exclusive).  */
extern void btrace_set_insn_history (struct btrace_thread_info *,
				     const struct btrace_insn_iterator *begin,
				     const struct btrace_insn_iterator *end);

/* Set the branch trace function call history from BEGIN (inclusive) to
   END (exclusive).  */
extern void btrace_set_call_history (struct btrace_thread_info *,
				     const struct btrace_call_iterator *begin,
				     const struct btrace_call_iterator *end);

/* Determine if branch tracing is currently replaying TP.  */
extern int btrace_is_replaying (struct thread_info *tp);

/* Return non-zero if the branch trace for TP is empty; zero otherwise.  */
extern int btrace_is_empty (struct thread_info *tp);


#endif /* BTRACE_H */
