#	$NetBSD: Makefile.inc,v 1.7 2014/06/13 21:31:20 matt Exp $

.PATH:	${NETBSDSRCDIR}/sys/lib/libunwind

SRCS+=	libunwind.cxx \
	unwind_registers.S

INCS+=	unwind.h

COPTS.libunwind.cxx+=	${${ACTIVE_CXX} == "gcc":? -std=c++0x : -std=c++11 }
COPTS.libunwind.cxx+=	-funwind-tables -fno-rtti
COPTS.libunwind.cxx+=	-fno-exceptions -fvisibility=hidden
COPTS.libunwind.cxx+=	-Wno-old-style-cast
