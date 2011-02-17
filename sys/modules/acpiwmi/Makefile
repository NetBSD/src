# $NetBSD: Makefile,v 1.2.6.1 2011/02/17 12:00:47 bouyer Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=   acpiwmi
IOCONF=	acpiwmi.ioconf
SRCS=	wmi_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
