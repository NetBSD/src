#	$NetBSD: Makefile,v 1.4.8.2 2004/09/18 14:37:09 skrll Exp $

.include <bsd.own.mk>

.if (${USETOOLS} != "yes")
SUBDIR=		fixcoff .WAIT
.endif
SUBDIR+=	bootxx ofwboot

.include <bsd.subdir.mk>
