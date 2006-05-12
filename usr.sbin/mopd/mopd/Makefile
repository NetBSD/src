#	$NetBSD: Makefile,v 1.9 2006/05/12 01:54:57 mrg Exp $

PROG=	mopd
SRCS=	mopd.c process.c
MAN=	mopd.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4
COPTS.process.c+=	-Wno-pointer-sign
.endif
