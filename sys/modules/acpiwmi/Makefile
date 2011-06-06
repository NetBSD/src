# $NetBSD: Makefile,v 1.2.4.1 2011/06/06 09:09:46 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=   acpiwmi
IOCONF=	acpiwmi.ioconf
SRCS=	wmi_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
