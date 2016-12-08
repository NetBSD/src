# $NetBSD: Makefile,v 1.2 2016/12/08 21:42:42 nat Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev

KMOD=	spkr
IOCONF=	spkr.ioconf
SRCS=	spkr_synth.c

WARNS=	4

.include <bsd.kmodule.mk>
