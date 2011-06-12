#	$NetBSD: Makefile,v 1.4 2011/06/12 07:25:43 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	aibs
IOCONF=	aibs.ioconf
SRCS=	aibs_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
