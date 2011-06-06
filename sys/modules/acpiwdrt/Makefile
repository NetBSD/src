#	$NetBSD: Makefile,v 1.1.8.2 2011/06/06 09:09:46 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpiwdrt
IOCONF=	acpiwdrt.ioconf
SRCS=	acpi_wdrt.c

WARNS=	4

.include <bsd.kmodule.mk>
