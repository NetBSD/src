# $NetBSD: Makefile,v 1.20 2002/10/17 02:05:39 lukem Exp $
# Build a smaller ifconfig (i.e. for boot media)

SRCDIR=		${.CURDIR}/../../../sbin/ifconfig

PROG=		ifconfig
NOMAN=		# defined

CPPFLAGS+=	-DINET_ONLY -I${SRCDIR}
.if defined(SMALLPROG_INET6)
CPPFLAGS+=	-DINET6
.endif

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
