# $NetBSD: Makefile,v 1.1 2016/12/10 00:40:27 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa ${S}/dev

KMOD=	spkr_synth
IOCONF=	spkr_synth.ioconf
SRCS=	spkr.c spkr_synth.c

WARNS=	4

.include <bsd.kmodule.mk>
