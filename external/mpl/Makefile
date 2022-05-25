#	$NetBSD: Makefile,v 1.3 2022/05/25 20:44:01 nia Exp $

.include <bsd.own.mk>

SUBDIR=	bind

.if ${MKDHCPD} != "no"
SUBDIR+=	dhcp
.endif

.include <bsd.subdir.mk>
