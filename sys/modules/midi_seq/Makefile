#	$NetBSD: Makefile,v 1.1 2022/06/04 03:31:10 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev

KMOD=	midi_seq
IOCONF=	midi_seq.ioconf

SRCS=	midi_seq_mod.c
SRCS+=	midi.c \
	midictl.c \
	midisyn.c
SRCS+=	sequencer.c

CPPFLAGS+=	-DNMIDI=1 -DNSEQUENCER=1

# Rather than our usual WARNS=5, we need to use 3, since there are a
# lot of signed-vs-unsigned compares

WARNS=  3

.include <bsd.kmodule.mk>
