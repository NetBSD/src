#	$NetBSD: Makefile,v 1.8 1997/05/14 15:57:07 gwr Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 3/12/91

PROG=	ranlib
SRCS=	archive.c build.c misc.c ranlib.c touch.c
MAN=	ranlib.1 ranlib.5

CFLAGS+= -I${.CURDIR}
CFLAGS+= -I${.CURDIR}/../ar
.PATH:     ${.CURDIR}/../ar

.include <bsd.prog.mk>
