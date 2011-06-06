#	$NetBSD: Makefile,v 1.2.4.1 2011/06/06 09:09:45 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpitz
IOCONF=	acpitz.ioconf
SRCS=	acpi_tz.c

WARNS=	4

.include <bsd.kmodule.mk>
