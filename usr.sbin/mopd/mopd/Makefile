#	$NetBSD: Makefile,v 1.10 2008/08/29 00:02:25 gmcgarry Exp $

PROG=	mopd
SRCS=	mopd.c process.c
MAN=	mopd.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.process.c+=	-Wno-pointer-sign
.endif
