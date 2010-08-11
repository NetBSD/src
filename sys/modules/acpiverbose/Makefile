#	$NetBSD: Makefile,v 1.3 2010/08/11 14:50:38 pooka Exp $

.include "../Makefile.inc"

KMOD=	acpiverbose

.PATH:	${S}/dev/acpi
SRCS=	acpi_verbose.c

IOCONF=	ACPIVERBOSE.ioconf

.include <bsd.kmodule.mk>
