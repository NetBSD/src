# $NetBSD: Makefile,v 1.7 2021/10/22 13:25:57 roy Exp $
#

.include <bsd.own.mk>

.include "../../Makefile.inc"
HOOKSRC=	${DIST}/hooks
.PATH:		${HOOKSRC}

FILES=		01-test 20-resolv.conf 30-hostname
FILES+=		50-ntp.conf

FILESDIR=	/libexec/dhcpcd-hooks

.include <bsd.prog.mk>
