# $NetBSD: Makefile,v 1.1.22.1 2019/06/10 22:09:40 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/sysmon

KMOD=	sysmon_taskq

SRCS=	sysmon_taskq.c

.if ${ACTIVE_CC} == "clang"
COPTS.sysmon_taskq.c+=	-Wno-missing-noreturn
.endif

.include <bsd.kmodule.mk>
