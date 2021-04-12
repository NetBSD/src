/*	$NetBSD: global.h,v 1.2 2021/04/12 08:54:11 mrg Exp $ */

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

/* global.h:  The global variables for bc.  */

/* The current break level's label. */
EXTERN int break_label;

/* The current if statement's else label or label after else. */
EXTERN int if_label;

/* The current for statement label for continuing the loop. */
EXTERN int continue_label;

/* Next available label number. */
EXTERN int next_label;

/* Byte code character storage.  Used in many places for generation of code. */
EXTERN char  *genstr  INIT(NULL);
EXTERN int    genlen  INIT(0);

/* Count of characters printed to the output in compile_only mode. */
EXTERN int out_count;

/* Have we generated any code since the last initialization of the code
   generator.  */
EXTERN char did_gen;

/* Is this run an interactive execution.  (Is stdin a terminal?) */
EXTERN char interactive  INIT(FALSE);

/* Just generate the byte code.  -c flag. */
EXTERN int compile_only INIT(FALSE);

/* Load the standard math functions.  -l flag. */
EXTERN int use_math  INIT(FALSE);

/* Give a warning on use of any non-standard feature (non-POSIX).  -w flag. */
EXTERN int warn_not_std  INIT(FALSE);

/* Accept POSIX bc only!  -s flag. */
EXTERN int std_only  INIT(FALSE);

/* Don't print the banner at start up.  -q flag. */
EXTERN int quiet  INIT(FALSE);

/* The list of file names to process. */
EXTERN file_node *file_names  INIT(NULL);

/* The name of the current file being processed. */
EXTERN char *file_name;

/* Is the current file a named file or standard input? */
EXTERN char   is_std_in;

/* global variables for the bc machine. All will be dynamic in size.*/
/* Function storage. main is (0) and functions (1-f_count) */

EXTERN bc_function *functions;
EXTERN char **f_names;
EXTERN int  f_count;

/* Variable stoarge and reverse names. */

EXTERN bc_var **variables;
EXTERN char **v_names;
EXTERN int  v_count;

/* Array Variable storage and reverse names. */

EXTERN bc_var_array **arrays;
EXTERN char **a_names;
EXTERN int  a_count;

/* Execution stack. */
EXTERN estack_rec *ex_stack;

/* Function return stack. */
EXTERN fstack_rec *fn_stack;

/* Current ibase, obase, scale, and n_history (if needed). */
EXTERN int i_base;
EXTERN int o_base;
EXTERN int scale;
#if defined(READLINE) || defined(LIBEDIT)
EXTERN int n_history;
#endif

#if defined(LIBEDIT)
/* LIBEDIT data */
EXTERN EditLine *edit INIT(NULL);
EXTERN History  *hist;
EXTERN HistEvent histev;
#endif

/* "Condition code" -- false (0) or true (1) */
EXTERN char c_code;

/* Records the number of the runtime error. */
EXTERN char runtime_error;

/* Holds the current location of execution. */
EXTERN program_counter pc;

/* For POSIX bc, this is just for number output, not strings. */
EXTERN int out_col;

/* Keeps track of the current number of characters per output line.
   This includes the \n at the end of the line. */
EXTERN int line_size;

/* Input Line numbers and other error information. */
EXTERN int line_no;
EXTERN int had_error;

/* For larger identifiers, a tree, and how many "storage" locations
   have been allocated. */

EXTERN int next_array;
EXTERN int next_func;
EXTERN int next_var;

EXTERN id_rec *name_tree;

/* For use with getopt.  Do not declare them here.*/
extern int optind;

/* Access to the yy input file.  Defined in scan.c. */
extern FILE *yyin;

/* Access to libmath */
extern CONST char *libmath[];
