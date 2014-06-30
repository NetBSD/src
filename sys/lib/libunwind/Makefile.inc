#	$NetBSD: Makefile.inc,v 1.9 2014/06/30 20:42:31 joerg Exp $

.PATH:	${NETBSDSRCDIR}/sys/lib/libunwind

SRCS+=	libunwind.cxx \
	unwind_registers.S

INCS+=	unwind.h

COPTS.libunwind.cxx+=	${${ACTIVE_CXX} == "gcc":? -std=c++0x : -std=c++11 }
COPTS.libunwind.cxx+=	-funwind-tables -fno-rtti
COPTS.libunwind.cxx+=	-fno-exceptions -fvisibility=hidden
COPTS.libunwind.cxx+=	-Wno-old-style-cast

.if ${MACHINE_CPU} == "arm"
AFLAGS.unwind_registers.S+=	${${ACTIVE_CC} == "clang":? -no-integrated-as :}
.endif
