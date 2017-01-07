# $NetBSD: Makefile,v 1.1.4.1 2017/01/07 08:56:50 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa ${S}/dev

KMOD=	spkr
IOCONF=	spkr.ioconf
SRCS=	spkr.c spkr_pcppi.c

.PATH:	${S}/dev
SRCS+=	spkr_audio.c audiobell.c

WARNS=	4

.include <bsd.kmodule.mk>
