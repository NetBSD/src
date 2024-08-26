#	$NetBSD: Makefile,v 1.1 2024/08/26 13:38:28 riastradh Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpivmgenid
IOCONF=	acpivmgenid.ioconf
SRCS=	acpi_vmgenid.c

.include <bsd.kmodule.mk>
