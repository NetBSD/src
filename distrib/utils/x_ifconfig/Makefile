# $NetBSD: Makefile,v 1.40 2022/05/11 10:45:48 rin Exp $
# Build a smaller ifconfig (i.e. for boot media)

# You can drop IEEE 802.11 support by setting NOIEEE80211=1 in
# parent Makefile's. This saves ~9KB for, e.g., m68k.

NOMAN=		# defined
SMALLPROG=1

.include <bsd.own.mk>

PROG=		ifconfig
SRCDIR=		${.CURDIR}/../../../sbin/ifconfig

CPPFLAGS+=	-DCRUNCHOPS

.if defined(SMALLPROG_INET6) && (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
SRCS+=		af_inet6.c
.endif

.PATH:		${SRCDIR}

.include "${SRCDIR}/Makefile.common"

.include <bsd.prog.mk>
