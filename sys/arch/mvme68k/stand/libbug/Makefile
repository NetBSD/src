#	$NetBSD: Makefile,v 1.13 2001/11/14 23:01:14 tv Exp $

LIB=bug

MKPIC= no
MKLINT= no
MKPROFILE= no

S=${.CURDIR}/../../../..
DIR_SA=$S/lib/libsa

SRCS=	bugcrt.c delay.c diskrd.c diskwr.c getbrdid.c inchr.c instat.c \
	outln.c outstr.c putchar.c return.c rtc_rd.c
CLEANFILES+= bugstart.o

.include "../Makefile.booters"

# only needed during build
libinstall::

# separate rule for bugstart.c
# this is build separately from rest of libbug
bugstart.o: bugstart.s
	${CC} -x assembler-with-cpp -traditional-cpp -nostdinc ${INCPATH} \
	-D_STANDALONE -c ${.IMPSRC}

all realall: lib${LIB}.a bugstart.o

.include <bsd.own.mk>
.undef DESTDIR
.include <bsd.lib.mk>
