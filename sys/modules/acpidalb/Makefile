#	$NetBSD: Makefile,v 1.1 2010/10/25 17:17:56 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpidalb
IOCONF=	ACPIDALB.ioconf
SRCS=	dalb_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
