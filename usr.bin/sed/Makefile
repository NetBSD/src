#	from: @(#)Makefile	5.1 (Berkeley) 8/24/92
#	$Id: Makefile,v 1.2 1993/07/30 23:51:18 mycroft Exp $

PROG=	sed
CFLAGS+=-DGNU_REGEX
SRCS=	compile.c main.c misc.c process.c
LDADD=-lgnuregex

.include <bsd.prog.mk>
