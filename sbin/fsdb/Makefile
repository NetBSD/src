#	$NetBSD: Makefile,v 1.4 1996/09/28 19:30:35 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	fsdb
MAN=	fsdb.8
SRCS= fsdb.c fsdbutil.c
SRCS+= dir.c inode.c pass1.c pass1b.c pass2.c pass3.c pass4.c \
	pass5.c setup.c utilities.c
SRCS+= ffs_subr.c ffs_tables.c
SRCS+= preen.c fsutil.o

CFLAGS+= -I${.CURDIR}/../fsck_ffs -I${.CURDIR}/../fsck
LDADD+=	-ledit -ltermcap
DPADD+=	${LIBEDIT} ${LIBTERMCAP}
.PATH:	${.CURDIR}/../fsck ${.CURDIR}/../fsck_ffs ${.CURDIR}/../../sys/ufs/ffs

.include <bsd.prog.mk>
