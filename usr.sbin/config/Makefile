#	from: @(#)Makefile	5.12 (Berkeley) 1/16/91
#	$Id: Makefile,v 1.6 1993/07/30 22:13:27 mycroft Exp $

PROG=	config
CFLAGS+=-I. -I${.CURDIR}
SRCS=	config.c main.c lang.c mkioconf.c mkmakefile.c mkglue.c mkheaders.c \
	mkswapconf.c specfile.c
MAN8=	config.0
CLEANFILES+=y.tab.h lang.c config.c y.tab.c

.include <bsd.prog.mk>
