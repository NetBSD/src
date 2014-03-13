#	$NetBSD: Makefile.inc,v 1.4 2014/03/13 01:50:50 joerg Exp $

.PATH:	${NETBSDSRCDIR}/sys/lib/libunwind

SRCS+=	libunwind.cxx \
	unwind_registers.S

INCS+=	unwind.h

COPTS.libunwind.cxx+=	${${ACTIVE_CXX} == "gcc":? -std=c++0x : -std=c++11 }
COPTS.libunwind.cxx+=	-funwind-tables -fno-rtti -fno-exceptions -fvisibility=hidden -Wno-old-style-cast
