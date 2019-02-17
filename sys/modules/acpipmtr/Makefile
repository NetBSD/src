# $NetBSD: Makefile,v 1.3 2019/02/17 04:05:42 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpipmtr
IOCONF=	acpipmtr.ioconf
SRCS=	acpi_pmtr.c

.include <bsd.kmodule.mk>
