# $NetBSD: Makefile,v 1.1 2011/02/20 08:37:51 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	fujhk
IOCONF=	fujhk.ioconf
SRCS=	fujhk_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
