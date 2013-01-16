/*	$NetBSD: extern.h,v 1.7.2.1 2013/01/16 05:32:33 yamt Exp $	*/

/*
 * Copyright (c) 1997 Manuel Bouyer.
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

void	adjust(struct inodesc *, short);
int	allocblk(void);
int	allocdir(ino_t, ino_t, int);
void	blkerror(ino_t, const char *, daddr_t);
int	bread(int, char *, daddr_t, long);
void	bufinit(void);
void	bwrite(int, char *, daddr_t, long);
void	cacheino(struct ext2fs_dinode *, ino_t);
int	changeino(ino_t, const char *, ino_t);
int	chkrange(daddr_t, int);
void	ckfini(int);
int	ckinode(struct ext2fs_dinode *, struct inodesc *);
void	clri(struct inodesc *, const char *, int);
int	dircheck(struct inodesc *, struct ext2fs_direct *);
void	direrror(ino_t, const char *);
int	dirscan(struct inodesc *);
int	dofix(struct inodesc *, const char *);
void	fileerror(ino_t, ino_t, const char *);
int	findino(struct inodesc *);
int	findname(struct inodesc *);
void	flush(int, struct bufarea *);
void	freeblk(daddr_t);
void	freeino(ino_t);
void	freeinodebuf(void);
int	ftypeok(struct ext2fs_dinode *);
void	getpathname(char *, size_t, ino_t, ino_t);
void	inocleanup(void);
void	inodirty(void);
u_int64_t inosize(struct ext2fs_dinode *);
void	inossize(struct ext2fs_dinode *, u_int64_t);
int	linkup(ino_t, ino_t);
int	makeentry(ino_t, ino_t, const char *);
void	pass1(void);
void	pass1b(void);
void	pass2(void);
void	pass3(void);
void	pass4(void);
int	pass1check(struct inodesc *);
int	pass4check(struct inodesc *);
void	pass5(void);
void	pinode(ino_t);
void	propagate(void);
int	reply(const char *);
void	resetinodebuf(void);
int	setup(const char *);
struct	ext2fs_dinode * getnextinode(ino_t);
uint64_t inonblock(struct ext2fs_dinode *);
void	inosnblock(struct ext2fs_dinode *, uint64_t);
