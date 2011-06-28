#	$NetBSD: Makefile,v 1.3 2011/06/28 04:07:36 mrg Exp $

SUBDIR=		backward ext bits debug tr1
#SUBDIR+=	pb_assoc

.include <bsd.own.mk>

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

DIST=		${NETBSDSRCDIR}/external/gpl3/gcc/dist
GNUHOSTDIST=	${DIST}

INCS=		exception new typeinfo cxxabi.h exception_defines.h
INCS+=		cerrno
INCS+=		${G_std_headers} ${G_c_base_headers}
INCS+=		${G_c_compatibility_headers_extra}
INCSDIR=	/usr/include/g++

.PATH: ${DIST}/libstdc++-v3 ${DIST}/libstdc++-v3/libsupc++
.PATH: ${DIST}/libstdc++-v3/include/c_compatibility
.PATH: ${DIST}/libstdc++-v3/include/c_global
.PATH: ${DIST}/libstdc++-v3/include/std

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
