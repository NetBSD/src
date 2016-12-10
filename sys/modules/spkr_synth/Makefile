# $NetBSD: Makefile,v 1.2 2016/12/10 05:09:10 nat Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa ${S}/dev

KMOD=	spkr_synth
IOCONF=	spkr_synth.ioconf
SRCS=	audiobell.c spkr.c spkr_synth.c

WARNS=	4

.include <bsd.kmodule.mk>
