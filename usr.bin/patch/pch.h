/*	$NetBSD: pch.h,v 1.9 2003/07/12 13:47:44 itojun Exp $	*/

/*
 * Copyright (c) 1988, Larry Wall
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following condition
 * is met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this condition and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

void re_patch(void);
void open_patch_file(char *);
void set_hunkmax(void);
void grow_hunkmax(void);
bool there_is_another_patch(void);
int intuit_diff_type(void);
void next_intuit_at(long, LINENUM);
void skip_to(long, LINENUM);
bool another_hunk(void);
char *pgets(char *, int, FILE *);
bool pch_swap(void);
LINENUM pch_first(void);
LINENUM pch_ptrn_lines(void);
LINENUM pch_newfirst(void);
LINENUM pch_repl_lines(void);
LINENUM pch_end(void);
LINENUM pch_context(void);
size_t pch_line_len(LINENUM);
char pch_char(LINENUM);
char *pfetch(LINENUM);
LINENUM pch_hunk_beg(void);
void do_ed_script(void);
