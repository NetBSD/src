# $NetBSD: Makefile,v 1.1.4.1 2011/03/05 20:55:35 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpibut
IOCONF=	acpibut.ioconf
SRCS=	acpi_button.c

WARNS=	4

.include <bsd.kmodule.mk>
