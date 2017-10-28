/*	$NetBSD: var.h,v 1.36 2017/10/28 03:59:11 kre Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)var.h	8.2 (Berkeley) 5/4/95
 */

#ifndef	VUNSET		/* double include protection */
/*
 * Shell variables.
 */

/* flags */
#define VUNSET		0x0001	/* the variable is not set */
#define VEXPORT		0x0002	/* variable is exported */
#define VREADONLY	0x0004	/* variable cannot be modified */
#define VNOEXPORT	0x0008	/* variable may not be exported */

#define VSTRFIXED	0x0010	/* variable struct is statically allocated */
#define VTEXTFIXED	0x0020	/* text is statically allocated */
#define VSTACK		0x0040	/* text is allocated on the stack */
#define VNOFUNC		0x0100	/* don't call the callback function */
#define VFUNCREF	0x0200	/* the function is called on ref, not set */

#define VNOSET		0x4000	/* do not set variable - just readonly test */
#define VNOERROR	0x8000	/* be quiet if set fails (no error msg) */

struct var;

union var_func_union {		/* function to be called when:  */
	void (*set_func)(const char *);		/* variable gets set/unset */
	char*(*ref_func)(struct var *);		/* variable is referenced */
};

struct var {
	struct var *next;		/* next entry in hash list */
	int flags;			/* flags are defined above */
	char *text;			/* name=value */
	int name_len;			/* length of name */
	union var_func_union v_u;	/* function to apply (sometimes) */
};


struct localvar {
	struct localvar *next;		/* next local variable in list */
	struct var *vp;			/* the variable that was made local */
	int flags;			/* saved flags */
	char *text;			/* saved text */
};


extern struct localvar *localvars;

extern struct var vifs;
extern char ifs_default[];
extern struct var vmail;
extern struct var vmpath;
extern struct var vpath;
extern struct var vps1;
extern struct var vps2;
extern struct var vps4;
extern struct var line_num;
#ifndef SMALL
extern struct var editrc;
extern struct var vterm;
extern struct var vtermcap;
extern struct var vhistsize;
extern struct var ps_lit;
extern struct var euname;
extern struct var random_num;
extern intmax_t sh_start_time;
#endif

extern int line_number;
extern int funclinebase;
extern int funclineabs;

/*
 * The following macros access the values of the above variables.
 * They have to skip over the name.  They return the null string
 * for unset variables.
 */

#define ifsset()	((vifs.flags & VUNSET) == 0)
#define ifsval()	(ifsset() ? (vifs.text + 4) : ifs_default)
#define mailval()	(vmail.text + 5)
#define mpathval()	(vmpath.text + 9)
#define pathval()	(vpath.text + 5)
#define ps1val()	(vps1.text + 4)
#define ps2val()	(vps2.text + 4)
#define ps4val()	(vps4.text + 4)
#define optindval()	(voptind.text + 7)
#ifndef SMALL
#define histsizeval()	(vhistsize.text + 9)
#define termval()	(vterm.text + 5)
#endif

#define mpathset()	((vmpath.flags & VUNSET) == 0)

void initvar(void);
void setvar(const char *, const char *, int);
void setvareq(char *, int);
struct strlist;
void listsetvar(struct strlist *, int);
char *lookupvar(const char *);
char *bltinlookup(const char *, int);
char **environment(void);
void shprocvar(void);
int showvars(const char *, int, int, const char *);
void mklocal(const char *, int);
void listmklocal(struct strlist *, int);
void poplocalvars(void);
int unsetvar(const char *, int);
void choose_ps1(void);
int setvarsafe(const char *, const char *, int);
void print_quoted(const char *);
int validname(const char *, int, int *);

#endif
