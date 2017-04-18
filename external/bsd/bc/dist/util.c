/*	$NetBSD: util.c,v 1.2.2.2 2017/04/18 04:35:19 maya Exp $ */

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

/* util.c: Utility routines for bc. */

#include "bcdefs.h"
#ifndef VARARGS
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include "proto.h"


/* strcopyof mallocs new memory and copies a string to to the new
   memory. */

char *
strcopyof (const char *str)
{
  char *temp;

  temp = bc_malloc (strlen (str)+1);
  return (strcpy (temp,str));
}


/* nextarg adds another value to the list of arguments. */

arg_list *
nextarg (arg_list *args, int val, int is_var)
{ arg_list *temp;

  temp = bc_malloc (sizeof (arg_list));
  temp->av_name = val;
  temp->arg_is_var = is_var;
  temp->next = args;
 
  return (temp);
}


/* For generate, we must produce a string in the form
    "val,val,...,val".  We also need a couple of static variables
   for retaining old generated strings.  It also uses a recursive
   function that builds the string. */

static char *arglist1 = NULL, *arglist2 = NULL;


/* make_arg_str does the actual construction of the argument string.
   ARGS is the pointer to the list and LEN is the maximum number of
   characters needed.  1 char is the minimum needed. 
 */

static char *make_arg_str (arg_list *args, int len);

static char *
make_arg_str (arg_list *args, int len)
{
  char *temp;
  char sval[30];

  /* Recursive call. */
  if (args != NULL)
    temp = make_arg_str (args->next, len+12);
  else
    {
      temp = bc_malloc (len);
      *temp = 0;
      return temp;
    }

  /* Add the current number to the end of the string. */
  if (args->arg_is_var)
    if (len != 1) 
      snprintf (sval, sizeof(sval), "*%d,", args->av_name);
    else
      snprintf (sval, sizeof(sval), "*%d", args->av_name);
  else
    if (len != 1) 
      snprintf (sval, sizeof(sval), "%d,", args->av_name);
    else
      snprintf (sval, sizeof(sval), "%d", args->av_name);
  temp = strcat (temp, sval);
  return (temp);
}

char *
arg_str (arg_list *args)
{
  if (arglist2 != NULL) 
    free (arglist2);
  arglist2 = arglist1;
  arglist1 = make_arg_str (args, 1);
  return (arglist1);
}

char *
call_str (arg_list *args)
{
  arg_list *temp;
  int       arg_count;
  int       ix;
  
  if (arglist2 != NULL) 
    free (arglist2);
  arglist2 = arglist1;

  /* Count the number of args and add the 0's and 1's. */
  for (temp = args, arg_count = 0; temp != NULL; temp = temp->next)
    arg_count++;
  arglist1 = bc_malloc(arg_count+1);
  for (temp = args, ix=0; temp != NULL; temp = temp->next)
    arglist1[ix++] = ( temp->av_name ? '1' : '0');
  arglist1[ix] = 0;
      
  return (arglist1);
}

/* free_args frees an argument list ARGS. */

void
free_args (arg_list *args)
{ 
  arg_list *temp;
 
  temp = args;
  while (temp != NULL)
    {
      args = args->next;
      free (temp);
      temp = args;
    }
}


/* Check for valid parameter (PARAMS) and auto (AUTOS) lists.
   There must be no duplicates any where.  Also, this is where
   warnings are generated for array parameters. */

void
check_params (arg_list *params, arg_list *autos)
{
  arg_list *tmp1, *tmp2;

  /* Check for duplicate parameters. */
  if (params != NULL)
    {
      tmp1 = params;
      while (tmp1 != NULL)
	{
	  tmp2 = tmp1->next;
	  while (tmp2 != NULL)
	    {
	      if (tmp2->av_name == tmp1->av_name) 
		yyerror ("duplicate parameter names");
	      tmp2 = tmp2->next;
	    }
	  if (tmp1->arg_is_var)
	    ct_warn ("Variable array parameter");
	  tmp1 = tmp1->next;
	}
    }

  /* Check for duplicate autos. */
  if (autos != NULL)
    {
      tmp1 = autos;
      while (tmp1 != NULL)
	{
	  tmp2 = tmp1->next;
	  while (tmp2 != NULL)
	    {
	      if (tmp2->av_name == tmp1->av_name) 
		yyerror ("duplicate auto variable names");
	      tmp2 = tmp2->next;
	    }
	  if (tmp1->arg_is_var)
	    yyerror ("* not allowed here");
	  tmp1 = tmp1->next;
	}
    }

  /* Check for duplicate between parameters and autos. */
  if ((params != NULL) && (autos != NULL))
    {
      tmp1 = params;
      while (tmp1 != NULL)
	{
	  tmp2 = autos;
	  while (tmp2 != NULL)
	    {
	      if (tmp2->av_name == tmp1->av_name) 
		yyerror ("variable in both parameter and auto lists");
	      tmp2 = tmp2->next;
	    }
	  tmp1 = tmp1->next;
	}
    }
}

