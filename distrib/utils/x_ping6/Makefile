# $NetBSD: Makefile,v 1.3.4.2 2001/07/29 20:32:32 he Exp $
# Build ping6 without IPSEC

SRCDIR= ${.CURDIR}/../../../sbin/ping6
PROG=	ping6
NOMAN=	yes

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
