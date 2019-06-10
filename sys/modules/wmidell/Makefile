# $NetBSD: Makefile,v 1.3.58.1 2019/06/10 22:09:43 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=   wmidell
IOCONF=	wmidell.ioconf
SRCS=	wmi_dell.c

.include <bsd.kmodule.mk>
