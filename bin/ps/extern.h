/*	$NetBSD: extern.h,v 1.20 2001/07/14 06:53:43 matt Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

struct kinfo;
struct nlist;
struct var;
struct varent;

extern double ccpu;
extern int eval, fscale, mempages, nlistread, rawcpu, maxslp, uspace;
extern int sumrusage, termwidth, totwidth;
extern int needenv, needcomm, commandonly, dontuseprocfs, use_procfs;
extern uid_t myuid;
extern kvm_t *kd;
extern VAR var[];
extern VARENT *vhead;

__BEGIN_DECLS
void	 command __P((struct kinfo_proc2 *, VARENT *, int));
void	 cputime __P((struct kinfo_proc2 *, VARENT *, int));
int	 donlist __P((void));
int	 donlist_sysctl __P((void));
void	 fmt_puts __P((char *, int *));
void	 fmt_putc __P((int, int *));
double	 getpcpu __P((struct kinfo_proc2 *));
double	 getpmem __P((struct kinfo_proc2 *));
void	 logname __P((struct kinfo_proc2 *, VARENT *, int));
void	 longtname __P((struct kinfo_proc2 *, VARENT *, int));
void	 lstarted __P((struct kinfo_proc2 *, VARENT *, int));
void	 maxrss __P((struct kinfo_proc2 *, VARENT *, int));
void	 nlisterr __P((struct nlist *));
void	 p_rssize __P((struct kinfo_proc2 *, VARENT *, int));
void	 pagein __P((struct kinfo_proc2 *, VARENT *, int));
void	 parsefmt __P((char *));
void	 pcpu __P((struct kinfo_proc2 *, VARENT *, int));
void	 pmem __P((struct kinfo_proc2 *, VARENT *, int));
void	 pnice __P((struct kinfo_proc2 *, VARENT *, int));
void	 pri __P((struct kinfo_proc2 *, VARENT *, int));
void	 printheader __P((void));
struct kinfo_proc2
	*getkinfo_procfs __P((int, int, int *));
char	**procfs_getargv __P((const struct kinfo_proc2 *, int));
void	 pvar __P((struct kinfo_proc2 *, VARENT *, int));
void	 rssize __P((struct kinfo_proc2 *, VARENT *, int));
void	 runame __P((struct kinfo_proc2 *, VARENT *, int));
void	 showkey __P((void));
void	 started __P((struct kinfo_proc2 *, VARENT *, int));
void	 state __P((struct kinfo_proc2 *, VARENT *, int));
void	 tdev __P((struct kinfo_proc2 *, VARENT *, int));
void	 tname __P((struct kinfo_proc2 *, VARENT *, int));
void	 tsize __P((struct kinfo_proc2 *, VARENT *, int));
void	 ucomm __P((struct kinfo_proc2 *, VARENT *, int));
void	 uname __P((struct kinfo_proc2 *, VARENT *, int));
void	 uvar __P((struct kinfo_proc2 *, VARENT *, int));
void	 vsize __P((struct kinfo_proc2 *, VARENT *, int));
void	 wchan __P((struct kinfo_proc2 *, VARENT *, int));
__END_DECLS
