#	$NetBSD: Makefile,v 1.9.2.1 2025/08/02 05:48:31 perseant Exp $

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
