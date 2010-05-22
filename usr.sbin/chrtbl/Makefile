#	$NetBSD: Makefile,v 1.9 2010/05/22 06:38:15 tnozaki Exp $

.include <bsd.own.mk>

PROG=	chrtbl
MAN=	chrtbl.8

SRCS=	chrtbl.c ctypeio.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/locale

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.chrtbl.c+=	-Wno-pointer-sign
.endif
