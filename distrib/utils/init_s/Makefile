# $NetBSD: Makefile,v 1.4.2.1 1997/11/08 05:08:11 lukem Exp $
# Build a "small init" (i.e. for boot media)

PROG=	init
NOMAN=	

# Need -lutil for: logout, logwtmp, login_tty
DPADD=	${LIBUTIL}
LDADD=	-lutil

SRCDIR= ${.CURDIR}/../../../sbin/init
CPPFLAGS+= -DLETS_GET_SMALL -I${SRCDIR}

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
