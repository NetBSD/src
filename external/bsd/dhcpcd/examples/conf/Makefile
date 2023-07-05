# $NetBSD: Makefile,v 1.1 2023/07/05 00:26:38 gutteridge Exp $

.include <bsd.own.mk>

.include "../../Makefile.inc"
.PATH:			${DIST}/src

.if ${MKSHARE} != "no"
FILES=			dhcpcd.conf

FILESDIR=		/usr/share/examples/dhcpcd/
.endif

.include <bsd.prog.mk>
