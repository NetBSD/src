# $NetBSD: Makefile,v 1.4 2002/09/18 05:41:58 lukem Exp $

NOPROFILE=	yes

SRCS=	ccexc.cc construct.cc virt.cc

LIB=	test
TESTLIB=	testlib.so

CLEANFILES+=	${TESTLIB}
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
