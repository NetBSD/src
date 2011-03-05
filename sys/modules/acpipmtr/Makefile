# $NetBSD: Makefile,v 1.2.2.2 2011/03/05 20:55:36 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpipmtr
IOCONF=	acpipmtr.ioconf
SRCS=	acpi_pmtr.c

WARNS=	4

.include <bsd.kmodule.mk>
