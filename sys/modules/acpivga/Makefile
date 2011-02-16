#	$NetBSD: Makefile,v 1.2 2011/02/16 08:01:53 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpivga
IOCONF=	acpivga.ioconf
SRCS=	acpi_display.c

WARNS=	4

.include <bsd.kmodule.mk>
