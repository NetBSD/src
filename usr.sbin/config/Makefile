#	from: @(#)Makefile	5.12 (Berkeley) 1/16/91
#	$Id: Makefile,v 1.9 1994/12/22 11:33:37 cgd Exp $

PROG=	config
BINDIR=	/usr/sbin
CFLAGS+=-I. -I${.CURDIR}
SRCS=	config.c main.c lang.c mkioconf.c mkmakefile.c mkglue.c mkheaders.c \
	mkswapconf.c specfile.c
MAN=	config.8
LDADD+=	-ll
DPADD+=	${LIBL}
CLEANFILES+=y.tab.h lang.c config.c y.tab.c

.include <bsd.prog.mk>
