#	from: @(#)Makefile	5.12 (Berkeley) 7/1/90
#	$NetBSD: Makefile,v 1.6 1997/10/25 06:58:05 lukem Exp $

PROG=	htable
SRCS=	htable.c parse.c scan.c
MAN=	htable.8
CPPFLAGS+=-DYY_NO_UNPUT -I. -I${.CURDIR}
LDADD+=	-ll
DPADD+=	${LIBL}
CLEANFILES+=parse.c scan.c y.tab.h

.include <bsd.prog.mk>

${OBJS}: scan.c parse.c
