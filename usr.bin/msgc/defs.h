/*	$NetBSD: defs.h,v 1.4.56.1 2012/04/17 00:09:36 yamt Exp $	*/

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
 * 3. The name of Piermont Information Systems Inc. may not be used to endorse
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

/* defs.h: definitions needed for the message system. */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <stdio.h>
#include "msgdb.h"

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
EXTERN char *out_name INIT("msg_defs");
EXTERN char *sys_name INIT("msg_sys.def");

EXTERN int line_no INIT(1);
EXTERN int had_errors INIT(FALSE);
EXTERN int max_strlen INIT(1);

EXTERN id_rec *root INIT(NULL);

/* Prototypes. */

/* From util.c */
void yyerror(const char *, ...);
void buff_add_ch(char);
char *buff_copy(void); 

/* From avl.c */
id_rec *find_id(id_rec *, char *);
int insert_id(id_rec **, id_rec *);

/* from scan.l */
int yylex(void);

/* from parse.y */
int yyparse(void);

/* Vars not defined in main.c */
extern FILE *yyin;

/* from mdb.c */
void define_msg(char *, char *);
void write_msg_file(void);
