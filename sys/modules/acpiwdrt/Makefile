#	$NetBSD: Makefile,v 1.1 2011/01/17 15:49:56 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpiwdrt
IOCONF=	acpiwdrt.ioconf
SRCS=	acpi_wdrt.c

WARNS=	4

.include <bsd.kmodule.mk>
