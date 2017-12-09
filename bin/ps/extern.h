/*	$NetBSD: extern.h,v 1.39 2017/12/09 14:56:54 kamil Exp $	*/

/*-
 * Copyright (c) 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)extern.h	8.3 (Berkeley) 4/2/94
 */

/* 
 * We expect to be included by ps.h, which will already have
 * defined the types we use.
 */

extern double log_ccpu;
extern int eval, fscale, mempages, nlistread, maxslp, uspace;
extern int sumrusage, termwidth, totwidth;
extern int needenv, needcomm, commandonly;
extern uid_t myuid;
extern kvm_t *kd;
extern VAR var[];
extern VARLIST displaylist;
extern VARLIST sortlist;

void	 command(struct pinfo *, VARENT *, enum mode);
void	 cpuid(struct pinfo *, VARENT *, enum mode);
void	 cputime(struct pinfo *, VARENT *, enum mode);
void	 donlist(void);
void	 donlist_sysctl(void);
void	 fmt_puts(char *, int *);
void	 fmt_putc(int, int *);
void	 elapsed(struct pinfo *, VARENT *, enum mode);
double	 getpcpu(const struct kinfo_proc2 *);
double	 getpmem(const struct kinfo_proc2 *);
void	 gname(struct pinfo *, VARENT *, enum mode);
void	 groups(struct pinfo *, VARENT *, enum mode);
void	 groupnames(struct pinfo *, VARENT *, enum mode);
void	 lcputime(struct pinfo *, VARENT *, enum mode);
void	 logname(struct pinfo *, VARENT *, enum mode);
void	 longtname(struct pinfo *, VARENT *, enum mode);
void	 lname(struct pinfo *, VARENT *, enum mode);
void	 lstarted(struct pinfo *, VARENT *, enum mode);
void	 lstate(struct pinfo *, VARENT *, enum mode);
void	 maxrss(struct pinfo *, VARENT *, enum mode);
void	 nlisterr(struct nlist *);
void	 p_rssize(struct pinfo *, VARENT *, enum mode);
void	 pagein(struct pinfo *, VARENT *, enum mode);
void	 parsefmt(const char *);
void	 parsefmt_insert(const char *, VARENT **);
void	 parsesort(const char *);
VARENT * varlist_find(VARLIST *, const char *);
void	 emul(struct pinfo *, VARENT *, enum mode);
void	 pcpu(struct pinfo *, VARENT *, enum mode);
void	 pmem(struct pinfo *, VARENT *, enum mode);
void	 pnice(struct pinfo *, VARENT *, enum mode);
void	 pri(struct pinfo *, VARENT *, enum mode);
void	 printheader(void);
void	 putimeval(struct pinfo *, VARENT *, enum mode);
void	 pvar(struct pinfo *, VARENT *, enum mode);
void	 rgname(struct pinfo *, VARENT *, enum mode);
void	 rssize(struct pinfo *, VARENT *, enum mode);
void	 runame(struct pinfo *, VARENT *, enum mode);
void	 showkey(void);
void	 started(struct pinfo *, VARENT *, enum mode);
void	 state(struct pinfo *, VARENT *, enum mode);
void	 svgname(struct pinfo *, VARENT *, enum mode);
void	 svuname(struct pinfo *, VARENT *, enum mode);
void	 tdev(struct pinfo *, VARENT *, enum mode);
void	 tname(struct pinfo *, VARENT *, enum mode);
void	 tsize(struct pinfo *, VARENT *, enum mode);
void	 ucomm(struct pinfo *, VARENT *, enum mode);
void	 usrname(struct pinfo *, VARENT *, enum mode);
void	 uvar(struct pinfo *, VARENT *, enum mode);
void	 vsize(struct pinfo *, VARENT *, enum mode);
void	 wchan(struct pinfo *, VARENT *, enum mode);
