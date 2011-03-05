#	$NetBSD: Makefile,v 1.1.4.2 2011/03/05 20:55:37 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpiwdrt
IOCONF=	acpiwdrt.ioconf
SRCS=	acpi_wdrt.c

WARNS=	4

.include <bsd.kmodule.mk>
