#	$NetBSD: Makefile,v 1.6 1997/05/06 20:45:06 gwr Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	fsdb
MAN=	fsdb.8
SRCS= fsdb.c fsdbutil.c
SRCS+= dir.c inode.c pass1.c pass1b.c pass2.c pass3.c pass4.c \
	pass5.c setup.c utilities.c
SRCS+= ffs_subr.c ffs_tables.c
SRCS+= fsutil.c

CFLAGS+= -I${.CURDIR}/../fsck_ffs
.PATH.c:   ${.CURDIR}/../fsck_ffs

CFLAGS+= -I${.CURDIR}/../fsck
.PATH.c:   ${.CURDIR}/../fsck

.PATH.c: ${.CURDIR}/../../sys/ufs/ffs

LDADD+=	-ledit -ltermcap
DPADD+=	${LIBEDIT} ${LIBTERMCAP}

.include <bsd.prog.mk>
