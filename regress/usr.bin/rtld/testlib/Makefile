# $NetBSD: Makefile,v 1.1 2000/12/08 19:21:28 drochner Exp $

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
.include <bsd.depall.mk>
