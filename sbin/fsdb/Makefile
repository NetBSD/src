#	$NetBSD: Makefile,v 1.11 1998/03/18 17:03:15 bouyer Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	fsdb
MAN=	fsdb.8
SRCS=	fsdb.c fsdbutil.c
SRCS+=	dir.c inode.c pass1.c pass1b.c pass2.c pass3.c pass4.c \
	pass5.c setup.c utilities.c
SRCS+=	ffs_bswap.c ffs_subr.c ffs_tables.c
SRCS+=	fsutil.c 

FSCK=	${.CURDIR}/../fsck
FSCK_FFS=${.CURDIR}/../fsck_ffs
CPPFLAGS+= -I${FSCK} -I${FSCK_FFS}
.PATH:	${FSCK} ${FSCK_FFS} ${.CURDIR}/../../sys/ufs/ffs

LDADD+=	-ledit -ltermcap -lutil
DPADD+=	${LIBEDIT} ${LIBTERMCAP} ${LIBUTIL}

.include <bsd.prog.mk>
