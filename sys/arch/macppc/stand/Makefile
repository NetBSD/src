#	$NetBSD: Makefile,v 1.5 2003/12/11 00:27:42 matt Exp $

.include <bsd.own.mk>

.if ${USETOOLS} != "yes"
SUBDIR=		fixcoff .WAIT
.endif
SUBDIR+=	bootxx ofwboot

.include <bsd.subdir.mk>
