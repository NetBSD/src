# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:47 rin Exp $

.include "../Makefile.inc"

.PATH: ${S}/kern

KMOD=		drvctl
IOCONF=		drvctl.ioconf
SRCS=		kern_drvctl.c

WARNS=		3

.include <bsd.kmodule.mk>
