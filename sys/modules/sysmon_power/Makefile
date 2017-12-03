# $NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:39:00 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/sysmon

KMOD=	sysmon_power

SRCS=	sysmon_power.c

COPTS+=	-DCOMPAT_40

.include <bsd.kmodule.mk>
