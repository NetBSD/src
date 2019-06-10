# $NetBSD: Makefile,v 1.3.58.1 2019/06/10 22:09:08 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=   acpiwmi
IOCONF=	acpiwmi.ioconf
SRCS=	wmi_acpi.c

.include <bsd.kmodule.mk>
