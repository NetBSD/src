#	$NetBSD: Makefile,v 1.3 2001/02/25 14:31:53 jdc Exp $

PROG=	ahdilabel
SRCS=	ahdilabel.c build.c check.c cksum.c magic.c openraw.c read.c write.c \
	writedtab.c

MAN=	ahdilabel.8
MANSUBDIR=	/atari
BINDIR=	/sbin

WARNS=	1

.if defined(DEBUG)
CFLAGS += -g
CPPFLAGS+=-DDEBUG
.endif
.include <bsd.prog.mk>
