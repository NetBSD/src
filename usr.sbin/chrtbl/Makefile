#	$NetBSD: Makefile,v 1.5 2002/11/30 03:10:56 lukem Exp $

.include <bsd.own.mk>

PROG=	chrtbl
MAN=	chrtbl.8

.PATH:	${NETBSDSRCDIR}/lib/libc/locale

CPPFLAGS+=-I${NETBSDSRCDIR}/lib/libc/locale

# ctypeio.c pulled in from ../../lib/libc/locale
SRCS=	chrtbl.c ctypeio.c

.include <bsd.prog.mk>
