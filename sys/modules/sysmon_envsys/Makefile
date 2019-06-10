# $NetBSD: Makefile,v 1.1.22.1 2019/06/10 22:09:39 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/sysmon

KMOD=	sysmon_envsys

SRCS=	sysmon_envsys.c
SRCS+=	sysmon_envsys_events.c
SRCS+=	sysmon_envsys_tables.c
SRCS+=	sysmon_envsys_util.c

WARNS=	3

.include <bsd.kmodule.mk>
