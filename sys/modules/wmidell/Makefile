# $NetBSD: Makefile,v 1.3 2011/02/16 08:19:56 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=   wmidell
IOCONF=	wmidell.ioconf
SRCS=	wmi_dell.c

WARNS=	4

.include <bsd.kmodule.mk>
