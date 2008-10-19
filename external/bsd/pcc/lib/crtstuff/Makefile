#	$NetBSD: Makefile,v 1.1.6.2 2008/10/19 22:40:48 haad Exp $

.include <bsd.own.mk>

DIST=   ${NETBSDSRCDIR}/external/bsd/pcc/dist/pcc-libs
SUBDIST=${DIST}/csu/netbsd
.PATH:  ${SUBDIST}

#CPPFLAGS+=-I${SUBDIST}

#.include "../../Makefile.inc"

SRCS+=		crtbegin.c crtend.c
OBJS+=		crtbegin.o crtend.o

realall: ${OBJS}

crtbegin.o: crtbegin.c
	${_MKTARGET_COMPILE}
	${COMPILE.c} ${.IMPSRC} -o ${.TARGET}.o
	${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	rm -f ${.TARGET}.o

crtend.o: crtend.c
	${_MKTARGET_COMPILE}
	${COMPILE.c} ${.IMPSRC} -o ${.TARGET}.o
	${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	rm -f ${.TARGET}.o

FILES=${OBJS}
FILESDIR=${LIBDIR}
CLEANFILES=${OBJS}

.include <bsd.prog.mk>
