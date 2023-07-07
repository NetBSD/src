# $NetBSD: Makefile,v 1.1.4.2 2023/07/07 18:46:57 martin Exp $

.include <bsd.own.mk>

.include "../../Makefile.inc"
.PATH:			${DIST}/src

.if ${MKSHARE} != "no"
FILES=			dhcpcd.conf

FILESDIR=		/usr/share/examples/dhcpcd/
.endif

.include <bsd.prog.mk>
