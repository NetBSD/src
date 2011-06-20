#	$NetBSD: Makefile,v 1.11 2011/06/20 07:44:03 mrg Exp $

PROG=	mopd
SRCS=	mopd.c process.c
MAN=	mopd.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

.if defined(HAVE_GCC) || defined(HAVE_PCC)
COPTS.process.c+=	-Wno-pointer-sign
.endif
