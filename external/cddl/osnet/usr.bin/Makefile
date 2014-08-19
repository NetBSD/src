#	$NetBSD: Makefile,v 1.2.12.1 2014/08/19 23:52:23 tls Exp $

.include <bsd.own.mk>

.if ${MKCTF} != "no"
SUBDIR+=	ctfconvert ctfmerge ctfdump
.endif

.if ${MKZFS} != "no"
SUBDIR+=	ztest
.endif

.include <bsd.subdir.mk>
