#	$NetBSD: Makefile,v 1.4 2020/09/17 16:36:38 christos Exp $

.include "../Makefile.inc"

SUBDIR=		libbfd libiberty libopcodes libgdb libgdbsupport libdecnumber
SUBDIR+=	libgnulib libctf
.if ${USE_LIBEDIT} == no || ${USE_TUI} == yes
SUBDIR+=	libreadline
.endif

.if ${MACHINE_ARCH} == powerpc || ${MACHINE_ARCH} == powerpcle || \
    ${MACHINE_ARCH} == powerpc64
SUBDIR+= libsim
.endif


.include <bsd.subdir.mk>
