#	$NetBSD: Makefile,v 1.1 2002/06/17 16:29:08 christos Exp $
#	$OpenBSD: Makefile,v 1.4 2002/06/05 17:34:56 mickey Exp $

PROG=	systrace
CFLAGS+= -I. -I/sys
SRCS=	filter.c intercept-translate.c intercept.c \
	netbsd-syscalls.c util.c \
	policy.c systrace-errno.h systrace-error.c \
	systrace-translate.c systrace.c \
	parse.y lex.l
CLEANFILES+= parse.c lex.c y.tab.h
YHEADER=yes
LDADD+= -ll -ly
DPADD+= ${LIBL} ${LIBY}

.include <bsd.prog.mk>

.depend: parse.c lex.c
