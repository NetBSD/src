# $NetBSD: Makefile,v 1.6 2001/08/20 12:19:46 martin Exp $
# Build ping6 without IPSEC

SRCDIR= ${.CURDIR}/../../../sbin/ping6
PROG=	ping6
MKMAN=	no

CPPFLAGS+=-I${SRCDIR}
CPPFLAGS+=-DINET6
.if ${MACHINE_ARCH} != "sparc64"
COPTS+= -Os
.endif
LDSTATIC?=-static

# KAME scope id hack
CPPFLAGS+=-DKAME_SCOPEID

LDADD+=	-lm
DPADD+=	${LIBM}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
