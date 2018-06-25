#	$NetBSD: Makefile,v 1.100.16.1 2018/06/25 07:26:04 pgoyette Exp $

LIB=		kern
NOPIC=		# defined
LLIBS=		# defined

.include "Makefile.libkern"
.ifndef ARCHSUBDIR
.BEGIN:
	@echo no ARCHSUBDIR for ${MACHINE_ARCH} nor ${MACHINE_CPU}
	@false
.endif

# only needed during build
libinstall::

.undef DESTDIR
.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS:O} __buildstdlib

lib${LIB}.po:: ${POBJS:O} __buildproflib

showsources: ${SRCS:O}
	@echo ${.ALLSRC}
