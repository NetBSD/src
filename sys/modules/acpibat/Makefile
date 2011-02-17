# $NetBSD: Makefile,v 1.3.4.1 2011/02/17 12:00:45 bouyer Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpibat
IOCONF=	acpibat.ioconf
SRCS=	acpi_bat.c

WARNS=	4

.include <bsd.kmodule.mk>
