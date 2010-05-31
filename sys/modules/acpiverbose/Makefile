#	$NetBSD: Makefile,v 1.1 2010/05/31 20:32:29 pgoyette Exp $

.include "../Makefile.inc"

KMOD=	acpiverbose

.PATH:	${S}/dev/acpi
SRCS=	acpi_verbose.c

.PATH:	${S}/dev/acpi/wmi
SRCS+=	wmi_dump.c

.include <bsd.kmodule.mk>
