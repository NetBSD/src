#	from: @(#)Makefile	5.12 (Berkeley) 7/1/90
#	$Id: Makefile,v 1.3 1993/12/02 18:18:49 jtc Exp $

PROG=	htable
SRCS=	htable.c parse.c scan.c
MAN8=	htable.0
CFLAGS+=-I. -I${.CURDIR}
LDADD+=	-ll
DPADD+=	${LIBL}
CLEANFILES+=parse.c scan.c y.tab.h

.include <bsd.prog.mk>

${OBJS}: scan.c parse.c
