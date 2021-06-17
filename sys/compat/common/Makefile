#	$NetBSD: Makefile,v 1.65.16.1 2021/06/17 04:46:27 thorpej Exp $

LIB=		compat
NOPIC=		# defined
LLIBS=		# defined

CPPFLAGS=	${COMPATCPPFLAGS} ${SKIP_AUTODEP::=}

.PATH.c: ${COMPATDIR}

# Common compatibility code, used by all emulations
SRCS=	compat_exec.c

# Include compatibility code common to libcompat and the compat module

.include "Makefile.sysio"
.include "Makefile.sysv"

# only needed during build
libinstall::

.include <bsd.own.mk>
.undef DESTDIR
.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS:O} __buildstdlib

lib${LIB}.po:: ${POBJS:O} __buildproflib

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+=	-DCPU_UCODE
.endif

showsources: ${SRCS}
	@echo ${.ALLSRC}
