#	$NetBSD: Makefile,v 1.5 1996/07/12 00:46:18 thorpej Exp $
#	from: @(#)Makefile	5.11 (Berkeley) 3/12/91

PROG=	rdist
CFLAGS+=-I${.CURDIR}
SRCS=	docmd.c expand.c lookup.c main.c server.c
OBJS+=	gram.o
BINOWN=	root
BINMODE=4555
CLEANFILES=y.tab.h

.include <bsd.prog.mk>
