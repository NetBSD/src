#	$NetBSD: Makefile,v 1.4 1997/05/05 16:24:38 christos Exp $

PROG=	dc
CFLAGS+=-D_POSIX_SOURCE -I${.CURDIR} -I${.CURDIR}/../bc

SRCS= array.c dc.c eval.c misc.c numeric.c stack.c string.c \
	number.c getopt.c getopt1.c

.PATH.c: ${.CURDIR}/../bc

.include <bsd.prog.mk>
