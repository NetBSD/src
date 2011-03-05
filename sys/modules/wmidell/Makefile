# $NetBSD: Makefile,v 1.3.2.2 2011/03/05 20:55:49 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=   wmidell
IOCONF=	wmidell.ioconf
SRCS=	wmi_dell.c

WARNS=	4

.include <bsd.kmodule.mk>
