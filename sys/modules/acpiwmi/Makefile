# $NetBSD: Makefile,v 1.4 2019/02/17 04:05:42 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=   acpiwmi
IOCONF=	acpiwmi.ioconf
SRCS=	wmi_acpi.c

.include <bsd.kmodule.mk>
