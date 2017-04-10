/*	$NetBSD: storage.c,v 1.1 2017/04/10 02:28:23 phil Exp $ */

/*
 * Copyright (C) 1991-1994, 1997, 2006, 2008, 2012-2017 Free Software Foundation, Inc.
 * Copyright (C) 2016-2017 Philip A. Nelson.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names Philip A. Nelson and Free Software Foundation may not be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PHILIP A. NELSON ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL PHILIP A. NELSON OR THE FREE SOFTWARE FOUNDATION BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/* storage.c:  Code and data storage manipulations.  This includes labels. */

#include "bcdefs.h"
#include "proto.h"

/* Local prototypes */
static bc_array_node *copy_tree (bc_array_node *ary_node, int depth);
static bc_array *copy_array (bc_array *ary);


/* Initialize the storage at the beginning of the run. */

void
init_storage (void)
{

  /* Functions: we start with none and ask for more. */
  f_count = 0;
  more_functions ();
  f_names[0] = strdup("(main)");

  /* Variables. */
  v_count = 0;
  more_variables ();
  
  /* Arrays. */
  a_count = 0;
  more_arrays ();

  /* Other things... */
  ex_stack = NULL;
  fn_stack = NULL;
  i_base = 10;
  o_base = 10;
  scale  = 0;
#if defined(READLINE) || defined(LIBEDIT)
  n_history = -1;	
#endif
  c_code = FALSE;
  bc_init_numbers();
}

/* Three functions for increasing the number of functions, variables, or
   arrays that are needed.  This adds another 32 of the requested object. */

void
more_functions (void)
{
  int old_count;
  int indx;
  bc_function *old_f;
  bc_function *f;
  char **old_names;

  /* Save old information. */
  old_count = f_count;
  old_f = functions;
  old_names = f_names;

  /* Add a fixed amount and allocate new space. */
  f_count += STORE_INCR;
  functions = bc_malloc (f_count*sizeof (bc_function));
  f_names = bc_malloc (f_count*sizeof (char *));

  /* Copy old ones. */
  for (indx = 0; indx < old_count; indx++)
    {
      functions[indx] = old_f[indx];
      f_names[indx] = old_names[indx];
    }

  /* Initialize the new ones. */
  for (; indx < f_count; indx++)
    {
      f = &functions[indx];
      f->f_defined = FALSE;
      f->f_void = FALSE;
      f->f_body = bc_malloc (BC_START_SIZE);
      f->f_body_size = BC_START_SIZE;
      f->f_code_size = 0;
      f->f_label = NULL;
      f->f_autos = NULL;
      f->f_params = NULL;
    }

  /* Free the old elements. */
  if (old_count != 0)
    {
      free (old_f);
      free (old_names);
    }
}

void
more_variables (void)
{
  int indx;
  int old_count;
  bc_var **old_var;
  char **old_names;

  /* Save the old values. */
  old_count = v_count;
  old_var = variables;
  old_names = v_names;

  /* Increment by a fixed amount and allocate. */
  v_count += STORE_INCR;
  variables = bc_malloc (v_count*sizeof(bc_var *));
  v_names = bc_malloc (v_count*sizeof(char *));

  /* Copy the old variables. */
  for (indx = 3; indx < old_count; indx++)
    {
      variables[indx] = old_var[indx];
      v_names[indx] = old_names[indx];
    }

  /* Initialize the new elements. */
  for (; indx < v_count; indx++)
    variables[indx] = NULL;

  /* Free the old elements. */
  if (old_count != 0)
    {
      free (old_var);
      free (old_names);
    }
}

void
more_arrays (void)
{
  int indx;
  int old_count;
  bc_var_array **old_ary;
  char **old_names;

  /* Save the old values. */
  old_count = a_count;
  old_ary = arrays;
  old_names = a_names;

  /* Increment by a fixed amount and allocate. */
  a_count += STORE_INCR;
  arrays = bc_malloc (a_count*sizeof(bc_var_array *));
  a_names = bc_malloc (a_count*sizeof(char *));

  /* Copy the old arrays. */
  for (indx = 1; indx < old_count; indx++)
    {
      arrays[indx] = old_ary[indx];
      a_names[indx] = old_names[indx];
    }


  /* Initialize the new elements. */
  for (; indx < a_count; indx++)
    arrays[indx] = NULL;

  /* Free the old elements. */
  if (old_count != 0)
    {
      free (old_ary);
      free (old_names);
    }
}


