#	$NetBSD: Makefile,v 1.1 1999/03/18 02:02:18 perseant Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	fsck_lfs
MAN=	fsck_lfs.8
SRCS=	dir.c inode.c main.c pass1.c pass2.c pass3.c pass4.c \
	fsutil.c setup.c utilities.c lfs_cksum.c vars.c # lfs_subr.c
SRCS+=  pass0.c
#SRCS+= pass1b.c pass5.c ffs_subr.c ffs_tables.c
FSCK=   ${.CURDIR}/../fsck
.PATH:	${.CURDIR}/../../sys/ufs/lfs ${FSCK}
CPPFLAGS+=-I${FSCK} #-I../../sys -I../../include -DVERBOSE_BLOCKMAP
CFLAGS+=-g
LDFLAGS+=-g

.include <bsd.prog.mk>
