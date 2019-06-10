# $NetBSD: Makefile,v 1.3.56.1 2019/06/10 22:09:07 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpilid
IOCONF=	acpilid.ioconf
SRCS=	acpi_lid.c

.include <bsd.kmodule.mk>
