/*	$Id: common.h,v 1.1.1.2 2009/09/04 00:27:35 gmcgarry Exp $	*/
/*-
 * Copyright (c) 2008 Gregory McGarry <g.mcgarry@ieee.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#if defined(__ppc__)
#define IDENT(x) asm(".cstring\n\t.ascii \"" x "\\0\"")
#elif defined(__i386__)
#define IDENT(x) asm(".cstring\n\t.ascii \"" x "\\0\"")
#endif

#define NULL (void *)0

extern int main(int argc, char *argv[], char *envp[]);
extern void exit(int);
extern int atexit(void (*fcn)(void));

#ifdef CRT
static char *_strrchr(char *, int);
static int _strcmp(char *, char *);
#endif

#if PROFILE
extern void moninit(void);
static void _mcleanup(void);
extern void monitor(char *, char *, char *, int, int);
#endif

#ifdef DYNAMIC
extern void _dyld_init(void);
extern void _dyld_fini(void);
extern int _dyld_func_lookup(const char *, void **);
#endif

extern int (*mach_init_routine)(void);
extern int (*_cthread_init_routine)(void);

extern void _init(void);
extern void _fini(void);
