#	$NetBSD: Makefile,v 1.10 1997/03/24 22:06:10 christos Exp $

PROG=	diff
SRCS=	diff.c analyze.c cmpbuf.c io.c context.c ed.c normal.c util.c dir.c \
	regex.c side.c ifdef.c version.c getopt.c getopt1.c

CFLAGS+=-DHAVE_CONFIG_H -I${.CURDIR} -DPR_PROGRAM=\"/usr/bin/pr\"
LDADD+=	-lgnumalloc
DPADD+=	${LIBGNUMALLOC}

.include <bsd.prog.mk>
