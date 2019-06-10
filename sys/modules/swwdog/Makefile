# $NetBSD: Makefile,v 1.1.22.1 2019/06/10 22:09:39 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/sysmon

KMOD=   swwdog

SRCS=	swwdog.c

.if ${ACTIVE_CC} == "clang"
COPTS.swwdog.c+=	-Wno-missing-noreturn
.endif

.include <bsd.kmodule.mk>
