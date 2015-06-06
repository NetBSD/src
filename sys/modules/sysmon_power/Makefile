# $NetBSD: Makefile,v 1.1.2.2 2015/06/06 14:40:24 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/sysmon

KMOD=	sysmon_power

SRCS=	sysmon_power.c

COPTS+=	-DCOMPAT_40

.include <bsd.kmodule.mk>
