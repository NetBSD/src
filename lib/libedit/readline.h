/*	$NetBSD: readline.h,v 1.2 1997/10/23 22:52:02 christos Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jaromir Dolecek.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _READLINE_H_
#define _READLINE_H_

#include <sys/types.h>

/* list of readline stuff supported by editline library's readline wrapper */

/* typedefs */
typedef int Function __P((const char *, int));
typedef void VFunction __P((void));
typedef char   *CPFunction __P((const char *, int));
typedef char  **CPPFunction __P((const char *, int, int));

typedef struct _hist_entry {
	const char     *line;
	const char     *data;
}               HIST_ENTRY;

/* global variables used by readline enabled applications */
__BEGIN_DECLS
extern const char  *rl_library_version;
extern char 	   *rl_readline_name;
extern FILE        *rl_instream;
extern FILE        *rl_outstream;
extern char	   *rl_line_buffer;
extern int          rl_point, rl_end;
extern int          history_base, history_length;
extern int          max_input_history;
extern char        *rl_basic_word_break_characters;
extern char        *rl_completer_word_break_characters;
extern char        *rl_completer_quote_characters;
extern CPFunction  *rl_completion_entry_function;
extern CPPFunction *rl_attempted_completion_function;

/* supported functions */
char *readline __P((const char *));
int rl_initialize __P((void));

void using_history __P((void));
int add_history __P((const char *));
void clear_history __P((void));
void stifle_history __P((int));
int unstifle_history __P((void));
int history_is_stifled __P((void));
int where_history __P((void));
HIST_ENTRY *current_history __P((void));
HIST_ENTRY *history_get __P((int));
int history_total_bytes __P((void));
int history_set_pos __P((int));
HIST_ENTRY *previous_history __P((void));
HIST_ENTRY *next_history __P((void));
int history_search __P((const char *, int));
int history_search_prefix __P((const char *, int));
int history_search_pos __P((const char *, int, int));
int read_history __P((const char *));
int write_history __P((const char *));
int history_expand __P((char *, char **));
char **history_tokenize __P((const char *));

char *tilde_expand __P((char *));
char *filename_completion_function __P((const char *, int));
char *username_completion_function __P((const char *, int));
int rl_complete __P((int, int));
int rl_read_key __P((void));
char **completion_matches __P((const char *, CPFunction *));

int rl_insert   __P((int, int));
void rl_reset_terminal __P((const char *));
int rl_bind_key __P((int, int (*)(int, int)));
__END_DECLS

#endif /* _READLINE_H_ */
