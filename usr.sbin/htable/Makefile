#	from: @(#)Makefile	5.12 (Berkeley) 7/1/90
#	$Id: Makefile,v 1.2 1993/07/30 22:13:03 mycroft Exp $

PROG=	htable
SRCS=	htable.c parse.c scan.c
MAN8=	htable.0
CFLAGS+=-I. -I${.CURDIR}
CLEANFILES+=parse.c scan.c y.tab.h

.include <bsd.prog.mk>

${OBJS}: scan.c parse.c
