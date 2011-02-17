# $NetBSD: Makefile,v 1.2.6.1 2011/02/17 12:00:48 bouyer Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=   wmidell
IOCONF=	wmidell.ioconf
SRCS=	wmi_dell.c

WARNS=	4

.include <bsd.kmodule.mk>
