#	$NetBSD: Makefile,v 1.7 1997/05/08 21:11:12 gwr Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	fsdb
MAN=	fsdb.8
SRCS= fsdb.c fsdbutil.c
SRCS+= dir.c inode.c pass1.c pass1b.c pass2.c pass3.c pass4.c \
	pass5.c setup.c utilities.c
SRCS+= ffs_subr.c ffs_tables.c
SRCS+= fsutil.c

CFLAGS+= -I${.CURDIR}/../fsck_ffs -I${.CURDIR}/../fsck
LDADD+=	-ledit -ltermcap
DPADD+=	${LIBEDIT} ${LIBTERMCAP}
.PATH:	${.CURDIR}/../fsck ${.CURDIR}/../fsck_ffs ${.CURDIR}/../../sys/ufs/ffs

.include <bsd.prog.mk>
