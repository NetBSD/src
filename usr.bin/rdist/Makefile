#	$NetBSD: Makefile,v 1.5.2.1 1997/02/17 15:24:51 mrg Exp $
#	from: @(#)Makefile	5.11 (Berkeley) 3/12/91

PROG=	rdist
CFLAGS+=-I${.CURDIR}
SRCS=	docmd.c expand.c lookup.c main.c server.c
OBJS+=	gram.o
CLEANFILES=y.tab.h

.include <bsd.prog.mk>