/* clear_func clears out function FUNC and makes it ready to redefine. */

void
clear_func (int func)
{
  bc_function *f;
  bc_label_group *lg;

  /* Set the pointer to the function. */
  f = &functions[func];
  f->f_defined = FALSE;
  /* XXX restore f_body to initial size??? */
  f->f_code_size = 0;
  if (f->f_autos != NULL)
    {
      free_args (f->f_autos);
      f->f_autos = NULL;
    }
  if (f->f_params != NULL)
    {
      free_args (f->f_params);
      f->f_params = NULL;
    }
  while (f->f_label != NULL)
    {
      lg = f->f_label->l_next;
      free (f->f_label);
      f->f_label = lg;
    }
}


/*  Pop the function execution stack and return the top. */

int
fpop(void)
{
  fstack_rec *temp;
  int retval;
  
  if (fn_stack != NULL)
    {
      temp = fn_stack;
      fn_stack = temp->s_next;
      retval = temp->s_val;
      free (temp);
    }
  else
    {
      retval = 0;
      rt_error ("function stack underflow, contact maintainer.");
    }
  return (retval);
}


/* Push VAL on to the function stack. */

void
fpush (int val)
{
  fstack_rec *temp;
  
  temp = bc_malloc (sizeof (fstack_rec));
  temp->s_next = fn_stack;
  temp->s_val = val;
  fn_stack = temp;
}


/* Pop and discard the top element of the regular execution stack. */

void
pop (void)
{
  estack_rec *temp;
  
  if (ex_stack != NULL)
    {
      temp = ex_stack;
      ex_stack = temp->s_next;
      bc_free_num (&temp->s_num);
      free (temp);
    }
}


/* Push a copy of NUM on to the regular execution stack. */

void
push_copy (bc_num num)
{
  estack_rec *temp;

  temp = bc_malloc (sizeof (estack_rec));
  temp->s_num = bc_copy_num (num);
  temp->s_next = ex_stack;
  ex_stack = temp;
}


/* Push NUM on to the regular execution stack.  Do NOT push a copy. */

void
push_num (bc_num num)
{
  estack_rec *temp;

  temp = bc_malloc (sizeof (estack_rec));
  temp->s_num = num;
  temp->s_next = ex_stack;
  ex_stack = temp;
}


/* Make sure the ex_stack has at least DEPTH elements on it.
   Return TRUE if it has at least DEPTH elements, otherwise
   return FALSE. */

char
check_stack (int depth)
{
  estack_rec *temp;

  temp = ex_stack;
  while ((temp != NULL) && (depth > 0))
    {
      temp = temp->s_next;
      depth--;
    }
  if (depth > 0)
    {
      rt_error ("Stack error.");
      return FALSE;
    }
  return TRUE;
}


/* The following routines manipulate simple variables and
   array variables. */

/* get_var returns a pointer to the variable VAR_NAME.  If one does not
   exist, one is created. */

bc_var *
get_var (int var_name)
{
  bc_var *var_ptr;

  var_ptr = variables[var_name];
  if (var_ptr == NULL)
    {
      var_ptr = variables[var_name] = bc_malloc (sizeof (bc_var));
      bc_init_num (&var_ptr->v_value);
    }
  return var_ptr;
}


/* get_array_num returns the address of the bc_num in the array
   structure.  If more structure is requried to get to the index,
   this routine does the work to create that structure. VAR_INDEX
   is a zero based index into the arrays storage array. INDEX is
   the index into the bc array. */

