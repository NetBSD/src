/*	$NetBSD: proto.h,v 1.2 2017/04/10 15:13:22 christos Exp $ */

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

/* proto.h: Prototype function definitions for "external" functions. */

/* For the pc version using k&r ACK. (minix1.5 and earlier.) */
#ifdef SHORTNAMES
#define init_numbers i_numbers
#define push_constant push__constant
#define load_const in_load_const
#define yy_get_next_buffer yyget_next_buffer
#define yy_init_buffer yyinit_buffer
#define yy_last_accepting_state yylast_accepting_state
#define arglist1 arg1list
#endif

/* Include the standard library header files. */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

/* From execute.c */
void stop_execution (int);
unsigned char byte (program_counter *pc_);
void execute (void);
int prog_char (void);
int input_char (void);
void push_constant (int (*in_char)(void), int conv_base);
void push_b10_const (program_counter *pc_);
void assign (char code);

/* From util.c */
char *strcopyof (const char *str);
arg_list *nextarg (arg_list *args, int val, int is_var);
char *arg_str (arg_list *args);
char *call_str (arg_list *args);
void free_args (arg_list *args);
void check_params (arg_list *params, arg_list *autos);
void set_genstr_size (int);
void init_gen (void);
void generate (const char *str);
void run_code (void);
void out_char (int ch);
void out_schar (int ch);
id_rec *find_id (id_rec *tree, const char *id);
int insert_id_rec (id_rec **root, id_rec *new_id);
void init_tree (void);
int lookup (char *name, int namekind);
void *bc_malloc (size_t);
void out_of_memory (void) __attribute__((__noreturn__));
void welcome (void);
void warranty (const char *);
void show_bc_version (void);
void limits (void);
void yyerror (const char *str, ...)
    __attribute__((__format__ (__printf__, 1, 2)));
void ct_warn (const char *mesg, ...)
    __attribute__((__format__ (__printf__, 1, 2)));
void rt_error (const char *mesg, ...)
    __attribute__((__format__ (__printf__, 1, 2)));
void rt_warn (const char *mesg, ...)
    __attribute__((__format__ (__printf__, 1, 2)));
void bc_exit (int) __attribute__((__noreturn__));

/* From load.c */
void init_load (void);
void addbyte (unsigned char thebyte);
void def_label (unsigned long lab);
long long_val (const char **str);
void load_code (const char *code);

/* From main.c */
int open_new_file (void);
void new_yy_file (FILE *file);
void use_quit (int) __attribute__((__noreturn__));

/* From storage.c */
void init_storage (void);
void more_functions (void);
void more_variables (void);
void more_arrays (void);
void clear_func (int func);
int fpop (void);
void fpush (int val);
void pop (void);
void push_copy (bc_num num);
void push_num (bc_num num);
char check_stack (int depth);
bc_var *get_var (int var_name);
bc_num *get_array_num (int var_index, unsigned long _index_);
void store_var (int var_name);
void store_array (int var_name);
void load_var (int var_name);
void load_array (int var_name);
void decr_var (int var_name);
void decr_array (int var_name);
void incr_var (int var_name);
void incr_array (int var_name);
void auto_var (int name);
void free_a_tree (bc_array_node *root, int depth);
void pop_vars (arg_list *list);
void process_params (program_counter *_pc_, int func);

/* For the scanner and parser.... */
int yyparse (void);
int yylex (void);

#if defined(LIBEDIT)
/* The *?*&^ prompt function */
char *null_prompt (EditLine *);
#endif

/* Other things... */
#ifndef HAVE_UNISTD_H
(int getopt (int, char *[], CONST char *);
#endif
