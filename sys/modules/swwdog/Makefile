# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:58 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/sysmon

KMOD=   swwdog

SRCS=	swwdog.c

.if ${ACTIVE_CC} == "clang"
COPTS.swwdog.c+=	-Wno-missing-noreturn
.endif

.include <bsd.kmodule.mk>
