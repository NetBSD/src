#	$NetBSD: Makefile,v 1.1.1.2 1997/04/29 00:34:24 phil Exp $

PROG=	dc
CFLAGS+=-D_POSIX_SOURCE -I${.CURDIR} -I${.CURDIR}/../bc

SRCS= array.c dc.c eval.c misc.c numeric.c stack.c string.c \
	number.c getopt.c getopt1.c

.PATH: ${.CURDIR}/../bc

.include <bsd.prog.mk>
