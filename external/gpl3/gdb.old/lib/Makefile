#	$NetBSD: Makefile,v 1.11 2025/08/27 15:23:05 christos Exp $

.include "../Makefile.inc"

SUBDIR+=	libbacktrace libbfd libiberty libopcodes libgdb libgdbsupport
SUBDIR+=	libdecnumber libgnulib libctf libsframe
.if ${USE_LIBEDIT} == no || ${USE_TUI} == yes
SUBDIR+=	libreadline
.endif

#.if ${MACHINE_ARCH} == powerpc || ${MACHINE_ARCH} == powerpcle || \
#    ${MACHINE_ARCH} == powerpc64
#SUBDIR+= libsim
#.endif


.include <bsd.subdir.mk>
