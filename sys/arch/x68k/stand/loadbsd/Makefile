#	$NetBSD: Makefile,v 1.14.6.2 2017/12/03 11:36:49 jdolecek Exp $

S=	${.CURDIR}/../../../..

BASE=	loadbsd
PROG=	${BASE}.x	# Human68k ".x" executable
BINMODE=444		# not to be run on NetBSD
SRCS=	start.S loadbsd.c xprintf.c trampoline.S

NOMAN=	# defined
BINDIR=		/usr/mdec

.PATH:	${.CURDIR}/../common

.include <bsd.own.mk>
.include <bsd.klinks.mk>

STRIPFLAG=		# not an elf

AOUT2HUXDIR!=	cd ${.CURDIR}/../aout2hux && ${PRINTOBJDIR}
AOUT2HUX=	${AOUT2HUXDIR}/aout2hux
LIBDOS!=	cd ${.CURDIR}/../libdos && ${PRINTOBJDIR}
LIBIOCS!=	cd ${.CURDIR}/../libiocs && ${PRINTOBJDIR}

CPPFLAGS=	-W -Wall -O -fomit-frame-pointer -fno-unwind-tables
CPPFLAGS+=	-m68000 -Wa,-mc68000
CPPFLAGS+=	-nostdinc 
CPPFLAGS+=	-I$S -I.
CPPFLAGS+=	-I${.CURDIR}/../libdos -I${.CURDIR}/../libiocs
CPPFLAGS+=	-I${DESTDIR}/usr/include
LDFLAGS=	-nostdlib -static -N
LDLIBS=		-L${LIBDOS} -ldos -L${LIBIOCS} -liocs -L${DESTDIR}/usr/lib -lc
DPADD+=		${AOUT2HUX}

CLEANFILES+=	${BASE}1 ${BASE}2

${PROG}: ${BASE}1 ${BASE}2
	${_MKTARGET_LINK}
	${AOUT2HUX} -o ${.TARGET} ${BASE}1 11000 ${BASE}2 22000

.include <bsd.prog.mk>

.for i in 1 2
${BASE}${i}:	${OBJS} ${LIBDOS}/libdos.a ${LIBIOCS}/libiocs.a
	${_MKTARGET_LINK}
	${CC} -o ${.TARGET} ${LDFLAGS} -Wl,-Ttext,${i}${i}000 ${OBJS} ${LDLIBS}
.endfor

release: check_RELEASEDIR
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${PROG} \
	    ${RELEASEDIR}/${MACHINE}/installation/misc
	${TOOL_MANDOC_ASCII} ${DESTDIR}/usr/share/man/man8/x68k/loadbsd.8 > \
	    ${RELEASEDIR}/${MACHINE}/installation/misc/loadbsd.0
	${TOOL_MANDOC_HTML} ${DESTDIR}/usr/share/man/man8/x68k/loadbsd.8 > \
	    ${RELEASEDIR}/${MACHINE}/installation/misc/loadbsd.html
