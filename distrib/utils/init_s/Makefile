# $NetBSD: Makefile,v 1.12 2001/08/20 12:19:43 martin Exp $
# Build a "small init" (i.e. for boot media)

PROG=	init
MKMAN=	no

# Need -lutil for: logout, logwtmp, login_tty
DPADD=	${LIBUTIL}
LDADD=	-lutil
LDSTATIC?=-static

SRCDIR= ${.CURDIR}/../../../sbin/init
CPPFLAGS+= -DLETS_GET_SMALL -I${SRCDIR}
.if ${MACHINE_ARCH} != "sparc64"
COPTS+= -Os
.endif

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
.NOPATH: ${PROG}
