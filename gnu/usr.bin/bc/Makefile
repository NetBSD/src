#	$NetBSD: Makefile,v 1.18 1998/04/09 00:32:33 tv Exp $

PROG=	bc
CPPFLAGS+=-D_POSIX_SOURCE -I. -I${.CURDIR}
LFLAGS+=-I8
SRCS=	bc.y execute.c getopt.c getopt1.c global.c load.c main.c number.c \
	scan.l storage.c util.c
YHEADER=1
LDADD+=	-ll -ledit -ltermcap
DPADD+=	${LIBL}

.include <bsd.prog.mk>
