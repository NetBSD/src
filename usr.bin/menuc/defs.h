/*	$NetBSD: defs.h,v 1.3 1998/07/01 07:46:02 phil Exp $	 */

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software develooped for the NetBSD Project by
 *      Piermont Information Systems Inc.
 * 4. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* defs.h: definitions needed for the menu system. */

#include <stdio.h>

#include "mdb.h"

#ifdef MAIN
#define EXTERN 
#define INIT(x) = x
#else
#define EXTERN extern
#define INIT(x)
#endif

/* some constants */
#define TRUE 1
#define FALSE 0

/* Global variables .. to be defined in main.c, extern elsewhere. */

EXTERN char *prog_name;
EXTERN char *src_name;
EXTERN char *out_name INIT("menu_defs");
EXTERN char *sys_name INIT("menu_sys.def");

EXTERN int do_dynamic INIT(0);
EXTERN int line_no INIT(1);
EXTERN int had_errors INIT(FALSE);
EXTERN int max_strlen INIT(1);

EXTERN id_rec *root INIT(NULL);

EXTERN struct menu_info default_info;
EXTERN id_rec default_menu;

EXTERN action error_act INIT({NULL});

/* Prototypes. */

/* From util.c */
void yyerror (const char *, ...);
void buff_add_ch (char);
char *buff_copy (void); 

/* from scan.l */
int yylex (void);

/* from parse.y */
int yyparse (void);

/* Vars not defined in main.c */
extern FILE *yyin;

/* from avl.c */
id_rec *find_id (id_rec *tree, char *id);
int insert_id (id_rec **root, id_rec *new_id);

/* from mdb.c */
id_rec *get_menu (char *);
void check_defined (void);
void write_menu_file (char *);

