# $NetBSD: Makefile,v 1.7 2001/10/05 01:17:32 jmc Exp $
# Build ping6 without IPSEC

SRCDIR= ${.CURDIR}/../../../sbin/ping6
PROG=	ping6
MKMAN=	no

CPPFLAGS+=-I${SRCDIR}
CPPFLAGS+=-DINET6
.if ${MACHINE_ARCH} != "sparc64"
DBG= -Os
.endif
LDSTATIC?=-static

# KAME scope id hack
CPPFLAGS+=-DKAME_SCOPEID

LDADD+=	-lm
DPADD+=	${LIBM}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
