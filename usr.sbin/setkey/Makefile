# $NetBSD: Makefile,v 1.5 2000/03/13 21:04:07 itojun Exp $

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

# libpfkey.
# ipsec_strerror.c is for avoiding shlib reference to non-exported function.
.PATH: ${.CURDIR}/../../lib/libipsec ${.CURDIR}/../../sys/netkey
SRCS+= pfkey.c pfkey_dump.c key_debug.c ipsec_strerror.c
CPPFLAGS+=-I${.CURDIR}/../../lib/libipsec -I${.CURDIR}/../../sys/netkey

MAN=	setkey.8
LOCALPREFIX=	/usr/pkg

scriptdump: scriptdump.pl
	sed -e 's#@LOCALPREFIX@#${LOCALPREFIX}#' < $> > scriptdump

.include <bsd.prog.mk>
