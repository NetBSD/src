#	$NetBSD: Makefile,v 1.17 1997/09/16 16:44:43 lukem Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/27/95

PROG=	fsck_ffs
MAN=	fsck_ffs.8
SRCS=	dir.c inode.c main.c pass1.c pass1b.c pass2.c pass3.c pass4.c \
	pass5.c fsutil.c setup.c utilities.c ffs_subr.c ffs_tables.c
.PATH:	${.CURDIR}/../../sys/ufs/ffs ${.CURDIR}/../fsck
CFLAGS+=-I${.CURDIR}/../fsck -g

.if make(install)
SUBDIR+= SMM.doc
.endif

.include <bsd.prog.mk>
