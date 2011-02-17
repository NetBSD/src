# $NetBSD: Makefile,v 1.2.4.1 2011/02/17 12:00:45 bouyer Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpibut
IOCONF=	acpibut.ioconf
SRCS=	acpi_button.c

WARNS=	4

.include <bsd.kmodule.mk>
