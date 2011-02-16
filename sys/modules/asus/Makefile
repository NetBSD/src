#	$NetBSD: Makefile,v 1.2 2011/02/16 08:08:14 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	asus
IOCONF=	asus.ioconf
SRCS=	asus_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
