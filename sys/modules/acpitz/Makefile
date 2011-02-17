#	$NetBSD: Makefile,v 1.2.6.1 2011/02/17 12:00:46 bouyer Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpitz
IOCONF=	acpitz.ioconf
SRCS=	acpi_tz.c

WARNS=	4

.include <bsd.kmodule.mk>
