# $NetBSD: Makefile,v 1.6 2016/12/15 06:48:14 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa ${S}/dev

KMOD=	spkr
IOCONF=	spkr.ioconf
SRCS=	spkr.c spkr_pcppi.c

.PATH:	${S}/dev
SRCS+=	spkr_audio.c audiobell.c

WARNS=	4

.include <bsd.kmodule.mk>
