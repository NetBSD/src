#	$NetBSD: Makefile,v 1.6 1998/08/01 11:22:52 scw Exp $

LIB=bug

NOPIC=
NOPROFILE=

S=${.CURDIR}/../../../..
DIR_SA=$S/lib/libsa

CPPFLAGS+=-I${.CURDIR}/../../include -I${DIR_SA}

SRCS=	delay.c diskrd.c diskwr.c getbrdid.c inchr.c instat.c outln.c \
	outstr.c putchar.c return.c rtc_rd.c

# only needed during build
libinstall::

.include <bsd.lib.mk>
