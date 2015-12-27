# $NetBSD: Makefile,v 1.1.2.2 2015/12/27 12:10:05 skrll Exp $

.include "../Makefile.inc"

.PATH: ${S}/dev

KMOD=		clockctl
IOCONF=		clockctl.ioconf
SRCS=		clockctl.c

CPPFLAGS+=	-DNTP -DCOMPAT_50

.include <bsd.kmodule.mk>
