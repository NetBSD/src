# $NetBSD: Makefile,v 1.3 2011/02/16 08:35:52 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpibut
IOCONF=	acpibut.ioconf
SRCS=	acpi_button.c

WARNS=	4

.include <bsd.kmodule.mk>
