# $NetBSD: Makefile,v 1.4 2001/12/12 00:05:14 tv Exp $
#
# Build umount(8) without network support
#

SRCDIR=		${.CURDIR}/../../../sbin/umount

PROG=		umount
NOMAN=		# defined

CPPFLAGS+=	-DSMALL
WARNS?=		1

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
