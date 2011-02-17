# $NetBSD: Makefile,v 1.2.4.1 2011/02/17 12:00:46 bouyer Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpilid
IOCONF=	acpilid.ioconf
SRCS=	acpi_lid.c

WARNS=	4

.include <bsd.kmodule.mk>
