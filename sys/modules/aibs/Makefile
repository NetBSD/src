#	$NetBSD: Makefile,v 1.2 2010/10/26 03:11:10 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	aibs
IOCONF=	AIBS.ioconf
SRCS=	atk0110.c

WARNS=	4

.include <bsd.kmodule.mk>