/* genstr management to avoid buffer overflow. */
void
set_genstr_size (int size)
{
  if (size > genlen) {
    if (genstr != NULL)
      free(genstr);
    genstr = bc_malloc (size);
    genlen = size;
  }
}


/* Initialize the code generator the parser. */

void
init_gen (void)
{
  /* Get things ready. */
  break_label = 0;
  continue_label = 0;
  next_label  = 1;
  out_count = 2;
  if (compile_only) 
    printf ("@i");
  else
    init_load ();
  had_error = FALSE;
  did_gen = FALSE;
  set_genstr_size (64);
}


/* generate code STR for the machine. */

void
generate (const char *str)
{
  did_gen = TRUE;
  if (compile_only)
    {
      printf ("%s",str);
      out_count += strlen(str);
      if (out_count > 60)
	{
	  printf ("\n");
	  out_count = 0;
	}
    }
  else
    load_code (str);
}


/* Execute the current code as loaded. */

void
run_code(void)
{
  /* If no compile errors run the current code. */
  if (!had_error && did_gen)
    {
      if (compile_only)
	{
	  printf ("@r\n"); 
	  out_count = 0;
	}
      else
	execute ();
    }

  /* Reinitialize the code generation and machine. */
  if (did_gen)
    init_gen();
  else
    had_error = FALSE;
}


/* Output routines: Write a character CH to the standard output.
   It keeps track of the number of characters output and may
   break the output with a "\<cr>".  Always used for numbers. */

void
out_char (int ch)
{
  if (ch == '\n')
    {
      out_col = 0;
      putchar ('\n');
    }
  else
    {
      out_col++;
      if (out_col == line_size-1 && line_size != 0)
	{
	  putchar ('\\');
	  putchar ('\n');
	  out_col = 1;
	}
      putchar (ch);
    }
}

/* Output routines: Write a character CH to the standard output.
   It keeps track of the number of characters output and may
   break the output with a "\<cr>".  This one is for strings.
   In POSIX bc, strings are not broken across lines. */

void
out_schar (int ch)
{
  if (ch == '\n')
    {
      out_col = 0;
      putchar ('\n');
    }
  else
    {
      if (!std_only)
	{
	  out_col++;
	  if (out_col == line_size-1 && line_size != 0)
	    {
	      putchar ('\\');
	      putchar ('\n');
	      out_col = 1;
	    }
	}
      putchar (ch);
    }
}


/* The following are "Symbol Table" routines for the parser. */

/*  find_id returns a pointer to node in TREE that has the correct
    ID.  If there is no node in TREE with ID, NULL is returned. */

id_rec *
find_id (id_rec *tree, const char *id)
{
  int cmp_result;
  
  /* Check for an empty tree. */
  if (tree == NULL)
    return NULL;

  /* Recursively search the tree. */
  cmp_result = strcmp (id, tree->id);
  if (cmp_result == 0)
    return tree;  /* This is the item. */
  else if (cmp_result < 0)
    return find_id (tree->left, id);
  else
    return find_id (tree->right, id);  
}


/* insert_id_rec inserts a NEW_ID rec into the tree whose ROOT is
   provided.  insert_id_rec returns TRUE if the tree height from
   ROOT down is increased otherwise it returns FALSE.  This is a
   recursive balanced binary tree insertion algorithm. */

