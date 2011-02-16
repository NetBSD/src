# $NetBSD: Makefile,v 1.2 2011/02/16 08:01:52 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpifan
IOCONF=	acpifan.ioconf
SRCS=	acpi_fan.c

WARNS=	4

.include <bsd.kmodule.mk>
