# $NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:39:00 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/sysmon

KMOD=	sysmon_envsys

SRCS=	sysmon_envsys.c
SRCS+=	sysmon_envsys_events.c
SRCS+=	sysmon_envsys_tables.c
SRCS+=	sysmon_envsys_util.c

.include <bsd.kmodule.mk>
