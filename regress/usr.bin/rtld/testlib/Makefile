# $NetBSD: Makefile,v 1.6 2003/09/02 06:36:23 mrg Exp $

.include <bsd.own.mk>
# XXX
.if ${HAVE_GCC3} == "no"

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
.endif

regress:

.include <bsd.lib.mk>

