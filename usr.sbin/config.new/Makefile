#	from: @(#)Makefile	5.1 (Berkeley) 1/12/93
#	$Id: Makefile,v 1.8 1993/12/04 06:05:53 cgd Exp $

PROG=	config.new
SRCS=	files.c gram.y hash.c main.c mkheaders.c mkioconf.c mkmakefile.c \
	mkswap.c pack.c scan.l sem.c util.c
CFLAGS+=-I${.CURDIR} -I.
MAN8=	config.new.0
LDADD+=	-ll
DPADD+=	${LIBL}
CLEANFILES=gram.c scan.c y.tab.h

.include <bsd.prog.mk>

.depend: gram.c scan.c
