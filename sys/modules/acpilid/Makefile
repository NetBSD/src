# $NetBSD: Makefile,v 1.4 2019/02/17 04:05:42 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpilid
IOCONF=	acpilid.ioconf
SRCS=	acpi_lid.c

.include <bsd.kmodule.mk>
