#	$NetBSD: Makefile,v 1.7 1997/05/06 21:42:56 gwr Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 3/12/91

PROG=	ranlib
SRCS=	archive.c build.c misc.c ranlib.c touch.c
MAN=	ranlib.1 ranlib.5

CFLAGS+= -I${.CURDIR}
CFLAGS+= -I${.CURDIR}/../ar
.PATH.c:   ${.CURDIR}/../ar

.include <bsd.prog.mk>
