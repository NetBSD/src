# $NetBSD: Makefile,v 1.1.1.1 1995/10/08 23:08:47 gwr Exp $
# Build a "small init" (i.e. for boot media)

PROG=	init
NOMAN=	

# Need -lutil for: logout, logwtmp, login_tty
DPADD=	${LIBUTIL}
LDADD=	-lutil

SRCDIR=/usr/src/sbin/init
CFLAGS+= -DLETS_GET_SMALL -I${SRCDIR}

.PATH:  ${SRCDIR}

all: ${PROG}

.include <bsd.prog.mk>
