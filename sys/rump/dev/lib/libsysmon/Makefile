#	$NetBSD: Makefile,v 1.5.22.1 2014/05/18 17:46:16 rmind Exp $
#

.PATH:	${.CURDIR}/../../../../dev/sysmon

LIB=	rumpdev_sysmon
IOCONF=	SYSMON.ioconf

SRCS=	sysmon_taskq.c sysmon_power.c sysmon_envsys.c sysmon_envsys_events.c \
	sysmon_envsys_tables.c sysmon_envsys_util.c sysmon_wdog.c sysmon.c \
	swsensor.c swwdog.c

SRCS+=	sysmon_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs -I${.CURDIR}/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
