# $NetBSD: Makefile,v 1.19 2002/09/20 21:23:02 christos Exp $
# Build a smaller ifconfig (i.e. for boot media)

SRCDIR=		${.CURDIR}/../../../sbin/ifconfig

PROG=		ifconfig
NOMAN=		# defined

CPPFLAGS+=	-DINET_ONLY -I${SRCDIR}
.if (${MACHINE} == "i386" || ${MACHINE} == "hpcmips" || ${MACHINE} == "x68k" || ${MACHINE} == "macppc")
CPPFLAGS+=	-DINET6
.endif

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
