#	$NetBSD: Makefile,v 1.7 1999/02/13 02:54:47 lukem Exp $

LIB=bug

MKPIC=no
MKPROFILE=no

S=${.CURDIR}/../../../..
DIR_SA=$S/lib/libsa

CPPFLAGS+=-I${.CURDIR}/../../include -I${DIR_SA}

SRCS=	delay.c diskrd.c diskwr.c getbrdid.c inchr.c instat.c outln.c \
	outstr.c putchar.c return.c rtc_rd.c

# only needed during build
libinstall::

.include <bsd.lib.mk>
