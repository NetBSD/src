# $NetBSD: Makefile,v 1.16 2001/12/12 00:05:13 tv Exp $
# Build a smaller ifconfig (i.e. for boot media)

SRCDIR=		${.CURDIR}/../../../sbin/ifconfig

PROG=		ifconfig
NOMAN=		# defined

CPPFLAGS+=	-DINET_ONLY -I${SRCDIR}
.if (${MACHINE} == "i386") || (${MACHINE} == "hpcmips") || (${MACHINE} == "x68k")
CPPFLAGS+=	-DINET6
.endif

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
