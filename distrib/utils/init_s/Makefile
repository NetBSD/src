# $NetBSD: Makefile,v 1.7 1999/05/19 04:17:49 gwr Exp $
# Build a "small init" (i.e. for boot media)

PROG=	init
MKMAN=	no

# Need -lutil for: logout, logwtmp, login_tty
DPADD=	${LIBUTIL}
LDADD=	-lutil

SRCDIR= ${BSDSRCDIR}/sbin/init
CPPFLAGS+= -DLETS_GET_SMALL -I${SRCDIR}

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
