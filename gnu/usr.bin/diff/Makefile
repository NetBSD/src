#	$Id: Makefile,v 1.6 1994/03/09 19:06:47 jtc Exp $

PROG=	diff
SRCS=	diff.c analyze.c cmpbuf.o io.c context.c ed.c normal.c util.c dir.c \
	regex.c side.c ifdef.c version.c getopt.c getopt1.c

CFLAGS+=-DHAVE_CONFIG_H -I${.CURDIR} 
LDADD+=	-lgnumalloc
DPADD+=	/usr/lib/libgnumalloc.a

.include <bsd.prog.mk>
