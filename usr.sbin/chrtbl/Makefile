#	$NetBSD: Makefile,v 1.1.2.1 1997/11/08 15:11:27 lukem Exp $

PROG=	chrtbl
MAN=	chrtbl.8

.PATH:	${.CURDIR}/../../lib/libc/locale

CPPFLAGS+=-I${.CURDIR}/../../lib/libc/locale

# ctypeio.c pulled in from ../../lib/libc/locale
SRCS=	chrtbl.c ctypeio.c

.include <bsd.prog.mk>
