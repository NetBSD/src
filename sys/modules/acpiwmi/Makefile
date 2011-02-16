# $NetBSD: Makefile,v 1.3 2011/02/16 08:19:56 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=   acpiwmi
IOCONF=	acpiwmi.ioconf
SRCS=	wmi_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
