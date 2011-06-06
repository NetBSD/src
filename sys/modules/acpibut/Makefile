# $NetBSD: Makefile,v 1.2.2.1 2011/06/06 09:09:45 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpibut
IOCONF=	acpibut.ioconf
SRCS=	acpi_button.c

WARNS=	4

.include <bsd.kmodule.mk>
