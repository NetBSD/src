#	$NetBSD: Makefile,v 1.4 1997/05/30 13:43:29 cjs Exp $

LIB=bug

NOPIC=
NOPROFILE=

# this library is used only for the build
NOINSTALL=1

S=${.CURDIR}/../../../..
DIR_SA=$S/lib/libsa

CFLAGS+=-I${.CURDIR}/../../include -I${DIR_SA}

SRCS=	delay.c diskrd.c diskwr.c getbrdid.c inchr.c instat.c outln.c \
	outstr.c putchar.c return.c rtc_rd.c

.include <bsd.lib.mk>
