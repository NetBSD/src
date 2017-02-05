# $NetBSD: Makefile,v 1.1.2.3 2017/02/05 13:40:57 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa ${S}/dev

KMOD=	spkr
IOCONF=	spkr.ioconf
SRCS=	spkr.c spkr_pcppi.c

.PATH:	${S}/dev
SRCS+=	spkr_audio.c audiobell.c

WARNS=	4

.include <bsd.kmodule.mk>
