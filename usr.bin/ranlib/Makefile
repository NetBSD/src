#	$NetBSD: Makefile,v 1.6 1997/01/09 20:20:54 tls Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 3/12/91

PROG=	ranlib
SRCS=	archive.c build.c misc.c ranlib.c touch.c
CFLAGS+=-I${.CURDIR} -I${.CURDIR}/../ar
MAN=	ranlib.1 ranlib.5
VPATH=	${.CURDIR}/../ar

.include <bsd.prog.mk>
