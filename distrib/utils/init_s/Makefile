# $NetBSD: Makefile,v 1.10.4.1 2000/09/25 22:17:52 wrstuden Exp $
# Build a "small init" (i.e. for boot media)

PROG=	init
MKMAN=	no

# Need -lutil for: logout, logwtmp, login_tty
DPADD=	${LIBUTIL}
LDADD=	-lutil
LDSTATIC?=-static

SRCDIR= ${.CURDIR}/../../../sbin/init
CPPFLAGS+= -DLETS_GET_SMALL -I${SRCDIR}
COPTS+= -Os

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
.NOPATH: ${PROG}