bc_num *
get_array_num (int var_index, unsigned long idx)
{
  bc_var_array *ary_ptr;
  bc_array *a_var;
  bc_array_node *temp;
  int log;
  unsigned int ix, ix1;
  int sub [NODE_DEPTH];

  /* Get the array entry. */
  ary_ptr = arrays[var_index];
  if (ary_ptr == NULL)
    {
      ary_ptr = arrays[var_index] = bc_malloc (sizeof (bc_var_array));
      ary_ptr->a_value = NULL;
      ary_ptr->a_next = NULL;
      ary_ptr->a_param = FALSE;
    }

  a_var = ary_ptr->a_value;
  if (a_var == NULL) {
    a_var = ary_ptr->a_value = bc_malloc (sizeof (bc_array));
    a_var->a_tree = NULL;
    a_var->a_depth = 0;
  }

  /* Get the index variable. */
  sub[0] = idx & NODE_MASK;
  ix = idx >> NODE_SHIFT;
  log = 1;
  while (ix > 0 || log < a_var->a_depth)
    {
      sub[log] = ix & NODE_MASK;
      ix >>= NODE_SHIFT;
      log++;
    }
  
  /* Build any tree that is necessary. */
  while (log > a_var->a_depth)
    {
      temp = bc_malloc (sizeof(bc_array_node));
      if (a_var->a_depth != 0)
	{
	  temp->n_items.n_down[0] = a_var->a_tree;
	  for (ix=1; ix < NODE_SIZE; ix++)
	    temp->n_items.n_down[ix] = NULL;
	}
      else
	{
	  for (ix=0; ix < NODE_SIZE; ix++)
	    temp->n_items.n_num[ix] = bc_copy_num(_zero_);
	}
      a_var->a_tree = temp;
      a_var->a_depth++;
    }
  
  /* Find the indexed variable. */
  temp = a_var->a_tree;
  while ( log-- > 1)
    {
      ix1 = sub[log];
      if (temp->n_items.n_down[ix1] == NULL)
	{
	  temp->n_items.n_down[ix1] = bc_malloc (sizeof(bc_array_node));
	  temp = temp->n_items.n_down[ix1];
	  if (log > 1)
	    for (ix=0; ix < NODE_SIZE; ix++)
	      temp->n_items.n_down[ix] = NULL;
	  else
	    for (ix=0; ix < NODE_SIZE; ix++)
	      temp->n_items.n_num[ix] = bc_copy_num(_zero_);
	}
      else
	temp = temp->n_items.n_down[ix1];
    }
  
  /* Return the address of the indexed variable. */
  return &(temp->n_items.n_num[sub[0]]);
}


/* Store the top of the execution stack into VAR_NAME.  
   This includes the special variables ibase, obase, and scale. */

void
store_var (int var_name)
{
  bc_var *var_ptr;
  long temp;
  char toobig;

  if (var_name > 3)
    {
      /* It is a simple variable. */
      var_ptr = get_var (var_name);
      if (var_ptr != NULL)
	{
	  bc_free_num(&var_ptr->v_value);
	  var_ptr->v_value = bc_copy_num (ex_stack->s_num);
	}
    }
  else
    {
      /* It is a special variable... */
      toobig = FALSE;
      temp = 0;
      if (bc_is_neg (ex_stack->s_num))
	{
	  switch (var_name)
	    {
	    case 0:
	      rt_warn ("negative ibase, set to 2");
	      temp = 2;
	      break;
	    case 1:
	      rt_warn ("negative obase, set to 2");
	      temp = 2;
	      break;
	    case 2:
	      rt_warn ("negative scale, set to 0");
	      temp = 0;
	      break;
#if defined(READLINE) || defined(LIBEDIT)
	    case 3:
	      temp = -1;
	      break;
#endif
	    }
	}
      else
	{
	  temp = bc_num2long (ex_stack->s_num);
	  if (!bc_is_zero (ex_stack->s_num) && temp == 0)
	    toobig = TRUE;
	}
      switch (var_name)
	{
	case 0:
	  if (temp < 2 && !toobig)
	    {
	      i_base = 2;
	      rt_warn ("ibase too small, set to 2");
	    }
	  else
	    if (temp > 16 || toobig)
	      {
	        if (std_only)
                  {
		    i_base = 16;  
		    rt_warn ("ibase too large, set to 16");
                  } 
                else if (temp > 36 || toobig) 
                  {
		    i_base = 36;
		    rt_warn ("ibase too large, set to 36");
                  }
                else
                  { 
                     if (temp >= 16 && warn_not_std)
                       rt_warn ("ibase larger than 16 is non-standard");
		     i_base = temp;
                  }
	      }
	    else
	      i_base = (int) temp;
	  break;

	case 1:
	  if (temp < 2 && !toobig)
	    {
	      o_base = 2;
	      rt_warn ("obase too small, set to 2");
	    }
	  else
	    if (temp > BC_BASE_MAX || toobig)
	      {
		o_base = BC_BASE_MAX;
		rt_warn ("obase too large, set to %d", BC_BASE_MAX);
	      }
	    else
	      o_base = (int) temp;
	  break;

	case 2:
	  /*  WARNING:  The following if statement may generate a compiler
	      warning if INT_MAX == LONG_MAX.  This is NOT a problem. */
	  if (temp > BC_SCALE_MAX || toobig )
	    {
	      scale = BC_SCALE_MAX;
	      rt_warn ("scale too large, set to %d", BC_SCALE_MAX);
	    }
	  else
	    scale = (int) temp;
	  break;

#if defined(READLINE) || defined(LIBEDIT)
	case 3:
	  if (toobig)
	    {
	      temp = -1;
	      rt_warn ("history too large, set to unlimited");
	      UNLIMIT_HISTORY;
	    }
	  else
	    {
	      n_history = temp;
	      if (temp < 0)
		UNLIMIT_HISTORY;
	      else
		HISTORY_SIZE(n_history);
	    }
#endif
	}
    }
}


