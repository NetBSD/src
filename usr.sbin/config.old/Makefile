#	from: @(#)Makefile	5.12 (Berkeley) 1/16/91
#	$Id: Makefile,v 1.7 1993/12/02 18:18:30 jtc Exp $

PROG=	config
CFLAGS+=-I. -I${.CURDIR}
SRCS=	config.c main.c lang.c mkioconf.c mkmakefile.c mkglue.c mkheaders.c \
	mkswapconf.c specfile.c
MAN8=	config.0
LDADD+=	-ll
DPADD+=	${LIBL}
CLEANFILES+=y.tab.h lang.c config.c y.tab.c

.include <bsd.prog.mk>
