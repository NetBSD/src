#	$NetBSD: Makefile,v 1.1 2010/10/25 17:17:56 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpitz
IOCONF=	ACPITZ.ioconf
SRCS=	acpi_tz.c

WARNS=	0

.include <bsd.kmodule.mk>
