#	$NetBSD: Makefile,v 1.14.122.1 2010/03/11 15:02:43 yamt Exp $

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

# separate rule for bugstart.c
# this is build separately from rest of libbug
bugstart.o: bugstart.s
	${CC} -x assembler-with-cpp -traditional-cpp -nostdinc ${INCPATH} \
	-D_STANDALONE -c ${.IMPSRC}

all realall: lib${LIB}.a

.include <bsd.own.mk>
.undef DESTDIR
.include <bsd.lib.mk>
