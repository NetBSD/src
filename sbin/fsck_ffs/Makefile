#	$NetBSD: Makefile,v 1.22 1999/01/15 13:32:06 bouyer Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/27/95

PROG=	fsck_ffs
MAN=	fsck_ffs.8
SRCS=	dir.c inode.c main.c pass1.c pass1b.c pass2.c pass3.c pass4.c \
	pass5.c fsutil.c setup.c utilities.c ffs_bswap.c ffs_subr.c ffs_tables.c
FSCK=	${.CURDIR}/../fsck
CPPFLAGS+=-I${FSCK}
.PATH:	${.CURDIR}/../../sys/ufs/ffs ${FSCK}


.if make(install)
SUBDIR+=SMM.doc
.endif

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
