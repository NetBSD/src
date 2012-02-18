#	$NetBSD: Makefile,v 1.13.8.1 2012/02/18 07:33:32 mrg Exp $

S=	${.CURDIR}/../../../..

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
CPPFLAGS+=	-nostdinc 
CPPFLAGS+=	-I$S -I.
CPPFLAGS+=	-I${.CURDIR}/../libdos -I${.CURDIR}/../libiocs
CPPFLAGS+=	-I${DESTDIR}/usr/include
LDFLAGS=	-nostdlib -static -N
LDLIBS=		-L${LIBDOS} -ldos -L${LIBIOCS} -liocs -L${DESTDIR}/usr/lib -lc
DPADD+=		${AOUT2HUX}

CLEANFILES+=	${BASE}1 ${BASE}2
CLEANFILES+=	machine m68k

.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN:
	rm -f machine && ln -s $S/arch/x68k/include machine
	rm -f m68k && ln -s $S/arch/m68k/include m68k
.endif

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
