#	$NetBSD: Makefile,v 1.6 2004/03/13 05:55:40 dyoung Exp $

.include <bsd.own.mk>

.if (${USETOOLS} != "yes")
SUBDIR=		fixcoff .WAIT
.endif
SUBDIR+=	bootxx ofwboot

.include <bsd.subdir.mk>
