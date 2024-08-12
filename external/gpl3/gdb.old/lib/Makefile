#	$NetBSD: Makefile,v 1.1.1.3 2024/08/12 19:58:47 christos Exp $

.include "../Makefile.inc"

SUBDIR+=	libbacktrace libbfd libiberty libopcodes libgdb libgdbsupport
SUBDIR+=	libdecnumber libgnulib libctf libsframe
.if ${USE_LIBEDIT} == no || ${USE_TUI} == yes
SUBDIR+=	libreadline
.endif

.if ${MACHINE_ARCH} == powerpc || ${MACHINE_ARCH} == powerpcle || \
    ${MACHINE_ARCH} == powerpc64
SUBDIR+= libsim
.endif


.include <bsd.subdir.mk>
