#	$NetBSD: Makefile,v 1.2 2010/01/20 11:45:55 plunky Exp $

.include <bsd.init.mk>

.PATH: ${PCC_LIBS}/csu/${TARGOS}

SRCS+=		crtbegin.c crtend.c
OBJS+=		crtbegin.o crtend.o

CPPFLAGS+=	-I${PCC_LIBS}/csu/${TARGOS}
CPPFLAGS+=	-Wno-missing-prototypes

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
