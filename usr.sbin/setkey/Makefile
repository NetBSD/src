# $NetBSD: Makefile,v 1.4 2000/02/25 08:52:04 mycroft Exp $

PROG=	setkey
SRCS=	setkey.c parse.y token.l
CFLAGS+=-g
LDADD+=	-ll -ly
DPADD+=	${LIBL} ${LIBY}
CLEANFILES+=	y.tab.c y.tab.h key_test.o keytest
YFLAGS+=-d

SCRIPTS=	scriptdump

all: ${PROG} scriptdump

CPPFLAGS+=-DIPSEC_DEBUG -DINET6 -DYY_NO_UNPUT
CPPFLAGS+=-I. -I${.CURDIR}
LDADD+= -lipsec
DPADD+=	${LIBIPSEC}
CLEANFILES+=	scriptdump

MAN=	setkey.8
LOCALPREFIX=	/usr/pkg

scriptdump: scriptdump.pl
	sed -e 's#@LOCALPREFIX@#${LOCALPREFIX}#' < $> > scriptdump

.include <bsd.prog.mk>
