# $NetBSD: Makefile,v 1.4 2001/12/12 00:05:12 tv Exp $
# Build dmesg without kvm support

SRCDIR=		${.CURDIR}/../../../sbin/dmesg

PROG=		dmesg
NOMAN=		# defined

CPPFLAGS+=	-DSMALL
WARNS?=		1

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
