#	$NetBSD: Makefile,v 1.9 1997/10/19 05:50:26 mrg Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	ranlib
SRCS=	archive.c build.c misc.c ranlib.c touch.c
MAN=	ranlib.1 ranlib.5

CFLAGS+= -I${.CURDIR}
CFLAGS+= -I${.CURDIR}/../ar
.PATH:     ${.CURDIR}/../ar

.include <bsd.prog.mk>
