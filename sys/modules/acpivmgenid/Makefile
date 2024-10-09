#	$NetBSD: Makefile,v 1.1.2.2 2024/10/09 13:25:12 martin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpivmgenid
IOCONF=	acpivmgenid.ioconf
SRCS=	acpi_vmgenid.c

.include <bsd.kmodule.mk>
