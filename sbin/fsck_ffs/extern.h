/*	$NetBSD: extern.h,v 1.18 2003/07/13 08:16:15 itojun Exp $	*/

/*
 * Copyright (c) 1994 James A. Jegers
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

void		adjust __P((struct inodesc *, int));
daddr_t		allocblk __P((long));
ino_t		allocdir __P((ino_t, ino_t, int));
ino_t		allocino __P((ino_t request, int type));
void		blkerror __P((ino_t, char *, daddr_t));
int		bread __P((int, char *, daddr_t, long));
void		bufinit __P((void));
void		bwrite __P((int, char *, daddr_t, long));
void		cacheino __P((union dinode *, ino_t));
void		catch __P((int));
void		catchquit __P((int));
int		changeino __P((ino_t, char *, ino_t));
int		chkrange __P((daddr_t, int));
void		ckfini __P((void));
int		ckinode __P((union dinode *, struct inodesc *));
int		clearentry __P((struct inodesc *));
void		clri __P((struct inodesc *, char *, int));
int		cmpsblks __P((const struct fs *, struct fs *));
union		dinode * getnextinode __P((ino_t));
void		direrror __P((ino_t, char *));
int		dirscan __P((struct inodesc *));
int		dofix __P((struct inodesc *, char *));
void		fileerror __P((ino_t, ino_t, char *));
int		findino __P((struct inodesc *));
int		findname __P((struct inodesc *));
void		flush __P((int, struct bufarea *));
void		freeblk __P((daddr_t, long));
void		freeino __P((ino_t));
void		freeinodebuf __P((void));
int		ftypeok __P((union dinode *));
void		getblk __P((struct bufarea *, daddr_t, long));
struct bufarea *getdatablk __P((daddr_t, long));
struct inoinfo *getinoinfo __P((ino_t));
union dinode  *ginode __P((ino_t));
void		getpathname __P((char *, size_t, ino_t, ino_t));
void		infohandler __P((int sig));
void		inocleanup __P((void));
void		inodirty __P((void));
struct inostat *inoinfo __P((ino_t));
int		linkup __P((ino_t, ino_t, char *));
int		makeentry __P((ino_t, ino_t, char *));
void		pass1 __P((void));
void		pass1b __P((void));
int		pass1check __P((struct inodesc *));
void		pass2 __P((void));
void		pass3 __P((void));
void		pass4 __P((void));
int		pass4check __P((struct inodesc *));
void		pass5 __P((void));
void		pinode __P((ino_t));
void		propagate __P((ino_t));
int		reply __P((char *));
void		setinodebuf __P((ino_t));
int		setup __P((const char *));
void		voidquit __P((int));

void	swap_cg __P((struct cg *, struct cg *));
void copyback_cg __P((struct bufarea *));
void sb_oldfscompat_write(struct fs *);
