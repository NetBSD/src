# $NetBSD: Makefile,v 1.3 2002/08/28 17:05:30 drochner Exp $

SRCS=	ccexc.cc construct.cc virt.cc

LIB=	test
TESTLIB=	testlib.so

CLEANFILES+=	${TESTLIB}
NOPROFILE=	yes
SHLIB_MAJOR=	1
SHLIB_MINOR=	0
#CXXFLAGS+= -fno-rtti
LDADD+=	-lstdc++

.if (${MACHINE_ARCH} == "i386")
all realall:	${TESTLIB}
.else
all realall:
.endif

${TESTLIB}: lib${LIB}.so.1.0
	cp lib${LIB}.so.1.0 ${TESTLIB}

regress:

.include <bsd.lib.mk>
