#	$NetBSD: Makefile,v 1.4.4.1 2002/01/10 19:40:07 thorpej Exp $

PROG=	ahdilabel
SRCS=	ahdilabel.c build.c check.c cksum.c magic.c openraw.c read.c write.c \
	writedtab.c

NOMAN=	# defined
BINDIR=	/sbin

WARNS=	1

.if defined(DEBUG)
CFLAGS += -g
CPPFLAGS+=-DDEBUG
.endif
.include <bsd.prog.mk>
