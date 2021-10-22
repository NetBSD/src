# $NetBSD: Makefile,v 1.4 2021/10/22 13:25:57 roy Exp $
#

.include <bsd.own.mk>

.include "../../Makefile.inc"
.PATH:			${DIST}/hooks

.if ${MKSHARE} != "no"
FILES=			10-wpa_supplicant 15-timezone 29-lookup-hostname

.if ${MKYP} != "no"
FILES+=			50-ypbind
.endif

FILESDIR=		/usr/share/examples/dhcpcd/hooks
.endif

.include <bsd.prog.mk>
