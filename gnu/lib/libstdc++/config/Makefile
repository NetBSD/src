#	$NetBSD: Makefile,v 1.3 1999/02/13 23:53:57 lukem Exp $

# Makefile for _G_config.h, a machine-dependent automatically generated
# configuration file.

DPSRCS=		_G_config.h
INCS=		_G_config.h
INCSDIR=	/usr/include/${MACHINE}
CLEANFILES=	dummy.c dummy.h dummy.C _G_config.h
DIST=		${.CURDIR}/../../../dist

_G_config.h: Makefile ${DIST}/libio/gen-params
	rootme=${.OBJDIR} CC="${CC}" CXX="${CXX}" CONFIG_NM="${NM}" \
	    CPP="${CC} -E -nostdinc -idirafter ${DESTDIR}/usr/include" \
	    /bin/sh ${DIST}/libio/gen-params LIB_VERSION=2.9.0 >$@

all: ${DPSRCS}

.include <bsd.prog.mk>

incinstall:: linksinstall
