#	from: @(#)Makefile	5.12 (Berkeley) 1/16/91
#	$Id: Makefile,v 1.8 1994/12/11 22:29:42 mycroft Exp $

PROG=	config
BINDIR=	/usr/sbin
CFLAGS+=-I. -I${.CURDIR}
SRCS=	config.c main.c lang.c mkioconf.c mkmakefile.c mkglue.c mkheaders.c \
	mkswapconf.c specfile.c
MAN8=	config.0
LDADD+=	-ll
DPADD+=	${LIBL}
CLEANFILES+=y.tab.h lang.c config.c y.tab.c

.include <bsd.prog.mk>
