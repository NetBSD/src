#	from: @(#)Makefile	8.2 (Berkeley) 4/19/94
#	$Id: Makefile,v 1.9 1994/06/22 10:44:04 pk Exp $

PROG=	config.new
SRCS=	files.c gram.y hash.c main.c mkheaders.c mkioconf.c mkmakefile.c \
	mkswap.c pack.c scan.l sem.c util.c
CFLAGS+=-I${.CURDIR} -I.
LDADD+=	-ll
DPADD+=	${LIBL}
CLEANFILES=gram.c scan.c y.tab.h
MAN8=	config.new.0

.include <bsd.prog.mk>

.depend: gram.c scan.c
