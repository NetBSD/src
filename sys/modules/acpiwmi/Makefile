# $NetBSD: Makefile,v 1.3.2.2 2011/03/05 20:55:37 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=   acpiwmi
IOCONF=	acpiwmi.ioconf
SRCS=	wmi_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
