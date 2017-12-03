# $NetBSD: Makefile,v 1.7.4.2 2017/12/03 11:38:59 jdolecek Exp $

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
