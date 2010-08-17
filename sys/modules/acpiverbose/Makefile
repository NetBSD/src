#	$NetBSD: Makefile,v 1.3.4.2 2010/08/17 06:47:40 uebayasi Exp $

.include "../Makefile.inc"

KMOD=	acpiverbose

.PATH:	${S}/dev/acpi
SRCS=	acpi_verbose.c

IOCONF=	ACPIVERBOSE.ioconf

.include <bsd.kmodule.mk>
