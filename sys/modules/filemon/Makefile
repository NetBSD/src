# $NetBSD: Makefile,v 1.1.42.1 2015/09/22 12:06:09 skrll Exp $

.include "../Makefile.inc"

.PATH: ${S}/dev/filemon

KMOD = filemon
IOCONF=	filemon.ioconf
SRCS = filemon.c filemon_wrapper.c
NOMAN = no

.include <bsd.kmodule.mk>
