#	$NetBSD: Makefile,v 1.2 2010/02/21 01:46:36 darran Exp $

.include <bsd.own.mk>

.if ${MKDTRACE} != "no"
SUBDIR+=	ctfconvert ctfmerge ctfdump
.endif

.if ${MKZFS} != "no"
SUBDIR+=	ztest
.endif

.include <bsd.subdir.mk>
