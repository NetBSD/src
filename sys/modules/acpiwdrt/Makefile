#	$NetBSD: Makefile,v 1.2 2019/02/17 04:05:42 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpiwdrt
IOCONF=	acpiwdrt.ioconf
SRCS=	acpi_wdrt.c

.include <bsd.kmodule.mk>
