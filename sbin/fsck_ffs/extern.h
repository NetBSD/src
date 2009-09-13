/*	$NetBSD: extern.h,v 1.25 2009/09/13 14:25:28 bouyer Exp $	*/

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

void		adjust(struct inodesc *, int);
daddr_t		allocblk(long);
ino_t		allocdir(ino_t, ino_t, int);
ino_t		allocino(ino_t, int);
void		blkerror(ino_t, const char *, daddr_t);
int		bread(int, char *, daddr_t, long);
void		bufinit(void);
void		bwrite(int, char *, daddr_t, long);
void		cacheino(union dinode *, ino_t);
void		catch(int);
void		catchquit(int);
int		changeino(ino_t, const char *, ino_t);
int		chkrange(daddr_t, int);
void		ckfini(void);
int		ckinode(union dinode *, struct inodesc *);
int		clearentry(struct inodesc *);
void		clri(struct inodesc *, const char *, int);
int		cmpsblks(const struct fs *, struct fs *);
int		cmpsblks42(const struct fs *, struct fs *);
int		cmpsblks44(const struct fs *, struct fs *);
union		dinode * getnextinode(ino_t);
void		direrror(ino_t, const char *);
int		dirscan(struct inodesc *);
int		dofix(struct inodesc *, const char *);
void		fileerror(ino_t, ino_t, const char *);
int		findino(struct inodesc *);
int		findname(struct inodesc *);
void		flush(int, struct bufarea *);
void		freeblk(daddr_t, long);
void		freeino(ino_t);
void		freeinodebuf(void);
int		ftypeok(union dinode *);
void		getblk(struct bufarea *, daddr_t, long);
struct bufarea *getdatablk(daddr_t, long);
struct inoinfo *getinoinfo(ino_t);
union dinode  *ginode(ino_t);
void		getpathname(char *, size_t, ino_t, ino_t);
void		infohandler(int sig);
void		inocleanup(void);
void		inodirty(void);
struct inostat *inoinfo(ino_t);
int		linkup(ino_t, ino_t, char *);
int		makeentry(ino_t, ino_t, const char *);
void		pass1(void);
void		pass1b(void);
int		pass1check(struct inodesc *);
void		pass2(void);
void		pass3(void);
void		pass4(void);
int		pass4check(struct inodesc *);
void		pass5(void);
void		pinode(ino_t);
void		propagate(ino_t);
int		reply(const char *);
void		setinodebuf(ino_t);
int		setup(const char *, const char *);
void		voidquit(int);

int		check_wapbl(void);
void		replay_wapbl(void);
void		cleanup_wapbl(void);
int		read_wapbl(char *, long, daddr_t);
int		is_journal_inode(ino_t);

void		swap_cg(struct cg *, struct cg *);
void		copyback_cg(struct bufarea *);
void		sb_oldfscompat_write(struct fs *, struct fs *);
void		sb_oldfscompat_read(struct fs *, struct fs **);
