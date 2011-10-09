#	$NetBSD: Makefile,v 1.2 2011/10/09 02:03:19 christos Exp $

.include "../Makefile.inc"

#SUBDIR+= libsim .WAIT

SUBDIR=		libbfd libiberty libopcodes libgdb libdecnumber
.if ${USE_LIBEDIT} == no || ${USE_TUI} == yes
SUBDIR+=	libreadline
.endif

.if ${MACHINE_ARCH} == powerpc || ${MACHINE_ARCH} == powerpcle || \
    ${MACHINE_ARCH} == powerpc64
SUBDIR+= libsim
.endif


.include <bsd.subdir.mk>