int insert_id_rec (id_rec **root, id_rec *new_id)
{
  id_rec *A, *B;

  /* If root is NULL, this where it is to be inserted. */
  if (*root == NULL)
    {
      *root = new_id;
      new_id->left = NULL;
      new_id->right = NULL;
      new_id->balance = 0;
      return (TRUE);
    }

  /* We need to search for a leaf. */
  if (strcmp (new_id->id, (*root)->id) < 0)
    {
      /* Insert it on the left. */
      if (insert_id_rec (&((*root)->left), new_id))
	{
	  /* The height increased. */
	  (*root)->balance --;
	  
	  switch ((*root)->balance)
	    {
	    case  0:  /* no height increase. */
	      return (FALSE);
	    case -1:  /* height increase. */
	      return (TRUE);
	    case -2:  /* we need to do a rebalancing act. */
	      A = *root;
	      B = (*root)->left;
	      if (B->balance <= 0)
		{
		  /* Single Rotate. */
		  A->left = B->right;
		  B->right = A;
		  *root = B;
		  A->balance = 0;
		  B->balance = 0;
		}
	      else
		{
		  /* Double Rotate. */
		  *root = B->right;
		  B->right = (*root)->left;
		  A->left = (*root)->right;
		  (*root)->left = B;
		  (*root)->right = A;
		  switch ((*root)->balance)
		    {
		    case -1:
		      A->balance = 1;
		      B->balance = 0;
		      break;
		    case  0:
		      A->balance = 0;
		      B->balance = 0;
		      break;
		    case  1:
		      A->balance = 0;
		      B->balance = -1;
		      break;
		    }
		  (*root)->balance = 0;
		}
	    }     
	} 
    }
  else
    {
      /* Insert it on the right. */
      if (insert_id_rec (&((*root)->right), new_id))
	{
	  /* The height increased. */
	  (*root)->balance ++;

	  switch ((*root)->balance)
	    {
	    case 0:  /* no height increase. */
	      return (FALSE);
	    case 1:  /* height increase. */
	      return (TRUE);
	    case 2:  /* we need to do a rebalancing act. */
	      A = *root;
	      B = (*root)->right;
	      if (B->balance >= 0)
		{
		  /* Single Rotate. */
		  A->right = B->left;
		  B->left = A;
		  *root = B;
		  A->balance = 0;
		  B->balance = 0;
		}
	      else
		{
		  /* Double Rotate. */
		  *root = B->left;
		  B->left = (*root)->right;
		  A->right = (*root)->left;
		  (*root)->left = A;
		  (*root)->right = B;
		  switch ((*root)->balance)
		    {
		    case -1:
		      A->balance = 0;
		      B->balance = 1;
		      break;
		    case  0:
		      A->balance = 0;
		      B->balance = 0;
		      break;
		    case  1:
		      A->balance = -1;
		      B->balance = 0;
		      break;
		    }
		  (*root)->balance = 0;
		}
	    }     
	} 
    }
  
  /* If we fall through to here, the tree did not grow in height. */
  return (FALSE);
}


/* Initialize variables for the symbol table tree. */

void
init_tree(void)
{
  name_tree  = NULL;
  next_array = 1;
  next_func  = 1;
  /* 0 => ibase, 1 => obase, 2 => scale, 3 => history, 4 => last. */
  next_var   = 5;
}


/* Lookup routines for symbol table names. */

int
lookup (char *name, int  namekind)
{
  id_rec *id;

  /* Warn about non-standard name. */
  if (strlen(name) != 1)
    ct_warn ("multiple letter name - %s", name);

  /* Look for the id. */
  id = find_id (name_tree, name);
  if (id == NULL)
    {
      /* We need to make a new item. */
      id = bc_malloc (sizeof (id_rec));
      id->id = strcopyof (name);
      id->a_name = 0;
      id->f_name = 0;
      id->v_name = 0;
      insert_id_rec (&name_tree, id);
    }

  /* Return the correct value. */
  switch (namekind)
    {
      
    case ARRAY:
      /* ARRAY variable numbers are returned as negative numbers. */
      if (id->a_name != 0)
	{
	  free (name);
	  return (-id->a_name);
	}
      id->a_name = next_array++;
      if (id->a_name < MAX_STORE)
	{
	  if (id->a_name >= a_count)
	    more_arrays ();
	  a_names[id->a_name] = name;
	  return (-id->a_name);
	}
      yyerror ("Too many array variables");
      bc_exit (1);
      /*NOTREACHED*/

    case FUNCT:
    case FUNCTDEF:
      if (id->f_name != 0)
	{
          free(name);
	  /* Check to see if we are redefining a math lib function. */ 
	  if (use_math && namekind == FUNCTDEF && id->f_name <= 6)
	    id->f_name = next_func++;
	  return (id->f_name);
	}
      id->f_name = next_func++;
      if (id->f_name < MAX_STORE)
	{
	  if (id->f_name >= f_count)
	    more_functions ();
          f_names[id->f_name] = name;
	  return (id->f_name);
	}
      yyerror ("Too many functions");
      bc_exit (1);
      /*NOTREACHED*/

    case SIMPLE:
      if (id->v_name != 0)
	{
	  free(name);
	  return (id->v_name);
	}
      id->v_name = next_var++;
      if (id->v_name <= MAX_STORE)
	{
	  if (id->v_name >= v_count)
	    more_variables ();
          v_names[id->v_name - 1] = name;
	  return (id->v_name);
	}
      yyerror ("Too many variables");
      bc_exit (1);
      /*NOTREACHED*/

    }

  yyerror ("End of util.c/lookup() reached.  Please report this bug.");
  bc_exit (1);
  /*NOTREACHED*/
}

/* Print out the limits of this program. */

