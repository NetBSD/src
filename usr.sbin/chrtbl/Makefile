#	$NetBSD: Makefile,v 1.2 1997/10/23 06:32:02 lukem Exp $

PROG=	chrtbl
MAN=	chrtbl.8

.PATH:	${.CURDIR}/../../lib/libc/locale

CPPFLAGS+=-I${.CURDIR}/../../lib/libc/locale

# ctypeio.c pulled in from ../../lib/libc/locale
SRCS=	chrtbl.c ctypeio.c

.include <bsd.prog.mk>
