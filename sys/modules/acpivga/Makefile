#	$NetBSD: Makefile,v 1.1 2010/10/25 17:17:57 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpivga
IOCONF=	ACPIVGA.ioconf
SRCS=	acpi_display.c

WARNS=	4

.include <bsd.kmodule.mk>
