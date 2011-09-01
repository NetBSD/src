/*	Id: driver.h,v 1.2 2011/05/26 16:48:40 plunky Exp 	*/	
/*	$NetBSD: driver.h,v 1.1.1.1 2011/09/01 12:47:04 plunky Exp $	*/

/*-
 * Copyright (c) 2011 Joerg Sonnenberger <joerg@NetBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef DRIVER_H
#define DRIVER_H

#include "strlist.h"

extern const char *isysroot;
extern const char *sysroot;
extern const char *preprocessor;
extern const char *compiler;
extern const char *assembler;
extern const char *linker;

extern struct strlist crtdirs;
extern struct strlist sysincdirs;
extern struct strlist libdirs;
extern struct strlist progdirs;
extern struct strlist preprocessor_flags;
extern struct strlist compiler_flags;
extern struct strlist assembler_flags;
extern struct strlist early_linker_flags;
extern struct strlist middle_linker_flags;
extern struct strlist late_linker_flags;
extern struct strlist stdlib_flags;
extern struct strlist early_program_csu_files;
extern struct strlist late_program_csu_files;
extern struct strlist early_dso_csu_files;
extern struct strlist late_dso_csu_files;

void error(const char *fmt, ...);

void init_platform_specific(const char *, const char *);

#endif /* DRIVER_H */
