# $NetBSD: Makefile,v 1.1 2015/04/23 23:22:27 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/sysmon

KMOD=	sysmon_power

SRCS=	sysmon_power.c

COPTS+=	-DCOMPAT_40

.include <bsd.kmodule.mk>
