#	$NetBSD: Makefile,v 1.7 1995/04/23 07:58:31 cgd Exp $

PROG=	diff
SRCS=	diff.c analyze.c cmpbuf.o io.c context.c ed.c normal.c util.c dir.c \
	regex.c side.c ifdef.c version.c getopt.c getopt1.c

CFLAGS+=-DHAVE_CONFIG_H -I${.CURDIR} 
LDADD+=	-lgnumalloc
DPADD+=	/usr/lib/libgnumalloc.a

.include <bsd.prog.mk>
