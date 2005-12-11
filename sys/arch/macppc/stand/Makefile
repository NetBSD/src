#	$NetBSD: Makefile,v 1.7 2005/12/11 12:18:06 christos Exp $

.include <bsd.own.mk>

.if (${USETOOLS} != "yes")
SUBDIR=		fixcoff .WAIT
.endif
SUBDIR+=	bootxx ofwboot

.include <bsd.subdir.mk>
