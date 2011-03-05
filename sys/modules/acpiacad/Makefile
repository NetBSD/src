# $NetBSD: Makefile,v 1.1.4.1 2011/03/05 20:55:34 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpiacad
IOCONF=	acpiacad.ioconf
SRCS=	acpi_acad.c

WARNS=	4

.include <bsd.kmodule.mk>
