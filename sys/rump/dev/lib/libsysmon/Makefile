#	$NetBSD: Makefile,v 1.1.2.1 2010/04/30 14:44:24 uebayasi Exp $
#

.PATH:	${.CURDIR}/../../../../dev/sysmon

LIB=	rumpdev_sysmon

SRCS=	sysmon_taskq.c sysmon_power.c sysmon_envsys.c sysmon_envsys_events.c \
	sysmon_envsys_tables.c sysmon_envsys_util.c sysmon_wdog.c sysmon.c \
	swwdog.c

SRCS+=	component.c 

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs -I${.CURDIR}/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
