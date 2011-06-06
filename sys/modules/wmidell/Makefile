# $NetBSD: Makefile,v 1.2.4.1 2011/06/06 09:09:51 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=   wmidell
IOCONF=	wmidell.ioconf
SRCS=	wmi_dell.c

WARNS=	4

.include <bsd.kmodule.mk>