/* Store the top of the execution stack into array VAR_NAME. 
   VAR_NAME is the name of an array, and the next to the top
   of stack for the index into the array. */

void
store_array (int var_name)
{
  bc_num *num_ptr;
  long idx;

  if (!check_stack(2)) return;
  idx = bc_num2long (ex_stack->s_next->s_num);
  if (idx < 0 || idx > BC_DIM_MAX ||
      (idx == 0 && !bc_is_zero(ex_stack->s_next->s_num))) 
    rt_error ("Array %s subscript out of bounds.", a_names[var_name]);
  else
    {
      num_ptr = get_array_num (var_name, idx);
      if (num_ptr != NULL)
	{
	  bc_free_num (num_ptr);
	  *num_ptr = bc_copy_num (ex_stack->s_num);
	  bc_free_num (&ex_stack->s_next->s_num);
	  ex_stack->s_next->s_num = ex_stack->s_num;
	  bc_init_num (&ex_stack->s_num);
	  pop();
	}
    }
}


/*  Load a copy of VAR_NAME on to the execution stack.  This includes
    the special variables ibase, obase and scale.  */

void
load_var (int var_name)
{
  bc_var *var_ptr;

  switch (var_name)
    {

    case 0:
      /* Special variable ibase. */
      push_copy (_zero_);
      bc_int2num (&ex_stack->s_num, i_base);
      break;

    case 1:
      /* Special variable obase. */
      push_copy (_zero_);
      bc_int2num (&ex_stack->s_num, o_base);
      break;

    case 2:
      /* Special variable scale. */
      push_copy (_zero_);
      bc_int2num (&ex_stack->s_num, scale);
      break;

#if defined(READLINE) || defined(LIBEDIT)
    case 3:
      /* Special variable history. */
      push_copy (_zero_);
      bc_int2num (&ex_stack->s_num, n_history);
      break;
#endif

    default:
      /* It is a simple variable. */
      var_ptr = variables[var_name];
      if (var_ptr != NULL)
	push_copy (var_ptr->v_value);
      else
	push_copy (_zero_);
    }
}


/*  Load a copy of VAR_NAME on to the execution stack.  This includes
    the special variables ibase, obase and scale.  */

void
load_array (int var_name)
{
  bc_num *num_ptr;
  long   idx;

  if (!check_stack(1)) return;
  idx = bc_num2long (ex_stack->s_num);
  if (idx < 0 || idx > BC_DIM_MAX ||
     (idx == 0 && !bc_is_zero(ex_stack->s_num))) 
    rt_error ("Array %s subscript out of bounds.", a_names[var_name]);
  else
    {
      num_ptr = get_array_num (var_name, idx);
      if (num_ptr != NULL)
	{
	  pop();
	  push_copy (*num_ptr);
	}
    }
}


/* Decrement VAR_NAME by one.  This includes the special variables
   ibase, obase, and scale. */

void
decr_var (int var_name)
{
  bc_var *var_ptr;

  switch (var_name)
    {

    case 0: /* ibase */
      if (i_base > 2)
	i_base--;
      else
	rt_warn ("ibase too small in --");
      break;
      
    case 1: /* obase */
      if (o_base > 2)
	o_base--;
      else
	rt_warn ("obase too small in --");
      break;

    case 2: /* scale */
      if (scale > 0)
	scale--;
      else
	rt_warn ("scale can not be negative in -- ");
      break;

#if defined(READLINE) || defined(LIBEDIT)
    case 3: /* history */
      n_history--;
      if (n_history >= 0)
	HISTORY_SIZE(n_history);
      else
	{
	  n_history = -1;
	  rt_warn ("history is negative, set to unlimited");
	  UNLIMIT_HISTORY;
	}
      break;
#endif

    default: /* It is a simple variable. */
      var_ptr = get_var (var_name);
      if (var_ptr != NULL)
	bc_sub (var_ptr->v_value,_one_,&var_ptr->v_value, 0);
    }
}


