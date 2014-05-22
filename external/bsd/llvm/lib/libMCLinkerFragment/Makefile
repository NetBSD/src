#	$NetBSD: Makefile,v 1.1.12.2 2014/05/22 16:14:44 yamt Exp $

LIB=	MCLinkerFragment

.include <bsd.init.mk>

.PATH: ${MCLINKER_SRCDIR}/lib/Fragment

SRCS=	AlignFragment.cpp \
	FillFragment.cpp \
	Fragment.cpp \
	FragmentLinker.cpp \
	FragmentRef.cpp \
	NullFragment.cpp \
	RegionFragment.cpp \
	Relocation.cpp \
	Stub.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
