#	$NetBSD: Makefile,v 1.5 1997/10/25 10:10:28 lukem Exp $

PROG=	dc
CPPFLAGS+=-D_POSIX_SOURCE -I${.CURDIR} -I${.CURDIR}/../bc

SRCS= array.c dc.c eval.c misc.c numeric.c stack.c string.c \
	number.c getopt.c getopt1.c

.PATH.c: ${.CURDIR}/../bc

.include <bsd.prog.mk>
