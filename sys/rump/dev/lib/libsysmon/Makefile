#	$NetBSD: Makefile,v 1.1.2.2 2010/08/17 06:47:55 uebayasi Exp $
#

.PATH:	${.CURDIR}/../../../../dev/sysmon

LIB=	rumpdev_sysmon
IOCONF=	SYSMON.ioconf

SRCS=	sysmon_taskq.c sysmon_power.c sysmon_envsys.c sysmon_envsys_events.c \
	sysmon_envsys_tables.c sysmon_envsys_util.c sysmon_wdog.c sysmon.c \
	swwdog.c

SRCS+=	component.c 

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs -I${.CURDIR}/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
