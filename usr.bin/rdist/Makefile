#	from: @(#)Makefile	5.11 (Berkeley) 3/12/91
#	$Id: Makefile,v 1.2 1993/07/31 15:17:49 mycroft Exp $

PROG=	rdist
CFLAGS+=-I${.CURDIR}
SRCS=	docmd.c expand.c lookup.c main.c server.c
OBJS+=	gram.o
BINOWN=	root
BINMODE=4555
CLEANFILES=y.tab.h

.include <bsd.prog.mk>
