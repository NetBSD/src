#	from: @(#)Makefile	5.4 (Berkeley) 3/12/91
#	$Id: Makefile,v 1.5 1994/12/22 12:30:34 cgd Exp $

PROG=	ranlib
SRCS=	archive.c build.c misc.c ranlib.c touch.c
CFLAGS+=-I${.CURDIR} -I${.CURDIR}/../ar
MAN=	ranlib.1 ranlib.5
VPATH=	${.CURDIR}/../ar

.include <bsd.prog.mk>
