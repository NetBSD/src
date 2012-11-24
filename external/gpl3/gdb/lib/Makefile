#	$NetBSD: Makefile,v 1.2.4.1 2012/11/24 19:45:12 riz Exp $

.include "../Makefile.inc"

SUBDIR=		libbfd libiberty libopcodes libgdb libdecnumber
.if ${USE_LIBEDIT} == no || ${USE_TUI} == yes
SUBDIR+=	libreadline
.endif

.if ${MACHINE_ARCH} == powerpc || ${MACHINE_ARCH} == powerpcle || \
    ${MACHINE_ARCH} == powerpc64
SUBDIR+= libsim
.endif


.include <bsd.subdir.mk>
