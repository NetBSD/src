#	$NetBSD: Makefile,v 1.1 1997/06/02 10:16:41 kleink Exp $

PROG=	chrtbl
MAN=	chrtbl.8

.PATH:	${.CURDIR}/../../lib/libc/locale

CFLAGS+=-I${.CURDIR}/../../lib/libc/locale

# ctypeio.c pulled in from ../../lib/libc/locale
SRCS=	chrtbl.c ctypeio.c

.include <bsd.prog.mk>
