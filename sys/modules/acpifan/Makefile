# $NetBSD: Makefile,v 1.2.58.1 2019/06/10 22:09:07 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpifan
IOCONF=	acpifan.ioconf
SRCS=	acpi_fan.c

.include <bsd.kmodule.mk>
