#	$NetBSD: Makefile,v 1.1 2010/01/31 03:06:59 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/sysmon

LIB=	rumpdev_sysmon

SRCS=	sysmon_taskq.c sysmon_power.c sysmon_envsys.c sysmon_envsys_events.c \
	sysmon_envsys_tables.c sysmon_envsys_util.c sysmon_wdog.c sysmon.c \
	swwdog.c

SRCS+=	component.c 

CFLAGS+=	-Wno-pointer-sign
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs -I${.CURDIR}/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
