#	$NetBSD: Makefile,v 1.2.6.1 2014/05/22 14:01:24 yamt Exp $

.include <bsd.own.mk>

.if ${MKCTF} != "no"
SUBDIR+=	ctfconvert ctfmerge ctfdump
.endif

.if ${MKZFS} != "no"
SUBDIR+=	ztest
.endif

.include <bsd.subdir.mk>
