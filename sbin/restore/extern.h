/*	$NetBSD: extern.h,v 1.14 2005/06/27 02:03:28 christos Exp $	*/

/*-
 * Copyright (c) 1992, 1993
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
 *	@(#)extern.h	8.2 (Berkeley) 1/7/94
 */

struct entry	*addentry(const char *, ino_t, int);
long		 addfile(const char *, ino_t, int);
int		 addwhiteout(char *);
void		 badentry(struct entry *, const char *);
void	 	 canon(const char *, char *);
void		 checkrestore(void);
void 		 cleanup(void);
void		 closemt(void);
void		 createfiles(void);
void		 createleaves(const char *);
void		 createlinks(void);
long		 deletefile(const char *, ino_t, int);
void		 deleteino(ino_t);
void		 delwhiteout(struct entry *);
const struct digest_desc *
		 digest_lookup(const char *);
ino_t		 dirlookup(const char *);
void		 dumpsymtable(const char *, int32_t);
void	 	 extractdirs(int);
int		 extractfile(char *);
void		 findunreflinks(void);
char		*flagvalues(struct entry *);
void		 freeentry(struct entry *);
void		 freename(char *);
int	 	 genliteraldir(const char *, ino_t);
char		*gentempname(struct entry *);
void		 getfile(void (*)(char *, long), void (*)(char *, long));
void		 getvol(int);
void		 initsymtable(const char *);
int	 	 inodetype(ino_t);
int		 linkit(char *, char *, int);
struct entry	*lookupino(ino_t);
struct entry	*lookupname(const char *);
long		 listfile(const char *, ino_t, int);
ino_t		 lowerbnd(ino_t);
void		 mktempname(struct entry *);
void		 moveentry(struct entry *, const char *);
void		 msg(const char *, ...)
     __attribute__((__format__(__printf__,1,2)));
char		*myname(struct entry *);
void		 newnode(struct entry *);
void		 newtapebuf(long);
long		 nodeupdates(const char *, ino_t, int);
void	 	 onintr(int);
void		 panic(const char *, ...)
     __attribute__((__format__(__printf__,1,2)));
void		 pathcheck(char *);
struct direct	*pathsearch(const char *);
void		 printdumpinfo(void);
void		 removeleaf(struct entry *);
void		 removenode(struct entry *);
void		 removeoldleaves(void);
void		 removeoldnodes(void);
void		 renameit(char *, const char *);
int		 reply(const char *);
RST_DIR		*rst_opendir(const char *);
struct direct	*rst_readdir(RST_DIR *);
void		 rst_closedir(RST_DIR *);
void	 	 runcmdshell(void);
char		*savename(const char *);
void	 	 setdirmodes(int);
void		 setinput(const char *);
void		 setup(void);
void	 	 skipdirs(void);
void		 skipfile(void);
void		 skipmaps(void);
void		 swabst(u_char *, u_char *);
void	 	 treescan(const char *, ino_t,
    long (*)(const char *, ino_t, int));
ino_t		 upperbnd(ino_t);
long		 verifyfile(const char *, ino_t, int);
void		 writemtree(const char *, const char *, const uid_t,
				const gid_t, const mode_t, const u_long);
void		 xtrnull(char *, long);

/* From ../dump/dumprmt.c */
void		rmtclose(void);
int		rmthost(const char *);
int		rmtioctl(int, int);
int		rmtopen(const char *, int);
int		rmtread(const char *, int);
int		rmtseek(int, int);