/* Decrement VAR_NAME by one.  VAR_NAME is an array, and the top of
   the execution stack is the index and it is popped off the stack. */

void
decr_array (int var_name)
{
  bc_num *num_ptr;
  long   idx;

  /* It is an array variable. */
  if (!check_stack (1)) return;
  idx = bc_num2long (ex_stack->s_num);
  if (idx < 0 || idx > BC_DIM_MAX ||
     (idx == 0 && !bc_is_zero (ex_stack->s_num))) 
    rt_error ("Array %s subscript out of bounds.", a_names[var_name]);
  else
    {
      num_ptr = get_array_num (var_name, idx);
      if (num_ptr != NULL)
	{
	  pop ();
	  bc_sub (*num_ptr, _one_, num_ptr, 0);
	}
    }
}


/* Increment VAR_NAME by one.  This includes the special variables
   ibase, obase, and scale. */

void
incr_var (int var_name)
{
  bc_var *var_ptr;

  switch (var_name)
    {

    case 0: /* ibase */
      if (i_base < 16)
	i_base++;
      else
	rt_warn ("ibase too big in ++");
      break;

    case 1: /* obase */
      if (o_base < BC_BASE_MAX)
	o_base++;
      else
	rt_warn ("obase too big in ++");
      break;

    case 2:
      if (scale < BC_SCALE_MAX)
	scale++;
      else
	rt_warn ("Scale too big in ++");
      break;

#if defined(READLINE) || defined(LIBEDIT)
    case 3: /* history */
      n_history++;
      if (n_history > 0)
	HISTORY_SIZE(n_history);
      else
	{
	  n_history = -1;
	  rt_warn ("history set to unlimited");
	  UNLIMIT_HISTORY;
	}
      break;	
#endif

    default:  /* It is a simple variable. */
      var_ptr = get_var (var_name);
      if (var_ptr != NULL)
	bc_add (var_ptr->v_value, _one_, &var_ptr->v_value, 0);

    }
}


/* Increment VAR_NAME by one.  VAR_NAME is an array and top of
   execution stack is the index and is popped off the stack. */

void
incr_array (int var_name)
{
  bc_num *num_ptr;
  long   idx;

  if (!check_stack (1)) return;
  idx = bc_num2long (ex_stack->s_num);
  if (idx < 0 || idx > BC_DIM_MAX ||
      (idx == 0 && !bc_is_zero (ex_stack->s_num))) 
    rt_error ("Array %s subscript out of bounds.", a_names[var_name]);
  else
    {
      num_ptr = get_array_num (var_name, idx);
      if (num_ptr != NULL)
	{
	  pop ();
	  bc_add (*num_ptr, _one_, num_ptr, 0);
	}
    }
}


/* Routines for processing autos variables and parameters. */

/* NAME is an auto variable that needs to be pushed on its stack. */

void
auto_var (int name)
{
  bc_var *v_temp;
  bc_var_array *a_temp;
  int ix;

  if (name > 0)
    {
      /* A simple variable. */
      ix = name;
      v_temp = bc_malloc (sizeof (bc_var));
      v_temp->v_next = variables[ix];
      bc_init_num (&v_temp->v_value);
      variables[ix] = v_temp;
    }
  else
    {
      /* An array variable. */
      ix = -name;
      a_temp = bc_malloc (sizeof (bc_var_array));
      a_temp->a_next = arrays[ix];
      a_temp->a_value = NULL;
      a_temp->a_param = FALSE;
      arrays[ix] = a_temp;
    } 
}


/* Free_a_tree frees everything associated with an array variable tree.
   This is used when popping an array variable off its auto stack.  */

void
free_a_tree (bc_array_node *root, int depth)
{
  int ix;

  if (root != NULL)
    {
      if (depth > 1)
	for (ix = 0; ix < NODE_SIZE; ix++)
	  free_a_tree (root->n_items.n_down[ix], depth-1);
      else
	for (ix = 0; ix < NODE_SIZE; ix++)
	  bc_free_num ( &(root->n_items.n_num[ix]));
      free (root);
    }
}


/* LIST is an NULL terminated list of varible names that need to be
   popped off their auto stacks. */

