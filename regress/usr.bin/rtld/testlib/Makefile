# $NetBSD: Makefile,v 1.2 2002/04/14 21:44:22 wiz Exp $

SRCS=	ccexc.cc construct.cc virt.cc

LIB=	testlib.so

OBJS=	${SRCS:.cc=.o}

CPICFLAGS= -fpic -DPIC

.cc.o:
	${COMPILE.cc} ${CPICFLAGS} ${.IMPSRC} -o ${.TARGET}

.if (${MACHINE_ARCH} == "i386")
all realall:	${LIB}
.else
all realall:
.endif

${LIB}: ${OBJS}
	${CXX} -shared -o ${LIB} ${OBJS}

clean cleandir:
	rm -f *.o *.so

regress:

.include <bsd.obj.mk>
.include <bsd.dep.mk>
