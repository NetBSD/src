/*	$NetBSD: bcdefs.h,v 1.1.2.2 2017/04/10 02:28:24 phil Exp $ */

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

/* bcdefs.h:  The single file to include all constants and type definitions. */

/* Include the configuration file. */
#include "config.h"

/* Standard includes for all files. */
#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#if defined(LIBEDIT)
#include <histedit.h>
#endif

#if defined(READLINE)
#include <readline/readline.h>
#include <readline/history.h>
#endif

/* Initialization magic ... */
#ifdef _GLOBAL_C
#define EXTERN 
#define INIT(x) = x
#else
#define EXTERN extern
#define INIT(x)
#endif

/* Include the other definitions. */
#include "const.h"
#include "number.h"

/* These definitions define all the structures used in
   code and data storage.  This includes the representation of
   labels.   The "guiding" principle is to make structures that
   take a minimum of space when unused but can be built to contain
   the full structures.  */

/* Labels are first.  Labels are generated sequentially in functions
   and full code.  They just "point" to a single bye in the code.  The
   "address" is the byte number.  The byte number is used to get an
   actual character pointer. */

typedef struct bc_label_group
    {
      unsigned long l_adrs [ BC_LABEL_GROUP ];
      struct bc_label_group *l_next;
    } bc_label_group;

/* Argument list.  Recorded in the function so arguments can
   be checked at call time. */

typedef struct arg_list
    {
      int av_name;
      int arg_is_var;		/* Extension ... variable parameters. */
      struct arg_list *next;
    } arg_list;

/* Each function has its own code segments and labels.  There can be
   no jumps between functions so labels are unique to a function. */

typedef struct 
    {
      char f_defined;   /* Is this function defined yet. */
      char f_void;	/* Is this function a void function. */
      char *f_body;
      size_t f_body_size;  /* Size of body.  Power of 2. */
      size_t f_code_size;
      bc_label_group *f_label;
      arg_list *f_params;
      arg_list *f_autos;
    } bc_function;

/* Code addresses. */
typedef struct {
      unsigned int pc_func;
      unsigned int pc_addr;
    } program_counter;


/* Variables are "pushable" (auto) and thus we need a stack mechanism.
   This is built into the variable record. */

typedef struct bc_var
    {
      bc_num v_value;
      struct bc_var *v_next;
    }  bc_var;


/* bc arrays can also be "auto" variables and thus need the same
   kind of stacking mechanisms. */

typedef struct bc_array_node
    {
      union
	{
	  bc_num n_num [NODE_SIZE];
	  struct bc_array_node *n_down [NODE_SIZE];
	} n_items;
    } bc_array_node;

typedef struct bc_array
    {
      bc_array_node *a_tree;
      short a_depth;
    } bc_array;

typedef struct bc_var_array
    {
      bc_array *a_value;
      char      a_param;
      struct bc_var_array *a_next;
    } bc_var_array;


/* For the stacks, execution and function, we need records to allow
   for arbitrary size. */

typedef struct estack_rec {
	bc_num s_num;
	struct estack_rec *s_next;
} estack_rec;

typedef struct fstack_rec {
	int  s_val;
	struct fstack_rec *s_next;
} fstack_rec;


/* The following are for the name tree. */

typedef struct id_rec {
	char  *id;      /* The program name. */
			/* A name == 0 => nothing assigned yet. */
	int   a_name;   /* The array variable name (number). */
	int   f_name;   /* The function name (number).  */
	int   v_name;   /* The variable name (number).  */
        short balance;  /* For the balanced tree. */
	struct id_rec *left, *right; /* Tree pointers. */
} id_rec;


/* A list of files to process. */

typedef struct file_node {
	char *name;
	struct file_node *next;
} file_node;

/* Macro Definitions */

#if defined(LIBEDIT)
#define HISTORY_SIZE(n) history(hist, &histev, H_SETSIZE, n)
#define UNLIMIT_HISTORY history(hist, &histev, H_SETSIZE, INT_MAX)
#endif

#if defined(READLINE)
#define HISTORY_SIZE(n) stifle_history(n)
#define UNLIMIT_HISTORY unstifle_history()
#endif

/* Now the global variable declarations. */
#include "global.h"
