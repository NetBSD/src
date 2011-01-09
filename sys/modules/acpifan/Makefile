# $NetBSD: Makefile,v 1.1 2011/01/09 15:12:34 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpifan
IOCONF=	ACPIFAN.ioconf
SRCS=	acpi_fan.c

WARNS=	4

.include <bsd.kmodule.mk>
