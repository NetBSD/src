#	from: @(#)Makefile	5.12 (Berkeley) 7/1/90
#	$NetBSD: Makefile,v 1.5 1997/10/17 08:00:23 lukem Exp $

PROG=	htable
SRCS=	htable.c parse.c scan.c
MAN=	htable.8
CFLAGS+=-I. -I${.CURDIR}
CPPFLAGS+=-DYY_NO_UNPUT
LDADD+=	-ll
DPADD+=	${LIBL}
CLEANFILES+=parse.c scan.c y.tab.h

.include <bsd.prog.mk>

${OBJS}: scan.c parse.c
