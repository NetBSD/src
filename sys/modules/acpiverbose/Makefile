#	$NetBSD: Makefile,v 1.3.2.3 2010/10/09 03:32:35 yamt Exp $

.include "../Makefile.inc"

KMOD=	acpiverbose

.PATH:	${S}/dev/acpi
SRCS=	acpi_verbose.c

IOCONF=	ACPIVERBOSE.ioconf

.include <bsd.kmodule.mk>
