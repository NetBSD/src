# $NetBSD: Makefile,v 1.1 2017/09/17 00:00:39 sevan Exp $

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
