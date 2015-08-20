# $NetBSD: Makefile,v 1.2 2015/08/20 11:05:01 christos Exp $

.include "../Makefile.inc"

.PATH: ${S}/dev/filemon

KMOD = filemon
IOCONF=	filemon.ioconf
SRCS = filemon.c filemon_wrapper.c
NOMAN = no

.include <bsd.kmodule.mk>
