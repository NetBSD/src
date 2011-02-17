#	$NetBSD: Makefile,v 1.1.6.1 2011/02/17 12:00:47 bouyer Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpivga
IOCONF=	acpivga.ioconf
SRCS=	acpi_display.c

WARNS=	4

.include <bsd.kmodule.mk>
