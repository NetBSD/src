#	$NetBSD: Makefile,v 1.13 1997/05/09 14:07:03 mycroft Exp $

PROG=	bc
CFLAGS+=-D_POSIX_SOURCE -I. -I${.CURDIR}
LFLAGS+=-I8
SRCS=	bc.y execute.c getopt.c getopt1.c global.c load.c main.c number.c \
	scan.l storage.c util.c
LDADD+=	-ll
DPADD+=	${LIBL}

.include <bsd.prog.mk>
