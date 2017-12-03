#	$NetBSD: Makefile,v 1.1.10.2 2017/12/03 11:38:58 jdolecek Exp $

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
