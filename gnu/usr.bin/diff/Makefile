#	$NetBSD: Makefile,v 1.11 1997/10/25 10:10:30 lukem Exp $

PROG=	diff
SRCS=	diff.c analyze.c cmpbuf.c io.c context.c ed.c normal.c util.c dir.c \
	regex.c side.c ifdef.c version.c getopt.c getopt1.c

CPPFLAGS+=-DHAVE_CONFIG_H -I${.CURDIR} -DPR_PROGRAM=\"/usr/bin/pr\"
LDADD+=	-lgnumalloc
DPADD+=	${LIBGNUMALLOC}

.include <bsd.prog.mk>
