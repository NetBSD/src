#	$NetBSD: Makefile,v 1.10 2000/07/28 20:08:21 scw Exp $

LIB=bug

MKPIC= no
MKLINT= no
MKPROFILE= no

S=${.CURDIR}/../../../..
DIR_SA=$S/lib/libsa

SRCS=	delay.c diskrd.c diskwr.c getbrdid.c inchr.c instat.c outln.c \
	outstr.c putchar.c return.c rtc_rd.c
CLEANFILES+= bugcrt.o bugcrt.o.tmp

.include "../Makefile.booters"

# only needed during build
libinstall::

# separate rule for bugcrt.c - NEED NOT BE COMPILED WITH -O2
# this is build separately from rest of libbug
bugcrt.o: bugcrt.c
	${CC} -nostdinc ${INCPATH} -D_STANDALONE -o ${.TARGET}.tmp -c ${.IMPSRC}
	${LD} -x -r ${.TARGET}.tmp -o ${.TARGET}

all realall: lib${LIB}.a bugcrt.o

.include <bsd.lib.mk>
