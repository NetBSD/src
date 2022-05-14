#	$NetBSD: Makefile,v 1.1 2022/05/14 20:54:31 mrg Exp $

.include <bsd.own.mk>

.if ${MKRADEONFIRMWARE} != "no"
# Upper case already installed from sys/dev/microcode.
#SUBDIR=	uc
SUBDIR=	lc
.endif

.include <bsd.subdir.mk>
