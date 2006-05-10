#	$NetBSD: Makefile,v 1.1 2006/05/10 22:53:50 mrg Exp $

SUBDIR=		backward ext bits debug tr1
#SUBDIR+=	pb_assoc

.include <bsd.own.mk>

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

DIST=		${NETBSDSRCDIR}/gnu/dist/gcc4

INCS=		exception new typeinfo cxxabi.h exception_defines.h
INCS+=		${G_std_headers_rename} ${G_c_base_headers_rename}
INCS+=		${G_c_compatibility_headers_extra}
INCSDIR=	/usr/include/g++

${G_std_headers_rename}: ${.CURDIR}/Makefile
	cp ${DIST}/libstdc++-v3/include/std/std_${.TARGET}.h ${.TARGET}

CLEANFILES+=	${G_std_headers_rename} ${G_c_base_headers_rename}

.PATH: ${DIST}/libstdc++-v3 ${DIST}/libstdc++-v3/libsupc++

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>
.include <bsd.subdir.mk>

${G_c_base_headers_rename}: ${.CURDIR}/Makefile
	cp ${DIST}/libstdc++-v3/include/c_std/std_${.TARGET}.h ${.TARGET}


