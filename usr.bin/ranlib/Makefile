#	$NetBSD: Makefile,v 1.9.2.1 1997/11/08 10:38:00 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	ranlib
SRCS=	archive.c build.c misc.c ranlib.c touch.c
MAN=	ranlib.1 ranlib.5

CPPFLAGS+= -I${.CURDIR}
CPPFLAGS+= -I${.CURDIR}/../ar
.PATH:     ${.CURDIR}/../ar

.include <bsd.prog.mk>
