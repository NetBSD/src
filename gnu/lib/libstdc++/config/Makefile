#	$NetBSD: Makefile,v 1.4 2000/03/26 09:54:28 mycroft Exp $

# Makefile for _G_config.h, a machine-dependent automatically generated
# configuration file.

DPSRCS=		_G_config.h
INCS=		_G_config.h
INCSDIR=	/usr/include/${MACHINE}
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

.include <bsd.prog.mk>

incinstall:: linksinstall
