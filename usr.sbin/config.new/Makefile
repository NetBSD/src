#	from: @(#)Makefile	8.2 (Berkeley) 4/19/94
#	$Id: Makefile,v 1.11 1994/12/22 11:33:48 cgd Exp $

PROG=	config.new
BINDIR=	/usr/sbin
SRCS=	files.c gram.y hash.c main.c mkheaders.c mkioconf.c mkmakefile.c \
	mkswap.c pack.c scan.l sem.c util.c
CFLAGS+=-I${.CURDIR} -I.
LDADD+=	-ll
DPADD+=	${LIBL}
CLEANFILES=gram.c scan.c y.tab.h
MAN=	config.new.8

.include <bsd.prog.mk>

.depend: gram.c scan.c
