#	$NetBSD: Makefile,v 1.3 2020/09/21 06:48:48 mrg Exp $

NOCTF=
#HOSTPROG_CXX=   1

.include <bsd.own.mk>

.include "../Makefile.inc"

LIBISPRIVATE=	yes
LIB=		ctf
GDBARCHDIR=	${.CURDIR}/arch/${GDB_MACHINE_ARCH}

.include "${GDBARCHDIR}/defs.mk"

SRCS=		${G_OBJECTS:.o=.c}
GCPPFLAGS=	${G_INTERNAL_CFLAGS}
CPPFLAGS+=	-I${.CURDIR} \
		-I. \
		-I${DIST} \
		-I${DIST}/libctf \
		-I${DIST}/gnulib/import \
		-I${.CURDIR}/../libgnulib/arch/${GDB_MACHINE_ARCH} \
		-I${.CURDIR}/../libgnulib/arch/${GDB_MACHINE_ARCH}/import \
		-I${.CURDIR}/../libbfd/arch/${GDB_MACHINE_ARCH} \
		-I${GDBARCHDIR} \
		-I${DIST}/bfd \
		-I${DIST}/include \
		-Dsighandler_t=sig_t \
		-DHAVE_DECL_STPCPY \
		-DTARGET_SYSTEM_ROOT=\"\" \
		-DTARGET_SYSTEM_ROOT_RELOCATABLE=0 \
		-DBINDIR=\"${BINDIR}\" \
		${GCPPFLAGS:M-D*}

CWARNFLAGS.clang+=	-Wno-switch

CLEANDIRFILES+= ctf-error.h

${G_OBJECTS} ${G_OBJECTS:.o=.d}: ctf-error.h

ctf-error.h: ${DIST}/libctf/mkerrors.sed ${DIST}/include/ctf-api.h
	${TOOL_SED} -nf ${DIST}/libctf/mkerrors.sed < ${DIST}/include/ctf-api.h > $@


.include "../../Makefile.inc"
.include <bsd.lib.mk>

# Force c++
#CWARNFLAGS.clang+=	-Wno-deprecated
#HOST_CC:=		${HOST_CXX}
#CC:=			${CXX}
#CFLAGS:=		${CXXFLAGS} -std=gnu++11 -Wno-error=stack-protector

.PATH: ${DIST}/libctf ${GDBARCHDIR}
