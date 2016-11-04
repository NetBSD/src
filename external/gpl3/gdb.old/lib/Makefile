#	$NetBSD: Makefile,v 1.3.2.1 2016/11/04 14:48:35 pgoyette Exp $

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
