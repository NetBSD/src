# $NetBSD: Makefile,v 1.5 2001/10/05 01:17:31 jmc Exp $
# Build ping without IPSEC

SRCDIR= ${.CURDIR}/../../../sbin/ping
PROG=	ping
MKMAN=	no
LDADD=	-lm
LDSTATIC?=-static
CPPFLAGS+= -I${SRCDIR}
.if ${MACHINE_ARCH} != "sparc64"
DBG= -Os
.endif

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
