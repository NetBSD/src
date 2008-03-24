# $NetBSD: Makefile,v 1.8.14.1 2008/03/24 07:14:47 keiichi Exp $

NOPROFILE=	yes

SRCS=	ccexc.cc construct.cc virt.cc

LIB=	test
TESTLIB=	testlib.so

CLEANFILES+=	${TESTLIB}
SHLIB_MAJOR=	1
SHLIB_MINOR=	0
#CXXFLAGS+= -fno-rtti
LDADD+=	-lstdc++ -lm -lgcc_s

.if ${MACHINE_ARCH} == "i386" && "${MKPIC}" != "no"
all realall:	${TESTLIB}
.else
all realall:
.endif

${TESTLIB}: lib${LIB}.so.1.0
	cp lib${LIB}.so.1.0 ${TESTLIB}

regress:

.include <bsd.lib.mk>
