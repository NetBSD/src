#	$NetBSD: Makefile,v 1.6.20.1 2008/09/18 04:30:02 wrstuden Exp $

.include <bsd.own.mk>

PROG=	chrtbl
MAN=	chrtbl.8

.PATH:	${NETBSDSRCDIR}/lib/libc/locale

CPPFLAGS+=-I${NETBSDSRCDIR}/lib/libc/locale

# ctypeio.c pulled in from ../../lib/libc/locale
SRCS=	chrtbl.c ctypeio.c

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.chrtbl.c+=	-Wno-pointer-sign
.endif
