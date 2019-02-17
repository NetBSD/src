# $NetBSD: Makefile,v 1.8 2019/02/17 04:05:58 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa ${S}/dev

KMOD=	spkr
IOCONF=	spkr.ioconf
SRCS=	spkr.c spkr_pcppi.c

.PATH:	${S}/dev
SRCS+=	spkr_audio.c audiobell.c

CPPFLAGS+=	-DNWSMUX=1

.include <bsd.kmodule.mk>
