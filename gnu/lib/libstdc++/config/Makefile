#	$NetBSD: Makefile,v 1.5 2001/08/10 11:57:48 mrg Exp $

# Makefile for _G_config.h, a machine-dependent automatically generated
# configuration file.

INCS=		_G_config.h
INCSDIR=	/usr/include/${MACHINE}

.include <bsd.own.mk>

.if defined(USE_NEW_TOOLCHAIN)

.PATH: ${.CURDIR}/../arch/${MACHINE_ARCH}

.else
# Old toolchain goop

DPSRCS=		_G_config.h
CLEANFILES=	dummy.c dummy.h dummy.C _G_config.h
DIST=		${.CURDIR}/../../../dist

.ifdef DESTDIR
LDPRE=	-nostdlib -L${DESTDIR}/usr/lib -R${DESTDIR}/usr/lib ${LIBCRT0} \
	${LIBCRTBEGIN}
LDSUF=	-lgcc -lc -lgcc ${LIBCRTEND}
.endif

_G_config.h: Makefile ${DIST}/libio/gen-params
	rootme=${.OBJDIR} CC="${CC}" CXX="${CXX}" CONFIG_NM="${NM}" \
	    CPP="${CC} -E" CPPFLAGS="${CPPFLAGS}" LDPRE="${LDPRE}" \
	    LDSUF="${LDSUF}" \
	    /bin/sh ${DIST}/libio/gen-params LIB_VERSION=2.9.0 >$@

all: ${DPSRCS}

.endif

.include <bsd.prog.mk>

incinstall:: linksinstall
