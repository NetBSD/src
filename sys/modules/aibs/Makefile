#	$NetBSD: Makefile,v 1.2.4.1 2011/06/06 09:09:46 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	aibs
IOCONF=	aibs.ioconf
SRCS=	atk0110.c

WARNS=	4

.include <bsd.kmodule.mk>
