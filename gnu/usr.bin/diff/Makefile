#	$Id: Makefile,v 1.5 1993/10/07 07:23:57 cgd Exp $

PROG=	diff
SRCS=	diff.c analyze.c cmpbuf.o io.c context.c ed.c normal.c util.c dir.c \
	regex.c side.c ifdef.c version.c getopt.c getopt1.c

CFLAGS+=-DHAVE_CONFIG_H -I${.CURDIR} 
LDADD+=	-lgnumalloc
DPADD+=	/usr/lib/libgnumalloc.a
NOMAN=noman

.include <bsd.prog.mk>
