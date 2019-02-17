# $NetBSD: Makefile,v 1.3 2019/02/17 04:05:41 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpifan
IOCONF=	acpifan.ioconf
SRCS=	acpi_fan.c

.include <bsd.kmodule.mk>
