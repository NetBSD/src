/* Data structures and functions associated with agent expressions in GDB.
   Copyright (C) 2009-2013 Free Software Foundation, Inc.

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

#if !defined (AX_H)
#define AX_H 1

#include "server.h"
#include "regcache.h"

#ifdef IN_PROCESS_AGENT
extern int debug_agent;
#define debug_threads debug_agent
#endif

struct traceframe;

/* Enumeration of the different kinds of things that can happen during
   agent expression evaluation.  */

enum eval_result_type
  {
    expr_eval_no_error,
    expr_eval_empty_expression,
    expr_eval_empty_stack,
    expr_eval_stack_overflow,
    expr_eval_stack_underflow,
    expr_eval_unhandled_opcode,
    expr_eval_unrecognized_opcode,
    expr_eval_divide_by_zero,
    expr_eval_invalid_goto
  };

struct agent_expr
{
  int length;

  unsigned char *bytes;
};

#ifndef IN_PROCESS_AGENT

/* The packet form of an agent expression consists of an 'X', number
   of bytes in expression, a comma, and then the bytes.  */
struct agent_expr *gdb_parse_agent_expr (char **actparm);

/* Convert the bytes of an agent expression back into hex digits, so
   they can be printed or uploaded.  This allocates the buffer,
   callers should free when they are done with it.  */
char *gdb_unparse_agent_expr (struct agent_expr *aexpr);
void emit_prologue (void);
void emit_epilogue (void);
enum eval_result_type compile_bytecodes (struct agent_expr *aexpr);
#endif

/* The context when evaluating agent expression.  */

struct eval_agent_expr_context
{
  /* The registers when evaluating agent expression.  */
  struct regcache *regcache;
  /* The traceframe, if any, when evaluating agent expression.  */
  struct traceframe *tframe;
  /* The tracepoint, if any, when evaluating agent expression.  */
  struct tracepoint *tpoint;
};

enum eval_result_type
  gdb_eval_agent_expr (struct eval_agent_expr_context *ctx,
		       struct agent_expr *aexpr,
		       ULONGEST *rslt);
#endif /* AX_H */