void
limits(void)
{
  printf ("BC_BASE_MAX     = %d\n",  BC_BASE_MAX);
  printf ("BC_DIM_MAX      = %ld\n", (long) BC_DIM_MAX);
  printf ("BC_SCALE_MAX    = %d\n",  BC_SCALE_MAX);
  printf ("BC_STRING_MAX   = %d\n",  BC_STRING_MAX);
  printf ("MAX Exponent    = %ld\n", (long) LONG_MAX);
  printf ("Number of vars  = %ld\n", (long) MAX_STORE);
#ifdef OLD_EQ_OP
  printf ("Old assignment operatiors are valid. (=-, =+, ...)\n");
#endif 
}

/* bc_malloc will check the return value so all other places do not
   have to do it!  SIZE is the number of bytes to allocate. */

void *
bc_malloc (size_t size)
{
  void *ptr;

  ptr = (void *) malloc (size);
  if (ptr == NULL)
    out_of_memory ();

  return ptr;
}


/* The following routines are error routines for various problems. */

/* Malloc could not get enought memory. */

void
out_of_memory(void)
{
  fprintf (stderr, "Fatal error: Out of memory for malloc.\n");
  bc_exit (1);
  /*NOTREACHED*/
}



/* The standard yyerror routine.  Built with variable number of argumnets. */

#ifndef VARARGS
#ifdef __STDC__
void
yyerror (const char *str, ...)
#else
void
yyerror (str)
     const char *str;
#endif
#else
void
yyerror (str, va_alist)
     const char *str;
#endif
{
  const char *name;
  va_list args;

#ifndef VARARGS   
   va_start (args, str);
#else
   va_start (args);
#endif
  if (is_std_in)
    name = "(standard_in)";
  else
    name = file_name;
  fprintf (stderr,"%s %d: ",name,line_no);
  vfprintf (stderr, str, args);
  fprintf (stderr, "\n");
  had_error = TRUE;
  va_end (args);
}


/* The routine to produce warnings about non-standard features
   found during parsing. */

#ifndef VARARGS
#ifdef __STDC__
void 
ct_warn (const char *mesg, ...)
#else
void
ct_warn (mesg)
     const char *mesg;
#endif
#else
void
ct_warn (mesg, va_alist)
     const char *mesg;
#endif
{
  const char *name;
  va_list args;

#ifndef VARARGS   
  va_start (args, mesg);
#else
  va_start (args);
#endif
  if (std_only)
    {
      if (is_std_in)
	name = "(standard_in)";
      else
	name = file_name;
      fprintf (stderr,"%s %d: Error: ",name,line_no);
      vfprintf (stderr, mesg, args);
      fprintf (stderr, "\n");
      had_error = TRUE;
    }
  else
    if (warn_not_std)
      {
	if (is_std_in)
	  name = "(standard_in)";
	else
	  name = file_name;
	fprintf (stderr,"%s %d: (Warning) ",name,line_no);
	vfprintf (stderr, mesg, args);
	fprintf (stderr, "\n");
      }
  va_end (args);
}

/* Runtime error will  print a message and stop the machine. */

#ifndef VARARGS
#ifdef __STDC__
void
rt_error (const char *mesg, ...)
#else
void
rt_error (mesg)
     const char *mesg;
#endif
#else
void
rt_error (mesg, va_alist)
     const char *mesg;
#endif
{
  va_list args;

  fprintf (stderr, "Runtime error (func=%s, adr=%d): ",
	   f_names[pc.pc_func], pc.pc_addr);
#ifndef VARARGS   
  va_start (args, mesg);
#else
  va_start (args);
#endif
  vfprintf (stderr, mesg, args);
  va_end (args);
  
  fprintf (stderr, "\n");
  runtime_error = TRUE;
}


/* A runtime warning tells of some action taken by the processor that
   may change the program execution but was not enough of a problem
   to stop the execution. */

#ifndef VARARGS
#ifdef __STDC__
void
rt_warn (const char *mesg, ...)
#else
void
rt_warn (const char *mesg)
#endif
#else
void
rt_warn (const char *mesg)
#endif
{
  va_list args;

  fprintf (stderr, "Runtime warning (func=%s, adr=%d): ",
	   f_names[pc.pc_func], pc.pc_addr);
#ifndef VARARGS   
  va_start (args, mesg);
#else
  va_start (args);
#endif
  vfprintf (stderr, mesg, args);
  va_end (args);

  fprintf (stderr, "\n");
}

/* bc_exit: Make sure to reset the edit state. */

void bc_exit(int val)
{
#if defined(LIBEDIT)
  if (edit != NULL)
    el_end(edit);
#endif
  exit(val);
  /*NOTREACHED*/
}
