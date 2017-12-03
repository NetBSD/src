#	$NetBSD: Makefile.inc,v 1.9.4.3 2017/12/03 11:38:47 jdolecek Exp $

.PATH:	${NETBSDSRCDIR}/sys/lib/libunwind

SRCS+=	libunwind.cxx \
	unwind_registers.S

INCS+=	unwind.h

COPTS.libunwind.cxx+=	${${ACTIVE_CXX} == "gcc":? -std=c++0x : -std=c++11 }
COPTS.libunwind.cxx+=	-funwind-tables -fno-rtti
COPTS.libunwind.cxx+=	-fno-exceptions -fvisibility=hidden
COPTS.libunwind.cxx+=	-Wno-old-style-cast
CPPFLAGS.libunwind.cxx+=-I${NETBSDSRCDIR}/sys/lib/libunwind

.if ${LIBC_MACHINE_CPU} == "arm"
AFLAGS.unwind_registers.S+=	${${ACTIVE_CC} == "clang":? -mfpu=vfp3 :}
.endif
