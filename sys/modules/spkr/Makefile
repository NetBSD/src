# $NetBSD: Makefile,v 1.1.2.4 2017/08/28 17:53:10 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/isa ${S}/dev

KMOD=	spkr
IOCONF=	spkr.ioconf
SRCS=	spkr.c spkr_pcppi.c

.PATH:	${S}/dev
SRCS+=	spkr_audio.c audiobell.c

WARNS=	4
CPPFLAGS+=	-DNWSMUX=1

.include <bsd.kmodule.mk>
