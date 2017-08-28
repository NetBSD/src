#	$NetBSD: Makefile,v 1.1.6.2 2017/08/28 17:53:10 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev

KMOD=	midi
IOCONF=	midi.ioconf
SRCS=	midi.c \
	midictl.c \
	midisyn.c

CPPFLAGS+=	-DNMIDI=1 -DNSEQUENCER=1

# Rather than our usual WARNS=4, we need to use 3, since there are a
# lot of signed-vs-unsigned compares

WARNS=  3

.include <bsd.kmodule.mk>
