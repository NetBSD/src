#	$NetBSD: Makefile,v 1.3 1999/02/13 02:54:50 lukem Exp $

BASE=	loadbsd
PROG=	${BASE}.x	# Human68k ".x" executable
STRIPFLAG=		# not an a.out
BINMODE=444		# not to be run on NetBSD
SRCS=	start.S loadbsd.c xprintf.c trampoline.S

MKMAN=	no
BINDIR=		/usr/mdec

.PATH:	${.CURDIR}/../common

AOUT2HUX!=cd ${.CURDIR}/../aout2hux && echo `${MAKE} print-objdir`/aout2hux
LIBDOS!=cd ${.CURDIR}/../libdos && ${MAKE} print-objdir
LIBIOCS!=cd ${.CURDIR}/../libiocs && ${MAKE} print-objdir

CPPFLAGS=	-W -Wall -O -fomit-frame-pointer
CPPFLAGS+=	-m68000 -Wa,-mc68000
CPPFLAGS+=	-I${.CURDIR}/../libdos -I${.CURDIR}/../libiocs
LDFLAGS=	-nostdlib -static -N
LDLIBS=		-L${LIBDOS} -ldos -L${LIBIOCS} -liocs -L${DESTDIR}/usr/lib -lc
DPADD+=		${AOUT2HUX}

CLEANFILES+=	${BASE}1 ${BASE}2

${PROG}: ${BASE}1 ${BASE}2
	${AOUT2HUX} -o ${.TARGET} ${BASE}1 11000 ${BASE}2 22000

.include <bsd.prog.mk>

.for i in 1 2
${BASE}${i}:	${OBJS} ${LIBDOS}/libdos.a ${LIBIOCS}/libiocs.a
	${CC} -o ${.TARGET} ${LDFLAGS} -Wl,-T${i}${i}000 ${OBJS} ${LDLIBS}
.endfor
