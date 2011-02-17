# $NetBSD: Makefile,v 1.2.4.1 2011/02/17 12:00:45 bouyer Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpiacad
IOCONF=	acpiacad.ioconf
SRCS=	acpi_acad.c

WARNS=	4

.include <bsd.kmodule.mk>
