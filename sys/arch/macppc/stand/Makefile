#	$NetBSD: Makefile,v 1.4.8.1 2004/08/03 10:37:31 skrll Exp $

.include <bsd.own.mk>

.if (${USETOOLS} != "yes")
SUBDIR=		fixcoff .WAIT
.endif
SUBDIR+=	bootxx ofwboot

.include <bsd.subdir.mk>
