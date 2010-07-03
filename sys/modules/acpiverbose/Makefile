#	$NetBSD: Makefile,v 1.1.2.2 2010/07/03 01:19:58 rmind Exp $

.include "../Makefile.inc"

KMOD=	acpiverbose

.PATH:	${S}/dev/acpi
SRCS=	acpi_verbose.c

.PATH:	${S}/dev/acpi/wmi
SRCS+=	wmi_dump.c

.include <bsd.kmodule.mk>
