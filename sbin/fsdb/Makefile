#	$NetBSD: Makefile,v 1.13 2002/08/19 10:16:54 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	fsdb
MAN=	fsdb.8
SRCS=	fsdb.c fsdbutil.c
SRCS+=	dir.c inode.c pass1.c pass1b.c pass2.c pass3.c pass4.c \
	pass5.c setup.c utilities.c
SRCS+=	ffs_bswap.c ffs_subr.c ffs_tables.c
SRCS+=	fsutil.c 

FSCK=	${NETBSDSRCDIR}/sbin/fsck
FSCK_FFS=${NETBSDSRCDIR}/sbin/fsck_ffs
CPPFLAGS+= -I${FSCK} -I${FSCK_FFS}
.PATH:	${FSCK} ${FSCK_FFS} ${NETBSDSRCDIR}/sys/ufs/ffs

LDADD+= -ledit -ltermcap
DPADD+= ${LIBEDIT} ${LIBTERMCAP}

.include <bsd.prog.mk>
