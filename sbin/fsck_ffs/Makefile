#	$NetBSD: Makefile,v 1.16 1997/05/08 21:11:10 gwr Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	fsck_ffs
MAN=	fsck_ffs.8
SRCS=	dir.c inode.c main.c pass1.c pass1b.c pass2.c pass3.c pass4.c \
	pass5.c fsutil.c setup.c utilities.c ffs_subr.c ffs_tables.c
.PATH:	${.CURDIR}/../../sys/ufs/ffs ${.CURDIR}/../fsck
CFLAGS+=-I${.CURDIR}/../fsck

.if make(install)
SUBDIR+= SMM.doc
.endif

.include <bsd.prog.mk>
