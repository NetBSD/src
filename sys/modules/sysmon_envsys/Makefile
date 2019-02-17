# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:58 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/sysmon

KMOD=	sysmon_envsys

SRCS=	sysmon_envsys.c
SRCS+=	sysmon_envsys_events.c
SRCS+=	sysmon_envsys_tables.c
SRCS+=	sysmon_envsys_util.c

WARNS=	3

.include <bsd.kmodule.mk>
