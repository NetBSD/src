#	$NetBSD: Makefile,v 1.1 2016/01/12 22:58:00 christos Exp $

.include <bsd.own.mk>

SUBDIR=	lib .WAIT info bin 

.if ${MKNLS} != "no"
SUBDIR+=locale
.endif

.include <bsd.subdir.mk>
