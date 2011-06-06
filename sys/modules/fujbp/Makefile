# $NetBSD: Makefile,v 1.1.8.2 2011/06/06 09:09:48 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	fujbp
IOCONF=	fujbp.ioconf
SRCS=	fujbp_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
