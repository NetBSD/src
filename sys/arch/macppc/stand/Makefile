#	$NetBSD: Makefile,v 1.4.8.3 2004/09/21 13:18:30 skrll Exp $

.include <bsd.own.mk>

.if (${USETOOLS} != "yes")
SUBDIR=		fixcoff .WAIT
.endif
SUBDIR+=	bootxx ofwboot

.include <bsd.subdir.mk>
