#	$NetBSD: Makefile.inc,v 1.6 2014/06/13 01:17:46 mrg Exp $

.PATH:	${NETBSDSRCDIR}/sys/lib/libunwind

SRCS+=	libunwind.cxx \
	unwind_registers.S

INCS+=	unwind.h

COPTS.libunwind.cxx+=	${${ACTIVE_CXX} == "gcc":? -std=c++0x : -std=c++11 }
COPTS.libunwind.cxx+=	-funwind-tables -fno-rtti -fno-exceptions -fvisibility=hidden -Wno-old-style-cast
