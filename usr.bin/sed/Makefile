#	from: @(#)Makefile	5.1 (Berkeley) 8/24/92
#	$Id: Makefile,v 1.4 1993/11/04 01:36:21 andrew Exp $

PROG=	sed
SRCS=	compile.c main.c misc.c process.c
CFLAGS+= -DHISTORIC_PRACTICE
LDADD=-lgnuregex

.include <bsd.prog.mk>
