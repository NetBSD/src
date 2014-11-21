#	$NetBSD: Makefile,v 1.18 2014/11/21 20:21:08 christos Exp $

LIB=bug

NOPIC= # defined
NOLINT= # defined
NOPROFILE= # defined

S=${.CURDIR}/../../../..
DIR_SA=$S/lib/libsa

SRCS=	bugcrt.c delay.c diskrd.c diskwr.c getbrdid.c inchr.c instat.c \
	outln.c outstr.c putchar.c return.c rtc_rd.c
OBJS+=	bugstart.o
CLEANFILES+= bugstart.o

.include "../Makefile.booters"

# only needed during build
libinstall::

all realall: lib${LIB}.a

.include <bsd.own.mk>
.undef DESTDIR
.include <bsd.lib.mk>
