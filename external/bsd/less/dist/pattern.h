/*	$NetBSD: pattern.h,v 1.3.2.1 2014/05/22 15:45:43 yamt Exp $	*/

/*
 * Copyright (C) 1984-2012  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

#if HAVE_GNU_REGEX
#define __USE_GNU 1
#include <regex.h>
#define DEFINE_PATTERN(name)  struct re_pattern_buffer *name
#define CLEAR_PATTERN(name)   name = NULL
#endif

#if HAVE_POSIX_REGCOMP
#include <regex.h>
#ifdef REG_EXTENDED
extern int more_mode;
#define	REGCOMP_FLAG	(more_mode ? 0 : REG_EXTENDED)
#else
#define	REGCOMP_FLAG	0
#endif
#define DEFINE_PATTERN(name)  regex_t *name
#define CLEAR_PATTERN(name)   name = NULL
#endif

#if HAVE_PCRE
#include <pcre.h>
#define DEFINE_PATTERN(name)  pcre *name
#define CLEAR_PATTERN(name)   name = NULL
#endif

#if HAVE_RE_COMP
char *re_comp();
int re_exec();
#define DEFINE_PATTERN(name)  int name
#define CLEAR_PATTERN(name)   name = 0
#endif

#if HAVE_REGCMP
char *regcmp();
char *regex();
extern char *__loc1;
#define DEFINE_PATTERN(name)  char *name
#define CLEAR_PATTERN(name)   name = NULL
#endif

#if HAVE_V8_REGCOMP
#include "regexp.h"
#define DEFINE_PATTERN(name)  struct regexp *name
#define CLEAR_PATTERN(name)   name = NULL
#endif

#if NO_REGEX
#define DEFINE_PATTERN(name)  
#define CLEAR_PATTERN(name)   
#endif
