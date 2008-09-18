#	$NetBSD: Makefile,v 1.9.20.1 2008/09/18 04:30:07 wrstuden Exp $

PROG=	mopd
SRCS=	mopd.c process.c
MAN=	mopd.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.process.c+=	-Wno-pointer-sign
.endif
