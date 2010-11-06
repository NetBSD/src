#	$NetBSD: Makefile,v 1.1.2.2 2010/11/06 08:08:46 uebayasi Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpivga
IOCONF=	ACPIVGA.ioconf
SRCS=	acpi_display.c

WARNS=	4

.include <bsd.kmodule.mk>
