# $NetBSD: Makefile,v 1.10 2012/08/10 16:04:28 joerg Exp $

NOPROFILE=	yes

SRCS=	ccexc.cc construct.cc virt.cc

LIB=	test
LIBISCXX=	yes
TESTLIB=	testlib.so

CLEANFILES+=	${TESTLIB}
SHLIB_MAJOR=	1
SHLIB_MINOR=	0
#CXXFLAGS+= -fno-rtti

.if ${MACHINE_ARCH} == "i386" && "${MKPIC}" != "no"
all realall:	${TESTLIB}
.else
all realall:
.endif

${TESTLIB}: lib${LIB}.so.1.0
	cp lib${LIB}.so.1.0 ${TESTLIB}

regress:

.include <bsd.lib.mk>
