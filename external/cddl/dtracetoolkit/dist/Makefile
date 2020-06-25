# $NetBSD: Makefile,v 1.2 2020/06/25 09:48:29 sevan Exp $

.include <bsd.own.mk>

.if (${MKDTRACE} != "no")
FILES=		dtruss
FILES+= 	execsnoop
FILES+= 	opensnoop
FILES+= 	procsystime

FILESDIR= 	/usr/sbin
FILESMODE= 	555
.endif

.include <bsd.files.mk>
.include <bsd.subdir.mk>
