# $NetBSD: Makefile,v 1.17 2002/06/18 01:53:35 itojun Exp $
# Build a smaller ifconfig (i.e. for boot media)

SRCDIR=		${.CURDIR}/../../../sbin/ifconfig

PROG=		ifconfig
NOMAN=		# defined

CPPFLAGS+=	-DINET_ONLY -I${SRCDIR}
.if (${MACHINE} == "i386" || ${MACHINE} == "hpcmips" || ${MACHINE} == "x68k")
CPPFLAGS+=	-DINET6
.endif

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
