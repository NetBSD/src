# $NetBSD: Makefile,v 1.16.2.1 2002/11/03 13:19:20 he Exp $
# Build a smaller ifconfig (i.e. for boot media)

SRCDIR=		${.CURDIR}/../../../sbin/ifconfig

PROG=		ifconfig
NOMAN=		# defined

CPPFLAGS+=	-DINET_ONLY -I${SRCDIR}
.if (${MACHINE} == "i386" || ${MACHINE} == "hpcmips" || ${MACHINE} == "x68k" || ${MACHINE} == "macppc")
CPPFLAGS+=	-DINET6
.endif

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
