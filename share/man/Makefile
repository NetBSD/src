#	$NetBSD: Makefile,v 1.19 1999/02/26 11:03:08 ross Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/16/94

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
SUBDIR=	man0 man1 man3 man4 man5 man6 man7 man8 man9
MAKEWHATIS?=	${.CURDIR}/../../libexec/makewhatis/makewhatis.sh

makedb:
	sh ${MAKEWHATIS} ${DESTDIR}/usr/share/man
.else
makedb:
.endif

.include <bsd.subdir.mk>
