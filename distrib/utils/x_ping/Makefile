# $NetBSD: Makefile,v 1.4 2001/08/20 12:19:46 martin Exp $
# Build ping without IPSEC

SRCDIR= ${.CURDIR}/../../../sbin/ping
PROG=	ping
MKMAN=	no
LDADD=	-lm
LDSTATIC?=-static
CPPFLAGS+= -I${SRCDIR}
.if ${MACHINE_ARCH} != "sparc64"
COPTS+= -Os
.endif

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
