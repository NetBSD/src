/* Longjump free calls to gdb internal routines.
   Copyright 1999 Free Software Foundation, Inc.

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

#include "defs.h"
#include "value.h"
#include "frame.h"
#include "wrapper.h"

/* Use this struct to pass arguments to wrapper routines. We assume
   (arbitrarily) that no gdb function takes more than ten arguments. */
struct gdb_wrapper_arguments
  {

    /* Pointer to some result from the gdb function call, if any */
    union wrapper_results 
      {
	int   integer;
	void *pointer;
      } result;
	

    /* The list of arguments. */
    union wrapper_args 
      {
	int   integer;
	void *pointer;
      } args[10];
  };

int gdb_parse_exp_1 PARAMS ((char **, struct block *, 
			     int, struct expression **));
int wrap_parse_exp_1 PARAMS ((char *));

int gdb_evaluate_expression PARAMS ((struct expression *, value_ptr *));
int wrap_evaluate_expression PARAMS ((char *));

int gdb_value_fetch_lazy PARAMS ((value_ptr));
int wrap_value_fetch_lazy PARAMS ((char *));

int gdb_value_equal PARAMS ((value_ptr, value_ptr, int *));
int wrap_value_equal PARAMS ((char *));

int gdb_value_subscript PARAMS ((value_ptr, value_ptr, value_ptr * rval));
int wrap_value_subscript PARAMS ((char *));

int gdb_value_ind PARAMS ((value_ptr val, value_ptr * rval));
int wrap_value_ind PARAMS ((char *opaque_arg));

int gdb_parse_and_eval_type (char *, int, struct type **);
int wrap_parse_and_eval_type (char *);

int
gdb_parse_exp_1 (stringptr, block, comma, expression)
     char **stringptr;
     struct block *block;
     int comma;
     struct expression **expression;
{
  struct gdb_wrapper_arguments args;
  args.args[0].pointer = stringptr;
  args.args[1].pointer = block;
  args.args[2].integer = comma;

  if (!catch_errors ((catch_errors_ftype *) wrap_parse_exp_1, &args,
		     "", RETURN_MASK_ERROR))
    {
      /* An error occurred */
      return 0;
    }

  *expression = (struct expression *) args.result.pointer;
  return 1;
  
}

int
wrap_parse_exp_1 (argptr)
     char *argptr;
{
  struct gdb_wrapper_arguments *args 
    = (struct gdb_wrapper_arguments *) argptr;
  args->result.pointer = parse_exp_1((char **) args->args[0].pointer,
				     (struct block *) args->args[1].pointer,
				     args->args[2].integer);
  return 1;
}

int
gdb_evaluate_expression (exp, value)
     struct expression *exp;
     value_ptr *value;
{
  struct gdb_wrapper_arguments args;
  args.args[0].pointer = exp;

  if (!catch_errors ((catch_errors_ftype *) wrap_evaluate_expression, &args,
		     "", RETURN_MASK_ERROR))
    {
      /* An error occurred */
      return 0;
    }

  *value = (value_ptr) args.result.pointer;
  return 1;
}

int
wrap_evaluate_expression (a)
     char *a;
{
  struct gdb_wrapper_arguments *args = (struct gdb_wrapper_arguments *) a;

  (args)->result.pointer =
    (char *) evaluate_expression ((struct expression *) args->args[0].pointer);
  return 1;
}

int
gdb_value_fetch_lazy (value)
     value_ptr value;
{
  struct gdb_wrapper_arguments args;

  args.args[0].pointer = value;
  return catch_errors ((catch_errors_ftype *) wrap_value_fetch_lazy, &args,
		       "", RETURN_MASK_ERROR);
}

int
wrap_value_fetch_lazy (a)
     char *a;
{
  struct gdb_wrapper_arguments *args = (struct gdb_wrapper_arguments *) a;

  value_fetch_lazy ((value_ptr) (args)->args[0].pointer);
  return 1;
}

int
gdb_value_equal (val1, val2, result)
     value_ptr val1;
     value_ptr val2;
     int *result;
{
  struct gdb_wrapper_arguments args;

  args.args[0].pointer = val1;
  args.args[1].pointer = val2;

  if (!catch_errors ((catch_errors_ftype *) wrap_value_equal, &args,
		     "", RETURN_MASK_ERROR))
    {
      /* An error occurred */
      return 0;
    }

  *result = args.result.integer;
  return 1;
}

int
wrap_value_equal (a)
     char *a;
{
  struct gdb_wrapper_arguments *args = (struct gdb_wrapper_arguments *) a;
  value_ptr val1, val2;

  val1 = (value_ptr) (args)->args[0].pointer;
  val2 = (value_ptr) (args)->args[1].pointer;

  (args)->result.integer = value_equal (val1, val2);
  return 1;
}

int
gdb_value_subscript (val1, val2, rval)
     value_ptr val1;
     value_ptr val2;
     value_ptr * rval;
{
  struct gdb_wrapper_arguments args;

  args.args[0].pointer = val1;
  args.args[1].pointer = val2;

  if (!catch_errors ((catch_errors_ftype *) wrap_value_subscript, &args,
		     "", RETURN_MASK_ERROR))
    {
      /* An error occurred */
      return 0;
    }

  *rval = (value_ptr) args.result.pointer;
  return 1;
}

int
wrap_value_subscript (a)
     char *a;
{
  struct gdb_wrapper_arguments *args = (struct gdb_wrapper_arguments *) a;
  value_ptr val1, val2;

  val1 = (value_ptr) (args)->args[0].pointer;
  val2 = (value_ptr) (args)->args[1].pointer;

  (args)->result.pointer = value_subscript (val1, val2);
  return 1;
}

int
gdb_value_ind (val, rval)
     value_ptr val;
     value_ptr *rval;
{
  struct gdb_wrapper_arguments args;

  args.args[0].pointer = val;

  if (!catch_errors ((catch_errors_ftype *) wrap_value_ind, &args,
		     "", RETURN_MASK_ERROR))
    {
      /* An error occurred */
      return 0;
    }

  *rval = (value_ptr) args.result.pointer;
  return 1;
}

int
wrap_value_ind (opaque_arg)
     char *opaque_arg;
{
  struct gdb_wrapper_arguments *args = (struct gdb_wrapper_arguments *) opaque_arg;
  value_ptr val;

  val = (value_ptr) (args)->args[0].pointer;
  (args)->result.pointer = value_ind (val);
  return 1;
}

int
gdb_parse_and_eval_type (char *p, int length, struct type **type)
{
  struct gdb_wrapper_arguments args;
  args.args[0].pointer = p;
  args.args[1].integer = length;

  if (!catch_errors ((catch_errors_ftype *) wrap_parse_and_eval_type, &args,
		     "", RETURN_MASK_ALL))
    {
      /* An error occurred */
      return 0;
    }

  *type = (struct type *) args.result.pointer;
  return 1;
}

int
wrap_parse_and_eval_type (char *a)
{
  struct gdb_wrapper_arguments *args = (struct gdb_wrapper_arguments *) a;

  char *p = (char *) args->args[0].pointer;
  int length = args->args[1].integer;

  args->result.pointer = (char *) parse_and_eval_type (p, length);

  return 1;
}
