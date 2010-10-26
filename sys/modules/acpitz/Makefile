#	$NetBSD: Makefile,v 1.2 2010/10/26 04:37:33 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpitz
IOCONF=	ACPITZ.ioconf
SRCS=	acpi_tz.c

WARNS=	4

.include <bsd.kmodule.mk>
