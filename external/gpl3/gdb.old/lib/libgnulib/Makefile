#	$NetBSD: Makefile,v 1.2 2024/08/12 20:32:26 christos Exp $

NOCTF=
#HOSTPROG_CXX=   1

.include <bsd.own.mk>

.include "../Makefile.inc"

LIBISPRIVATE=	yes
LIB=		gnulib
GDBARCHDIR=	${.CURDIR}/arch/${GDB_MACHINE_ARCH}

.include "${GDBARCHDIR}/defs.mk"

SRCS=		${G_OBJS:.o=.c} ${G_OBJECTS:S/libgnu_a-//:.o=.c}
SRCS+=		free.c memchr.c fcntl.c glob.c globfree.c scratch_buffer_grow.c
SRCS+=		getcwd.c canonicalize-lgpl.c openat.c readlink.c cloexec.c
SRCS+=		scratch_buffer_set_array_size.c scratch_buffer_dupfree.c
SRCS+=		scratch_buffer_grow_preserve.c strerror_r.c fdopendir.c
SRCS+=		rawmemchr.c strerror-override.c
GCPPFLAGS=	${G_INTERNAL_CFLAGS}
CPPFLAGS+=	-I${.CURDIR} \
		-I${GDBARCHDIR}/gnulib \
		-I${GDBARCHDIR}/gnulib/import \
		-I${DIST} \
		-I${DIST}/gnulib \
		-I${DIST}/gnulib/import \
		-I${DIST}/bfd \
		-I${DIST}/include \
		-Dsighandler_t=sig_t \
		-DTARGET_SYSTEM_ROOT=\"\" \
		-DTARGET_SYSTEM_ROOT_RELOCATABLE=0 \
		-DBINDIR=\"${BINDIR}\" \
		${GCPPFLAGS:M-D*}

CWARNFLAGS.clang+=	-Wno-switch

.include "../../Makefile.inc"
.include <bsd.lib.mk>

CFLAGS:=		${CFLAGS} -std=gnu11 -Wno-error=stack-protector

.PATH: ${DIST}/gnulib/import ${DIST}/gnulib/import/glthread \
    ${DIST}/gnulib/import/malloc ${GDBARCHDIR}
