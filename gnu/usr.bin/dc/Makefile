#	$Id: Makefile,v 1.1.1.1 1994/12/01 04:26:47 phil Exp $

PROG=	dc
CFLAGS+=-D_POSIX_SOURCE -I${.CURDIR} -I${.CURDIR}/../bc

SRCS= array.c dc-number.c number.c eval.c misc.c stack.c string.c

.PATH: ${.CURDIR}/../bc

.include <bsd.prog.mk>
