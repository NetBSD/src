#	$NetBSD: Makefile,v 1.1.14.1 2019/06/10 22:09:33 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev

KMOD=	midi
IOCONF=	midi.ioconf
SRCS=	midi.c \
	midictl.c \
	midisyn.c

CPPFLAGS+=	-DNMIDI=1 -DNSEQUENCER=1

# Rather than our usual WARNS=5, we need to use 3, since there are a
# lot of signed-vs-unsigned compares

WARNS=  3

.include <bsd.kmodule.mk>
