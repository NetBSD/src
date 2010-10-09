# $NetBSD: Makefile,v 1.1.2.2 2010/10/09 03:32:36 yamt Exp $

.include "../Makefile.inc"

.PATH: ${S}/dev/filemon

KMOD = filemon
SRCS = filemon.c filemon_wrapper.c
NOMAN = no

.include <bsd.kmodule.mk>
