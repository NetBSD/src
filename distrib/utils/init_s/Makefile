# $NetBSD: Makefile,v 1.9 2000/05/18 00:43:05 matt Exp $
# Build a "small init" (i.e. for boot media)

PROG=	init
MKMAN=	no

# Need -lutil for: logout, logwtmp, login_tty
DPADD=	${LIBUTIL}
LDADD=	-lutil

SRCDIR= ${.CURDIR}/../../../sbin/init
CPPFLAGS+= -DLETS_GET_SMALL -I${SRCDIR}
COPTS+= -Os

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
