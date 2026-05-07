#	$NetBSD: Makefile,v 1.16 2026/05/07 20:32:11 kre Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	dumpfs
MAN=	dumpfs.8
SRCS=	dumpfs.c ffs_bswap.c
SRCS+=	openspecial.c

.ifndef HOSTPROG
LDADD+=	-lutil
DPADD+=	${LIBUTIL}
.endif

FSCK=${NETBSDSRCDIR}/sbin/fsck

CPPFLAGS+= -I${FSCK}

.PATH:  ${NETBSDSRCDIR}/sys/ufs/ffs
.PATH:  ${FSCK}


.include <bsd.prog.mk>
