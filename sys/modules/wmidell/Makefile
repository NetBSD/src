# $NetBSD: Makefile,v 1.4 2019/02/17 04:06:00 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=   wmidell
IOCONF=	wmidell.ioconf
SRCS=	wmi_dell.c

.include <bsd.kmodule.mk>
