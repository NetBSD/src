#	$NetBSD: Makefile,v 1.8.8.1 2002/05/30 15:36:41 gehenna Exp $

BASE=	loadbsd
PROG=	${BASE}.x	# Human68k ".x" executable
STRIPFLAG=		# not an a.out
BINMODE=444		# not to be run on NetBSD
SRCS=	start.S loadbsd.c xprintf.c trampoline.S

NOMAN=	# defined
BINDIR=		/usr/mdec

.PATH:	${.CURDIR}/../common

.include <bsd.own.mk>

AOUT2HUXDIR!=	cd ${.CURDIR}/../aout2hux && ${PRINTOBJDIR}
AOUT2HUX=	${AOUT2HUXDIR}/aout2hux
LIBDOS!=	cd ${.CURDIR}/../libdos && ${PRINTOBJDIR}
LIBIOCS!=	cd ${.CURDIR}/../libiocs && ${PRINTOBJDIR}

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
	${CC} -o ${.TARGET} ${LDFLAGS} -Wl,-Ttext,${i}${i}000 ${OBJS} ${LDLIBS}
.endfor

.ifdef RELEASEDIR
release: ${PROG}
	cp -ip ${PROG} ${RELEASEDIR}/installation/misc
.endif
