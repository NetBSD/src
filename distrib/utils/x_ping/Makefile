# $NetBSD: Makefile,v 1.3.4.1 2001/07/29 20:32:32 he Exp $
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
