# $NetBSD: Makefile,v 1.5 2019/02/17 04:05:41 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpibat
IOCONF=	acpibat.ioconf
SRCS=	acpi_bat.c

.include <bsd.kmodule.mk>
