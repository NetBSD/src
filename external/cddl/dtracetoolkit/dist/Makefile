# $NetBSD: Makefile,v 1.1.2.2 2017/09/23 17:50:43 snj Exp $

.include <bsd.own.mk>

.if (${MKDTRACE} != "no")
FILES= 	dtruss
FILES+= 	execsnoop
FILES+= 	opensnoop
FILES+= 	procsystime

FILESDIR= 	/usr/sbin
FILESMODE= 	555
.endif

.include <bsd.files.mk>
.include <bsd.subdir.mk>
