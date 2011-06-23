#	$NetBSD: Makefile,v 1.10.18.1 2011/06/23 14:20:48 cherry Exp $

PROG=	mopd
SRCS=	mopd.c process.c
MAN=	mopd.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

.if defined(HAVE_GCC) || defined(HAVE_PCC)
COPTS.process.c+=	-Wno-pointer-sign
.endif
