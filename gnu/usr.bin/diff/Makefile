#	$Id: Makefile,v 1.4 1993/09/16 17:39:00 jtc Exp $

PROG=	diff
SRCS=	diff.c analyze.c cmpbuf.o io.c context.c ed.c normal.c util.c dir.c \
	regex.c side.c ifdef.c version.c getopt.c getopt1.c

CFLAGS+=-DHAVE_CONFIG_H -I${.CURDIR} 
NOMAN=noman

.include <bsd.prog.mk>
