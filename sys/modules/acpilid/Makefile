# $NetBSD: Makefile,v 1.3 2011/02/16 08:35:52 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpilid
IOCONF=	acpilid.ioconf
SRCS=	acpi_lid.c

WARNS=	4

.include <bsd.kmodule.mk>
