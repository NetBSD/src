#	from: @(#)Makefile	8.2 (Berkeley) 4/19/94
#	$Id: Makefile,v 1.10 1994/12/11 22:29:40 mycroft Exp $

PROG=	config.new
BINDIR=	/usr/sbin
SRCS=	files.c gram.y hash.c main.c mkheaders.c mkioconf.c mkmakefile.c \
	mkswap.c pack.c scan.l sem.c util.c
CFLAGS+=-I${.CURDIR} -I.
LDADD+=	-ll
DPADD+=	${LIBL}
CLEANFILES=gram.c scan.c y.tab.h
MAN8=	config.new.0

.include <bsd.prog.mk>

.depend: gram.c scan.c