void
pop_vars (arg_list *list)
{
  bc_var *v_temp;
  bc_var_array *a_temp;
  int    ix;

  while (list != NULL)
    {
      ix = list->av_name;
      if (ix > 0)
	{
	  /* A simple variable. */
	  v_temp = variables[ix];
	  if (v_temp != NULL)
	    {
	      variables[ix] = v_temp->v_next;
	      bc_free_num (&v_temp->v_value);
	      free (v_temp);
	    }
	}
      else
	{
	  /* An array variable. */
	  ix = -ix;
	  a_temp = arrays[ix];
	  if (a_temp != NULL)
	    {
	      arrays[ix] = a_temp->a_next;
	      if (!a_temp->a_param && a_temp->a_value != NULL)
		{
		  free_a_tree (a_temp->a_value->a_tree,
			       a_temp->a_value->a_depth);
		  free (a_temp->a_value);
		}
	      free (a_temp);
	    }
	} 
      list = list->next;
    }
}

/* COPY_NODE: Copies an array node for a call by value parameter. */
static bc_array_node *
copy_tree (bc_array_node *ary_node, int depth)
{
  bc_array_node *res = bc_malloc (sizeof(bc_array_node));
  int i;

  if (depth > 1)
    for (i=0; i<NODE_SIZE; i++)
      if (ary_node->n_items.n_down[i] != NULL)
	res->n_items.n_down[i] =
	  copy_tree (ary_node->n_items.n_down[i], depth - 1);
      else
	res->n_items.n_down[i] = NULL;
  else
    for (i=0; i<NODE_SIZE; i++)
      if (ary_node->n_items.n_num[i] != NULL)
	res->n_items.n_num[i] = bc_copy_num (ary_node->n_items.n_num[i]);
      else
	res->n_items.n_num[i] = NULL;
  return res;
}

/* COPY_ARRAY: Copies an array for a call by value array parameter. 
   ARY is the pointer to the bc_array structure. */

static bc_array *
copy_array (bc_array *ary)
{
  bc_array *res = bc_malloc (sizeof(bc_array));
  res->a_depth = ary->a_depth;
  res->a_tree = copy_tree (ary->a_tree, ary->a_depth);
  return (res);
}


/* A call is being made to FUNC.  The call types are at PC.  Process
   the parameters by doing an auto on the parameter variable and then
   store the value at the new variable or put a pointer the the array
   variable. */

void
process_params (program_counter *progctr, int func)
{
  char ch;
  arg_list *params;
  int ix, ix1;
  bc_var *v_temp;
  bc_var_array *a_src, *a_dest;
  
  /* Get the parameter names from the function. */
  params = functions[func].f_params;

  while ((ch = byte(progctr)) != ':')
    {
      if (params != NULL)
	{
	  if ((ch == '0') && params->av_name > 0)
	    {
	      /* A simple variable. */
	      ix = params->av_name;
	      v_temp = bc_malloc (sizeof(bc_var));
	      v_temp->v_next = variables[ix];
	      v_temp->v_value = ex_stack->s_num;
	      bc_init_num (&ex_stack->s_num);
	      variables[ix] = v_temp;
	    }
	  else
	    if ((ch == '1') && (params->av_name < 0))
	      {
		/* The variables is an array variable. */
	
		/* Compute source index and make sure some structure exists. */
		ix = (int) bc_num2long (ex_stack->s_num);
		(void) get_array_num (ix, 0);    
	
		/* Push a new array and Compute Destination index */
		auto_var (params->av_name);  
		ix1 = -params->av_name;

		/* Set up the correct pointers in the structure. */
		if (ix == ix1) 
		  a_src = arrays[ix]->a_next;
		else
		  a_src = arrays[ix];
		a_dest = arrays[ix1];
		if (params->arg_is_var)
		  {
		    a_dest->a_param = TRUE;
		    a_dest->a_value = a_src->a_value;
		  }
		else
		  {
		    a_dest->a_param = FALSE;
		    a_dest->a_value = copy_array (a_src->a_value);
		  }
	      }
	    else
	      {
		if (params->av_name < 0)
		  rt_error ("Parameter type mismatch parameter %s.",
			    a_names[-params->av_name]);
		else
		  rt_error ("Parameter type mismatch, parameter %s.",
			    v_names[params->av_name]);
		params++;
	      }
	  pop ();
	}
      else
	{
	    rt_error ("Parameter number mismatch");
	    return;
	}
      params = params->next;
    }
  if (params != NULL) 
    rt_error ("Parameter number mismatch");
}
