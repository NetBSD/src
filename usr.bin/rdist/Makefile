#	$NetBSD: Makefile,v 1.6 1997/05/26 14:30:47 mrg Exp $
#	from: @(#)Makefile	5.11 (Berkeley) 3/12/91

PROG=	rdist
CFLAGS+=-I${.CURDIR}
SRCS=	docmd.c expand.c lookup.c main.c server.c
OBJS+=	gram.o
CLEANFILES=y.tab.h

.include <bsd.prog.mk>
